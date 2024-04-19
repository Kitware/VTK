from vtkmodules.test import Testing as vtkTesting
import sys

class TestDynamic(vtkTesting.vtkTest):

    def test(self):

        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkFiltersSources import vtkConeSource
        from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor

        ser_om = vtkObjectManager()
        ser_om.Initialize()

        deser_om = vtkObjectManager()
        deser_om.Initialize()

        s = vtkConeSource()

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

        rw.Render()

        last_mtimes = dict()

        def serialize(id_rwi):

            ser_om.UpdateStatesFromObjects()
            active_ids = ser_om.GetAllDependencies(0)

            for vtk_id in active_ids:
                vtk_obj = ser_om.GetObjectAtId(vtk_id)
            status = dict(ids=[], mtimes=[], hashes=[])
            status['ids'] = active_ids
            status['mtimes'] = { object_id: ser_om.GetObjectAtId(object_id).GetMTime() for object_id in active_ids }
            status['hashes'] = { blob_hash: ser_om.GetBlob(blob_hash) for blob_hash in ser_om.GetBlobHashes(active_ids) }
            return status

        def deserialize(status: dict):

            if not len(status.get('ids')):
                return

            deser_om.PruneUnusedBlobs()

            for object_id in status.get('ids'):
                new_mtime = status.get('mtimes').get(object_id)
                last_mtime = last_mtimes.get(object_id)
                if last_mtime is not None and new_mtime is not None:
                    if last_mtime < new_mtime:
                        deser_om.UnRegisterState(object_id)
                        deser_om.RegisterState(ser_om.GetState(object_id))
                elif last_mtime is None:
                    deser_om.RegisterState(ser_om.GetState(object_id))
                last_mtimes.update({object_id: new_mtime})
            for hash_text, blob in status.get('hashes').items():
                deser_om.RegisterBlob(hash_text, blob)

            deser_om.UpdateObjectsFromStates()
            active_ids = deser_om.GetAllDependencies(0)

            renderWindow = deser_om.GetObjectAtId(id_rwi).GetRenderWindow()
            renderWindow.SetPosition(400, 1)
            renderWindow.Render()

        id_rwi = ser_om.RegisterObject(rwi)

        maxRes = 8
        for i in range(4, maxRes, 1):
            s.SetResolution(i)
            m.SetInputConnection(s.GetOutputPort())
            rw.Render()

            deserialize(serialize(id_rwi))

        for i in range(maxRes - 1, 3, -1):
            s.SetResolution(i)
            m.SetInputConnection(s.GetOutputPort())
            rw.Render()

            deserialize(serialize(id_rwi))


if __name__ == "__main__":
    vtkTesting.main([(TestDynamic, 'test')])
