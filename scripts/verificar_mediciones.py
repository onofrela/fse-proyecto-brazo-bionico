#!/usr/bin/env python3
"""Verifica los archivos originales del sensor contra su manifiesto SHA256."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
EVIDENCE=Path('docs/mediciones/2026-11-09')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--git-ref',help='Verificar blobs de una revision, por ejemplo HEAD')
    args=parser.parse_args()
    def read(relative):
        if args.git_ref:
            return subprocess.check_output(['git','show',f'{args.git_ref}:{relative.as_posix()}'],cwd=ROOT)
        return (ROOT/relative).read_bytes()
    manifest=json.loads(read(EVIDENCE/'sha256.json').decode('utf-8-sig'))
    for name,expected in manifest.items():
        if hashlib.sha256(read(EVIDENCE/name)).hexdigest()!=expected:
            raise SystemExit('SHA256 distinto: '+name)
    print(f'EVIDENCIA|{len(manifest)} archivos originales con SHA256 correcto|PASA')

if __name__=='__main__':
    main()
