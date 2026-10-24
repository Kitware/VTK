#!/usr/bin/env python
# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "imgui-bundle",
#     # VTK::ViewsScivis is not part of a released wheel yet.
#     "vtk>=9.7",
# ]
# ///
"""Measuring an angle across the scene.

Like the distance widget, this one reports its measurement rather than its
state: the callback is handed the angle in degrees.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(use_light_kit=True, background="slate_grey")
view.show(vtkSphereSource(theta_resolution=64, phi_resolution=64),
          color="sea_green", specular=0.4, specular_power=30)

measured = {"angle": 0.0}
view.widgets.AddAngle(callback=lambda a: measured.update(angle=a))


def sidebar():
    if imgui.collapsing_header("Measurement", imgui.TreeNodeFlags_.default_open):
        imgui.text(f"Angle: {measured['angle']:.2f} deg")


run(view, "Angle Widget", sidebar, hint="Click three points:\nthe middle one is the vertex")
