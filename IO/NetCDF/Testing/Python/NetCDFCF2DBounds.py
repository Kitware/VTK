#!/usr/bin/env python
from GetReader import get_reader
from vtkmodules.vtkFiltersCore import vtkAssignAttribute
from vtkmodules.vtkFiltersExtraction import vtkExtractGrid
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

# This test checks netCDF CF reader for reading 2D bounds information.
renWin = vtkRenderWindow()
renWin.SetSize(400,200)
#############################################################################
# Case 1: Spherical coordinates off.
# Open the file.
reader_cartesian = get_reader(VTK_DATA_ROOT + "/Data/sampleCurveGrid4.nc")
# Set the arrays we want to load.
reader_cartesian.UpdateMetaData()
reader_cartesian.SetVariableArrayStatus("sample",1)
reader_cartesian.SphericalCoordinatesOff()

# Extract a region that is non-overlapping.
voi_cartesian = vtkExtractGrid()
voi_cartesian.SetInputConnection(reader_cartesian.GetOutputPort())
voi_cartesian.SetVOI(20,47,0,31,0,0)

# Assign the field to scalars.
aa_cartesian = vtkAssignAttribute()
aa_cartesian.SetInputConnection(voi_cartesian.GetOutputPort())
aa_cartesian.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_cartesian = vtkDataSetSurfaceFilter()
surface_cartesian.SetInputConnection(aa_cartesian.GetOutputPort())
mapper_cartesian = vtkPolyDataMapper()
mapper_cartesian.SetInputConnection(surface_cartesian.GetOutputPort())
mapper_cartesian.SetScalarRange(0,1535)
actor_cartesian = vtkActor()
actor_cartesian.SetMapper(mapper_cartesian)
ren_cartesian = vtkRenderer()
ren_cartesian.AddActor(actor_cartesian)
ren_cartesian.SetViewport(0.0,0.0,0.5,1.0)
renWin.AddRenderer(ren_cartesian)
#############################################################################
# Case 2: Spherical coordinates on.
# Open the file.
reader_spherical = get_reader(VTK_DATA_ROOT + "/Data/sampleCurveGrid4.nc")
# Set the arrays we want to load.
reader_spherical.UpdateMetaData()
reader_spherical.SetVariableArrayStatus("sample",1)
reader_spherical.SphericalCoordinatesOn()
# Assign the field to scalars.
aa_spherical = vtkAssignAttribute()
aa_spherical.SetInputConnection(reader_spherical.GetOutputPort())
aa_spherical.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_spherical = vtkDataSetSurfaceFilter()
surface_spherical.SetInputConnection(aa_spherical.GetOutputPort())
mapper_spherical = vtkPolyDataMapper()
mapper_spherical.SetInputConnection(surface_spherical.GetOutputPort())
mapper_spherical.SetScalarRange(0,1535)
actor_spherical = vtkActor()
actor_spherical.SetMapper(mapper_spherical)
ren_spherical = vtkRenderer()
ren_spherical.AddActor(actor_spherical)
ren_spherical.SetViewport(0.5,0.0,1.0,1.0)
renWin.AddRenderer(ren_spherical)
# --- end of script --
