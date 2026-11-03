"""Choosing the Viskores device from Python.

Serial is the one device every Viskores build can run, so the selection is
tested against it: whatever else this build has, forcing Serial must leave
Serial alone, and undoing that must bring the rest back.
"""

from vtkmodules.vtkAcceleratorsVTKmCore import vtkmDeviceSelection
from vtkmodules.vtkCommonCore import vtkObject
from vtkmodules.test import Testing


def available():
    return vtkmDeviceSelection.GetAvailableDevices().split()


class TestVtkmDeviceSelection(Testing.vtkTest):
    def setUp(self):
        vtkmDeviceSelection.Reset()
        self.default = available()

    def tearDown(self):
        vtkmDeviceSelection.Reset()

    def test_serial_is_available(self):
        self.assertIn("Serial", self.default)

    def test_force(self):
        self.assertTrue(vtkmDeviceSelection.ForceDevice("serial"))
        self.assertEqual(available(), ["Serial"])
        vtkmDeviceSelection.Reset()
        self.assertEqual(available(), self.default)

    def test_disable_and_enable(self):
        self.assertTrue(vtkmDeviceSelection.DisableDevice("Serial"))
        self.assertNotIn("Serial", available())
        self.assertTrue(vtkmDeviceSelection.EnableDevice("Serial"))
        self.assertEqual(available(), self.default)

    def test_unknown_device_is_refused(self):
        # The refusal warns, and ctest fails a test whose output looks like
        # an error, so keep the warning this test asks for off the console.
        vtkObject.GlobalWarningDisplayOff()
        try:
            self.assertFalse(vtkmDeviceSelection.ForceDevice("NoSuchDevice"))
            self.assertFalse(vtkmDeviceSelection.DisableDevice("NoSuchDevice"))
            self.assertFalse(vtkmDeviceSelection.EnableDevice(None))
        finally:
            vtkObject.GlobalWarningDisplayOn()
        self.assertEqual(available(), self.default)


if __name__ == "__main__":
    Testing.main([(TestVtkmDeviceSelection, "test")])
