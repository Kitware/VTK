# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""
Test that failed ANARI initialization does not crash or leak.
"""
import os

from vtkmodules.vtkCommonCore import vtkLogger
from vtkmodules.vtkRenderingAnariCore import vtkAnariRenderWindow
from vtkmodules.vtkRenderingCore import vtkRenderer, vtkWindowToImageFilter

vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_OFF)

os.environ["ANARI_LIBRARY"] = "this_device_does_not_exist"
window = vtkAnariRenderWindow()
renderer = vtkRenderer()
window.AddRenderer(renderer)

device = window.GetAnariDevice()
assert not device.SetupAnariDeviceFromLibrary(
    "this_device_does_not_exist", "default", False
), "Unexpectedly initialized a nonexistent ANARI library"
window.Render()

capture = vtkWindowToImageFilter()
capture.SetInput(window)
capture.ReadFrontBufferOff()
capture.Update()
