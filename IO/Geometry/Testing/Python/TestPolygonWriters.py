#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkCleanPolyData,
    vtkExtractEdges,
    vtkTriangleFilter,
    vtkWindowedSincPolyDataFilter,
)
from vtkmodules.vtkIOGeometry import (
    vtkBYUWriter,
    vtkCGMWriter,
    vtkIVWriter,
    vtkMCubesWriter,
    vtkSTLWriter,
)
from vtkmodules.vtkIOLegacy import (
    vtkDataSetWriter,
    vtkPolyDataReader,
    vtkPolyDataWriter,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
import os

VTK_DATA_ROOT = vtkGetDataRoot()

# seems should always be true
has_vtkIVWriter = True

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
input = vtkPolyDataReader()
input.SetFileName(VTK_DATA_ROOT + "/Data/brainImageSmooth.vtk")
#
# generate vectors
clean = vtkCleanPolyData()
clean.SetInputConnection(input.GetOutputPort())
smooth = vtkWindowedSincPolyDataFilter()
smooth.SetInputConnection(clean.GetOutputPort())
smooth.GenerateErrorVectorsOn()
smooth.GenerateErrorScalarsOn()
# Test with the Hamming window function, as vtkWindowedSincPolyDataFilter used for many years
smooth.SetWindowFunctionToHamming()
smooth.Update()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(smooth.GetOutputPort())
mapper.SetScalarRange(smooth.GetOutput().GetScalarRange())
brain = vtkActor()
brain.SetMapper(mapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(brain)
renWin.SetSize(320,240)
ren1.GetActiveCamera().SetPosition(149.653,-65.3464,96.0401)
ren1.GetActiveCamera().SetFocalPoint(146.003,22.3839,0.260541)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(-0.255578,-0.717754,-0.647695)
ren1.GetActiveCamera().SetClippingRange(79.2526,194.052)
iren.Initialize()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
#
# If the current directory is writable, then test the witers
#
if os.access(".", os.W_OK):
    # test the writers
    dsw = vtkDataSetWriter()
    dsw.SetInputConnection(smooth.GetOutputPort())
    dsw.SetFileName("brain.dsw")
    dsw.Write()
    os.remove("brain.dsw")
    pdw = vtkPolyDataWriter()
    pdw.SetInputConnection(smooth.GetOutputPort())
    pdw.SetFileName("brain.pdw")
    pdw.Write()
    os.remove("brain.pdw")
    if has_vtkIVWriter:
        iv = vtkIVWriter()
        iv.SetInputConnection(smooth.GetOutputPort())
        iv.SetFileName("brain.iv")
        iv.Write()
        os.remove("brain.iv")
    #
    # the next writers only handle triangles
    triangles = vtkTriangleFilter()
    triangles.SetInputConnection(smooth.GetOutputPort())
    if has_vtkIVWriter:
        iv2 = vtkIVWriter()
        iv2.SetInputConnection(triangles.GetOutputPort())
        iv2.SetFileName("brain2.iv")
        iv2.Write()
        os.remove("brain2.iv")
    if has_vtkIVWriter:
        edges = vtkExtractEdges()
        edges.SetInputConnection(triangles.GetOutputPort())
        iv3 = vtkIVWriter()
        iv3.SetInputConnection(edges.GetOutputPort())
        iv3.SetFileName("brain3.iv")
        iv3.Write()
        os.remove("brain3.iv")
    byu = vtkBYUWriter()
    byu.SetGeometryFileName("brain.g")
    byu.SetScalarFileName("brain.s")
    byu.SetDisplacementFileName("brain.d")
    byu.SetInputConnection(triangles.GetOutputPort())
    byu.Write()
    os.remove("brain.g")
    os.remove("brain.s")
    os.remove("brain.d")
    mcubes = vtkMCubesWriter()
    mcubes.SetInputConnection(triangles.GetOutputPort())
    mcubes.SetFileName("brain.tri")
    mcubes.SetLimitsFileName("brain.lim")
    mcubes.Write()
    os.remove("brain.lim")
    os.remove("brain.tri")
    stl = vtkSTLWriter()
    stl.SetInputConnection(triangles.GetOutputPort())
    stl.SetFileName("brain.stl")
    stl.Write()
    os.remove("brain.stl")
    stlBinary = vtkSTLWriter()
    stlBinary.SetInputConnection(triangles.GetOutputPort())
    stlBinary.SetFileName("brainBinary.stl")
    stlBinary.SetFileType(2)
    stlBinary.Write()
    os.remove("brainBinary.stl")
    cgm = vtkCGMWriter()
    cgm.SetInputConnection(triangles.GetOutputPort())
    cgm.SetFileName("brain.cgm")
    cgm.Write()
    os.remove("brain.cgm")
# --- end of script --
