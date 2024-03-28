from vtkmodules.test import Testing as vtkTesting

class TestObjectManagerRendering(vtkTesting.vtkTest):

    def testSceneManagement(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkFiltersSources import vtkSphereSource
        from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor

        id_rwi = 0

        def serialize():
            global id_rwi

            manager = vtkObjectManager()
            manager.Initialize()

            s = vtkSphereSource()
            m = vtkPolyDataMapper()
            m.SetInputConnection(s.GetOutputPort())
            a = vtkActor()
            a.SetMapper(m)
            r = vtkRenderer()
            r.AddActor(a)
            rw = vtkRenderWindow()
            rw.AddRenderer(r)
            rwi = vtkRenderWindowInteractor()
            rwi.SetRenderWindow(rw)
            r.ResetCamera()
            rw.Render()

            id_rwi = manager.RegisterObject(rwi)
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
            manager.GetObjectAtId(id_rwi).Render()

        deserialize(*serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestObjectManagerRendering, 'testSceneManagement')])
