catch {load vtktcl}
# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

set types  "UnsignedChar Char Short UnsignedShort Int UnsignedInt Long UnsignedLong Float Double"
set i 1
set j 0
set c 0
foreach vtkType $types {
  # create pipeline
  # reader reads slices
  vtkVolume16Reader v16$vtkType
    v16$vtkType SetDataDimensions 256 256
    v16$vtkType SetDataByteOrderToLittleEndian
    v16$vtkType SetFilePrefix "../../../vtkdata/fullHead/headsq"
    v16$vtkType SetDataSpacing 0.8 0.8 1.5
    v16$vtkType SetImageRange $i [expr $i + 9]
    v16$vtkType SetDataMask 0x7fff
    v16$vtkType SetDataOrigin 0 0 [expr $i * 1.5]

  incr i 9

  if { $j == 0 } {
    set value 1150; set j 1
  } else {
    set value 600; set j 0
  }

# write isosurface to file
  vtkSliceCubes mcubes$vtkType
    mcubes$vtkType SetReader v16$vtkType
    mcubes$vtkType SetValue $value
    mcubes$vtkType SetFileName "fullHead$vtkType.tri"
    mcubes$vtkType SetLimitsFileName "fullHead$vtkType.lim"
    mcubes$vtkType Update
}

set colors "$flesh $banana $grey $pink $carrot $gainsboro $tomato $gold $thistle $chocolate"
set i 1
set c 0
foreach vtkType $types {
  # read from file
  vtkMCubesReader reader$vtkType
    reader$vtkType SetFileName "fullHead$vtkType.tri"
    reader$vtkType SetLimitsFileName "fullHead$vtkType.lim"

  vtkPolyDataMapper mapper$vtkType
    mapper$vtkType SetInput [reader$vtkType GetOutput]
    mapper$vtkType ScalarVisibilityOff

  vtkActor head$vtkType
    head$vtkType SetMapper mapper$vtkType
    [head${vtkType} GetProperty] SetColor [lindex $colors $c] [lindex $colors [expr $c + 1]] [lindex $colors [expr $c + 2]]
  ren1 AddActor head$vtkType
  incr c 3
}

# Add the actors to the renderer, set the background and size
#
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Elevation 90

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

