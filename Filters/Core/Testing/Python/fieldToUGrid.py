#!/usr/bin/env python
import os
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersCore import (
    vtkConnectivityFilter,
    vtkContourFilter,
    vtkDataObjectToDataSetFilter,
    vtkDataSetToDataObjectFilter,
    vtkFieldDataToAttributeDataFilter,
    vtkPolyDataNormals,
)
from vtkmodules.vtkFiltersGeneral import vtkWarpVector
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOLegacy import (
    vtkDataObjectReader,
    vtkDataObjectWriter,
    vtkUnstructuredGridReader,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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

# Read a field representing unstructured grid and display it (similar to blow.tcl)

# create a reader and write out field data
reader = vtkUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/blow.vtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")

ds2do = vtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())

# we must be able to write here
try:
    channel = open("UGridField.vtk", "wb")
    channel.close()

    write = vtkDataObjectWriter()
    write.SetInputConnection(ds2do.GetOutputPort())
    write.SetFileName("UGridField.vtk")
    write.Write()

    # Read the field and convert to unstructured grid.
    dor = vtkDataObjectReader()
    dor.SetFileName("UGridField.vtk")

    do2ds = vtkDataObjectToDataSetFilter()
    do2ds.SetInputConnection(dor.GetOutputPort())
    do2ds.SetDataSetTypeToUnstructuredGrid()
    do2ds.SetPointComponent(0, "Points", 0)
    do2ds.SetPointComponent(1, "Points", 1)
    do2ds.SetPointComponent(2, "Points", 2)
    do2ds.SetCellTypeComponent("CellTypes", 0)
    do2ds.SetCellConnectivityComponent("Cells", 0)
    do2ds.Update()

    fd2ad = vtkFieldDataToAttributeDataFilter()
    fd2ad.SetInputData(do2ds.GetUnstructuredGridOutput())
    fd2ad.SetInputFieldToDataObjectField()
    fd2ad.SetOutputAttributeDataToPointData()
    fd2ad.SetVectorComponent(0, "displacement9", 0)
    fd2ad.SetVectorComponent(1, "displacement9", 1)
    fd2ad.SetVectorComponent(2, "displacement9", 2)
    fd2ad.SetScalarComponent(0, "thickness9", 0)
    fd2ad.Update()

    # Now start visualizing
    warp = vtkWarpVector()
    warp.SetInputData(fd2ad.GetUnstructuredGridOutput())

    # extract mold from mesh using connectivity
    connect = vtkConnectivityFilter()
    connect.SetInputConnection(warp.GetOutputPort())
    connect.SetExtractionModeToSpecifiedRegions()
    connect.AddSpecifiedRegion(0)
    connect.AddSpecifiedRegion(1)

    moldMapper = vtkDataSetMapper()
    moldMapper.SetInputConnection(connect.GetOutputPort())
    moldMapper.ScalarVisibilityOff()

    moldActor = vtkActor()
    moldActor.SetMapper(moldMapper)
    moldActor.GetProperty().SetColor(.2, .2, .2)
    moldActor.GetProperty().SetRepresentationToWireframe()

    # extract parison from mesh using connectivity
    connect2 = vtkConnectivityFilter()
    connect2.SetInputConnection(warp.GetOutputPort())
    connect2.SetExtractionModeToSpecifiedRegions()
    connect2.AddSpecifiedRegion(2)

    parison = vtkGeometryFilter()
    parison.SetInputConnection(connect2.GetOutputPort())

    normals2 = vtkPolyDataNormals()
    normals2.SetInputConnection(parison.GetOutputPort())
    normals2.SetFeatureAngle(60)

    lut = vtkLookupTable()
    lut.SetHueRange(0.0, 0.66667)

    parisonMapper = vtkPolyDataMapper()
    parisonMapper.SetInputConnection(normals2.GetOutputPort())
    parisonMapper.SetLookupTable(lut)
    parisonMapper.SetScalarRange(0.12, 1.0)

    parisonActor = vtkActor()
    parisonActor.SetMapper(parisonMapper)

    cf = vtkContourFilter()
    cf.SetInputConnection(connect2.GetOutputPort())
    cf.SetValue(0, .5)

    contourMapper = vtkPolyDataMapper()
    contourMapper.SetInputConnection(cf.GetOutputPort())

    contours = vtkActor()
    contours.SetMapper(contourMapper)

    # Create graphics stuff
    ren1 = vtkRenderer()
    renWin = vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    # Add the actors to the renderer, set the background and size
    ren1.AddActor(moldActor)
    ren1.AddActor(parisonActor)
    ren1.AddActor(contours)

    ren1.ResetCamera()
    ren1.GetActiveCamera().Azimuth(60)
    ren1.GetActiveCamera().Roll(-90)
    ren1.GetActiveCamera().Dolly(2)
    ren1.ResetCameraClippingRange()
    ren1.SetBackground(1, 1, 1)

    renWin.SetSize(380, 200)
    renWin.SetMultiSamples(0)
    iren.Initialize()

    # render the image
    #
    renWin.Render()

    # cleanup
    #
    try:
        os.remove("UGridField.vtk")
    except OSError:
        pass

#    iren.Start()

except IOError:
    print("Couldn't open UGridField.vtk for writing.")
