#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkFiltersParallel import vtkTransmitStructuredDataPiece
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOLegacy import vtkDataSetWriter
from vtkmodules.vtkIOParallel import (
    vtkMultiBlockPLOT3DReader,
    vtkPDataSetReader,
    vtkPDataSetWriter,
)
from vtkmodules.vtkImagingSources import vtkImageMandelbrotSource
from vtkmodules.vtkParallelCore import vtkDummyController
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

def DoPlot3DReaderTests(reader):
    # Ensure disable function works.
    reader.RemoveAllFunctions()
    reader.AddFunction(211) # vorticity magnitude.
    reader.Update()
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("VorticityMagnitude") is None:
        print("Failed to read `VorticityMagnitude`")
        sys.exit(1)
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("Velocity") is None:
        print("Failed to read `Velocity` to compute `VorticityMagnitude`")
        sys.exit(1)
    reader.RemoveAllFunctions()
    reader.Update()
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("VorticityMagnitude") is not None:
        print("Failed to not-read `VorticityMagnitude`")
        sys.exit(1)

    # Let's ensure intermediate results can be dropped.
    reader.PreserveIntermediateFunctionsOff()
    reader.AddFunction(211) # vorticity magnitude.
    reader.Update()
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("Velocity") is not None:
        print("PreserveIntermediateFunctionsOff is not working as expected.")
        sys.exit(1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# If the current directory is writable, then test the witers
#
if os.access(".", os.W_OK):
    # ====== Structured Grid ======
    # First save out a grid in parallel form.
    reader = vtkMultiBlockPLOT3DReader()
    reader.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
    reader.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
    reader.Update()

    # before we continue on with the test, let's quickly do some
    # vtkMultiBlockPLOT3DReader option tests.
    DoPlot3DReaderTests(reader)

    contr = vtkDummyController()
    extract = vtkTransmitStructuredDataPiece()
    extract.SetController(contr)
    extract.SetInputData(reader.GetOutput().GetBlock(0))
    writer = vtkPDataSetWriter()
    writer.SetFileName("comb.pvtk")
    writer.SetInputConnection(extract.GetOutputPort())
    writer.SetNumberOfPieces(4)
    writer.Write()
    pReader = vtkPDataSetReader()
    pReader.SetFileName("comb.pvtk")
    surface = vtkDataSetSurfaceFilter()
    surface.SetInputConnection(pReader.GetOutputPort())
    mapper = vtkPolyDataMapper()
    mapper.SetInputConnection(surface.GetOutputPort())
    mapper.SetNumberOfPieces(2)
    mapper.SetPiece(0)
    mapper.SetGhostLevel(1)
    mapper.Update()
    w = vtkDataSetWriter()
#    w.SetInputData(mapper.GetInput())
    w.SetInputData(surface.GetInput())
    w.SetFileName(os.path.join(".", "foo.vtk"))
    w.SetFileTypeToASCII()
    w.Write()
    os.remove("comb.pvtk")
    os.remove("comb.0.vtk")
    os.remove("comb.1.vtk")
    os.remove("comb.2.vtk")
    os.remove("comb.3.vtk")
    actor = vtkActor()
    actor.SetMapper(mapper)
    actor.SetPosition(-5,0,-29)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor)
    # ====== ImageData ======
    # First save out a grid in parallel form.
    fractal = vtkImageMandelbrotSource()
    fractal.SetWholeExtent(0,9,0,9,0,9)
    fractal.SetSampleCX(0.1,0.1,0.1,0.1)
    fractal.SetMaximumNumberOfIterations(10)
    extract2 = vtkTransmitStructuredDataPiece()
    extract2.SetController(contr)
    extract2.SetInputConnection(fractal.GetOutputPort())
    writer2 = vtkPDataSetWriter()
    writer.SetFileName("fractal.pvtk")
    writer.SetInputConnection(extract2.GetOutputPort())
    writer.SetNumberOfPieces(4)
    writer.Write()
    pReader2 = vtkPDataSetReader()
    pReader2.SetFileName("fractal.pvtk")
    iso = vtkContourFilter()
    iso.SetInputConnection(pReader2.GetOutputPort())
    iso.SetValue(0,4)
    mapper2 = vtkPolyDataMapper()
    mapper2.SetInputConnection(iso.GetOutputPort())
    mapper2.SetNumberOfPieces(3)
    mapper2.SetPiece(0)
    mapper2.SetGhostLevel(0)
    mapper2.Update()
    # Strip the ghost cells requested by the contour filter
    mapper2.GetInput().RemoveGhostCells()
    os.remove("fractal.pvtk")
    os.remove("fractal.0.vtk")
    os.remove("fractal.1.vtk")
    os.remove("fractal.2.vtk")
    os.remove("fractal.3.vtk")
    actor2 = vtkActor()
    actor2.SetMapper(mapper2)
    actor2.SetScale(5,5,5)
    actor2.SetPosition(6,6,6)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor2)
    # ====== PolyData ======
    # First save out a grid in parallel form.
    sphere = vtkSphereSource()
    sphere.SetRadius(2)
    writer3 = vtkPDataSetWriter()
    writer3.SetFileName("sphere.pvtk")
    writer3.SetInputConnection(sphere.GetOutputPort())
    writer3.SetNumberOfPieces(4)
    writer3.Write()
    pReader3 = vtkPDataSetReader()
    pReader3.SetFileName("sphere.pvtk")
    mapper3 = vtkPolyDataMapper()
    mapper3.SetInputConnection(pReader3.GetOutputPort())
    mapper3.SetNumberOfPieces(2)
    mapper3.SetPiece(0)
    mapper3.SetGhostLevel(1)
    mapper3.Update()
    os.remove("sphere.pvtk")
    os.remove("sphere.0.vtk")
    os.remove("sphere.1.vtk")
    os.remove("sphere.2.vtk")
    os.remove("sphere.3.vtk")
    actor3 = vtkActor()
    actor3.SetMapper(mapper3)
    actor3.SetPosition(6,6,6)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor3)

    # do some extra checking to make sure we have the proper number of cells
    if surface.GetOutput().GetNumberOfCells() != 4016:
        print("surface output should have 4016 cells but has %d" % surface.GetOutput().GetNumberOfCells())
        sys.exit(1)
    if iso.GetOutput().GetNumberOfCells() != 89:
        print("iso output should have 89 cells but has %d" % iso.GetOutput().GetNumberOfCells())
        sys.exit(1)
    if pReader3.GetOutput().GetNumberOfCells() != 48:
        print("pReader3 output should have 48 cells but has %d" % pReader3.GetOutput().GetNumberOfCells())
        sys.exit(1)

ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Azimuth(20)
cam1.Elevation(40)
ren1.ResetCamera()
cam1.Zoom(1.2)
iren.Initialize()
# --- end of script --
