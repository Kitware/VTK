from pathlib import Path

from vtkmodules.test import Testing as vtkTesting

import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingOpenGL2

class TestSingleUpdate(vtkTesting.vtkTest):

    def testSceneManagement(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkFiltersSources import vtkSphereSource
        from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor

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

            self.id_rwi = manager.RegisterObject(rwi)
            manager.UpdateStatesFromObjects()
            active_ids = manager.GetAllDependencies(0)

            states = map(manager.GetState, active_ids)
            hash_to_blob_map = { blob_hash: manager.GetBlob(blob_hash, True) for blob_hash in manager.GetBlobHashes(active_ids) }
            return states, hash_to_blob_map

        def deserialize(states, hash_to_blob_map):

            manager = vtkObjectManager()
            manager.Initialize()
            for state in states:
                manager.RegisterState(state)
            for hash_text, blob in hash_to_blob_map.items():
                manager.RegisterBlob(hash_text, blob)

            manager.UpdateObjectsFromStates()
            interactor = manager.GetObjectAtId(self.id_rwi)
            interactor.render_window.Render()
            vtkTesting.compareImage(interactor.render_window, Path(vtkTesting.getAbsImagePath(f"{__class__.__name__}.png")).as_posix())


        deserialize(*serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestSingleUpdate, 'testSceneManagement')])
