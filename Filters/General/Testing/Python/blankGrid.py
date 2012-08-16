#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Demonstrate how to use structured grid blanking with an image. There are two
# techniques demonstrated: one uses an image to perform the blanking;
# the other uses scalar values to do the same thing. Both images should
# be identical.
#
# create pipeline - start by extracting a single plane from the grid
#
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

plane = vtk.vtkExtractGrid()
plane.SetInputData(output)
plane.SetVOI(0, 57, 0, 33, 0, 0)
plane.Update()

# Create some data to use for the (image) blanking
#
blankImage = vtk.vtkImageData()

# vtkType.h has definitions for vtk datatypes VTK_INT, VTK_FLOAT, etc. that
# don't get wrapped in Tcl.
VTK_UNSIGNED_CHAR = 3

blankImage.SetDimensions(57, 33, 1)
blankImage.AllocateScalars(VTK_UNSIGNED_CHAR, 1)
blankImage.GetPointData().GetScalars().SetName("blankScalars")

blanking = blankImage.GetPointData().GetScalars()
numBlanks = 57 * 33
i = 0
while i < numBlanks:
    blanking.SetComponent(i, 0, vtk.vtkDataSetAttributes.HIDDENPOINT)
    i += 1

# Manually blank out areas corresponding to dilution holes
blanking.SetComponent(318, 0, 0)
blanking.SetComponent(945, 0, 0)
blanking.SetComponent(1572, 0, 0)
blanking.SetComponent(641, 0, 0)
blanking.SetComponent(1553, 0, 0)

# The first blanking technique uses the image to set the blanking values
#
blankIt = vtk.vtkBlankStructuredGridWithImage()
blankIt.SetInputConnection(plane.GetOutputPort())
blankIt.SetBlankingInputData(blankImage)

blankedPlane = vtk.vtkStructuredGridGeometryFilter()
blankedPlane.SetInputConnection(blankIt.GetOutputPort())
blankedPlane.SetExtent(0, 100, 0, 100, 0, 0)

planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(blankedPlane.GetOutputPort())
planeMapper.SetScalarRange(0.197813, 0.710419)

planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)

# The second blanking technique uses grid data values to create the blanking.
# Here we borrow the image data and threshold on that.
#
anotherGrid = vtk.vtkStructuredGrid()
anotherGrid.CopyStructure(plane.GetOutput())
anotherGrid.GetPointData().SetScalars(blankImage.GetPointData().GetScalars())

blankGrid = vtk.vtkBlankStructuredGrid()
blankGrid.SetInputData(anotherGrid)
blankGrid.SetArrayName("blankScalars")
blankGrid.SetMinBlankingValue(-0.5)
blankGrid.SetMaxBlankingValue(0.5)

blankedPlane2 = vtk.vtkStructuredGridGeometryFilter()
blankedPlane2.SetInputConnection(blankGrid.GetOutputPort())
blankedPlane2.SetExtent(0, 100, 0, 100, 0, 0)

planeMapper2 = vtk.vtkPolyDataMapper()
planeMapper2.SetInputConnection(blankedPlane2.GetOutputPort())
planeMapper2.SetScalarRange(0.197813, 0.710419)

planeActor2 = vtk.vtkActor()
planeActor2.SetMapper(planeMapper2)

# An outline around the data
#
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(GetRGBColor('black'))

outlineMapper2 = vtk.vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline.GetOutputPort())

outlineActor2 = vtk.vtkActor()
outlineActor2.SetMapper(outlineMapper2)
outlineActor2.GetProperty().SetColor(GetRGBColor('black'))

# create planes
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0, 0, 0.5, 1)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5, 0, 1, 1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(planeActor)

ren2.AddActor(outlineActor2)
ren2.AddActor(planeActor2)

ren1.SetBackground(1, 1, 1)
ren2.SetBackground(1, 1, 1)

renWin.SetSize(500, 250)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam1.SetPosition(-12.3332, 31.7479, 41.2387)
cam1.SetViewUp(0.060772, -0.319905, 0.945498)

ren2.SetActiveCamera(ren1.GetActiveCamera())

# render the image
#
renWin.Render()

iren.Initialize()
#iren.Start()
