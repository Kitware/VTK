#!/usr/bin/env python
# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "imgui-bundle",
#     # VTK::ViewsScivis is not part of a released wheel yet.
#     "vtk>=9.7",
# ]
# ///
"""A caption pointing at a place in the scene.

The caption is dragged to where it reads well while its leader goes on pointing
at the anchor; the callback reports where that anchor has ended up.  The text
lives on the caption actor, so changing it is a matter of setting it there.
"""
from imgui_bundle import imgui
from _widget_shell import run

from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(use_light_kit=True, background="slate_grey")
view.show(vtkConeSource(resolution=48, height=3.0, radius=1.0),
          color="goldenrod", specular=0.4, specular_power=30)
view.ResetCamera()

state = {"at": (1.5, 0.0, 0.0), "text": "the tip"}
caption = view.widgets.AddCaption(state["text"], 1.5, 0.0, 0.0,
                                  callback=lambda at: state.update(at=at))


def sidebar():
    if imgui.collapsing_header("Caption", imgui.TreeNodeFlags_.default_open):
        changed, state["text"] = imgui.input_text("Text", state["text"])
        if changed:
            caption.GetRepresentation().GetCaptionActor2D().SetCaption(state["text"])
        x, y, z = state["at"]
        imgui.text(f"anchored at: {x:.2f} {y:.2f} {z:.2f}")


run(view, "Caption Widget", sidebar, hint="Drag the label; the leader\nkeeps pointing at the tip")
