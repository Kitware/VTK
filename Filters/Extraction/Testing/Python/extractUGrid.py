#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonExecutionModel import vtkCastToConcrete
from vtkmodules.vtkFiltersCore import (
    vtkConnectivityFilter,
    vtkPolyDataNormals,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractUnstructuredGrid
from vtkmodules.vtkFiltersGeneral import vtkWarpVector
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOLegacy import vtkDataSetReader
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

# create reader and warp data with vectors
reader = vtkDataSetReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/blow.vtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")
castToUnstructuredGrid = vtkCastToConcrete()
castToUnstructuredGrid.SetInputConnection(reader.GetOutputPort())
castToUnstructuredGrid.Update()
warp = vtkWarpVector()
warp.SetInputData(castToUnstructuredGrid.GetUnstructuredGridOutput())
# extract mold from mesh using connectivity
connect = vtkConnectivityFilter()
connect.SetInputConnection(warp.GetOutputPort())
connect.SetExtractionModeToSpecifiedRegions()
connect.AddSpecifiedRegion(0)
connect.AddSpecifiedRegion(1)
moldMapper = vtkDataSetMapper()
moldMapper.SetInputConnection(reader.GetOutputPort())
moldMapper.ScalarVisibilityOff()
moldActor = vtkActor()
moldActor.SetMapper(moldMapper)
moldActor.GetProperty().SetColor(.2,.2,.2)
moldActor.GetProperty().SetRepresentationToWireframe()
# extract parison from mesh using connectivity
connect2 = vtkConnectivityFilter()
connect2.SetInputConnection(warp.GetOutputPort())
connect2.SetExtractionModeToSpecifiedRegions()
connect2.AddSpecifiedRegion(2)
extractGrid = vtkExtractUnstructuredGrid()
extractGrid.SetInputConnection(connect2.GetOutputPort())
extractGrid.CellClippingOn()
extractGrid.SetCellMinimum(0)
extractGrid.SetCellMaximum(23)
parison = vtkGeometryFilter()
parison.SetInputConnection(extractGrid.GetOutputPort())
normals2 = vtkPolyDataNormals()
normals2.SetInputConnection(parison.GetOutputPort())
normals2.SetFeatureAngle(60)
lut = vtkLookupTable()
lut.SetHueRange(0.0,0.66667)
parisonMapper = vtkPolyDataMapper()
parisonMapper.SetInputConnection(normals2.GetOutputPort())
parisonMapper.SetLookupTable(lut)
parisonMapper.SetScalarRange(0.12,1.0)
parisonActor = vtkActor()
parisonActor.SetMapper(parisonMapper)
# graphics stuff
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(parisonActor)
ren1.AddActor(moldActor)
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(60)
ren1.GetActiveCamera().Roll(-90)
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()
renWin.SetSize(500,380)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
