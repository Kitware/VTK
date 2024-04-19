from vtkmodules.vtkGeovisCore import vtkGeoProjection

np = vtkGeoProjection.GetNumberOfProjections()
for i in range(np):
    print("Projection: {}\n\t{}".format(
        vtkGeoProjection.GetProjectionName(i),
        vtkGeoProjection.GetProjectionDescription(i)))
projName = "rouss"
proj = vtkGeoProjection()
proj.SetName(projName)
print("Projection: {}\n\t{}".format(projName, proj.GetDescription()))
