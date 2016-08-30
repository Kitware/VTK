#!/usr/bin/python
import vtk
from vtk.test import Testing
from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase

class MovingSphereSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
            nInputPorts=0,
            nOutputPorts=1, outputType='vtkPolyData')

    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        t = range(0, 10)
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_STEPS(), t, len(t))
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [t[0], t[-1]], 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtk.vtkPolyData.GetData(outInfo)

        t = info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        sphere = vtk.vtkSphereSource()
        sphere.SetCenter(t * 2, 0, 0)
        sphere.Update()

        output.ShallowCopy(sphere.GetOutput())
        return 1

source = MovingSphereSource()
source.Update()

group = vtk.vtkMultiBlockFromTimeSeriesFilter()
group.SetInputConnection(source.GetOutputPort())
group.Update()

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

mapper = vtk.vtkCompositePolyDataMapper2()
mapper.SetInputConnection(group.GetOutputPort())
mapper.Update()

actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)

ren1.ResetCamera()
renWin.SetSize(300, 300)

renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
