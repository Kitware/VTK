#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestSynchronizedTemplates3D(Testing.vtkTest):
  def testAll(self):
    reader = vtk.vtkImageReader()
    reader.SetDataByteOrderToLittleEndian()
    reader.SetDataExtent(0,63,0,63,1,93)
    reader.SetDataSpacing(3.2,3.2,1.5)
    reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
    reader.SetDataMask(0x7fff)
    # write isosurface to file
    #vtkSynchronizedTemplates3D stemp
    stemp = vtk.vtkContourFilter()
    stemp.SetInputConnection(reader.GetOutputPort())
    stemp.SetValue(0,1150)

    stemp.GenerateTrianglesOff()
    stemp.Update()
    self.failUnlessEqual(stemp.GetOutputDataObject(0).GetNumberOfPoints(),39315)
    self.failUnlessEqual(stemp.GetOutputDataObject(0).GetNumberOfCells(),39067)

    stemp.GenerateTrianglesOn()
    stemp.Update()
    self.failUnlessEqual(stemp.GetOutputDataObject(0).GetNumberOfPoints(),39315)
    self.failUnlessEqual(stemp.GetOutputDataObject(0).GetNumberOfCells(),78268)

    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(stemp.GetOutputPort())
    mapper.ScalarVisibilityOff()
    head = vtk.vtkActor()
    head.SetMapper(mapper)
    head.GetProperty().SetColor(1,0.7,0.6)
    # Create the RenderWindow, Renderer and Interactor
    #
    ren1 = vtk.vtkRenderer()
    renWin = vtk.vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(head)
    ren1.SetBackground(1,1,1)
    renWin.SetSize(400,400)
    ren1.SetBackground(0.5,0.5,0.6)
    ren1.GetActiveCamera().SetPosition(99.8847,537.926,15)
    ren1.GetActiveCamera().SetFocalPoint(99.8847,109.81,15)
    ren1.GetActiveCamera().SetViewAngle(20)
    ren1.GetActiveCamera().SetViewUp(0,0,-1)
    ren1.ResetCameraClippingRange()
    # render the image
    #
    renWin.Render()
    # prevent the tk window from showing up then start the event loop
    # --- end of script --

if __name__ == "__main__":
  Testing.main([(TestSynchronizedTemplates3D, 'test')])
