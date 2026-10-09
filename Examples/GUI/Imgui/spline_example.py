#!/usr/bin/env python
"""A tube that follows a spline the user drags into shape.

The tube is shown with view.show() like any other data.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import vtkTubeFilter
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

# The spline's shape, refilled by the widget and tubed for something to look at.
curve = vtkPolyData()
tube = vtkTubeFilter(radius=0.05, number_of_sides=16)
tube.SetInputData(curve)

view = vtkScivisView(use_light_kit=True, background="slate_grey", gradient_background=True)
representation = view.show(tube, color="steel_blue", specular=0.4, specular_power=30)

state = {"radius": 0.05, "sides": 16, "closed": False, "handles": 5}


def follow(shape):
    """Take the spline's new shape as the tube's input."""
    curve.ShallowCopy(shape)
    tube.Modified()


spline = view.widgets.AddSpline(handles=state["handles"], callback=follow)

# The tube has nothing to show until the spline says what shape it is.
initial = vtkPolyData()
spline.GetRepresentation().GetPolyData(initial)
follow(initial)
view.ResetCamera()


def reread():
    shape = vtkPolyData()
    spline.GetRepresentation().GetPolyData(shape)
    follow(shape)


def sidebar():
    if imgui.collapsing_header("Tube", imgui.TreeNodeFlags_.default_open):
        changed, state["radius"] = imgui.slider_float("Radius", state["radius"], 0.01, 0.3)
        if changed:
            tube.radius = state["radius"]

        changed, state["sides"] = imgui.slider_int("Sides", state["sides"], 3, 32)
        if changed:
            tube.number_of_sides = state["sides"]

    imgui.spacing()

    if imgui.collapsing_header("Spline", imgui.TreeNodeFlags_.default_open):
        changed, state["closed"] = imgui.checkbox("Closed loop", state["closed"])
        if changed:
            spline.GetRepresentation().SetClosed(state["closed"])
            reread()

        imgui.text(f"{spline.GetRepresentation().GetNumberOfHandles()} handles")


run(view, "Spline Tube", sidebar, hint="Drag the handles to bend\nthe tube along the spline")
