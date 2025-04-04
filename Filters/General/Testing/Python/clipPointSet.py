from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkPointSet
)
from vtkmodules.vtkFiltersGeneral import vtkTableBasedClipDataSet

# Create a sphere source
sphere_source = vtkSphereSource()
sphere_source.SetRadius(1.0)
sphere_source.SetThetaResolution(30)
sphere_source.SetPhiResolution(30)
sphere_source.Update()

# Convert PolyData to PointSet
poly = sphere_source.GetOutput()
pointset = vtkPointSet()
pointset.SetPoints(poly.GetPoints())

# Define a clipping plane
plane = vtkPlane()
plane.SetOrigin(0.0, 0.0, 0.0)  # Pass through the origin
plane.SetNormal(1.0, 0.0, 0.0)  # Clip along the X-axis

# Use vtkTableBasedClipDataSet to clip the point set
clipper = vtkTableBasedClipDataSet()
clipper.SetInputData(pointset)
clipper.SetClipFunction(plane)
clipper.SetInsideOut(False)  # Keep points on the positive side of the plane
clipper.Update()
output = clipper.GetOutput()

assert(output.GetNumberOfPoints() == 422)
assert(output.GetNumberOfCells() == 0)
