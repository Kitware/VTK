from vtkmodules.test import Testing as vtkTesting
import json

class TestBiDirectionalSerialization(vtkTesting.vtkTest):

    def test(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkRenderingCore import vtkRenderWindow

        id_rw = 0

        def serialize():
            global id_rw

            manager = vtkObjectManager()
            manager.Initialize()

            rw = vtkRenderWindow()
            id_rw = manager.RegisterObject(rw)
            manager.UpdateStatesFromObjects()
            active_ids = manager.GetAllDependencies(0)

            states = map(manager.GetState, active_ids)
            hash_to_blob_map = { blob_hash: manager.GetBlob(blob_hash) for blob_hash in manager.GetBlobHashes(active_ids) }
            return states, hash_to_blob_map

        def deserialize(states, hash_to_blob_map):
            global id_rw

            manager = vtkObjectManager()
            manager.Initialize()
            for state in states:
                manager.RegisterState(state)
            for hash_text, blob in hash_to_blob_map.items():
                manager.RegisterBlob(hash_text, blob)

            manager.UpdateObjectsFromStates()
            active_ids = manager.GetAllDependencies(0)
            rw = manager.GetObjectAtId(id_rw)
            self.assertIsNotNone(rw)

            rw.SetWindowName("Window #1")
            manager.UpdateStatesFromObjects()
            state = json.loads(manager.GetState(id_rw))
            self.assertEqual(state["WindowName"], "Window #1")

        deserialize(*serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestBiDirectionalSerialization, 'test')])
