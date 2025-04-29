from vtkmodules.test import Testing as vtkTesting


class TestPolyDataMapperMTime(vtkTesting.vtkTest):
    def test(self):
        from vtkmodules import vtkInteractionStyle as _
        from vtkmodules import vtkRenderingOpenGL2 as _
        from vtkmodules.vtkFiltersSources import vtkSphereSource
        from vtkmodules.vtkFiltersCore import vtkElevationFilter
        from vtkmodules.vtkRenderingCore import (
            vtkActor,
            vtkColorTransferFunction,
            vtkPolyDataMapper,
            vtkRenderWindow,
            vtkRenderer
        )
        from vtkmodules.vtkSerializationManager import vtkObjectManager

        def serialize():
            manager = vtkObjectManager()
            manager.Initialize()

            actor = vtkActor()
            actor.mapper = (vtkSphereSource() >>
                            vtkElevationFilter() >> vtkPolyDataMapper()).last

            ctf = vtkColorTransferFunction()
            ctf.AddRGBPoint(0, 0.5, 0.5, 0.5)
            ctf.AddRGBPoint(0.5, 0.1, 0.2, 0.3)
            ctf.AddRGBPoint(1, 0.8, 0.8, 0.2)
            actor.mapper.lookup_table = ctf

            renderer = vtkRenderer()
            renderer.AddActor(actor)

            window = vtkRenderWindow()
            window.AddRenderer(renderer)
            window.Render()

            manager.RegisterObject(window)
            manager.UpdateStatesFromObjects()
            active_ids = manager.GetAllDependencies(0)

            states = map(manager.GetState, active_ids)
            hash_to_blob_map = {blob_hash: manager.GetBlob(
                blob_hash) for blob_hash in manager.GetBlobHashes(active_ids)}

            test_ids = (manager.GetId(actor.mapper), manager.GetId(ctf), manager.GetId(actor.mapper.GetInputDataObject(0, 0)))
            return states, hash_to_blob_map, test_ids

        import json

        manager = vtkObjectManager()
        manager.Initialize()

        states, hash_to_blob_map, test_ids = serialize()
        for state in states:
            manager.RegisterState(state)
        for hash_text, blob in hash_to_blob_map.items():
            manager.RegisterBlob(hash_text, blob)

        manager.UpdateObjectsFromStates()

        for i in test_ids:
            print(manager.GetObjectAtId(i))
        mtimes_before_update = [manager.GetObjectAtId(_id).GetMTime() for _id in test_ids]

        # Change opacity
        display_property_state = json.loads(manager.GetState(8))
        display_property_state["Opacity"] = 0.4
        manager.RegisterState(json.dumps(display_property_state))
        manager.UpdateObjectsFromStates()

        # Capture MTime of mapper, lut, input_polydata after update
        mtimes_after_update = [manager.GetObjectAtId(_id).GetMTime() for _id in test_ids]
        # Since only the display property state was modified, the MTimes of mapper,
        # lut and input_polydata must remain the same.
        self.assertEqual(mtimes_after_update, mtimes_before_update)


if __name__ == "__main__":
    vtkTesting.main([(TestPolyDataMapperMTime, 'test')])
