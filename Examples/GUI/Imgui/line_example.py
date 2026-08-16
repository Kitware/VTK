#!/usr/bin/env python
"""A line the user drags about, reporting both of its ends.

The line widget is one of the kinds that occupies a region, so it arrives
around the data rather than around a unit cube.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(use_light_kit=True, background="slate_grey", gradient_background=True)
view.show(vtkSphereSource(radius=2.0, theta_resolution=64, phi_resolution=64),
          color="tomato", specular=0.4, specular_power=30)
view.ResetCamera()

ends = {"p1": (0.0, 0.0, 0.0), "p2": (0.0, 0.0, 0.0)}
view.widgets.AddLine(callback=lambda pair: ends.update(p1=pair[0], p2=pair[1]))


def sidebar():
    if imgui.collapsing_header("Line", imgui.TreeNodeFlags_.default_open):
        for name in ("p1", "p2"):
            x, y, z = ends[name]
            imgui.text(f"{name}: {x:6.2f} {y:6.2f} {z:6.2f}")
        (x1, y1, z1), (x2, y2, z2) = ends["p1"], ends["p2"]
        length = ((x2 - x1) ** 2 + (y2 - y1) ** 2 + (z2 - z1) ** 2) ** 0.5
        imgui.text(f"length: {length:.3f}")


run(view, "Line Widget", sidebar, hint="Drag either end, or the\nline itself, to move it")
