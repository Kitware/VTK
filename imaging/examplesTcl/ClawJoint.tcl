catch {load vtktcl}
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
# Test the robot classes which draw 2d robots.

# Build the robot
vtkRobotSegment2D bodyLeft1
bodyLeft1 SetPointA -100 0
bodyLeft1 SetPointB -50 20
vtkRobotSegment2D bodyLeft2
bodyLeft2 SetPointA  0 0
bodyLeft2 SetPointB -50 20
vtkRobotGroup2D groupLeft
groupLeft AddRobot bodyLeft1
groupLeft AddRobot bodyLeft2

vtkRobotSegment2D bodyRight1
bodyRight1 SetPointA 100 0
bodyRight1 SetPointB 50 20
vtkRobotSegment2D bodyRight2
bodyRight2 SetPointA  0 0
bodyRight2 SetPointB 50 20
vtkRobotGroup2D groupRight
groupRight AddRobot bodyRight1
groupRight AddRobot bodyRight2

vtkRobotJoint2D joint
joint SetRobotA groupRight
joint SetRobotB groupLeft

vtkRobotTransform2D robot
robot SetRobot joint


vtkImageRobotSpace2D space
space SetWorkSpace canvas
space SetRobot robot
space SetNumberOfJoints 1
space AddJoint joint


# show start and goal.
space SetDrawValue 50
space DrawRobot 120 65 0 0
space DrawRobot 135 230 1.5708 0
vtkImageXViewer viewer
viewer SetInput [[space GetCanvas] GetOutput]
viewer Render




# Set up the path planner
vtkClaw claw
claw SetStateSpace space
claw SetStartState 120 65 0 0
claw SetGoalState 135 230 1.5708 0
claw SetGoalPercentage 80.0
claw DebugOn
#claw LoadPath "ClawJointSmooth.path"
claw GeneratePath
puts "Smoothing ------------------------------------------------"
claw DebugOff
#claw SmoothPath 20
#claw SavePath "ClawJointSmooth.path"
#exit
space AnimatePath claw
#exit


wm withdraw .





