import argparse

from pathlib import Path

from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkCommonDataModel import vtkPiecewiseFunction
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkRenderingCore import (
    vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor,
    vtkVolume, vtkVolumeProperty, vtkColorTransferFunction,)
from vtkmodules.vtkRenderingVolume import (
    vtkFixedPointVolumeRayCastMapper, vtkGPUVolumeRayCastMapper)
from vtkmodules.vtkRenderingVolumeOpenGL2 import (vtkOpenGLGPUVolumeRayCastMapper, vtkSmartVolumeMapper)
from vtkmodules.vtkRenderingAnnotation import vtkCubeAxesActor

import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkInteractionStyle

MAPPERS = {
    "FixedPoint": vtkFixedPointVolumeRayCastMapper(),
    "GPU": vtkOpenGLGPUVolumeRayCastMapper(),
    "RayCast": vtkGPUVolumeRayCastMapper(),
    "Smart": vtkSmartVolumeMapper(),
}


class TestVolume(vtkTesting.vtkTest):

    def test(self):

        parser = argparse.ArgumentParser(
            "TestVolume", description="Exercise volume mapper (de)serialization.")
        parser.add_argument('mapper', choices=MAPPERS.keys())
        args, _ = parser.parse_known_args()
        self.mapper_type = args.mapper

        ser_om = vtkObjectManager()
        ser_om.Initialize()

        deser_om = vtkObjectManager()
        deser_om.Initialize()

        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        interactor = vtkRenderWindowInteractor()
        interactor.SetRenderWindow(renWin)
        interactor.GetInteractorStyle().SetCurrentStyleToTrackballCamera()

        source = vtkRTAnalyticSource()
        source.Update()
        mapper = MAPPERS[self.mapper_type]
        mapper.SetInputConnection(source.GetOutputPort())
        actor = vtkVolume()
        actor.SetMapper(mapper)
        actor.GetProperty().SetScalarOpacityUnitDistance(10)
        ren.AddActor(actor)

        colorTransferFunction = vtkColorTransferFunction()
        colorTransferFunction.AddRGBPoint(0.0, 0.0, 0.0, 0.0)
        colorTransferFunction.AddRGBPoint(64.0, 1.0, 0.0, 0.0)
        colorTransferFunction.AddRGBPoint(128.0, 0.0, 0.0, 1.0)
        colorTransferFunction.AddRGBPoint(192.0, 0.0, 1.0, 0.0)
        colorTransferFunction.AddRGBPoint(255.0, 0.0, 0.2, 0.0)

        opacityTransferFunction = vtkPiecewiseFunction()
        opacityTransferFunction.AddPoint(20, 0.0)
        opacityTransferFunction.AddPoint(255, 0.2)

        volumeProperty = vtkVolumeProperty()
        volumeProperty.SetColor(colorTransferFunction)
        volumeProperty.SetScalarOpacity(opacityTransferFunction)
        volumeProperty.ShadeOn()
        volumeProperty.SetInterpolationTypeToLinear()

        actor.SetProperty(volumeProperty)

        cube = vtkCubeAxesActor()
        cube.SetCamera(ren.GetActiveCamera())
        cube.SetBounds(source.GetOutput().GetBounds())
        ren.AddActor(cube)

        ren.ResetCamera()
        ren.SetBackground(0.7, 0.7, 0.7)
        renWin.Render()

        last_mtimes = dict()

        def serialize(id_rwi):

            ser_om.UpdateStatesFromObjects()
            active_ids = ser_om.GetAllDependencies(id_rwi)

            status = dict(ids=[], mtimes=[], hashes=[])
            status['ids'] = active_ids
            status['mtimes'] = {object_id: ser_om.GetObjectAtId(
                object_id).GetMTime() for object_id in active_ids}
            status['hashes'] = {blob_hash: ser_om.GetBlob(
                blob_hash) for blob_hash in ser_om.GetBlobHashes(active_ids)}
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

            renderWindow = deser_om.GetObjectAtId(id_rwi).GetRenderWindow()
            renderWindow.SetPosition(400, 1)
            renderWindow.Render()

        id_rwi = ser_om.RegisterObject(interactor)

        deserialize(serialize(id_rwi))
        interactor = deser_om.GetObjectAtId(id_rwi)
        vtkTesting.compareImage(interactor.render_window, Path(vtkTesting.getAbsImagePath(f"{__class__.__name__}{self.mapper_type}Mapper.png")).as_posix())


if __name__ == "__main__":
    vtkTesting.main([(TestVolume, 'test')])
