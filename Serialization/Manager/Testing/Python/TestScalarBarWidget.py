#!/usr/bin/env python3

from vtkmodules.test import Testing as vtkTesting

def get_program_parameters():
    import argparse
    description = 'Scalar bar widget.'
    epilogue = '''
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('filename', help='uGridEx.vtk')
    args = parser.parse_args()
    return args.filename

class TestObjectManagerScalarBarWidget(vtkTesting.vtkTest):

    def test(self):
        from vtkmodules import vtkInteractionStyle as _
        from vtkmodules import vtkRenderingOpenGL2 as _
        from vtkmodules.vtkCommonColor import vtkNamedColors
        from vtkmodules.vtkCommonCore import vtkLookupTable
        from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader
        from vtkmodules.vtkInteractionWidgets import vtkScalarBarWidget
        from vtkmodules.vtkRenderingAnnotation import vtkScalarBarActor
        from vtkmodules.vtkRenderingCore import (
            vtkActor,
            vtkDataSetMapper,
            vtkRenderWindow,
            vtkRenderWindowInteractor,
            vtkRenderer
        )
        from vtkmodules.vtkSerializationManager import vtkObjectManager

        id_rwi = ""

        def serialize():
            global id_rwi

            manager = vtkObjectManager()
            manager.Initialize()

            colors = vtkNamedColors()

            # The source file
            file_name = get_program_parameters()

            # Create a custom lut. The lut is used for both at the mapper and at the
            # scalar_bar
            lut = vtkLookupTable()
            lut.Build()

            # Read the source file.
            reader = vtkUnstructuredGridReader()
            reader.SetFileName(file_name)
            reader.Update()  # Needed because of GetScalarRange
            output = reader.GetOutput()
            scalar_range = output.GetScalarRange()

            mapper = vtkDataSetMapper()
            mapper.SetInputData(output)
            mapper.SetScalarRange(scalar_range)
            mapper.SetLookupTable(lut)

            actor = vtkActor()
            actor.SetMapper(mapper)

            renderer = vtkRenderer()
            renderer.AddActor(actor)
            renderer.SetBackground(colors.GetColor3d('MidnightBLue'))

            render_window = vtkRenderWindow()
            render_window.AddRenderer(renderer)
            render_window.SetSize(300, 300)
            render_window.SetWindowName("ScalarBarWidget")

            interactor = vtkRenderWindowInteractor()
            interactor.SetRenderWindow(render_window)

            # create the scalar_bar
            scalar_bar = vtkScalarBarActor()
            scalar_bar.SetOrientationToHorizontal()
            scalar_bar.SetLookupTable(lut)

            # create the scalar_bar_widget
            scalar_bar_widget = vtkScalarBarWidget()
            scalar_bar_widget.SetInteractor(interactor)
            scalar_bar_widget.SetScalarBarActor(scalar_bar)
            scalar_bar_widget.On()

            interactor.Initialize()
            render_window.Render()
            renderer.GetActiveCamera().SetPosition(-6.4, 10.3, 1.4)
            renderer.GetActiveCamera().SetFocalPoint(1.0, 0.5, 3.0)
            renderer.GetActiveCamera().SetViewUp(0.6, 0.4, -0.7)
            render_window.Render()
            interactor.Start()
            id_rwi = manager.RegisterObject(interactor)
            id_sbw = manager.RegisterObject(scalar_bar_widget)
            manager.UpdateStatesFromObjects()
            active_ids = manager.GetAllDependencies(0)

            states = map(manager.GetState, active_ids)
            hash_to_blob_map = { blob_hash: manager.GetBlob(blob_hash) for blob_hash in manager.GetBlobHashes(active_ids) }
            return states, hash_to_blob_map

        def deserialize(states, hash_to_blob_map):
            global id_rwi

            manager = vtkObjectManager()
            manager.Initialize()
            for state in states:
                manager.RegisterState(state)
            for hash_text, blob in hash_to_blob_map.items():
                manager.RegisterBlob(hash_text, blob)

            manager.UpdateObjectsFromStates()
            active_ids = manager.GetAllDependencies(0)
            manager.GetObjectAtId(id_rwi).Start()

        deserialize(*serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestObjectManagerScalarBarWidget, 'test')])
