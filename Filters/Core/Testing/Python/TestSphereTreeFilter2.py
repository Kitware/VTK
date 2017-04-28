#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create 3x3 array of renderers. Vary the matrix of renderers (and hence data
# pipelines) by data type (default, structured, and unstructured) and
# geometric operation (point, line, plane).

# Control debugging parameters
res = 10

# Create the RenderWindow, Renderer, and Interactor
#
ren00 = vtk.vtkRenderer()
ren01 = vtk.vtkRenderer()
ren02 = vtk.vtkRenderer()
ren10 = vtk.vtkRenderer()
ren11 = vtk.vtkRenderer()
ren12 = vtk.vtkRenderer()
ren20 = vtk.vtkRenderer()
ren21 = vtk.vtkRenderer()
ren22 = vtk.vtkRenderer()

renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren00)
renWin.AddRenderer(ren01)
renWin.AddRenderer(ren02)
renWin.AddRenderer(ren10)
renWin.AddRenderer(ren11)
renWin.AddRenderer(ren12)
renWin.AddRenderer(ren20)
renWin.AddRenderer(ren21)
renWin.AddRenderer(ren22)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# A spherical glyph is used to represent the selected cells from the sphere
# tree.
sph = vtk.vtkSphereSource()
sph.SetPhiResolution(6)
sph.SetThetaResolution(12)
sph.SetRadius(1)

# Three dataset types will be defined in the following: image data,
# structured grid, and unstructured grid. This sampling of datasets tests the
# three execution paths currently in the sphere tree.

# Create a synthetic image data: sample a sphere across a volume
sphere = vtk.vtkSphere()
sphere.SetCenter(0.0,0.0,0.0)
sphere.SetRadius(0.25)

image = vtk.vtkSampleFunction()
image.SetImplicitFunction(sphere)
image.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
image.SetSampleDimensions(res,res,res)
image.Update()

# Handy dandy filter converts image data to structured grid
sgrid = vtk.vtkImageDataToPointSet()
sgrid.SetInputConnection(image.GetOutputPort())
sgrid.Update()

# Convert the image data to unstructured grid
extractionSphere = vtk.vtkSphere()
extractionSphere.SetRadius(100)
extractionSphere.SetCenter(0,0,0)

extract = vtk.vtkExtractGeometry()
extract.SetImplicitFunction(extractionSphere)
extract.SetInputConnection(image.GetOutputPort())
extract.Update()

# ======= 00
# Create a sphere tree and see what it look like
# (image sphere tree)
stf00 = vtk.vtkSphereTreeFilter()
stf00.SetInputConnection(image.GetOutputPort())
stf00.SetExtractionModeToPoint()
stf00.SetPoint(0,0,0)

stfGlyphs00 = vtk.vtkGlyph3D()
stfGlyphs00.SetInputConnection(stf00.GetOutputPort())
stfGlyphs00.SetSourceConnection(sph.GetOutputPort())

stfMapper00 = vtk.vtkPolyDataMapper()
stfMapper00.SetInputConnection(stfGlyphs00.GetOutputPort())
stfMapper00.ScalarVisibilityOff()

stfActor00 = vtk.vtkActor()
stfActor00.SetMapper(stfMapper00)
stfActor00.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline00 = vtk.vtkOutlineFilter()
outline00.SetInputConnection(image.GetOutputPort())

outlineMapper00 = vtk.vtkPolyDataMapper()
outlineMapper00.SetInputConnection(outline00.GetOutputPort())

outlineActor00 = vtk.vtkActor()
outlineActor00.SetMapper(outlineMapper00)

# ======= 01
# Create a sphere tree and see what it look like
# (image sphere tree)
stf01 = vtk.vtkSphereTreeFilter()
stf01.SetInputConnection(image.GetOutputPort())
stf01.SetExtractionModeToLine()
stf01.SetPoint(0,0,0)
stf01.SetRay(1,1,1)

stfGlyphs01 = vtk.vtkGlyph3D()
stfGlyphs01.SetInputConnection(stf01.GetOutputPort())
stfGlyphs01.SetSourceConnection(sph.GetOutputPort())

stfMapper01 = vtk.vtkPolyDataMapper()
stfMapper01.SetInputConnection(stfGlyphs01.GetOutputPort())
stfMapper01.ScalarVisibilityOff()

stfActor01 = vtk.vtkActor()
stfActor01.SetMapper(stfMapper01)
stfActor01.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline01 = vtk.vtkOutlineFilter()
outline01.SetInputConnection(image.GetOutputPort())

outlineMapper01 = vtk.vtkPolyDataMapper()
outlineMapper01.SetInputConnection(outline01.GetOutputPort())

outlineActor01 = vtk.vtkActor()
outlineActor01.SetMapper(outlineMapper01)

# ======= 02
# Create a sphere tree and see what it look like
# (image sphere tree)
stf02 = vtk.vtkSphereTreeFilter()
stf02.SetInputConnection(image.GetOutputPort())
stf02.SetExtractionModeToPlane()
stf02.SetPoint(0,0,0)
stf02.SetNormal(1,1,1)

stfGlyphs02 = vtk.vtkGlyph3D()
stfGlyphs02.SetInputConnection(stf02.GetOutputPort())
stfGlyphs02.SetSourceConnection(sph.GetOutputPort())

stfMapper02 = vtk.vtkPolyDataMapper()
stfMapper02.SetInputConnection(stfGlyphs02.GetOutputPort())
stfMapper02.ScalarVisibilityOff()

stfActor02 = vtk.vtkActor()
stfActor02.SetMapper(stfMapper02)
stfActor02.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline02 = vtk.vtkOutlineFilter()
outline02.SetInputConnection(image.GetOutputPort())

outlineMapper02 = vtk.vtkPolyDataMapper()
outlineMapper02.SetInputConnection(outline02.GetOutputPort())

outlineActor02 = vtk.vtkActor()
outlineActor02.SetMapper(outlineMapper02)

# ======= 10
# Create a sphere tree and see what it look like
# (structured sphere tree)
stf10 = vtk.vtkSphereTreeFilter()
stf10.SetInputConnection(sgrid.GetOutputPort())
stf10.SetExtractionModeToPoint()
stf10.SetPoint(0,0,0)

stfGlyphs10 = vtk.vtkGlyph3D()
stfGlyphs10.SetInputConnection(stf10.GetOutputPort())
stfGlyphs10.SetSourceConnection(sph.GetOutputPort())

stfMapper10 = vtk.vtkPolyDataMapper()
stfMapper10.SetInputConnection(stfGlyphs10.GetOutputPort())
stfMapper10.ScalarVisibilityOff()

stfActor10 = vtk.vtkActor()
stfActor10.SetMapper(stfMapper10)
stfActor10.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline10 = vtk.vtkOutlineFilter()
outline10.SetInputConnection(sgrid.GetOutputPort())

outlineMapper10 = vtk.vtkPolyDataMapper()
outlineMapper10.SetInputConnection(outline10.GetOutputPort())

outlineActor10 = vtk.vtkActor()
outlineActor10.SetMapper(outlineMapper10)

# ======= 11
# Create a sphere tree and see what it look like
# (structured sphere tree)
stf11 = vtk.vtkSphereTreeFilter()
stf11.SetInputConnection(sgrid.GetOutputPort())
stf11.SetExtractionModeToLine()
stf11.SetPoint(0,0,0)
stf11.SetRay(1,1,1)

stfGlyphs11 = vtk.vtkGlyph3D()
stfGlyphs11.SetInputConnection(stf11.GetOutputPort())
stfGlyphs11.SetSourceConnection(sph.GetOutputPort())

stfMapper11 = vtk.vtkPolyDataMapper()
stfMapper11.SetInputConnection(stfGlyphs11.GetOutputPort())
stfMapper11.ScalarVisibilityOff()

stfActor11 = vtk.vtkActor()
stfActor11.SetMapper(stfMapper11)
stfActor11.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline11 = vtk.vtkOutlineFilter()
outline11.SetInputConnection(sgrid.GetOutputPort())

outlineMapper11 = vtk.vtkPolyDataMapper()
outlineMapper11.SetInputConnection(outline11.GetOutputPort())

outlineActor11 = vtk.vtkActor()
outlineActor11.SetMapper(outlineMapper11)

# ======= 12
# Create a sphere tree and see what it look like
# (structured sphere tree)
stf12 = vtk.vtkSphereTreeFilter()
stf12.SetInputConnection(sgrid.GetOutputPort())
stf12.SetExtractionModeToPlane()
stf12.SetPoint(0,0,0)
stf12.SetNormal(1,1,1)

stfGlyphs12 = vtk.vtkGlyph3D()
stfGlyphs12.SetInputConnection(stf12.GetOutputPort())
stfGlyphs12.SetSourceConnection(sph.GetOutputPort())

stfMapper12 = vtk.vtkPolyDataMapper()
stfMapper12.SetInputConnection(stfGlyphs12.GetOutputPort())
stfMapper12.ScalarVisibilityOff()

stfActor12 = vtk.vtkActor()
stfActor12.SetMapper(stfMapper12)
stfActor12.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline12 = vtk.vtkOutlineFilter()
outline12.SetInputConnection(sgrid.GetOutputPort())

outlineMapper12 = vtk.vtkPolyDataMapper()
outlineMapper12.SetInputConnection(outline12.GetOutputPort())

outlineActor12 = vtk.vtkActor()
outlineActor12.SetMapper(outlineMapper12)

# ======= 20
# Create a sphere tree and see what it look like
# (unstructured sphere tree)
stf20 = vtk.vtkSphereTreeFilter()
stf20.SetInputConnection(extract.GetOutputPort())
stf20.SetExtractionModeToPoint()
stf20.SetPoint(0,0,0)

stfGlyphs20 = vtk.vtkGlyph3D()
stfGlyphs20.SetInputConnection(stf20.GetOutputPort())
stfGlyphs20.SetSourceConnection(sph.GetOutputPort())

stfMapper20 = vtk.vtkPolyDataMapper()
stfMapper20.SetInputConnection(stfGlyphs20.GetOutputPort())
stfMapper20.ScalarVisibilityOff()

stfActor20 = vtk.vtkActor()
stfActor20.SetMapper(stfMapper20)
stfActor20.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline20 = vtk.vtkOutlineFilter()
outline20.SetInputConnection(extract.GetOutputPort())

outlineMapper20 = vtk.vtkPolyDataMapper()
outlineMapper20.SetInputConnection(outline20.GetOutputPort())

outlineActor20 = vtk.vtkActor()
outlineActor20.SetMapper(outlineMapper20)

# ======= 21
# Create a sphere tree and see what it look like
# (unstructured sphere tree)
stf21 = vtk.vtkSphereTreeFilter()
stf21.SetInputConnection(extract.GetOutputPort())
stf21.SetExtractionModeToLine()
stf21.SetPoint(0,0,0)
stf21.SetRay(1,1,1)

stfGlyphs21 = vtk.vtkGlyph3D()
stfGlyphs21.SetInputConnection(stf21.GetOutputPort())
stfGlyphs21.SetSourceConnection(sph.GetOutputPort())

stfMapper21 = vtk.vtkPolyDataMapper()
stfMapper21.SetInputConnection(stfGlyphs21.GetOutputPort())
stfMapper21.ScalarVisibilityOff()

stfActor21 = vtk.vtkActor()
stfActor21.SetMapper(stfMapper21)
stfActor21.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline21 = vtk.vtkOutlineFilter()
outline21.SetInputConnection(extract.GetOutputPort())

outlineMapper21 = vtk.vtkPolyDataMapper()
outlineMapper21.SetInputConnection(outline21.GetOutputPort())

outlineActor21 = vtk.vtkActor()
outlineActor21.SetMapper(outlineMapper21)

# ======= 22
# Create a sphere tree and see what it look like
# (unstructured sphere tree)
stf22 = vtk.vtkSphereTreeFilter()
stf22.SetInputConnection(extract.GetOutputPort())
stf22.SetExtractionModeToPlane()
stf22.SetPoint(0,0,0)
stf22.SetNormal(1,1,1)

stfGlyphs22 = vtk.vtkGlyph3D()
stfGlyphs22.SetInputConnection(stf22.GetOutputPort())
stfGlyphs22.SetSourceConnection(sph.GetOutputPort())

stfMapper22 = vtk.vtkPolyDataMapper()
stfMapper22.SetInputConnection(stfGlyphs22.GetOutputPort())
stfMapper22.ScalarVisibilityOff()

stfActor22 = vtk.vtkActor()
stfActor22.SetMapper(stfMapper22)
stfActor22.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline22 = vtk.vtkOutlineFilter()
outline22.SetInputConnection(extract.GetOutputPort())

outlineMapper22 = vtk.vtkPolyDataMapper()
outlineMapper22.SetInputConnection(outline22.GetOutputPort())

outlineActor22 = vtk.vtkActor()
outlineActor22.SetMapper(outlineMapper22)

# Add the actors to the renderer, set the background and size
#
ren00.AddActor(stfActor00)
ren00.AddActor(outlineActor00)
ren01.AddActor(stfActor01)
ren01.AddActor(outlineActor01)
ren02.AddActor(stfActor02)
ren02.AddActor(outlineActor02)
ren10.AddActor(stfActor10)
ren10.AddActor(outlineActor10)
ren11.AddActor(stfActor11)
ren11.AddActor(outlineActor11)
ren12.AddActor(stfActor12)
ren12.AddActor(outlineActor12)
ren20.AddActor(stfActor20)
ren20.AddActor(outlineActor20)
ren21.AddActor(stfActor21)
ren21.AddActor(outlineActor21)
ren22.AddActor(stfActor22)
ren22.AddActor(outlineActor22)

ren00.SetViewport(0,0,0.33,0.33);
ren01.SetViewport(0.33,0,0.67,0.33);
ren02.SetViewport(0.67,0,1,0.33);
ren10.SetViewport(0,0.33,0.33,0.67);
ren11.SetViewport(0.33,0.33,0.67,0.67);
ren12.SetViewport(0.67,0.33,1,0.67);
ren20.SetViewport(0.0,0.67,0.33,1);
ren21.SetViewport(0.33,0.67,0.67,1);
ren22.SetViewport(0.67,0.67,1,1);

ren00.SetBackground(0,0,0)
ren01.SetBackground(0,0,0)
ren02.SetBackground(0,0,0)
ren10.SetBackground(0,0,0)
ren11.SetBackground(0,0,0)
ren12.SetBackground(0,0,0)
ren20.SetBackground(0,0,0)
ren21.SetBackground(0,0,0)
ren22.SetBackground(0,0,0)

renWin.SetSize(450,450)
ren00.ResetCamera()
ren01.ResetCamera()
ren02.ResetCamera()
ren10.ResetCamera()
ren11.ResetCamera()
ren12.ResetCamera()
ren20.ResetCamera()
ren21.ResetCamera()
ren22.ResetCamera()
iren.Initialize()

renWin.Render()
#iren.Start()
# --- end of script --
