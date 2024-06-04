from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkFiltersSources import vtkSuperquadricSource, vtkPlaneSource
from vtkmodules.vtkRenderingCore import vtkActor, vtkColorTransferFunction, vtkCompositeDataDisplayAttributes, vtkGlyph3DMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor

import vtkmodules.vtkRenderingOpenGL2

import sys

class TestGlyph3DMapper(vtkTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def setUp(self):
        self.id_rwi = None

        s = vtkSuperquadricSource()
        p = vtkPlaneSource()
        p.SetResolution(6, 6)

        m = vtkGlyph3DMapper()
        m.SetInputConnection(p.GetOutputPort())
        m.SetSourceConnection(s.GetOutputPort())

        a = vtkActor()
        a.SetMapper(m)

        r = vtkRenderer()
        r.AddActor(a)

        rw = vtkRenderWindow()
        rw.AddRenderer(r)

        self.server_rwi = vtkRenderWindowInteractor()
        self.server_rwi.SetRenderWindow(rw)

        self.server_rwi.Render()

    def serialize(self):

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_rwi = manager.RegisterObject(self.server_rwi)

        manager.UpdateStatesFromObjects()
        active_ids = manager.GetAllDependencies(0)
        manager.Export("state.json")

        states = map(manager.GetState, active_ids)
        hash_to_blob_map = {blob_hash: manager.GetBlob(
            blob_hash) for blob_hash in manager.GetBlobHashes(active_ids)}
        return states, hash_to_blob_map

    def deserialize(self, states, hash_to_blob_map):

        manager = vtkObjectManager()
        manager.Initialize()
        for state in states:
            manager.RegisterState(state)
        for hash_text, blob in hash_to_blob_map.items():
            manager.RegisterBlob(hash_text, blob)

        manager.UpdateObjectsFromStates()
        active_ids = manager.GetAllDependencies(0)
        self.deserialized_rwi = manager.GetObjectAtId(self.id_rwi)
        self.deserialized_rwi.Render()

    def test(self):
        self.deserialize(*self.serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestGlyph3DMapper, 'test')])
