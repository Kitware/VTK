package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Generate some random colors
proc MakeColors {lut n } {
   catch {vtkMath math}
   $lut SetNumberOfColors $n
   $lut SetTableRange 0 [expr $n-1]
   $lut SetScaleToLinear
   $lut Build
   $lut SetTableValue 0 0 0 0 1
   math RandomSeed 5071
   for {set i 1} {$i < $n } {incr i} {
     $lut  SetTableValue $i [math Random .2 1]  [math Random .2 1]  [math Random .2 1]  1
   }
}

vtkLookupTable lut
MakeColors lut 256



set n 20
set radius 10

# This has been moved outside the loop so that the code can be correctly
# translated to python
catch {vtkImageData blobImage}
for {set i 0} {$i < $n} {incr i} {
  catch {vtkSphere sphere}
    sphere SetRadius $radius
    set max [expr 50 - $radius]
    sphere SetCenter [expr int ( [math Random -$max $max] ) ] [expr int ( [math Random -$max $max] ) ] [expr int ( [math Random -$max $max] ) ]

  catch {vtkSampleFunction sampler}
    sampler SetImplicitFunction sphere
    sampler SetOutputScalarTypeToFloat
    sampler SetSampleDimensions 51 51 51
    sampler SetModelBounds -50 50 -50 50 -50 50

  catch {vtkImageThreshold thres}
    thres SetInputConnection [sampler GetOutputPort]
    thres ThresholdByLower [expr $radius * $radius]
    thres ReplaceInOn
    thres ReplaceOutOn
    thres SetInValue [expr $i + 1]
    thres SetOutValue 0
    thres Update

  if {$i == 0} {
      blobImage DeepCopy [thres GetOutput]
  }

  catch {vtkImageMathematics maxValue}
    maxValue SetInput 0 blobImage
    maxValue SetInput 1 [thres GetOutput]
    maxValue SetOperationToMax
    maxValue Modified
    maxValue Update

  blobImage DeepCopy [maxValue GetOutput]
}

vtkDiscreteMarchingCubes discrete
  discrete SetInput blobImage
  discrete GenerateValues $n 1 $n

vtkPolyDataMapper mapper
  mapper SetInputConnection [discrete GetOutputPort]
  mapper SetLookupTable lut
  mapper SetScalarRange 0 [lut GetNumberOfColors]

vtkActor actor
  actor SetMapper mapper

ren1 AddActor actor

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


