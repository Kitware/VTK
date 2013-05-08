#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestGridSynchronizedTemplates3D(Testing.vtkTest):
    def testAll(self):
        # cut data
        pl3d = vtk.vtkMultiBlockPLOT3DReader()
        pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
        pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
        pl3d.SetScalarFunctionNumber(100)
        pl3d.SetVectorFunctionNumber(202)
        pl3d.Update()
        pl3d_output = pl3d.GetOutput().GetBlock(0)

        range = pl3d_output.GetPointData().GetScalars().GetRange()
        min = range[0]
        max = range[1]
        value = (min + max) / 2.0

        # cf = vtk.vtkGridSynchronizedTemplates3D()
        cf = vtk.vtkContourFilter()
        cf.SetInputData(pl3d_output)
        cf.SetValue(0, value)
        cf.GenerateTrianglesOff()
        cf.Update()
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfPoints(), 4674)
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfCells(), 4011)

        cf.GenerateTrianglesOn()
        cf.Update()
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfPoints(), 4674)
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfCells(), 8076)

        # cf ComputeNormalsOff
        cfMapper = vtk.vtkPolyDataMapper()
        cfMapper.ImmediateModeRenderingOn()
        cfMapper.SetInputConnection(cf.GetOutputPort())
        cfMapper.SetScalarRange(
          pl3d_output.GetPointData().GetScalars().GetRange())

        cfActor = vtk.vtkActor()
        cfActor.SetMapper(cfMapper)

        # outline
        outline = vtk.vtkStructuredGridOutlineFilter()
        outline.SetInputData(pl3d_output)

        outlineMapper = vtk.vtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())

        outlineActor = vtk.vtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.GetProperty().SetColor(0, 0, 0)

         # # Graphics stuff
         # Create the RenderWindow, Renderer and both Actors
         #
        ren1 = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren1)
        iren = vtk.vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        # Add the actors to the renderer, set the background and size
        #
        ren1.AddActor(outlineActor)
        ren1.AddActor(cfActor)
        ren1.SetBackground(1, 1, 1)

        renWin.SetSize(400, 400)

        cam1 = ren1.GetActiveCamera()
        cam1.SetClippingRange(3.95297, 50)
        cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
        cam1.SetPosition(2.7439, -37.3196, 38.7167)
        cam1.SetViewUp(-0.16123, 0.264271, 0.950876)
        iren.Initialize()

        # render the image
        #
        # loop over surfaces
        i = 0
        while i < 17:
            cf.SetValue(0, min + (i / 16.0) * (max - min))

            renWin.Render()

            cf.SetValue(0, min + (0.2) * (max - min))

            renWin.Render()

            i += 1

#        iren.Start()

if __name__ == "__main__":
    Testing.main([(TestGridSynchronizedTemplates3D, 'test')])
