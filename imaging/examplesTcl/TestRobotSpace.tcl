# Test the robot classes which draw 2d robots.


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




vtkImageDraw canvas;
canvas SetScalarType $VTK_SHORT
canvas SetExtent 0 511 0 511;
canvas SetDrawValue 0;
canvas FillBox 0 511 0 511;
canvas SetDrawValue 500;
canvas FillBox 100 411 100 411;
canvas SetDrawValue 0;
canvas FillBox 210 301 210 301;


# Build the robot
vtkRobotSegment2D bodyLeft;
bodyLeft SetPointA -100 0;
bodyLeft SetPointB 0 0;
vtkRobotSegment2D fingerLeft1;
fingerLeft1 SetPointA -100 0;
fingerLeft1 SetPointB -120 20;
vtkRobotSegment2D fingerLeft2;
fingerLeft2 SetPointA -100 0;
fingerLeft2 SetPointB -120 -20;
vtkRobotGroup2D groupLeft
groupLeft AddRobot bodyLeft
groupLeft AddRobot fingerLeft1
groupLeft AddRobot fingerLeft2

vtkRobotSegment2D bodyRight;
bodyRight SetPointA 100 0;
bodyRight SetPointB 0 0;
vtkRobotSegment2D fingerRight1;
fingerRight1 SetPointA 100 0;
fingerRight1 SetPointB 80 20;
vtkRobotSegment2D fingerRight2;
fingerRight2 SetPointA 100 0;
fingerRight2 SetPointB 80 -20;
vtkRobotGroup2D groupRight
groupRight AddRobot bodyRight
groupRight AddRobot fingerRight1
groupRight AddRobot fingerRight2

vtkRobotJoint2D robot;
robot SetRobotA groupLeft;
robot SetRobotB groupRight;

vtkRobotTransform2D robotActor;
robotActor SetRobot robot;
robotActor SetX 255;
robotActor SetY 255;





vtkImageRobotSpace2D space;
space SetWorkSpace canvas;
space SetRobot robotActor;
space SetNumberOfJoints 1;
space AddJoint robot;


space DrawRobot 300 190 0 0;
space PrintCollision 300 190 0 0;



puts [space GetRotationFactor];
puts [robot GetFactor];


vtkImageXViewer viewer;
viewer SetInput [[space GetCanvas] GetOutput];
viewer SetColorWindow 256
viewer SetColorLevel 128
#viewer DebugOn;



#make interface
#

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Robot;
scale .wl.f1.window -from -180 -to 180 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Joint;
scale .wl.f2.level -from -180 -to 180  -orient horizontal -command SetLevel


.wl.f1.window set 0
.wl.f2.level set 0


pack .wl -side left
pack .wl.f1 .wl.f2  -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left



proc SetWindow window {
   global viewer robotActor robot space;
   space ClearCanvas;
   robotActor SetTheta [expr $window * 0.017453293];
   canvas SetDrawValue 0;
   robotActor Draw [space GetCanvas];
   space PrintCollision 300 190 [robotActor GetTheta] [robot GetTheta];
   viewer Render;
}

proc SetLevel level {
   global viewer robotActor robot space;
   space ClearCanvas;
   robot SetTheta [expr $level * 0.017453293];
   canvas SetDrawValue 0;
   robotActor Draw [space GetCanvas];
   space PrintCollision 300 190 [robotActor GetTheta] [robot GetTheta];
   viewer Render;
}

puts "Done";


#$renWin Render
#wm withdraw .








