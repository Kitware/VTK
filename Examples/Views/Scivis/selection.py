#!/usr/bin/env python
"""Selecting from a region of the screen, without a GUI to drag one out with.

What a screen region means is the selector's, reached as view.selector.  An
interactive application drives it from a rubber band; here SelectRegion() is
called directly, which is the same path.
"""

from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisSelector, vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(window_title="Selection", size=(800, 600),
                     selector={"mode": "frustum"})
sphere = vtkSphereSource(theta_resolution=32, phi_resolution=32)
representation = view.show(sphere, color="steel_blue")


def on_selection(caller, event):
    # vtkSelection is a sequence of its nodes, and a node's list is an array.
    selection = view.selector.current_selection
    picked = len(selection[0].selection_list) if selection and len(selection) else 0
    print(f"selection changed: {picked} cell(s)")


view.AddObserver(vtkCommand.SelectionChangedEvent, on_selection)

view.ResetCamera()
view.Render()

# The middle of the window, in pixels.
width, height = view.size
view.selector.SelectCells()
view.selector.SelectRegion(width // 4, height // 4, 3 * width // 4, 3 * height // 4)

# How a region is read is a property of the selector, so it can be changed and
# the same region asked again.
view.selector.mode = "surface"
print("mode is now", "frustum" if view.selector.mode == vtkScivisSelector.FRUSTUM else "surface")
view.selector.SelectRegion(width // 4, height // 4, 3 * width // 4, 3 * height // 4)

# Dragging in the window selects once the view is put in selection mode.
view.interaction_mode = "selection"
print("drag in the window to select; the observer above reports what was picked")
view.Start()
