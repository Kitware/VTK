#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkQuadricDecimation,
    vtkTriangleFilter,
)
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
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

# pipeline stuff
#
pl3d = vtkMultiBlockPLOT3DReader(file_name=VTK_DATA_ROOT + "/Data/combxyz.bin",
                                 q_file_name=VTK_DATA_ROOT + "/Data/combq.bin",
                                 scalar_function_number=100,
                                 vector_function_number=202)
#pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
#pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d_output = pl3d.update().output.GetBlock(0)
gf = vtkGeometryFilter(input_data=pl3d_output)
tf = vtkTriangleFilter()
gMapper = vtkPolyDataMapper()
gf >> tf >> gMapper
gMapper.scalar_range = pl3d_output.GetScalarRange()
gActor = vtkActor(mapper=gMapper)
# Don't look at attributes
mesh = vtkQuadricDecimation(target_reduction=0.90, attribute_error_metric=True)
mapper = vtkPolyDataMapper()
tf >> mesh >> mapper
actor = vtkActor(mapper=mapper)
# This time worry about attributes
mesh2 = vtkQuadricDecimation(target_reduction=.9, attribute_error_metric=False)
mapper2 = vtkPolyDataMapper()
tf >> mesh2 >> mapper2
actor2 = vtkActor(mapper=mapper2)
actor2.AddPosition(0,12,0)
# Create rendering instances
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Set up the camera parameters
#
camera = vtkCamera()
camera.SetPosition(19.34,6.128,-11.96)
camera.SetFocalPoint(8.25451,6.0,29.77)
camera.SetViewUp(0.9664,0.00605,0.256883)
camera.SetViewAngle(30)
camera.SetClippingRange(26,64)
ren1.SetActiveCamera(camera)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(actor2)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
threshold = 50
# --- end of script --
