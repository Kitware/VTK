catch {load vtktcl}
# demonstrates a pipeline loop.

# user interface command widget
source ../../examplesTcl/vtkInt.tcl



vtkSphereSource sphere
    sphere SetThetaResolution 12
    sphere SetPhiResolution 12

vtkShrinkFilter shrink
    shrink SetInput [sphere GetOutput]
    shrink SetShrinkFactor 0.95

vtkElevationFilter colorIt
    colorIt SetInput [shrink GetOutput]
    colorIt SetLowPoint 0 0 -.5
    colorIt SetHighPoint 0 0 .5

vtkDataSetMapper mapper
    mapper SetInput [colorIt GetOutput]

vtkActor actor
    actor SetMapper mapper


# prevent the tk window from showing up then start the event loop
# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
  ren1 SetBackground 1 1 1
  renWin SetSize 300 300  
  


#execute first time
renWin Render 

# create the loop
shrink  SetInput [colorIt GetOutput]
# begin looping

for {set i 0} {$i < 20} {incr i} {
   renWin Render
}




# enable user interface interactor
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


