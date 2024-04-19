from vtkmodules.test import Testing as vtkTesting
import re

class TestBlobs(vtkTesting.vtkTest):

    def test(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkCommonCore import vtkTypeFloat32Array, vtkUnsignedIntArray

        arrayF32Id = ""
        arrayUI32Id = ""

        def serialize():
            global arrayF32Id, arrayUI32Id

            manager = vtkObjectManager()
            if not manager.Initialize():
                return


            arrayF32 = vtkTypeFloat32Array()
            arrayUI32 = vtkUnsignedIntArray()
            for i in range(10):
                arrayF32.InsertNextValue(10 - i)
                arrayUI32.InsertNextValue(20 + i)

            arrayF32Id = manager.RegisterObject(arrayF32)
            self.assertEqual(arrayF32Id, 1)

            arrayUI32Id = manager.RegisterObject(arrayUI32)
            self.assertEqual(arrayUI32Id, 2)

            manager.UpdateStatesFromObjects()
            active_ids = manager.GetAllDependencies(0)

            # these are not guaranteed to appear in the exact same order
            # due to the unordered_map in use within vtkObjectManager.
            self.assertEqual(len(active_ids), 2)
            self.assertEqual(arrayF32Id in active_ids, True)
            self.assertEqual(arrayUI32Id in active_ids, True)

            states = [ manager.GetState(object_id) for object_id in active_ids ]
            hash_to_blob_map = { blob_hash: manager.GetBlob(blob_hash) for blob_hash in manager.GetBlobHashes(active_ids) }

            manager.UnRegisterObject(arrayF32Id)
            manager.UnRegisterObject(arrayUI32Id)

            return states, hash_to_blob_map

        def deserialize(states, hash_to_blob_map):
            global arrayF32Id, arrayUI32Id

            if not len(states):
                return

            manager = vtkObjectManager()
            if not manager.Initialize():
                return

            for state in states:
                manager.RegisterState(state)
            for hash_text, blob in hash_to_blob_map.items():
                manager.RegisterBlob(hash_text, blob)

            manager.UpdateObjectsFromStates()
            active_ids = manager.GetAllDependencies(0)

            self.assertEqual(len(active_ids), 2)
            self.assertEqual(arrayF32Id in active_ids, True)
            self.assertEqual(arrayUI32Id in active_ids, True)

            arrayF32 = manager.GetObjectAtId(arrayF32Id)
            self.assertIsInstance(arrayF32, vtkTypeFloat32Array)

            arrayUI32 = manager.GetObjectAtId(arrayUI32Id)
            self.assertIsInstance(arrayUI32, vtkUnsignedIntArray)

            for i in range(10):
                self.assertEqual(10 - i, arrayF32.GetValue(i))
                self.assertEqual(20 + i, arrayUI32.GetValue(i))


        deserialize(*serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestBlobs, 'test')])
