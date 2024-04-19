from vtkmodules.test import Testing as vtkTesting

import argparse
import vtk

MAPPERS = {
    "FixedPoint": vtk.vtkFixedPointVolumeRayCastMapper(),
    "GPU": vtk.vtkOpenGLGPUVolumeRayCastMapper(),
    "RayCast": vtk.vtkGPUVolumeRayCastMapper(),
    "Smart": vtk.vtkSmartVolumeMapper(),
}


class TestDynamic(vtkTesting.vtkTest):

    def test(self):

        from vtkmodules.vtkSerializationManager import vtkObjectManager

        parser = argparse.ArgumentParser(
            "TestVolume", description="Exercise volume mapper (de)serialization.")
        parser.add_argument('mapper', choices=MAPPERS.keys())

        args = parser.parse_args()
        MAPPER_TYPE = args.mapper

        ser_om = vtkObjectManager()
        ser_om.Initialize()

        deser_om = vtkObjectManager()
        deser_om.Initialize()

        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        interactor = vtk.vtkRenderWindowInteractor()
        interactor.SetRenderWindow(renWin)
        interactor.GetInteractorStyle().SetCurrentStyleToTrackballCamera()

        source = vtk.vtkRTAnalyticSource()
        source.Update()
        mapper = MAPPERS[MAPPER_TYPE]
        mapper.SetInputConnection(source.GetOutputPort())
        actor = vtk.vtkVolume()
        actor.SetMapper(mapper)
        actor.GetProperty().SetScalarOpacityUnitDistance(10)
        ren.AddActor(actor)

        colorTransferFunction = vtk.vtkColorTransferFunction()
        colorTransferFunction.AddRGBPoint(0.0, 0.0, 0.0, 0.0)
        colorTransferFunction.AddRGBPoint(64.0, 1.0, 0.0, 0.0)
        colorTransferFunction.AddRGBPoint(128.0, 0.0, 0.0, 1.0)
        colorTransferFunction.AddRGBPoint(192.0, 0.0, 1.0, 0.0)
        colorTransferFunction.AddRGBPoint(255.0, 0.0, 0.2, 0.0)

        opacityTransferFunction = vtk.vtkPiecewiseFunction()
        opacityTransferFunction.AddPoint(20, 0.0)
        opacityTransferFunction.AddPoint(255, 0.2)

        volumeProperty = vtk.vtkVolumeProperty()
        volumeProperty.SetColor(colorTransferFunction)
        volumeProperty.SetScalarOpacity(opacityTransferFunction)
        volumeProperty.ShadeOn()
        volumeProperty.SetInterpolationTypeToLinear()

        actor.SetProperty(volumeProperty)

        cube = vtk.vtkCubeAxesActor()
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

            # print(f"update({active_ids=})")
            for vtk_id in active_ids:
                vtk_obj = ser_om.GetObjectAtId(vtk_id)
                # print(f" - {vtk_id}:{vtk_obj.GetClassName()}")

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
            active_ids = deser_om.GetAllDependencies(0)

            renderWindow = deser_om.GetObjectAtId(id_rwi).GetRenderWindow()
            renderWindow.SetPosition(400, 1)
            renderWindow.Render()

        id_rwi = ser_om.RegisterObject(interactor)

        deserialize(serialize(id_rwi))
        rwi = deser_om.GetObjectAtId(id_rwi)
        rwi.GetRenderWindow().Render()


if __name__ == "__main__":
    vtkTesting.main([(TestDynamic, 'test')])
