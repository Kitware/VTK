#!/usr/bin/env python
import vtk
import sys

from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
size=400
image1 = vtk.vtkImageCanvasSource2D()
image1.SetNumberOfScalarComponents(3)
image1.SetScalarTypeToUnsignedChar()
image1.SetExtent(0,size,0,size,0,0)
image1.SetDrawColor(255,255,0)
image1.FillBox(0,size,0,size)
pad1 = vtk.vtkImageWrapPad()
pad1.SetInputConnection(image1.GetOutputPort())
pad1.SetOutputWholeExtent(0,size,0,size,0,10)
pad1.Update()
image2 = vtk.vtkImageCanvasSource2D()
image2.SetNumberOfScalarComponents(3)
image2.SetScalarTypeToUnsignedChar()
image2.SetExtent(0,size,0,size,0,0)
image2.SetDrawColor(0,255,255)
image2.FillBox(0,size,0,size)
pad2 = vtk.vtkImageWrapPad()
pad2.SetInputConnection(image2.GetOutputPort())
pad2.SetOutputWholeExtent(0,size,0,size,0,10)
pad2.Update()
checkers = vtk.vtkImageCheckerboard()
checkers.SetInput1Data(pad1.GetOutput())
checkers.SetInput2Data(pad2.GetOutput())
checkers.SetNumberOfDivisions(11,6,0)

mapper=vtk.vtkImageSliceMapper()
mapper.SetInputConnection(checkers.GetOutputPort())
mapper.SliceAtFocalPointOn()
mapper.SliceFacesCameraOn()
imageSlice = vtk.vtkImageSlice()
imageSlice.SetMapper(mapper)
imageSlice.GetProperty().SetColorLevel(127.5)
imageSlice.GetProperty().SetColorWindow(255)
renderer=vtk.vtkRenderer()
renderer.AddViewProp(imageSlice)

window=vtk.vtkRenderWindow()
window.SetSize(size,size)
window.AddRenderer(renderer)

window.Render()
pp = vtk.vtkPointPicker()
cp = vtk.vtkCellPicker()
p  = vtk.vtkPicker()
errors = 0
for (i,j,cid,pid) in ((20,40,-1,-1),(160,100,821341,825800),(240,300,938658,943010)):
    pos=[i,j]

    # Test prop picker
    result = p.Pick(pos[0],pos[1],0,renderer)
    if (result != 0) != (pid != -1):
      errors += 1
      print("Picker at {} returns {} expected".format(pos,result) )

    # Test cell picker
    result = cp.Pick(pos[0],pos[1],0,renderer)
    if (result != 0) != (pid != -1):
      errors += 1
      print("Cellpicker at {} returns {} expected {}".format(pos, (result != 0), (pid != -1) ) )
    if pid != cp.GetPointId():
      errors += 1
      print("Cellpicker at {} found point {} expected {}".format(pos, cp.GetPointId(), pid) )
    if cid != cp.GetCellId():
      errors += 1
      print("Cellpicker at {} found cell {} expected {}".format(pos, cp.GetCellId(), cid) )

    result = pp.Pick(pos[0],pos[1],0,renderer)
    if (result != 0) != (pid != -1):
      errors += 1
      print("Pointpicker at {} returns {} expected {}".format(pos, (result != 0), (pid != -1) ) )
    if pid != pp.GetPointId():
      errors += 1
      print("Pointpicker at {} found point {} expected {}".format(pos, pp.GetPointId(), pid) )

if errors:
  sys.exit(1)
