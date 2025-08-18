# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

# Example to show how vtkCocoaAutoreleasePool can be used to reduce
# peak RAM consumption in a VTK-Cocoa program.

# Please note, the memory tracking done by this example is very crude.
# The results should not be taken as an accurate measurement of memory
# use by VTK, Cocoa, or Python.

import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.vtkRenderingCore import vtkRenderer, vtkRenderWindow
from vtkmodules.vtkRenderingUI import vtkCocoaAutoreleasePool

import time
import resource

# Number of iterations for testing.  Keep this under 200 or your
# computer might freeze during the loop where vtkCocoaAutoreleasePool
# is not used to reduce peak memory consumption.
N = 60

print("Testing over", N, "iterations.")

# ------------------------------------------------------------------------
def update():
    """Code that creates and initializes vtkCocoaRenderWindow"""
    win = vtkRenderWindow()
    ren = vtkRenderer()
    win.AddRenderer(ren)
    win.Initialize()

# ------------------------------------------------------------------------
def resident_mem():
    """Get RAM usage in bytes"""
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss

# ------------------------------------------------------------------------
# RAM usage with vtkCocoaAutoreleasePool
bytes_used_1 = list(range(N))
baseline_1 = resident_mem()

for i in range(N):
    pool = vtkCocoaAutoreleasePool()
    update()
    bytes_used_1[i] = resident_mem()
    pool.Release()
    time.sleep(0.001)

# ------------------------------------------------------------------------
# RAM usage without vtkCocoaAutoreleasePool
bytes_used_2 = list(range(N))
baseline_2 = resident_mem()

for i in range(N):
    update()
    bytes_used_2[i] = resident_mem()
    time.sleep(0.001)

# ------------------------------------------------------------------------
print("Peak RAM minus baseline with vtkCocoaAutoreleasePool:   ",
      max(bytes_used_1) - baseline_1, "bytes")
print("Peak RAM minus baseline without vtkCocoaAutoreleasePool:",
      max(bytes_used_2) - baseline_2, "bytes")
