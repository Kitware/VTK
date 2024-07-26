# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
import sys
import re
from pathlib import Path
from vtkmodules.test import Testing
from vtkmodules.vtkCommonCore import *
from vtkmodules.vtkCommonDataModel import *
from vtkmodules.vtkInteractionStyle import *
from vtkmodules.vtkIOCellGrid import *
from vtkmodules.vtkFiltersCellGrid import *
from vtkmodules.vtkFiltersSources import *
from vtkmodules.vtkRenderingAnnotation import *
from vtkmodules.vtkRenderingCore import *
from vtkmodules.vtkRenderingCellGrid import *

import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


# Register render responder for DG cells
vtkRenderingCellGrid.RegisterCellsAndResponders()

events = """# StreamVersion 1.2
ExposeEvent 0 479 0 0 0 0 0
RenderEvent 0 479 0 0 0 0 0
EnterEvent 496 4 0 0 0 0 0
MouseMoveEvent 496 4 0 0 0 0 0
MouseMoveEvent 239 145 0 0 0 0 0
KeyPressEvent 239 145 0 114 1 r 0
CharEvent 239 145 0 114 1 r 0
MouseMoveEvent 239 145 0 0 0 r 0
MouseMoveEvent 245 155 0 0 0 r 0
KeyReleaseEvent 245 155 0 114 1 r 0
MouseMoveEvent 246 157 0 0 0 r 0
MouseMoveEvent 251 172 0 0 0 r 0
LeftButtonPressEvent 251 172 0 0 0 r 0
MouseMoveEvent 252 172 0 0 0 r 0
MouseMoveEvent 277 150 0 0 0 r 0
LeftButtonReleaseEvent 277 150 0 0 0 r 0
StartPickEvent 277 150 0 0 0 r 0
EndPickEvent 277 150 0 0 0 r 0
RenderEvent 277 150 0 0 0 r 0
MouseMoveEvent 277 150 0 0 0 r 0
MouseMoveEvent 291 242 0 0 0 r 0
LeftButtonPressEvent 291 242 0 0 0 r 0
MouseMoveEvent 293 242 0 0 0 r 0
MouseMoveEvent 340 201 0 0 0 r 0
LeftButtonReleaseEvent 340 201 0 0 0 r 0
StartPickEvent 340 201 0 0 0 r 0
EndPickEvent 340 201 0 0 0 r 0
RenderEvent 340 201 0 0 0 r 0
MouseMoveEvent 339 201 0 0 0 r 0
MouseMoveEvent 264 191 0 0 0 r 0
LeftButtonPressEvent 264 191 0 0 0 r 0
MouseMoveEvent 265 191 0 0 0 r 0
MouseMoveEvent 339 141 0 0 0 r 0
MouseMoveEvent 340 141 0 0 0 r 0
LeftButtonReleaseEvent 340 141 0 0 0 r 0
StartPickEvent 340 141 0 0 0 r 0
EndPickEvent 340 141 0 0 0 r 0
RenderEvent 340 141 0 0 0 r 0
MouseMoveEvent 339 142 0 0 0 r 0
MouseMoveEvent 265 194 0 0 0 r 0
LeftButtonPressEvent 265 194 0 0 0 r 0
MouseMoveEvent 265 194 0 0 0 r 0
MouseMoveEvent 211 249 0 0 0 r 0
LeftButtonReleaseEvent 211 249 0 0 0 r 0
StartPickEvent 211 249 0 0 0 r 0
EndPickEvent 211 249 0 0 0 r 0
RenderEvent 211 249 0 0 0 r 0
MouseMoveEvent 211 248 0 0 0 r 0
MouseMoveEvent 147 403 0 0 0 r 0
LeftButtonPressEvent 147 403 0 0 0 r 0
MouseMoveEvent 147 401 0 0 0 r 0
MouseMoveEvent 599 41 0 0 0 r 0
LeftButtonReleaseEvent 599 41 0 0 0 r 0
StartPickEvent 599 41 0 0 0 r 0
EndPickEvent 599 41 0 0 0 r 0
RenderEvent 599 41 0 0 0 r 0
MouseMoveEvent 595 42 0 0 0 r 0
MouseMoveEvent 607 7 0 0 0 r 0
LeaveEvent 608 -4 0 0 0 r 0"""

record_events = False


class TestCellGridRenderedAreaPickerWithPolyData(Testing.vtkTest):

    def test(self):
        data_file = Path(VTK_DATA_ROOT).joinpath(
            'Data', 'dgQuadraticQuadrilaterals.dg')
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

        sphere_actor = vtkActor()
        sphere_actor.mapper = (vtkSphereSource() >> vtkPolyDataMapper()).last

        self.expected_selection_nodes = [
            {
                'pick_id': 0,
                'nodes': [
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 26,
                        'ZBUFFER_VALUE': 0.6403792500495911,
                        'CELLGRID_CELL_TYPE_INDEX': -1,
                        'PROP': sphere_actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': -1,
                        'PROP_ID': 2,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            2,
                            44
                        ]
                    }
                ]
            },
            {
                'pick_id': 1,
                'nodes': [
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 2100,
                        'ZBUFFER_VALUE': 0.7080743908882141,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 0,
                        'PROP_ID': 0,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0
                        ]
                    }
                ]
            },
            {
                'pick_id': 2,
                'nodes': [
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 70,
                        'ZBUFFER_VALUE': 0.6403792500495911,
                        'CELLGRID_CELL_TYPE_INDEX': -1,
                        'PROP': sphere_actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': -1,
                        'PROP_ID': 2,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            2,
                            3,
                            4,
                            9,
                            45
                        ]
                    },
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 176,
                        'ZBUFFER_VALUE': 0.6976379156112671,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor_sides,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 1,
                        'PROP_ID': 1,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0
                        ]
                    },
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 1130,
                        'ZBUFFER_VALUE': 0.6987160444259644,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 0,
                        'PROP_ID': 0,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0
                        ]
                    }
                ]
            },
            {
                'pick_id': 3,
                'nodes': [
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 96,
                        'ZBUFFER_VALUE': 0.6588415503501892,
                        'CELLGRID_CELL_TYPE_INDEX': -1,
                        'PROP': sphere_actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': -1,
                        'PROP_ID': 2,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            15,
                            16,
                            17,
                            21,
                            22,
                            23
                        ]
                    },
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 469,
                        'ZBUFFER_VALUE': 0.7515520453453064,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 0,
                        'PROP_ID': 0,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0
                        ]
                    },
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 69,
                        'ZBUFFER_VALUE': 0.7761813402175903,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor_sides,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 1,
                        'PROP_ID': 1,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            2
                        ]
                    }
                ]
            },
            {
                'pick_id': 4,
                'nodes': [
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 2737,
                        'ZBUFFER_VALUE': 0.3899713456630707,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor_sides,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 1,
                        'PROP_ID': 1,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0,
                            1,
                            2,
                            3,
                            4,
                            5
                        ]
                    },
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 41259,
                        'ZBUFFER_VALUE': 0.4132111966609955,
                        'CELLGRID_CELL_TYPE_INDEX': 0,
                        'PROP': actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': 0,
                        'PROP_ID': 0,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0,
                            1
                        ]
                    },
                    {
                        'COMPOSITE_INDEX': 1,
                        'PIXEL_COUNT': 625,
                        'ZBUFFER_VALUE': 0.6337828636169434,
                        'CELLGRID_CELL_TYPE_INDEX': -1,
                        'PROP': sphere_actor,
                        'CELLGRID_SOURCE_SPECIFICATION_INDEX': -1,
                        'PROP_ID': 2,
                        'FIELD_TYPE': 1,
                        'CONTENT_TYPE': 3,
                        'SelectedIds': [
                            0,
                            2,
                            3,
                            4,
                            8,
                            9,
                            10,
                            11,
                            14,
                            15,
                            16,
                            17,
                            20,
                            21,
                            22,
                            23,
                            26,
                            27,
                            28,
                            29,
                            32,
                            33,
                            34,
                            38,
                            39,
                            40,
                            41,
                            44,
                            45,
                            46,
                            47
                        ]
                    }
                ]
            },]

        renderer = vtkRenderer()
        renderer.AddActor(actor)
        renderer.AddActor(actor_sides)
        renderer.AddActor(sphere_actor)

        window = vtkRenderWindow(size=(640, 480), multi_samples=0)
        window.AddRenderer(renderer)

        interactor = vtkRenderWindowInteractor(
            interactor_style=vtkInteractorStyleRubberBandPick())
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
            recorder.input_string = re.sub(
                r'^EndPickEvent.*\n?',
                '', events, flags=re.MULTILINE)
            # Render once so that camera is reset to geometry bounds.
            window.Render()
            recorder.Play()

    def _on_pick_finished(self, interactor, event):
        cursor = interactor.GetEventPosition()
        renderer = interactor.FindPokedRenderer(cursor[0], cursor[1])
        selection = vtkSelection()
        rect = (renderer.pick_x1, renderer.pick_y1,
                renderer.pick_x2, renderer.pick_y2)
        renderer.PickProp(
            *rect,
            # fieldSelection=
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
                    selected_ids_arr = node.selection_data.GetArray(
                        "SelectedIds")
                    selected_ids = [int(selected_ids_arr.GetValue(i)) for i in range(
                        selected_ids_arr.GetNumberOfValues())]
                    self.assertListEqual(v, selected_ids)
                    continue
                vtk_info_key = getattr(vtkSelectionNode, k)()
                expected[k] = node.GetProperties().Get(vtk_info_key)
                if k == "ZBUFFER_VALUE":
                    # More lenient because zbuffer value can vary lightly based on graphics implementations
                    self.assertAlmostEqual(v, node.GetProperties().Get(vtk_info_key), places=5)
                else:
                    self.assertEqual(v, node.GetProperties().Get(vtk_info_key))


if __name__ == "__main__":
    Testing.main([(TestCellGridRenderedAreaPickerWithPolyData,
                   'test')])
