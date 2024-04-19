#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonDataModel import vtkPlanes
from vtkmodules.vtkFiltersCore import (
    vtkPolyDataNormals,
    vtkTriangleFilter,
)
from vtkmodules.vtkFiltersTexture import vtkImplicitTextureCoords
from vtkmodules.vtkIOGeometry import vtkBYUReader
from vtkmodules.vtkIOLegacy import vtkStructuredPointsReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create cutting planes
planes = vtkPlanes()
points = vtkPoints()
norms = vtkFloatArray()

norms.SetNumberOfComponents(3)
points.InsertPoint(0, 0.0, 0.0, 0.0)
norms.InsertTuple3(0, 0.0, 0.0, 1.0)
points.InsertPoint(1, 0.0, 0.0, 0.0)
norms.InsertTuple3(1, -1.0, 0.0, 0.0)
planes.SetPoints(points)
planes.SetNormals(norms)

# texture
texReader = vtkStructuredPointsReader()
texReader.SetFileName(VTK_DATA_ROOT + "/Data/texThres2.vtk")
texture = vtkTexture()
texture.SetInputConnection(texReader.GetOutputPort())
texture.InterpolateOff()
texture.RepeatOff()

# read motor parts...each part colored separately
#
byu = vtkBYUReader()
byu.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu.SetPartNumber(1)

normals = vtkPolyDataNormals()
normals.SetInputConnection(byu.GetOutputPort())

tex1 = vtkImplicitTextureCoords()
tex1.SetInputConnection(normals.GetOutputPort())
tex1.SetRFunction(planes)
# tex1.FlipTextureOn()

byuMapper = vtkDataSetMapper()
byuMapper.SetInputConnection(tex1.GetOutputPort())

byuActor = vtkActor()
byuActor.SetMapper(byuMapper)
byuActor.SetTexture(texture)
byuActor.GetProperty().SetColor(GetRGBColor('cold_grey'))

byu2 = vtkBYUReader()
byu2.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu2.SetPartNumber(2)

normals2 = vtkPolyDataNormals()
normals2.SetInputConnection(byu2.GetOutputPort())

tex2 = vtkImplicitTextureCoords()
tex2.SetInputConnection(normals2.GetOutputPort())
tex2.SetRFunction(planes)
# tex2.FlipTextureOn()

byuMapper2 = vtkDataSetMapper()
byuMapper2.SetInputConnection(tex2.GetOutputPort())

byuActor2 = vtkActor()
byuActor2.SetMapper(byuMapper2)
byuActor2.SetTexture(texture)
byuActor2.GetProperty().SetColor(GetRGBColor('peacock'))

byu3 = vtkBYUReader()
byu3.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu3.SetPartNumber(3)

triangle3 = vtkTriangleFilter()
triangle3.SetInputConnection(byu3.GetOutputPort())

normals3 = vtkPolyDataNormals()
normals3.SetInputConnection(triangle3.GetOutputPort())

tex3 = vtkImplicitTextureCoords()
tex3.SetInputConnection(normals3.GetOutputPort())
tex3.SetRFunction(planes)
# tex3.FlipTextureOn()

byuMapper3 = vtkDataSetMapper()
byuMapper3.SetInputConnection(tex3.GetOutputPort())

byuActor3 = vtkActor()
byuActor3.SetMapper(byuMapper3)
byuActor3.SetTexture(texture)
byuActor3.GetProperty().SetColor(GetRGBColor('raw_sienna'))

byu4 = vtkBYUReader()
byu4.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu4.SetPartNumber(4)

normals4 = vtkPolyDataNormals()
normals4.SetInputConnection(byu4.GetOutputPort())

tex4 = vtkImplicitTextureCoords()
tex4.SetInputConnection(normals4.GetOutputPort())
tex4.SetRFunction(planes)
# tex4.FlipTextureOn()

byuMapper4 = vtkDataSetMapper()
byuMapper4.SetInputConnection(tex4.GetOutputPort())

byuActor4 = vtkActor()
byuActor4.SetMapper(byuMapper4)
byuActor4.SetTexture(texture)
byuActor4.GetProperty().SetColor(GetRGBColor('banana'))

byu5 = vtkBYUReader()
byu5.SetGeometryFileName(VTK_DATA_ROOT + "/Data/motor.g")
byu5.SetPartNumber(5)

normals5 = vtkPolyDataNormals()
normals5.SetInputConnection(byu5.GetOutputPort())

tex5 = vtkImplicitTextureCoords()
tex5.SetInputConnection(normals5.GetOutputPort())
tex5.SetRFunction(planes)
# tex5.FlipTextureOn()

byuMapper5 = vtkDataSetMapper()
byuMapper5.SetInputConnection(tex5.GetOutputPort())

byuActor5 = vtkActor()
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

camera = vtkCamera()
camera.SetFocalPoint(0.0286334, 0.0362996, 0.0379685)
camera.SetPosition(1.37067, 1.08629, -1.30349)
camera.SetViewAngle(17.673)
camera.SetClippingRange(1, 10)
camera.SetViewUp(-0.376306, -0.5085, -0.774482)
ren1.SetActiveCamera(camera)

# render the image
iren.Initialize()
#iren.Start()
