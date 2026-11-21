"""Formato de registros locales: cifrado continuo y manifiesto autenticado."""
from __future__ import annotations

import getpass
import hashlib
import hmac
import json
import os
import shutil
import time
from pathlib import Path

from cryptography.exceptions import InvalidTag
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.scrypt import Scrypt

DEFAULT_OUTPUT = Path.home() / "registros-emg"
RECORD_FILES = ("senal.csv", "eventos.csv", "resumen.json")
MANIFEST = "manifiesto.hmac.json"
MAGIC = b"EMG-AES256-GCM-1\n"
SCRYPT_N, SCRYPT_R, SCRYPT_P = 2**14, 8, 1
SALT_BYTES, NONCE_BYTES, TAG_BYTES = 16, 12, 16
RETENTION_DAYS = 30
CHUNK_BYTES = 64 * 1024


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("ascii")


def derive(passphrase, salt, *, authentication=False):
    if not isinstance(passphrase, str) or not passphrase:
        raise ValueError("Se requiere una contraseña de sesión no vacía.")
    if len(salt) != SALT_BYTES:
        raise ValueError("Sal de registro inválida.")
    # Dominios separados: la clave HMAC nunca es la clave AES.
    domain = b"EMG-HMAC-v1\0" if authentication else b"EMG-AES-v1\0"
    return Scrypt(salt=domain + salt, length=32, n=SCRYPT_N, r=SCRYPT_R,
                  p=SCRYPT_P).derive(passphrase.encode("utf-8"))


def ask_passphrase(*, new=False, dialog=None):
    """El entorno se reserva para pruebas; no se serializa ni se imprime."""
    value = os.environ.get("EMG_PASSPHRASE")
    if value is None:
        prompt = dialog or getpass.getpass
        value = prompt("Contraseña de sesión: ")
        if not value:
            raise ValueError("Contraseña cancelada o vacía.")
        if new and value != prompt("Repite la contraseña de sesión: "):
            raise ValueError("Las contraseñas no coinciden.")
    if not value:
        raise ValueError("La contraseña no puede estar vacía.")
    return value


def notice(directory, *, encrypted=True, include_time=False):
    return ("El AD8232 captura ECG: datos biométricos/de salud que pueden identificar a una persona.\n"
            "Se guardarán señal ADC, base, envolvente, estados, eventos y estadísticas en:\n"
            f"{Path(directory).resolve()}\n"
            + ("CSV y JSON cifrados con AES-256-GCM; manifiesto HMAC-SHA256.\n" if encrypted else
               "ADVERTENCIA --sin-cifrar: CSV y JSON legibles en disco; sólo integridad HMAC.\n")
            + ("Se incluirá host_utc (hora absoluta).\n" if include_time else
               "Sin host_utc; identificador aleatorio y tiempo relativo del dispositivo.\n")
            + "No se guarda la contraseña. Si se pierde, los datos cifrados no se recuperan.")


class RecordWriter:
    """Interfaz de texto para csv: jamás crea un temporal en claro."""
    def __init__(self, path, passphrase, *, encrypted=True):
        path = Path(path)
        self.closed = False
        self.encryptor = None
        if encrypted:
            salt, nonce = os.urandom(SALT_BYTES), os.urandom(NONCE_BYTES)
            header = MAGIC + canonical({"salt": salt.hex(), "nonce": nonce.hex()}) + b"\n"
            self.encryptor = Cipher(algorithms.AES(derive(passphrase, salt)), modes.GCM(nonce)).encryptor()
            self.encryptor.authenticate_additional_data(path.name.encode("utf-8") + b"\0" + header)
        self.stream = path.open("xb")
        if encrypted:
            self.stream.write(header)

    def write(self, text):
        if self.closed:
            raise ValueError("Registro cerrado.")
        data = text.encode("utf-8")
        self.stream.write(self.encryptor.update(data) if self.encryptor else data)
        return len(text)

    def flush(self):
        self.stream.flush()

    def close(self):
        if not self.closed:
            try:
                if self.encryptor:
                    self.stream.write(self.encryptor.finalize())
                    self.stream.write(self.encryptor.tag)
            finally:
                self.stream.close()
                self.closed = True


def is_encrypted(path):
    with Path(path).open("rb") as stream:
        return stream.read(len(MAGIC)) == MAGIC


def decrypt_file(path, passphrase):
    """Sólo entrega texto después de validar la etiqueta GCM completa."""
    path = Path(path)
    try:
        with path.open("rb") as stream:
            if stream.read(len(MAGIC)) != MAGIC:
                raise ValueError("El archivo no está cifrado con el formato EMG v1.")
            raw_header = stream.readline(1024)
            metadata = json.loads(raw_header)
            salt, nonce = bytes.fromhex(metadata["salt"]), bytes.fromhex(metadata["nonce"])
            if len(nonce) != NONCE_BYTES or len(raw_header) >= 1024:
                raise ValueError("Cabecera inválida.")
            header = MAGIC + raw_header
            encrypted = stream.read()
            if len(encrypted) < TAG_BYTES:
                raise ValueError("Etiqueta GCM incompleta.")
        return AESGCM(derive(passphrase, salt)).decrypt(
            nonce, encrypted, path.name.encode("utf-8") + b"\0" + header)
    except (InvalidTag, ValueError, KeyError, TypeError) as exc:
        raise ValueError("Contraseña incorrecta o archivo cifrado alterado.") from exc


def file_mac(path, key):
    mac = hmac.new(key, Path(path).name.encode("utf-8") + b"\0", hashlib.sha256)
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(CHUNK_BYTES), b""):
            mac.update(chunk)
    return mac.hexdigest()


def write_manifest(directory, passphrase):
    directory = Path(directory)
    salt = os.urandom(SALT_BYTES)
    key = derive(passphrase, salt, authentication=True)
    payload = {"version": 1, "salt": salt.hex(),
               "files": {name: file_mac(directory / name, key) for name in RECORD_FILES}}
    document = {"payload": payload,
                "hmac": hmac.new(key, canonical(payload), hashlib.sha256).hexdigest()}
    with (directory / MANIFEST).open("xb") as stream:
        stream.write(canonical(document) + b"\n")


def verify_directory(directory, passphrase):
    directory = Path(directory)
    try:
        manifest_path = directory / MANIFEST
        if manifest_path.is_symlink():
            raise ValueError("Manifiesto enlazado.")
        raw = manifest_path.read_bytes()
        document = json.loads(raw)
        if set(document) != {"payload", "hmac"} or raw != canonical(document) + b"\n":
            raise ValueError("Formato del manifiesto alterado.")
        payload = document["payload"]
        if set(payload) != {"version", "salt", "files"}:
            raise ValueError("Campos del manifiesto alterados.")
        key = derive(passphrase, bytes.fromhex(payload["salt"]), authentication=True)
        expected = hmac.new(key, canonical(payload), hashlib.sha256).hexdigest()
        if not hmac.compare_digest(expected, document["hmac"]):
            raise ValueError("HMAC del manifiesto distinto.")
        if payload["version"] != 1 or set(payload["files"]) != set(RECORD_FILES):
            raise ValueError("Lista de archivos inválida.")
        for name in RECORD_FILES:
            path = directory / name
            if path.is_symlink() or not hmac.compare_digest(file_mac(path, key), payload["files"][name]):
                raise ValueError(f"Archivo alterado: {name}")
        return True
    except (OSError, ValueError, KeyError, TypeError, UnicodeError) as exc:
        raise ValueError("Verificación fallida: contraseña incorrecta, registro incompleto o datos alterados.") from exc


def old_records(base, now=None):
    """Sólo sesiones reconocidas, sin seguir enlaces; no afecta archivos sueltos."""
    base = Path(base)
    cutoff = (time.time() if now is None else now) - RETENTION_DAYS * 86400
    if not base.is_dir():
        return []
    result = []
    for path in sorted(base.glob("registro_*")):
        if path.is_symlink() or not path.is_dir():
            continue
        files = list(path.iterdir())
        if not any(p.name in RECORD_FILES for p in files) or any(p.is_symlink() or p.is_dir() for p in files):
            continue
        if max([path.stat().st_mtime] + [p.stat().st_mtime for p in files]) < cutoff:
            result.append(path)
    return result


def offer_cleanup(base, confirm, inform):
    records = old_records(base)
    if not records:
        return
    message = (f"Registros sin cambios desde hace más de {RETENTION_DAYS} días:\n"
               + "\n".join(str(p.resolve()) for p in records))
    inform(message)
    if confirm(message + "\n¿Borrar permanentemente estas carpetas completas?"):
        for path in records:
            # Revalidar antes de borrar y mantener el destino dentro de la base anunciada.
            if path in old_records(base) and path.resolve().parent == Path(base).resolve():
                shutil.rmtree(path)
