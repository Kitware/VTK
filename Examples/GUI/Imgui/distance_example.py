#!/usr/bin/env python
"""Measuring a distance across the scene.

The widget reports the distance itself, so nothing here reads the widget's
representation: view.widgets.AddDistance(callback=...) hands over the number.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(use_light_kit=True, background="slate_grey", gradient_background=True)
view.show(vtkSphereSource(theta_resolution=64, phi_resolution=64),
          color="steel_blue", specular=0.4, specular_power=30)

measured = {"distance": 0.0}
view.widgets.AddDistance(callback=lambda d: measured.update(distance=d))


def sidebar():
    if imgui.collapsing_header("Measurement", imgui.TreeNodeFlags_.default_open):
        imgui.text(f"Distance: {measured['distance']:.4f}")


run(view, "Distance Widget", sidebar, hint="Click two points to\nmeasure the distance")
