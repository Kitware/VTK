from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkFiltersSources import vtkPartitionedDataSetCollectionSource
from vtkmodules.vtkRenderingCore import vtkActor, vtkColorTransferFunction, vtkCompositeDataDisplayAttributes, vtkCompositePolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor
from pathlib import Path

import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingOpenGL2


class TestCompositePolyDataMapper(vtkTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def setUp(self):
        self.id_rwi = None

        s = vtkPartitionedDataSetCollectionSource()
        s.SetNumberOfShapes(12)
        s.Update()

        m = vtkCompositePolyDataMapper()
        m.SetInputDataObject(s.GetOutput())

        cda = vtkCompositeDataDisplayAttributes()
        m.SetCompositeDataDisplayAttributes(cda)

        # Override few display attributes
        m.SetBlockVisibility(11, False)
        m.SetBlockOpacity(10, 0.5)

        ctf = vtkColorTransferFunction()
        ctf.AddRGBPoint(0, 1, 0, 0)
        ctf.AddRGBPoint(1, 1, 1, 0)
        cda.SetBlockLookupTable(s.GetOutput().GetPartition(2, 0), ctf)
        cda.SetBlockScalarVisibility(s.GetOutput().GetPartition(3, 0), False)
        cda.SetBlockColor(s.GetOutput().GetPartition(3, 0), (0.3, 1.0, 0.5))

        a = vtkActor()
        a.SetMapper(m)

        r = vtkRenderer()
        r.AddActor(a)
        r.ResetCamera()

        rw = vtkRenderWindow()
        rw.AddRenderer(r)

        self.server_rwi = vtkRenderWindowInteractor()
        self.server_rwi.SetRenderWindow(rw)

        rw.Render()

    def serialize(self):

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_rwi = manager.RegisterObject(self.server_rwi)

        manager.UpdateStatesFromObjects()
        active_ids = manager.GetAllDependencies(0)

        states = map(manager.GetState, active_ids)
        hash_to_blob_map = {blob_hash: manager.GetBlob(
            blob_hash, True) for blob_hash in manager.GetBlobHashes(active_ids)}
        return states, hash_to_blob_map

    def deserialize(self, states, hash_to_blob_map):

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

    def test(self):
        self.deserialize(*self.serialize())


if __name__ == "__main__":
    vtkTesting.main([(TestCompositePolyDataMapper, 'test')])
