catch {load vtktcl}
# Simple viewer for images.


set sliceNumber 20
set sliceMax 127

set VTK_FLOAT              1
set VTK_INT                2
set VTK_SHORT              3
set VTK_UNSIGNED_SHORT     4
set VTK_UNSIGNED_CHAR      5

set VTK_IMAGE_X_AXIS             0
set VTK_IMAGE_Y_AXIS             1
set VTK_IMAGE_Z_AXIS             2
set VTK_IMAGE_TIME_AXIS          3
set VTK_IMAGE_COMPONENT_AXIS     4




# Image pipeline

# Make the WorkSpace
vtkImageDraw canvas
canvas SetScalarType $VTK_SHORT
canvas SetExtent 0 550 0 550
canvas SetDrawValue 0
canvas FillBox 0 550 0 550
canvas SetDrawValue 500
canvas FillBox 10 500 50 100
canvas FillBox 100 150 50 350
canvas FillBox 300 500 50 500


# Make the state space (and robot)
vtkImage2DRobotSpace space
space SetWorkSpace canvas
space SetNumberOfSegments 2
space AddSegment 52 -22 0 0
space AddSegment 0 0 -52 -22

# show start and goal.
#space SetDrawValue 50

#vtkImageXViewer viewer
#viewer SetInput [[space GetCanvas] GetOutput]
#viewer Render




# Set up the path planner
vtkClaw claw
claw SetStateSpace space
claw SetStartState 70 90 0
claw SetGoalState 140 290 1.5708
claw GeneratePath
puts "Smoothing ------------------------------------------------"
#claw DebugOn
claw SmoothPath 20
#claw SavePath "ClawRobotSmooth.path"

space AnimatePath claw
#exit


wm withdraw .





