#!/usr/bin/env python
#
# a modified version of COne.py that shows how to add callbacks, see Step1 for 
# comments on the basic pipeline setup
#

import sys

for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

from vtkpython import *

#
# define the callback
#
def myCallback(obj,string):
    print "Starting a render"


#
# create the basic pipeline as in Step1
#
cone = vtkConeSource()
cone.SetHeight( 3.0 )
cone.SetRadius( 1.0 )
cone.SetResolution( 10 )
  
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput( cone.GetOutput() )
coneActor = vtkActor()
coneActor.SetMapper( coneMapper )

ren1= vtkRenderer()
ren1.AddActor( coneActor )
ren1.SetBackground( 0.1, 0.2, 0.4 )

#
# Add the observer here
#
ren1.AddObserver("StartEvent",myCallback)

renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.SetSize( 300, 300 )

#
# now we loop over 360 degreeees and render the cone each time
#
for i in range(0,360):
    renWin.Render()
    ren1.GetActiveCamera().Azimuth( 1 )
  
#
# Free up any objects we created
#
cone = None
coneMapper = None
coneActor = None 
ren1 = None
renWin = None

sys.exit( 0 )
