#!/usr/bin/env python
# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules.vtkRenderingWebGPU import vtkWebGPURenderer

# These methods used to be absent from Python, and the framebuffer was unavailable
# until rendering. Configuration must not require a GPU or a first frame.
renderer = vtkWebGPURenderer()
renderer.SetDeepFramebufferAverageLayersPerPixel(24)
renderer.SetDeepFramebufferMaximumMemoryInBytes(2**33)
renderer.SetDeepFramebufferMaximumLayersPerPixel(32)
renderer.DeepFramebufferAutoResizeOff()
framebuffer = renderer.GetDeepFramebuffer()
assert framebuffer.GetAverageLayersPerPixel() == 24
assert framebuffer.GetMaximumMemoryInBytes() == 2**33
assert framebuffer.GetFragmentNodeCapacity() == 0
assert renderer.GetDeepFramebufferMaximumLayersPerPixel() == 32
assert not framebuffer.GetAutoResize()
framebuffer.SetAverageLayersPerPixel(12)
assert renderer.GetDeepFramebufferAverageLayersPerPixel() == 12
