#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
input = vtk.vtkPolyDataReader()
input.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/brainImageSmooth.vtk")
#
# generate vectors
clean = vtk.vtkCleanPolyData()
clean.SetInputConnection(input.GetOutputPort())
smooth = vtk.vtkWindowedSincPolyDataFilter()
smooth.SetInputConnection(clean.GetOutputPort())
smooth.GenerateErrorVectorsOn()
smooth.GenerateErrorScalarsOn()
smooth.Update()
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(smooth.GetOutputPort())
mapper.SetScalarRange(smooth.GetOutput().GetScalarRange())
brain = vtk.vtkActor()
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
if (catch.catch(globals(),"""channel = open("test.tmp", "w")""") == 0):
    channel.close()
    file.delete("-force", "test.tmp")
    #
    #
    # test the writers
    dsw = vtk.vtkDataSetWriter()
    dsw.SetInputConnection(smooth.GetOutputPort())
    dsw.SetFileName("brain.dsw")
    dsw.Write()
    file.delete("-force", "brain.dsw")
    pdw = vtk.vtkPolyDataWriter()
    pdw.SetInputConnection(smooth.GetOutputPort())
    pdw.SetFileName("brain.pdw")
    pdw.Write()
    file.delete("-force", "brain.pdw")
    if (info.command(globals(), locals(),  "vtkIVWriter") != ""):
        iv = vtk.vtkIVWriter()
        iv.SetInputConnection(smooth.GetOutputPort())
        iv.SetFileName("brain.iv")
        iv.Write()
        file.delete("-force", "brain.iv")
        pass
    #
    # the next writers only handle triangles
    triangles = vtk.vtkTriangleFilter()
    triangles.SetInputConnection(smooth.GetOutputPort())
    if (info.command(globals(), locals(),  "vtkIVWriter") != ""):
        iv2 = vtk.vtkIVWriter()
        iv2.SetInputConnection(triangles.GetOutputPort())
        iv2.SetFileName("brain2.iv")
        iv2.Write()
        file.delete("-force", "brain2.iv")
        pass
    if (info.command(globals(), locals(),  "vtkIVWriter") != ""):
        edges = vtk.vtkExtractEdges()
        edges.SetInputConnection(triangles.GetOutputPort())
        iv3 = vtk.vtkIVWriter()
        iv3.SetInputConnection(edges.GetOutputPort())
        iv3.SetFileName("brain3.iv")
        iv3.Write()
        file.delete("-force", "brain3.iv")
        pass
    byu = vtk.vtkBYUWriter()
    byu.SetGeometryFileName("brain.g")
    byu.SetScalarFileName("brain.s")
    byu.SetDisplacementFileName("brain.d")
    byu.SetInputConnection(triangles.GetOutputPort())
    byu.Write()
    file.delete("-force", "brain.g")
    file.delete("-force", "brain.s")
    file.delete("-force", "brain.d")
    mcubes = vtk.vtkMCubesWriter()
    mcubes.SetInputConnection(triangles.GetOutputPort())
    mcubes.SetFileName("brain.tri")
    mcubes.SetLimitsFileName("brain.lim")
    mcubes.Write()
    file.delete("-force", "brain.lim")
    file.delete("-force", "brain.tri")
    stl = vtk.vtkSTLWriter()
    stl.SetInputConnection(triangles.GetOutputPort())
    stl.SetFileName("brain.stl")
    stl.Write()
    file.delete("-force", "brain.stl")
    stlBinary = vtk.vtkSTLWriter()
    stlBinary.SetInputConnection(triangles.GetOutputPort())
    stlBinary.SetFileName("brainBinary.stl")
    stlBinary.SetFileType(2)
    stlBinary.Write()
    file.delete("-force", "brainBinary.stl")
    cgm = vtk.vtkCGMWriter()
    cgm.SetInputConnection(triangles.GetOutputPort())
    cgm.SetFileName("brain.cgm")
    cgm.Write()
    file.delete("-force", "brain.cgm")
    pass
# --- end of script --
