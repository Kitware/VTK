#!/usr/local/bin/python

# A very basic medical image volume visualization tool

# When you run this example, type 'o' and 'c' to switch
# between object and camera interaction,  otherwise you
# will miss the full effect

try:
    from libVTKCommonPython import *
    from libVTKGraphicsPython import *
    from libVTKImagingPython import *
    from libVTKContribPython import *
except ImportError:
    from vtkpython import *

# read data file
# set the origin so that (0.0,0.0,0.0) is the center of the image
spacing = (1.0,1.0,2.0)
extent = (0,255,0,255,1,93)
origin = (-127.5,-127.5,-94.0)

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataSpacing(spacing)
reader.SetDataExtent(extent)
reader.SetDataOrigin(origin)
reader.SetFilePrefix('../../../vtkdata/fullHead/headsq')
reader.SetDataMask(0x7fff)
reader.UpdateWholeExtent()

# transforms for each slice actor
matrix = vtkMatrix4x4()
transform = vtkMatrixToLinearTransform()
transform.SetInput(matrix)

# slice extraction filter for each slice actor
reslice = vtkImageReslice()
reslice.SetInput(reader.GetOutput())
reslice.SetResliceTransform(transform)
reslice.InterpolateOn()
reslice.SetBackgroundLevel(1023)
reslice.SetOutputSpacing(spacing)
reslice.SetOutputOrigin((origin[0],origin[1],0.0))
reslice.SetOutputExtent((extent[0],extent[1],extent[2],extent[3],0,0))

# lookup table for textures
table = vtkLookupTable()
table.SetTableRange(100,2000)
table.SetSaturationRange(0,0)
table.SetHueRange(0,0)
table.SetValueRange(0,1)
table.Build()

# texture for each actor
atext = vtkTexture()
atext.SetInput(reslice.GetOutput())
atext.SetLookupTable(table)
atext.InterpolateOn()

# need a plane to texture map onto
plane = vtkPlaneSource()
plane.SetXResolution(1)
plane.SetYResolution(1)
plane.SetOrigin(origin[0]+spacing[0]*(extent[0] - 0.5), \
                origin[1]+spacing[1]*(extent[2] - 0.5), \
                0.0)
plane.SetPoint1(origin[0]+spacing[0]*(extent[1] + 0.5), \
                origin[1]+spacing[1]*(extent[2] - 0.5), \
                0.0)
plane.SetPoint2(origin[0]+spacing[0]*(extent[0] - 0.5), \
                origin[1]+spacing[1]*(extent[3] + 0.5), \
                0.0)

# these two aren't used for anything in this example
tmapper = vtkTextureMapToPlane()
tmapper.SetInput(plane.GetOutput())

# mapper for the textured plane
mapper = vtkDataSetMapper()
mapper.SetInput(tmapper.GetOutput())

# put everything together (note that the same transform
# is used for slice extraction and actor positioning)
actor = vtkActor()
actor.SetMapper(mapper)
actor.SetTexture(atext)
actor.SetUserMatrix(transform.GetMatrix())
actor.SetOrigin((0.0,0.0,0.0))

# Create rendering stuff
ren1 = vtkRenderer()

renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add a frame around the volume
#
frame = vtkOutlineFilter()
frame.SetInput(reader.GetOutput())

frameMapper = vtkPolyDataMapper()
frameMapper.SetInput(frame.GetOutput())

frameActor = vtkActor()
frameActor.SetMapper(frameMapper)
frameActor.GetProperty().SetColor(1.0000, 0.8431, 0.0000)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(frameActor)
ren1.SetBackground(1,1,1)

renWin.SetSize(500,500)

# Add the actors to the renderer, set the background and size
#
tmpTrans = vtkTransform()
tmpTrans.RotateX(10.0)
tmpTrans.RotateY(10.0)
tmpTrans.GetMatrix(matrix)

# render the image
#
renWin.Render()

iren.Start()




