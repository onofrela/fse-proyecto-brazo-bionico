"""Prueba el estado de la interfaz con widgets dobles, sin ventana ni actuadores."""
import unittest
from contextlib import ExitStack
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

from analizador import run_gui


class Variable:
    def __init__(self, value):
        self.value = value

    def get(self):
        return self.value

    def set(self, value):
        self.value = value


class GuiControlTests(unittest.TestCase):
    def setUp(self):
        stack = self.enterContext(ExitStack())
        root = MagicMock()
        root.winfo_screenwidth.return_value = 1280
        root.winfo_screenheight.return_value = 1024
        stack.enter_context(patch("tkinter.Tk", return_value=root))
        stack.enter_context(patch("tkinter.StringVar", Variable))
        stack.enter_context(patch("tkinter.Text"))
        for widget in ("Frame", "Label", "Combobox", "Entry", "Button"):
            stack.enter_context(patch(f"tkinter.ttk.{widget}", side_effect=lambda *a, **kw: MagicMock()))
        stack.enter_context(patch("matplotlib.backends.backend_tkagg.FigureCanvasTkAgg"))
        stack.enter_context(patch("matplotlib.backends.backend_tkagg.NavigationToolbar2Tk"))
        stack.enter_context(patch("analizador.make_figure", return_value=(MagicMock(), [])))
        stack.enter_context(patch("serial.tools.list_ports.comports", return_value=[]))
        run_gui(SimpleNamespace(port=None, reference=5.0, csv=None))
        self.app = root.protocol.call_args.args[1].__self__
        self.app.dirty = False

    def event(self, name, value=0):
        self.app.handle_event({"name": name, "value": value})

    def assert_controls(self, state):
        for button in (self.app.resume_button, self.app.pause_button):
            button.configure.assert_called_with(state=state)

    def test_only_fw8_usb_capture_enables_control(self):
        self.app.thread = object()
        for version in (0, 6, 7, 9):
            self.event("FW", version)
            self.assert_controls("disabled")
            self.assertIn("FW8", self.app.prompt)
        self.event("FW", 8)
        self.assert_controls("normal")
        self.app.resume_button.configure.assert_called_with(state="normal")
        self.app.send("c")
        self.app.send("x")
        self.assertEqual([self.app.commands.get_nowait() for _ in range(2)], ["c", "x"])
        self.app.mode.set("Demo sintética")
        self.app.update_controls()
        self.assert_controls("disabled")
        self.app.send("c")
        self.assertTrue(self.app.commands.empty())
        self.app.mode.set("Sensor USB")
        self.app.thread = None
        self.app.update_controls()
        self.assert_controls("disabled")

    def test_fw8_status_pause_and_reboot(self):
        self.app.thread = object()
        self.event("FW", 8)
        self.event("MEAN_X10", 67)
        self.event("TARGET_DEG", 90)
        self.assertEqual(self.app.activity_status.get(), "Promedio: 6.7 · Objetivo: 90°")
        self.event("TARGET_STATUS", 180)
        self.assertIn("180°", self.app.activity_status.get())
        self.event("CONTROL_PAUSED", 1)
        self.assertIn("pausado", self.app.prompt)
        self.event("CONTROL_PAUSED", 0)
        self.assertIn("activo", self.app.prompt)
        prompt, activity = self.app.prompt, self.app.activity_status.get()
        for name in ("CAL_PENDING", "CAL_REST", "CAL_CONTRACT", "CAL_FAILED", "CAL_VALID", "TH_CLOSE", "CAL_CANCEL", "ACTIVITY"):
            self.event(name, 1)
            self.assertEqual((self.app.prompt, self.app.activity_status.get()), (prompt, activity))
        self.event("BOOT", 1)
        self.assert_controls("disabled")
        self.assertEqual(self.app.firmware_version, 0)
        self.assertIn("Consultando firmware", self.app.prompt)

    def test_missing_version_requests_fw8(self):
        self.app.thread = object()
        self.app.capture_started = 1
        self.event("BOOT", 1)
        with patch("analizador.time.monotonic", return_value=8):
            self.app.tick()
        self.assert_controls("disabled")
        self.assertIn("Sin respuesta de versión", self.app.control_status.get())
        self.assertIn("FW8", self.app.control_status.get())
