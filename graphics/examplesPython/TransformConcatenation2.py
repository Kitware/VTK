#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# This example demonstrates how to set up flexible joints using 
# transform concatenation and vtkActor::SetUserTransform.
# The methods demonstrated here are preferable to those demonstrated
# in TransformConcatenation1.tcl.  

# You can use vtkAssembly to do the same thing, but this approach provides
# hands-on control over the orientation/position of each part relative to 
# the others i.e. you are not restricted to using vtkProp3D's methods for
# modifying the transform.

# create a rendering window and renderer
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)


# set up first set of polydata
c1 = vtkCylinderSource()
c1.SetHeight(1.6)
c1.SetRadius(0.2)
c1.SetCenter(0,0.8,0)

t1 = vtkTransform()

m1 = vtkDataSetMapper()
m1.SetInput(c1.GetOutput())

a1 = vtkActor()
a1.SetUserTransform(t1)
a1.SetMapper(m1)
a1.GetProperty().SetColor(1,0,0)


# set up second set, at a relative transform to the first
c2 = vtkCylinderSource()
c2.SetHeight(1.6)
c2.SetRadius(0.15)
c2.SetCenter(0,0.8,0)

# relative rotation for first joint
joint1 = vtkTransform()

# set input to initial transform
t2 = vtkTransform()
t2.SetInput(t1)
t2.Translate(0,1.6,0)
t2.Concatenate(joint1)

m2 = vtkDataSetMapper()
m2.SetInput(c2.GetOutput())

a2 = vtkActor()
a2.SetUserTransform(t2)
a2.SetMapper(m2)
a2.GetProperty().SetColor(0.0,0.7,1.0)


# set up third set, at a relative transform to the second
c3 = vtkCylinderSource()
c3.SetHeight(0.5)
c3.SetRadius(0.1)
c3.SetCenter(0,0.25,0)

# relative rotation
joint2 = vtkTransform()

# set input to previous transform
t3 = vtkTransform()
t3.SetInput(t2)
t3.Translate(0,1.6,0)
t3.Concatenate(joint2)

m3 = vtkDataSetMapper()
m3.SetInput(c3.GetOutput())

a3 = vtkActor()
a3.SetUserTransform(t3)
a3.SetMapper(m3)
a3.GetProperty().SetColor(0.9,0.9,0)

# combine actors into an assembly
# (this is just an option - you don't have to use an assembly)
# vtkAssembly assembly
# assembly AddPart a1
# assembly AddPart a2
# assembly AddPart a3

# add to renderer
# ren1 AddActor assembly

# You can add actors individually instead of using an assembly
ren.AddActor(a1)
ren.AddActor(a2)
ren.AddActor(a3)

# set clipping range
ren.ResetCamera(-1,1,-0.1,2,-3,3)

# set angles for first joint
phi2 = 70
theta2 = 85

# set angles for second joint
phi3 = 50
theta3 = 90

# create control procedures

def SetPhi2(x):
    global phi2,theta2

    phi2 = float(x)
    joint1.Identity()
    joint1.RotateY(phi2)
    joint1.RotateX(theta2)

    renWin.Render()
  

def SetTheta2(x):
    global phi2,theta2

    theta2 = float(x)
    joint1.Identity()
    joint1.RotateY(phi2)
    joint1.RotateX(theta2)

    renWin.Render()
    

def SetPhi3(x):
    global phi3,theta3

    phi3 = float(x)
    joint2.Identity()
    joint2.RotateY(phi3)
    joint2.RotateX(theta3)

    renWin.Render()
 

def SetTheta3(x):
    global phi3,theta3

    theta3 = float(x)
    joint2.Identity()
    joint2.RotateY(phi3)
    joint2.RotateX(theta3)

    renWin.Render()
    

from Tkinter import *

root = Tk()

w = Frame(root)

w.c2 = Frame(w)
w.c2.l2 = Label(w.c2,text="Joint #1")
w.c2.phi2 = Scale(w.c2,from_=-180,to=180,orient="horizontal",
                  command=SetPhi2)
w.c2.phi2.set(phi2)
w.c2.theta2 = Scale(w.c2,from_=-90,to=90,orient="horizontal",
                  command=SetTheta2)
w.c2.theta2.set(theta2)
w.c2.l2.pack(side="top")
w.c2.phi2.pack(side="top")
w.c2.theta2.pack(side="top")
w.c2.pack(side="left")

w.c3 = Frame(w)
w.c3.l3 = Label(w.c3,text="Joint #2")
w.c3.phi3 = Scale(w.c3,from_=-180,to=180,orient="horizontal",
                  command=SetPhi3)
w.c3.phi3.set(phi3)
w.c3.theta3 = Scale(w.c3,from_=-90,to=90,orient="horizontal",
                  command=SetTheta3)
w.c3.theta3.set(theta3)
w.c3.l3.pack(side="top")
w.c3.phi3.pack(side="top")
w.c3.theta3.pack(side="top")
w.c3.pack(side="left")

w.pack(side="top")

ex = Frame(root)
ex.exit = Button(ex,text="Exit",command=root.quit)
ex.exit.pack(side="top")
ex.pack(side="top")

root.mainloop()
