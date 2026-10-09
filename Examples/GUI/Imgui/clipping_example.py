#!/usr/bin/env python
"""Clipping what a representation draws with a box the user drags.

The box's six planes become the representation's clipping planes.  The work
is done in end_callback, so it runs once when the drag finishes rather than on
every mouse move, which is where an expensive filter belongs.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkCommonDataModel import vtkPlane, vtkPlaneCollection, vtkPlanes
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(use_light_kit=True, background="slate_grey", gradient_background=True)
representation = view.show(vtkSphereSource(theta_resolution=64, phi_resolution=64),
                           color="tomato", specular=0.4, specular_power=30)
view.ResetCamera()

state = {"clipping": True, "planes": 0}
box_planes = vtkPlanes()


def clip(_transform):
    """Put the box's six planes on the representation."""
    box.GetRepresentation().GetPlanes(box_planes)
    collection = vtkPlaneCollection()
    for i in range(box_planes.GetNumberOfPlanes()):
        plane = vtkPlane()
        box_planes.GetPlane(i, plane)
        collection.AddItem(plane)
    representation.SetClippingPlanes(collection)
    state["planes"] = collection.GetNumberOfItems()


box = view.widgets.AddBox(end_callback=clip)
# Keep what is inside the box rather than what is outside it.
box.GetRepresentation().InsideOutOn()
clip(None)


def sidebar():
    if imgui.collapsing_header("Clipping", imgui.TreeNodeFlags_.default_open):
        changed, state["clipping"] = imgui.checkbox("Clip to the box", state["clipping"])
        if changed:
            if state["clipping"]:
                clip(None)
            else:
                representation.RemoveAllClippingPlanes()
        imgui.text(f"{state['planes']} plane(s) in use")


run(view, "Clipping", sidebar, hint="Drag the box handles;\nclipping follows when you let go")
