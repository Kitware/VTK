# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules.vtkCommonCore import *
from vtkmodules.vtkCommonDataModel import *
from vtkmodules.vtkInteractionStyle import *
from vtkmodules.vtkIOCellGrid import *
from vtkmodules.vtkFiltersCellGrid import *
from vtkmodules.vtkRenderingAnnotation import *
from vtkmodules.vtkRenderingCore import *
from vtkmodules.vtkRenderingCellGrid import *

import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtkmodules.test import Testing
from pathlib import Path
import re
import sys

# Register render responder for DG cells:
vtkRenderingCellGrid.RegisterCellsAndResponders()

events = """# StreamVersion 1.2
ExposeEvent 0 479 0 0 0 0 0
RenderEvent 0 479 0 0 0 0 0
EnterEvent 387 11 0 0 0 0 0
MouseMoveEvent 349 196 0 0 0 0 0
KeyPressEvent 349 196 0 112 1 p 0
CharEvent 349 196 0 112 1 p 0
StartPickEvent 349 196 0 112 1 p 0
EndPickEvent 349 196 0 112 1 p 0
RenderEvent 349 196 0 112 1 p 0
KeyReleaseEvent 349 196 0 112 1 p 0
MouseMoveEvent 439 238 0 0 0 p 0
KeyPressEvent 439 238 0 112 1 p 0
CharEvent 439 238 0 112 1 p 0
StartPickEvent 439 238 0 112 1 p 0
EndPickEvent 439 238 0 112 1 p 0
RenderEvent 439 238 0 112 1 p 0
KeyReleaseEvent 439 238 0 112 1 p 0
MouseMoveEvent 332 134 0 0 0 p 0
KeyPressEvent 332 134 0 112 1 p 0
CharEvent 332 134 0 112 1 p 0
StartPickEvent 332 134 0 112 1 p 0
EndPickEvent 332 134 0 112 1 p 0
RenderEvent 332 134 0 112 1 p 0
KeyReleaseEvent 332 134 0 112 1 p 0
MouseMoveEvent 310 361 0 0 0 p 0
KeyPressEvent 310 361 0 112 1 p 0
CharEvent 310 361 0 112 1 p 0
StartPickEvent 310 361 0 112 1 p 0
EndPickEvent 310 361 0 112 1 p 0
RenderEvent 310 361 0 112 1 p 0
KeyReleaseEvent 310 361 0 112 1 p 0
MouseMoveEvent 224 259 0 0 0 p 0
KeyPressEvent 224 259 0 112 1 p 0
CharEvent 224 259 0 112 1 p 0
StartPickEvent 224 259 0 112 1 p 0
EndPickEvent 224 259 0 112 1 p 0
RenderEvent 224 259 0 112 1 p 0
KeyReleaseEvent 224 259 0 112 1 p 0
MouseMoveEvent 436 123 0 0 0 p 0
KeyPressEvent 436 123 0 112 1 p 0
CharEvent 436 123 0 112 1 p 0
StartPickEvent 436 123 0 112 1 p 0
EndPickEvent 436 123 0 112 1 p 0
RenderEvent 436 123 0 112 1 p 0
KeyReleaseEvent 436 123 0 112 1 p 0
MouseMoveEvent 459 292 0 0 0 p 0
KeyPressEvent 459 292 0 112 1 p 0
CharEvent 459 292 0 112 1 p 0
StartPickEvent 459 292 0 112 1 p 0
EndPickEvent 459 292 0 112 1 p 0
RenderEvent 459 292 0 112 1 p 0
KeyReleaseEvent 459 292 0 112 1 p 0
MouseMoveEvent 435 356 0 0 0 p 0
KeyPressEvent 435 356 0 112 1 p 0
CharEvent 435 356 0 112 1 p 0
StartPickEvent 435 356 0 112 1 p 0
EndPickEvent 435 356 0 112 1 p 0
RenderEvent 435 356 0 112 1 p 0
KeyReleaseEvent 435 356 0 112 1 p 0
"""

record_events = False

class TestCellGridHardwarePicker(Testing.vtkTest):

    def test(self):
        data_file = Path(VTK_DATA_ROOT).joinpath('Data', 'dgQuadraticQuadrilaterals.dg')
        reader = vtkCellGridReader(file_name=data_file)

        compute_sides = vtkCellGridComputeSides(
            output_dimension_control=vtkCellGridSidesQuery.NextLowestDimension,
            preserve_renderable_inputs=False,
            omit_sides_for_renderable_inputs=False,)
        mapper_sides = vtkCellGridMapper()
        reader >> compute_sides >> mapper_sides

        mapper = vtkCellGridMapper(array_component=-2,
            array_name="scalar2",
            scalar_mode=VTK_SCALAR_MODE_USE_CELL_FIELD_DATA,
            scalar_visibility=True,)
        reader >> mapper

        actor_sides = vtkActor()
        actor_sides.mapper = mapper_sides
        actor_sides.property.line_width = 4.0

        actor = vtkActor()
        actor.mapper = mapper

        self.expected_selection_ids = [
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 0,
                "cell_grid_tuple_id" : 0,
            },
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 0,
                "cell_grid_tuple_id" : 1,
            },
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 1,
                "cell_grid_tuple_id" : 0,
            },
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 1,
                "cell_grid_tuple_id" : 1,
            },
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 1,
                "cell_grid_tuple_id" : 2,
            },
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 1,
                "cell_grid_tuple_id" : 3,
            },
            {
                "cell_grid_cell_type_id" : 0,
                "cell_grid_source_spec_id" : 1,
                "cell_grid_tuple_id" : 4,
            },
            {
                "cell_grid_cell_type_id": 0,
                "cell_grid_source_spec_id": 1,
                "cell_grid_tuple_id": 5,
            },
        ]

        renderer = vtkRenderer()
        renderer.AddActor(actor)
        renderer.AddActor(actor_sides)

        window = vtkRenderWindow(size=(640, 480), multi_samples=0)
        window.AddRenderer(renderer)

        interactor = vtkRenderWindowInteractor(interactor_style=vtkInteractorStyleRubberBandPick())
        interactor.picker = vtkHardwarePicker()
        interactor.render_window = window
        interactor.AddObserver(vtkCommand.EndPickEvent, self._on_pick_finished)
        recorder = vtkInteractorEventRecorder(interactor=interactor)
        recorder.SetEnabled(True)
        self._pick_event_id = 0
        if record_events:
            recorder.SetFileName("events.txt")
            recorder.Record()
            interactor.Start()
            recorder.Stop()
        else:
            recorder.read_from_input_string = True
            # Scrub away all EndPickEvent(s) from the recorded events
            recorder.input_string = re.sub(r'^EndPickEvent.*\n?', '', events, flags=re.MULTILINE)
            # Render once so that camera is reset to geometry bounds.
            window.Render()
            recorder.Play()

    def _on_pick_finished(self, interactor, event):
        cursor = interactor.GetEventPosition()
        event_id = self._pick_event_id
        self._pick_event_id += 1
        expected = self.expected_selection_ids[event_id]
        for k, v in expected.items():
            self.assertEqual(v, getattr(interactor.picker, k))


if __name__ == "__main__":
    Testing.main([(TestCellGridHardwarePicker, 'test')])
