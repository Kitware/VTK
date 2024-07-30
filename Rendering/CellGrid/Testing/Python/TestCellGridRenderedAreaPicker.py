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

# Register render responder for DG cells
vtkRenderingCellGrid.RegisterCellsAndResponders()

events = """# StreamVersion 1.2
ExposeEvent 0 479 0 0 0 0 0
RenderEvent 0 479 0 0 0 0 0
EnterEvent 256 33 0 0 0 0 0
MouseMoveEvent 256 33 0 0 0 0 0
MouseMoveEvent 357 249 0 0 0 0 0
LeftButtonPressEvent 357 249 0 0 0 0 0
StartInteractionEvent 357 249 0 0 0 0 0
MouseMoveEvent 359 249 0 0 0 0 0
RenderEvent 359 249 0 0 0 0 0
InteractionEvent 359 249 0 0 0 0 0
MouseMoveEvent 362 248 0 0 0 0 0
RenderEvent 362 248 0 0 0 0 0
InteractionEvent 362 248 0 0 0 0 0
MouseMoveEvent 367 248 0 0 0 0 0
RenderEvent 367 248 0 0 0 0 0
InteractionEvent 367 248 0 0 0 0 0
MouseMoveEvent 372 246 0 0 0 0 0
RenderEvent 372 246 0 0 0 0 0
InteractionEvent 372 246 0 0 0 0 0
MouseMoveEvent 373 246 0 0 0 0 0
RenderEvent 373 246 0 0 0 0 0
InteractionEvent 373 246 0 0 0 0 0
MouseMoveEvent 377 245 0 0 0 0 0
RenderEvent 377 245 0 0 0 0 0
InteractionEvent 377 245 0 0 0 0 0
MouseMoveEvent 381 244 0 0 0 0 0
RenderEvent 381 244 0 0 0 0 0
InteractionEvent 381 244 0 0 0 0 0
MouseMoveEvent 396 243 0 0 0 0 0
RenderEvent 396 243 0 0 0 0 0
InteractionEvent 396 243 0 0 0 0 0
MouseMoveEvent 404 242 0 0 0 0 0
RenderEvent 404 242 0 0 0 0 0
InteractionEvent 404 242 0 0 0 0 0
MouseMoveEvent 411 242 0 0 0 0 0
RenderEvent 411 242 0 0 0 0 0
InteractionEvent 411 242 0 0 0 0 0
MouseMoveEvent 417 242 0 0 0 0 0
RenderEvent 417 242 0 0 0 0 0
InteractionEvent 417 242 0 0 0 0 0
MouseMoveEvent 423 242 0 0 0 0 0
RenderEvent 423 242 0 0 0 0 0
InteractionEvent 423 242 0 0 0 0 0
MouseMoveEvent 426 242 0 0 0 0 0
RenderEvent 426 242 0 0 0 0 0
InteractionEvent 426 242 0 0 0 0 0
MouseMoveEvent 429 242 0 0 0 0 0
RenderEvent 429 242 0 0 0 0 0
InteractionEvent 429 242 0 0 0 0 0
MouseMoveEvent 431 242 0 0 0 0 0
RenderEvent 431 242 0 0 0 0 0
InteractionEvent 431 242 0 0 0 0 0
MouseMoveEvent 432 242 0 0 0 0 0
RenderEvent 432 242 0 0 0 0 0
InteractionEvent 432 242 0 0 0 0 0
LeftButtonReleaseEvent 432 242 0 0 0 0 0
EndInteractionEvent 432 242 0 0 0 0 0
RenderEvent 432 242 0 0 0 0 0
MouseMoveEvent 431 242 0 0 0 0 0
MouseMoveEvent 233 264 0 0 0 0 0
KeyPressEvent 233 264 0 114 1 r 0
CharEvent 233 264 0 114 1 r 0
MouseMoveEvent 233 265 0 0 0 r 0
MouseMoveEvent 233 267 0 0 0 r 0
KeyReleaseEvent 233 267 0 114 1 r 0
MouseMoveEvent 233 268 0 0 0 r 0
MouseMoveEvent 232 272 0 0 0 r 0
LeftButtonPressEvent 232 272 0 0 0 r 0
MouseMoveEvent 233 272 0 0 0 r 0
MouseMoveEvent 290 224 0 0 0 r 0
LeftButtonReleaseEvent 290 224 0 0 0 r 0
StartPickEvent 290 224 0 0 0 r 0
EndPickEvent 290 224 0 0 0 r 0
RenderEvent 290 224 0 0 0 r 0
MouseMoveEvent 291 224 0 0 0 r 0
MouseMoveEvent 420 269 0 0 0 r 0
LeftButtonPressEvent 420 269 0 0 0 r 0
MouseMoveEvent 421 269 0 0 0 r 0
MouseMoveEvent 482 225 0 0 0 r 0
LeftButtonReleaseEvent 482 225 0 0 0 r 0
StartPickEvent 482 225 0 0 0 r 0
EndPickEvent 482 225 0 0 0 r 0
RenderEvent 482 225 0 0 0 r 0
MouseMoveEvent 481 225 0 0 0 r 0
MouseMoveEvent 266 162 0 0 0 r 0
LeftButtonPressEvent 266 162 0 0 0 r 0
MouseMoveEvent 268 162 0 0 0 r 0
MouseMoveEvent 334 107 0 0 0 r 0
LeftButtonReleaseEvent 334 107 0 0 0 r 0
StartPickEvent 334 107 0 0 0 r 0
EndPickEvent 334 107 0 0 0 r 0
RenderEvent 334 107 0 0 0 r 0
MouseMoveEvent 333 107 0 0 0 r 0
MouseMoveEvent 109 286 0 0 0 r 0
LeftButtonPressEvent 109 286 0 0 0 r 0
MouseMoveEvent 110 286 0 0 0 r 0
MouseMoveEvent 221 226 0 0 0 r 0
LeftButtonReleaseEvent 221 226 0 0 0 r 0
StartPickEvent 221 226 0 0 0 r 0
EndPickEvent 221 226 0 0 0 r 0
RenderEvent 221 226 0 0 0 r 0
MouseMoveEvent 221 227 0 0 0 r 0
MouseMoveEvent 254 397 0 0 0 r 0
LeftButtonPressEvent 254 397 0 0 0 r 0
MouseMoveEvent 255 396 0 0 0 r 0
MouseMoveEvent 325 319 0 0 0 r 0
LeftButtonReleaseEvent 325 319 0 0 0 r 0
StartPickEvent 325 319 0 0 0 r 0
EndPickEvent 325 319 0 0 0 r 0
RenderEvent 325 319 0 0 0 r 0
MouseMoveEvent 325 318 0 0 0 r 0
MouseMoveEvent 429 152 0 0 0 r 0
LeftButtonPressEvent 429 152 0 0 0 r 0
MouseMoveEvent 429 151 0 0 0 r 0
MouseMoveEvent 490 93 0 0 0 r 0
LeftButtonReleaseEvent 490 93 0 0 0 r 0
StartPickEvent 490 93 0 0 0 r 0
EndPickEvent 490 93 0 0 0 r 0
RenderEvent 490 93 0 0 0 r 0
MouseMoveEvent 489 93 0 0 0 r 0
MouseMoveEvent 471 265 0 0 0 r 0
LeftButtonPressEvent 471 265 0 0 0 r 0
MouseMoveEvent 472 265 0 0 0 r 0
MouseMoveEvent 580 197 0 0 0 r 0
LeftButtonReleaseEvent 580 197 0 0 0 r 0
StartPickEvent 580 197 0 0 0 r 0
EndPickEvent 580 197 0 0 0 r 0
RenderEvent 580 197 0 0 0 r 0
MouseMoveEvent 577 197 0 0 0 r 0
MouseMoveEvent 420 392 0 0 0 r 0
LeftButtonPressEvent 420 392 0 0 0 r 0
MouseMoveEvent 421 390 0 0 0 r 0
MouseMoveEvent 488 318 0 0 0 r 0
LeftButtonReleaseEvent 488 318 0 0 0 r 0
StartPickEvent 488 318 0 0 0 r 0
EndPickEvent 488 318 0 0 0 r 0
RenderEvent 488 318 0 0 0 r 0"""

record_events = False

class TestCellGridRenderedAreaPicker(Testing.vtkTest):

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

        self.expected_selection_nodes = [
            {
                "pick_id": 0,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 2891,
                        "ZBUFFER_VALUE": 0.724821,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [0],
                    },
                ],
            },
            {
                "pick_id": 1,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 2835,
                        "ZBUFFER_VALUE": 0.637797,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [1],
                    },
                ],
            },
            {
                "pick_id": 2,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 276,
                        "ZBUFFER_VALUE": 0.73828,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor_sides,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 1,
                        "PROP_ID": 1,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [0],
                    },
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 1639,
                        "ZBUFFER_VALUE": 0.739102,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [0],
                    },
                ],
            },
            {
                "pick_id": 3,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 3196,
                        "ZBUFFER_VALUE": 0.738914,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [0],
                    },
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 122,
                        "ZBUFFER_VALUE": 0.776003,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor_sides,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 1,
                        "PROP_ID": 1,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [2],
                    },
                ],
            },
            {
                "pick_id": 4,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 288,
                        "ZBUFFER_VALUE": 0.641407,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor_sides,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 1,
                        "PROP_ID": 1,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [1],
                    },
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 2641,
                        "ZBUFFER_VALUE": 0.644091,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [0],
                    },
                ],
            },
            {
                "pick_id": 5,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 2023,
                        "ZBUFFER_VALUE": 0.626466,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [1],
                    },
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 124,
                        "ZBUFFER_VALUE": 0.626935,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor_sides,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 1,
                        "PROP_ID": 1,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [3],
                    },
                ],
            },
            {
                "pick_id": 6,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 276,
                        "ZBUFFER_VALUE": 0.586716,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor_sides,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 1,
                        "PROP_ID": 1,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [4],
                    },
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 2830,
                        "ZBUFFER_VALUE": 0.590431,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [1],
                    },
                ],
            },
            {
                "pick_id": 7,
                "nodes": [
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 276,
                        "ZBUFFER_VALUE": 0.627337,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor_sides,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 1,
                        "PROP_ID": 1,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [5],
                    },
                    {
                        "COMPOSITE_INDEX": 1,
                        "PIXEL_COUNT": 2237,
                        "ZBUFFER_VALUE": 0.627361,
                        "CELLGRID_CELL_TYPE_INDEX": 0,
                        "PROP": actor,
                        "CELLGRID_SOURCE_SPECIFICATION_INDEX": 0,
                        "PROP_ID": 0,
                        "FIELD_TYPE": 1,
                        "CONTENT_TYPE": 3,
                        "SelectedIds": [1],
                    },
                ],
            },
        ]

        renderer = vtkRenderer()
        renderer.AddActor(actor)
        renderer.AddActor(actor_sides)

        window = vtkRenderWindow(size=(640, 480), multi_samples=0)
        window.AddRenderer(renderer)

        interactor = vtkRenderWindowInteractor(interactor_style=vtkInteractorStyleRubberBandPick())
        interactor.picker = vtkRenderedAreaPicker()
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
        renderer = interactor.FindPokedRenderer(cursor[0], cursor[1])
        selector = vtkHardwareSelector()
        selection = vtkSelection()
        rect = (renderer.pick_x1, renderer.pick_y1, renderer.pick_x2, renderer.pick_y2)
        renderer.PickProp(
            *rect,
            #fieldSelection=
            0,
            selection)
        num_nodes = selection.GetNumberOfNodes()
        event_id = self._pick_event_id
        self._pick_event_id += 1
        for i in range(num_nodes):
            node = selection.GetNode(i)
            expected = self.expected_selection_nodes[event_id].get("nodes")[i]
            for k, v in expected.items():
                if k == "SelectedIds":
                    selected_ids_arr = node.selection_data.GetArray("SelectedIds")
                    selected_ids = [int(selected_ids_arr.GetValue(i)) for i in range(selected_ids_arr.GetNumberOfValues())]
                    self.assertListEqual(v, selected_ids)
                    continue
                vtk_info_key = getattr(vtkSelectionNode, k)()
                if k == "ZBUFFER_VALUE":
                    # More lenient because zbuffer value can vary lightly based on graphics implementations
                    self.assertAlmostEqual(v, node.GetProperties().Get(vtk_info_key), places=5)
                else:
                    self.assertEqual(v, node.GetProperties().Get(vtk_info_key))


if __name__ == "__main__":
    Testing.main([(TestCellGridRenderedAreaPicker, 'test')])
