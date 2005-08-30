#!/usr/bin/env python

# A game with VTK and Tkinter. :)

import Tkinter
import vtk
from vtk.tk.vtkTkRenderWindowInteractor import vtkTkRenderWindowInteractor

# Create the pipeline
puzzle = vtk.vtkSpherePuzzle()
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(puzzle.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)

arrows = vtk.vtkSpherePuzzleArrows()
mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(arrows.GetOutputPort())
actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)

renWin = vtk.vtkRenderWindow()
ren = vtk.vtkRenderer()
renWin.AddRenderer(ren)

# Add the actors to the renderer, set the background and size
ren.AddActor(actor)
ren.AddActor(actor2)
ren.SetBackground(0.1, 0.2, 0.4)

ren.ResetCamera()
cam = ren.GetActiveCamera()
cam.Elevation(-40)


## Generate the GUI
root = Tkinter.Tk()
root.withdraw()

# Define a quit method that exits cleanly.
def quit(obj=root):
    obj.quit()

# Create the toplevel window
top = Tkinter.Toplevel(root)
top.title("Sphere Puzzle")
top.protocol("WM_DELETE_WINDOW", quit)

# Create some frames
f1 = Tkinter.Frame(top)
f2 = Tkinter.Frame(top)

f1.pack(side="top", anchor="n", expand=1, fill="both")
f2.pack(side="bottom", anchor="s", expand="t", fill="x")

# Create the Tk render widget, and bind the events
rw = vtkTkRenderWindowInteractor(f1, width=400, height=400, rw=renWin)
rw.pack(expand="t", fill="both")

def reset(evt=None):
    puzzle.Reset()
    renWin.Render()

# Display some information
l1 = Tkinter.Label(f2, text="Position cursor over the rotation plane.")
l2 = Tkinter.Label(f2, text="Moving pieces will be highlighted.")
l3 = Tkinter.Label(f2, text="Press 'm' to make a move.")
reset = Tkinter.Button(f2, text="Reset", command=reset)

b1 = Tkinter.Button(f2, text="Quit", command=quit)

for i in (l1, l2, l3, reset, b1):
    i.pack(side="top", expand="t", fill="x")

# Done with the GUI.  Create callback functions.

in_piece_rotation = 0
LastVal = None

# Highlight pieces
def MotionCallback(obj, event):
    global in_piece_rotation
    global LastVal
    if in_piece_rotation:
        return 

    iren = renWin.GetInteractor()
    istyle = iren.GetInteractorStyle().GetCurrentStyle()

    # Return if the user is performing interaction
    if istyle.GetState():
        return
 
    # Get mouse position
    pos = iren.GetEventPosition()
    x, y = pos

    # Get world point
    ren.SetDisplayPoint(x, y, ren.GetZ(x, y))
    ren.DisplayToWorld()
    pt = ren.GetWorldPoint()
    val = puzzle.SetPoint(pt[0], pt[1], pt[2])

    if (not LastVal) or val != LastVal:
        renWin.Render()
        LastVal = val

# Rotate the puzzle
def CharCallback(obj, event):
    iren = renWin.GetInteractor()

    keycode = iren.GetKeyCode()
    if keycode != "m" and keycode != "M":
        return
    
    pos = iren.GetEventPosition()
    ButtonCallback(pos[0], pos[1])
 

def ButtonCallback(x, y):
    global in_piece_rotation
    if in_piece_rotation:
        return
    in_piece_rotation = 1

    # Get world point
    ren.SetDisplayPoint(x, y, ren.GetZ(x,y))
    ren.DisplayToWorld()
    pt = ren.GetWorldPoint()
    x, y, z = pt[:3]

    for i in range(0, 101, 10):
        puzzle.SetPoint(x, y, z)
        puzzle.MovePoint(i)
        renWin.Render()
        root.update()

    in_piece_rotation = 0
 
root.update()

# Modify some bindings, use the interactor style 'switch'
iren = renWin.GetInteractor()
istyle = vtk.vtkInteractorStyleSwitch()

iren.SetInteractorStyle(istyle)
istyle.SetCurrentStyleToTrackballCamera()

iren.AddObserver("MouseMoveEvent", MotionCallback)
iren.AddObserver("CharEvent", CharCallback)

# Shuffle the puzzle
ButtonCallback(218, 195)
ButtonCallback(261, 128)
ButtonCallback(213, 107)
ButtonCallback(203, 162)
ButtonCallback(134, 186)

iren.Initialize()
renWin.Render()
iren.Start()

root.mainloop()
