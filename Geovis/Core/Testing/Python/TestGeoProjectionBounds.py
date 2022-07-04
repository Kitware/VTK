from vtkmodules.vtkGeovisCore import vtkGeoProjection, vtkGeoTransform
from vtkmodules.vtkCommonDataModel import vtkRectilinearGrid
from vtkmodules.vtkFiltersGeneral import vtkRectilinearGridToPointSet
from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkPoints
from vtk.numpy_interface import dataset_adapter as dsa
from math import pi, sqrt
import numpy as np

# construct a 2D rectilinear grid with x coordinates ranging
# between -PI and PI, and y coordinates ranging between -PI/2 and PI/2.
grid = vtkRectilinearGrid()
Nx = 36+1
Ny = 18+1
Nz = 1
grid.SetDimensions(Nx, Ny, Nz)

dims = [Nx, Ny, Nz]

xaxis = np.linspace(-pi, +pi, dims[0])
yaxis = np.linspace(-pi/2, +pi/2, dims[1])
zaxis = np.linspace(0., 0.0, dims[2])

grid.SetXCoordinates(dsa.numpyTovtkDataArray( xaxis , "X"))
grid.SetYCoordinates(dsa.numpyTovtkDataArray( yaxis , "Y"))
grid.SetZCoordinates(dsa.numpyTovtkDataArray( zaxis , "Z"))

# transform that grid  to a vtkStructuredGrid
grid2points = vtkRectilinearGridToPointSet()
grid2points.SetInputData(grid)
grid2points.Update()

in_Points = grid2points.GetOutput().GetPoints()
#apply a projection
destProj = vtkGeoProjection()
destProj.SetPROJ4String("+proj=moll +R=1")
# get a default source projection ("latlong")
sourceProj = vtkGeoProjection()
sourceProj.SetPROJ4String("+proj=latlong")

newPoints = vtkPoints()
transform = vtkGeoTransform()
transform.SetSourceProjection(sourceProj)
transform.SetDestinationProjection(destProj)
transform.TransformPoints(in_Points, newPoints)
transform.Update()
# verify that the bounds of the transformed set are:
# The x-coordinate has a range of [−2R√2, 2R√2],
# and the y-coordinate has a range of [−R√2, R√2]
# https://en.wikipedia.org/wiki/Mollweide_projection

R = 1 # see destProj's definition
val = sqrt(2)*R
xformedBounds = np.array([-2*val, 2*val, -val, val, 0.0, 0.0])

assert np.allclose(xformedBounds, np.asarray(newPoints.GetBounds()))
