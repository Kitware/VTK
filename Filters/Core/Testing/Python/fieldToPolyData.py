#!/usr/bin/env python
import os
from vtkmodules.vtkFiltersCore import (
    vtkDataObjectToDataSetFilter,
    vtkDataSetToDataObjectFilter,
    vtkFieldDataToAttributeDataFilter,
)
from vtkmodules.vtkIOLegacy import (
    vtkDataObjectReader,
    vtkDataObjectWriter,
    vtkPolyDataReader,
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

# This example demonstrates the reading of a field and conversion to PolyData
# The output should be the same as polyEx.tcl.

# Create a reader and write out the field
reader = vtkPolyDataReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/polyEx.vtk")

ds2do = vtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())

# NOTE: This test only works if the current directory is writable
#
try:
    channel = open("PolyField.vtk", "wb")
    channel.close()

    writer = vtkDataObjectWriter()
    writer.SetInputConnection(ds2do.GetOutputPort())
    writer.SetFileName("PolyField.vtk")
    writer.Write()

    # create pipeline
    #
    dor = vtkDataObjectReader()
    dor.SetFileName("PolyField.vtk")

    do2ds = vtkDataObjectToDataSetFilter()
    do2ds.SetInputConnection(dor.GetOutputPort())
    do2ds.SetDataSetTypeToPolyData()
    do2ds.SetPointComponent(0, "Points", 0)
    do2ds.SetPointComponent(1, "Points", 1)
    do2ds.SetPointComponent(2, "Points", 2)
    do2ds.SetPolysComponent("Polys", 0)

    fd2ad = vtkFieldDataToAttributeDataFilter()
    fd2ad.SetInputConnection(do2ds.GetOutputPort())
    fd2ad.SetInputFieldToDataObjectField()
    fd2ad.SetOutputAttributeDataToPointData()
    fd2ad.SetScalarComponent(0, "my_scalars", 0)

    mapper = vtkPolyDataMapper()
    mapper.SetInputConnection(fd2ad.GetOutputPort())
    mapper.SetScalarRange(fd2ad.GetOutput().GetScalarRange())

    actor = vtkActor()
    actor.SetMapper(mapper)

    # Create the RenderWindow, Renderer and both Actors
    ren1 = vtkRenderer()
    renWin = vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    ren1.AddActor(actor)
    ren1.SetBackground(1, 1, 1)

    renWin.SetSize(300, 300)

    ren1.ResetCamera()
    cam1 = ren1.GetActiveCamera()
    cam1.SetClippingRange(.348, 17.43)
    cam1.SetPosition(2.92, 2.62, -0.836)
    cam1.SetViewUp(-0.436, -0.067, -0.897)
    cam1.Azimuth(90)

    # render the image
    #
    renWin.Render()

    # cleanup
    #
    try:
        os.remove("PolyField.vtk")
    except OSError:
        pass


#    iren.Start()

except IOError:
    print("Couldn't open PolyField.vtk for writing.")
