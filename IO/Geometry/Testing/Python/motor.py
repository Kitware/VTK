#!/usr/bin/env python
import vtk
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create cutting planes
planes = vtk.vtkPlanes()
points = vtk.vtkPoints()
norms = vtk.vtkFloatArray()

norms.SetNumberOfComponents(3)
points.InsertPoint(0, 0.0, 0.0, 0.0)
norms.InsertTuple3(0, 0.0, 0.0, 1.0)
points.InsertPoint(1, 0.0, 0.0, 0.0)
norms.InsertTuple3(1, -1.0, 0.0, 0.0)
planes.SetPoints(points)
planes.SetNormals(norms)

# texture
texReader = vtk.vtkStructuredPointsReader()
texReader.SetFileName(VTK_DATA_ROOT + "/Data/texThres2.vtk")
texture = vtk.vtkTexture()
texture.SetInputConnection(texReader.GetOutputPort())
texture.InterpolateOff()
texture.RepeatOff()

# read motor parts...each part colored separately
#
byu = vtk.vtkBYUReader()
byu.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu.SetPartNumber(1)

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(byu.GetOutputPort())

tex1 = vtk.vtkImplicitTextureCoords()
tex1.SetInputConnection(normals.GetOutputPort())
tex1.SetRFunction(planes)
# tex1.FlipTextureOn()

byuMapper = vtk.vtkDataSetMapper()
byuMapper.SetInputConnection(tex1.GetOutputPort())

byuActor = vtk.vtkActor()
byuActor.SetMapper(byuMapper)
byuActor.SetTexture(texture)
byuActor.GetProperty().SetColor(GetRGBColor('cold_grey'))

byu2 = vtk.vtkBYUReader()
byu2.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu2.SetPartNumber(2)

normals2 = vtk.vtkPolyDataNormals()
normals2.SetInputConnection(byu2.GetOutputPort())

tex2 = vtk.vtkImplicitTextureCoords()
tex2.SetInputConnection(normals2.GetOutputPort())
tex2.SetRFunction(planes)
# tex2.FlipTextureOn()

byuMapper2 = vtk.vtkDataSetMapper()
byuMapper2.SetInputConnection(tex2.GetOutputPort())

byuActor2 = vtk.vtkActor()
byuActor2.SetMapper(byuMapper2)
byuActor2.SetTexture(texture)
byuActor2.GetProperty().SetColor(GetRGBColor('peacock'))

byu3 = vtk.vtkBYUReader()
byu3.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu3.SetPartNumber(3)

triangle3 = vtk.vtkTriangleFilter()
triangle3.SetInputConnection(byu3.GetOutputPort())

normals3 = vtk.vtkPolyDataNormals()
normals3.SetInputConnection(triangle3.GetOutputPort())

tex3 = vtk.vtkImplicitTextureCoords()
tex3.SetInputConnection(normals3.GetOutputPort())
tex3.SetRFunction(planes)
# tex3.FlipTextureOn()

byuMapper3 = vtk.vtkDataSetMapper()
byuMapper3.SetInputConnection(tex3.GetOutputPort())

byuActor3 = vtk.vtkActor()
byuActor3.SetMapper(byuMapper3)
byuActor3.SetTexture(texture)
byuActor3.GetProperty().SetColor(GetRGBColor('raw_sienna'))

byu4 = vtk.vtkBYUReader()
byu4.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu4.SetPartNumber(4)

normals4 = vtk.vtkPolyDataNormals()
normals4.SetInputConnection(byu4.GetOutputPort())

tex4 = vtk.vtkImplicitTextureCoords()
tex4.SetInputConnection(normals4.GetOutputPort())
tex4.SetRFunction(planes)
# tex4.FlipTextureOn()

byuMapper4 = vtk.vtkDataSetMapper()
byuMapper4.SetInputConnection(tex4.GetOutputPort())

byuActor4 = vtk.vtkActor()
byuActor4.SetMapper(byuMapper4)
byuActor4.SetTexture(texture)
byuActor4.GetProperty().SetColor(GetRGBColor('banana'))

byu5 = vtk.vtkBYUReader()
byu5.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu5.SetPartNumber(5)

normals5 = vtk.vtkPolyDataNormals()
normals5.SetInputConnection(byu5.GetOutputPort())

tex5 = vtk.vtkImplicitTextureCoords()
tex5.SetInputConnection(normals5.GetOutputPort())
tex5.SetRFunction(planes)
# tex5.FlipTextureOn()

byuMapper5 = vtk.vtkDataSetMapper()
byuMapper5.SetInputConnection(tex5.GetOutputPort())

byuActor5 = vtk.vtkActor()
byuActor5.SetMapper(byuMapper5)
byuActor5.SetTexture(texture)
byuActor5.GetProperty().SetColor(GetRGBColor('peach_puff'))

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(byuActor)
ren1.AddActor(byuActor2)
ren1.AddActor(byuActor3)
byuActor3.VisibilityOff()
ren1.AddActor(byuActor4)
ren1.AddActor(byuActor5)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

camera = vtk.vtkCamera()
camera.SetFocalPoint(0.0286334, 0.0362996, 0.0379685)
camera.SetPosition(1.37067, 1.08629, -1.30349)
camera.SetViewAngle(17.673)
camera.SetClippingRange(1, 10)
camera.SetViewUp(-0.376306, -0.5085, -0.774482)
ren1.SetActiveCamera(camera)

# render the image
iren.Initialize()
#iren.Start()
