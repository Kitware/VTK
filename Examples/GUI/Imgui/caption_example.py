#!/usr/bin/env python
"""A caption pointing at a place in the scene.

The caption is dragged to where it reads well while its leader goes on pointing
at the anchor; the callback reports where that anchor has ended up.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(use_light_kit=True, background="slate_grey", gradient_background=True)
view.show(vtkConeSource(resolution=48, height=3.0, radius=1.0),
          color="goldenrod", specular=0.4, specular_power=30)
view.ResetCamera()

anchor = {"at": (1.5, 0.0, 0.0)}
view.widgets.AddCaption("the tip", 1.5, 0.0, 0.0, callback=lambda at: anchor.update(at=at))


def sidebar():
    if imgui.collapsing_header("Caption", imgui.TreeNodeFlags_.default_open):
        x, y, z = anchor["at"]
        imgui.text(f"anchored at: {x:.2f} {y:.2f} {z:.2f}")


run(view, "Caption Widget", sidebar, hint="Drag the label; the leader\nkeeps pointing at the tip")
