# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

""" Apply a series of events produced by vtk-js RenderWindowInteractor to a
vtkRenderwindow via the vtkRemoteInteractionAdapter class. The final image is
the expected scene after all interactions have been applied.
"""

from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
)
from vtkmodules.vtkWebCore import vtkRemoteInteractionAdapter
from vtkmodules.vtkFiltersSources import vtkConeSource

from vtkmodules.test import Testing
import os
import json

# Required for rendering initialization,
import vtkmodules.vtkRenderingOpenGL2  # noqa

# Required for interactor initialization
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleSwitch  # noqa


# The scene to test. In some platforms re-using the window & renderer across
# the two test cases causes segfault. We start all tests from a clean state by
# creating the scene from scratch each time.
class Scene:
    def __init__(self):
        self.dataFile = os.path.join(
            Testing.VTK_DATA_ROOT, "Data", "remote_events.json"
        )
        self.imageFile = "TestRemoteInteractionAdapter.png"
        self.adapter = vtkRemoteInteractionAdapter()

        print("dataFile: {}".format(self.dataFile))
        if not os.path.isfile(self.dataFile):
            raise RuntimeError("Datafile is missing")

        self.renderer = vtkRenderer()
        self.renderWindow = vtkRenderWindow()
        self.renderWindow.AddRenderer(self.renderer)
        self.renderWindow.SetSize(300, 300)

        self.renderWindowInteractor = vtkRenderWindowInteractor()
        self.renderWindowInteractor.SetRenderWindow(self.renderWindow)
        self.renderWindowInteractor.GetInteractorStyle().SetCurrentStyleToTrackballCamera()

        self.cone_source = vtkConeSource()
        self.mapper = vtkPolyDataMapper()
        self.mapper.SetInputConnection(self.cone_source.GetOutputPort())
        self.actor = vtkActor()
        self.actor.SetMapper(self.mapper)

        self.renderer.AddActor(self.actor)
        self.renderer.ResetCamera()
        self.renderWindowInteractor.Initialize()


class TestRemoteInteractorAdapter(Testing.vtkTest):
    def test0(self):
        """Use class methods API for ProcessEvent"""
        scene = Scene()

        adapter = vtkRemoteInteractionAdapter()
        adapter.SetInteractor(scene.renderWindowInteractor)

        with open(scene.dataFile, "r") as f:
            data = json.load(f)
            for event in data["events"]:
                event_str = json.dumps(event)
                status = adapter.ProcessEvent(event_str)
                assert status, f"Failed to process event\n {event_str}"
                scene.renderWindowInteractor.Render()
        self.assertImageMatch(scene.renderWindow, scene.imageFile)

    def test1(self):
        """Use static method API for ProcessEvent"""
        scene = Scene()

        with open(scene.dataFile, "r") as f:
            data = json.load(f)
            for event in data["events"]:
                event_str = json.dumps(event)
                status = vtkRemoteInteractionAdapter.ProcessEvent(
                    scene.renderWindowInteractor, event_str
                )
                assert status, f"Failed to process event\n {event_str}"
                scene.renderWindowInteractor.Render()
        self.assertImageMatch(scene.renderWindow, scene.imageFile)


if __name__ == "__main__":
    Testing.main([(TestRemoteInteractorAdapter, "test")])
