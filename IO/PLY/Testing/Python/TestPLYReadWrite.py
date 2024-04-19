#!/usr/bin/env python
import os
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersCore import (
    vtkPointDataToCellData,
    vtkSimpleElevationFilter,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOPLY import (
    vtkPLYReader,
    vtkPLYWriter,
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
VTK_DATA_ROOT = vtkGetDataRoot()

# NOTE: This test only works if the current directory is writable
#
try:
    filename = "plyWriter.ply"
    channel = open(filename, "wb")
    channel.close()

    ss = vtkSphereSource()
    ss.SetPhiResolution(10)
    ss.SetThetaResolution(20)

    ele = vtkSimpleElevationFilter()
    ele.SetInputConnection(ss.GetOutputPort())

    pd2cd = vtkPointDataToCellData()
    pd2cd.SetInputConnection(ele.GetOutputPort())

    # First way or writing
    w = vtkPLYWriter()
    w.SetInputConnection(pd2cd.GetOutputPort())
    w.SetFileName(filename)
    w.SetFileTypeToBinary()
    w.SetDataByteOrderToLittleEndian()
    w.SetColorModeToUniformCellColor()
    w.SetColor(255, 0, 0)
    w.Write()

    r = vtkPLYReader()
    r.SetFileName(filename)
    r.Update()

    # cleanup
    #
    try:
        os.remove(filename)
    except OSError:
        pass

    plyMapper = vtkPolyDataMapper()
    plyMapper.SetInputConnection(r.GetOutputPort())

    plyActor = vtkActor()
    plyActor.SetMapper(plyMapper)

    # Second way or writing - it will map through a lookup table
    lut = vtkLookupTable()
    lut.Build()

    w2 = vtkPLYWriter()
    w2.SetInputConnection(pd2cd.GetOutputPort())
    w2.SetFileName(filename)
    w2.SetFileTypeToBinary()
    w2.SetDataByteOrderToLittleEndian()
    w2.SetColorModeToDefault()
    w2.SetLookupTable(lut)
    w2.SetArrayName("Elevation")
    w2.SetComponent(0)
    w2.Write()

    r2 = vtkPLYReader()
    r2.SetFileName(filename)
    r2.Update()

    plyMapper2 = vtkPolyDataMapper()
    plyMapper2.SetInputConnection(r2.GetOutputPort())

    plyActor2 = vtkActor()
    plyActor2.SetMapper(plyMapper2)
    plyActor2.AddPosition(1, 0, 0)

    # Third way or writing - it will read the previous file with rgb cell color
    r3 = vtkPLYReader()
    r3.SetFileName(filename)
    r3.Update()

    w3 = vtkPLYWriter()
    w3.SetInputConnection(r3.GetOutputPort())
    w3.SetFileName(filename)
    w3.SetFileTypeToBinary()
    w3.SetDataByteOrderToLittleEndian()
    w3.SetColorModeToDefault()
    w3.SetArrayName("RGB")
    w3.SetComponent(0)
    w3.Write()

    r4 = vtkPLYReader()
    r4.SetFileName(filename)
    r4.Update()

    plyMapper3 = vtkPolyDataMapper()
    plyMapper3.SetInputConnection(r4.GetOutputPort())
    plyActor3 = vtkActor()
    plyActor3.SetMapper(plyMapper3)
    plyActor3.AddPosition(2, 0, 0)

    # cleanup
    #
    try:
        os.remove(filename)
    except OSError:
        pass

    # Create the RenderWindow, Renderer and both Actors
    #
    ren1 = vtkRenderer()
    renWin = vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(plyActor)
    ren1.AddActor(plyActor2)
    ren1.AddActor(plyActor3)

    renWin.SetSize(330, 120)

    iren.Initialize()
    renWin.Render()
    ren1.GetActiveCamera().Zoom(3.0)

#    iren.Start()

except IOError:
    print("Unable to test the writers.")
