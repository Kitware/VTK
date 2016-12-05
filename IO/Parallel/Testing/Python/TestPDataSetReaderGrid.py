#!/usr/bin/env python

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# If the current directory is writable, then test the witers
#
if (catch.catch(globals(),"""channel = open("test.tmp", "w")""") == 0):
    channel.close()
    file.delete("-force", "test.tmp")
    # ====== Structured Grid ======
    # First save out a grid in parallel form.
    reader = vtk.vtkMultiBlockPLOT3DReader()
    reader.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
    reader.SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/combq.bin")
    reader.Update()
    contr = vtk.vtkDummyController()
    extract = vtk.vtkTransmitStructuredDataPiece()
    extract.SetController(contr)
    extract.SetInputData(reader.GetOutput().GetBlock(0))
    writer = vtk.vtkPDataSetWriter()
    writer.SetFileName("comb.pvtk")
    writer.SetInputConnection(extract.GetOutputPort())
    writer.SetNumberOfPieces(4)
    writer.Write()
    pReader = vtk.vtkPDataSetReader()
    pReader.SetFileName("comb.pvtk")
    surface = vtk.vtkDataSetSurfaceFilter()
    surface.SetInputConnection(pReader.GetOutputPort())
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(surface.GetOutputPort())
    mapper.SetNumberOfPieces(2)
    mapper.SetPiece(0)
    mapper.SetGhostLevel(1)
    mapper.Update()
    w = vtk.vtkDataSetWriter()
#    w.SetInputData(mapper.GetInput())
    w.SetInputData(surface.GetInput())
    w.SetFileName("foo.vtk")
    w.SetFileTypeToASCII()
    w.Write()
    file.delete("-force", "comb.pvtk")
    file.delete("-force", "comb.0.vtk")
    file.delete("-force", "comb.1.vtk")
    file.delete("-force", "comb.2.vtk")
    file.delete("-force", "comb.3.vtk")
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.SetPosition(-5,0,-29)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor)
    # ====== ImageData ======
    # First save out a grid in parallel form.
    fractal = vtk.vtkImageMandelbrotSource()
    fractal.SetWholeExtent(0,9,0,9,0,9)
    fractal.SetSampleCX(0.1,0.1,0.1,0.1)
    fractal.SetMaximumNumberOfIterations(10)
    extract2 = vtk.vtkTransmitStructuredDataPiece()
    extract2.SetController(contr)
    extract2.SetInputConnection(fractal.GetOutputPort())
    writer2 = vtk.vtkPDataSetWriter()
    writer.SetFileName("fractal.pvtk")
    writer.SetInputConnection(extract2.GetOutputPort())
    writer.SetNumberOfPieces(4)
    writer.Write()
    pReader2 = vtk.vtkPDataSetReader()
    pReader2.SetFileName("fractal.pvtk")
    iso = vtk.vtkContourFilter()
    iso.SetInputConnection(pReader2.GetOutputPort())
    iso.SetValue(0,4)
    mapper2 = vtk.vtkPolyDataMapper()
    mapper2.SetInputConnection(iso.GetOutputPort())
    mapper2.SetNumberOfPieces(3)
    mapper2.SetPiece(0)
    mapper2.SetGhostLevel(0)
    mapper2.Update()
    # Strip the ghost cells requested by the contour filter
    mapper2.GetInput().RemoveGhostCells()
    file.delete("-force", "fractal.pvtk")
    file.delete("-force", "fractal.0.vtk")
    file.delete("-force", "fractal.1.vtk")
    file.delete("-force", "fractal.2.vtk")
    file.delete("-force", "fractal.3.vtk")
    actor2 = vtk.vtkActor()
    actor2.SetMapper(mapper2)
    actor2.SetScale(5,5,5)
    actor2.SetPosition(6,6,6)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor2)
    # ====== PolyData ======
    # First save out a grid in parallel form.
    sphere = vtk.vtkSphereSource()
    sphere.SetRadius(2)
    writer3 = vtk.vtkPDataSetWriter()
    writer3.SetFileName("sphere.pvtk")
    writer3.SetInputConnection(sphere.GetOutputPort())
    writer3.SetNumberOfPieces(4)
    writer3.Write()
    pReader3 = vtk.vtkPDataSetReader()
    pReader3.SetFileName("sphere.pvtk")
    mapper3 = vtk.vtkPolyDataMapper()
    mapper3.SetInputConnection(pReader3.GetOutputPort())
    mapper3.SetNumberOfPieces(2)
    mapper3.SetPiece(0)
    mapper3.SetGhostLevel(1)
    mapper3.Update()
    file.delete("-force", "sphere.pvtk")
    file.delete("-force", "sphere.0.vtk")
    file.delete("-force", "sphere.1.vtk")
    file.delete("-force", "sphere.2.vtk")
    file.delete("-force", "sphere.3.vtk")
    actor3 = vtk.vtkActor()
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
    pass
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
# prevent the tk window from showing up then start the event loop
# --- end of script --
