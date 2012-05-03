#!/usr/bin/env python

# This code is a direct translation of the Tcl code in
# ImagePlaneWidget.tcl.  It could easily be written using a nice class
# to do the job but the present code should definitely make for an
# illustrative example.

# This example demonstrates how to use the vtkImagePlaneWidget
# to probe a 3D image dataset with three orthogonal planes.
# Buttons are provided to:
# a) capture the render window display to a tiff file
# b) x,y,z buttons reset the widget to orthonormal
#    positioning, set the horizontal slider to move the
#    associated widget along its normal, and set the
#    camera to face the widget
# c) right clicking on x,y,z buttons pops up a menu to set
#    the associated widget's reslice interpolation mode

import vtk
import Tkinter
from vtk.tk.vtkTkRenderWindowInteractor import \
     vtkTkRenderWindowInteractor
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Start by loading some data.
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64, 64)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
v16.SetImageRange(1, 93)
v16.SetDataSpacing(3.2, 3.2, 1.5)
v16.Update()

xMin, xMax, yMin, yMax, zMin, zMax = v16.GetExecutive().GetWholeExtent(v16.GetOutputInformation(0))

spacing = v16.GetOutput().GetSpacing()
sx, sy, sz = spacing

origin = v16.GetOutput().GetOrigin()
ox, oy, oz = origin

# An outline is shown for context.
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# The shared picker enables us to use 3 planes at one time
# and gets the picking order right
picker = vtk.vtkCellPicker()
picker.SetTolerance(0.005)

# The 3 image plane widgets are used to probe the dataset.
planeWidgetX = vtk.vtkImagePlaneWidget()
planeWidgetX.DisplayTextOn()
planeWidgetX.SetInputConnection(v16.GetOutputPort())
planeWidgetX.SetPlaneOrientationToXAxes()
planeWidgetX.SetSliceIndex(32)
planeWidgetX.SetPicker(picker)
planeWidgetX.SetKeyPressActivationValue("x")
prop1 = planeWidgetX.GetPlaneProperty()
prop1.SetColor(1, 0, 0)

planeWidgetY = vtk.vtkImagePlaneWidget()
planeWidgetY.DisplayTextOn()
planeWidgetY.SetInputConnection(v16.GetOutputPort())
planeWidgetY.SetPlaneOrientationToYAxes()
planeWidgetY.SetSliceIndex(32)
planeWidgetY.SetPicker(picker)
planeWidgetY.SetKeyPressActivationValue("y")
prop2 = planeWidgetY.GetPlaneProperty()
prop2.SetColor(1, 1, 0)
planeWidgetY.SetLookupTable(planeWidgetX.GetLookupTable())

# for the z-slice, turn off texture interpolation:
# interpolation is now nearest neighbour, to demonstrate
# cross-hair cursor snapping to pixel centers
planeWidgetZ = vtk.vtkImagePlaneWidget()
planeWidgetZ.DisplayTextOn()
planeWidgetZ.SetInputConnection(v16.GetOutputPort())
planeWidgetZ.SetPlaneOrientationToZAxes()
planeWidgetZ.SetSliceIndex(46)
planeWidgetZ.SetPicker(picker)
planeWidgetZ.SetKeyPressActivationValue("z")
prop3 = planeWidgetZ.GetPlaneProperty()
prop3.SetColor(0, 0, 1)
planeWidgetZ.SetLookupTable(planeWidgetX.GetLookupTable())

# Create the RenderWindow and Renderer
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

# Add the outline actor to the renderer, set the background color and size
ren.AddActor(outlineActor)
renWin.SetSize(600, 600)
ren.SetBackground(0.1, 0.1, 0.2)

current_widget = planeWidgetZ
mode_widget = planeWidgetZ

# Create the GUI
# We first create the supporting functions (callbacks) for the GUI
#
# Align the camera so that it faces the desired widget
def AlignCamera():
    #global ox, oy, oz, sx, sy, sz, xMax, xMin, yMax, yMin, zMax, \
    #      zMin, slice_number
    #global current_widget
    cx = ox+(0.5*(xMax-xMin))*sx
    cy = oy+(0.5*(yMax-yMin))*sy
    cz = oy+(0.5*(zMax-zMin))*sz
    vx, vy, vz = 0, 0, 0
    nx, ny, nz = 0, 0, 0
    iaxis = current_widget.GetPlaneOrientation()
    if iaxis == 0:
        vz = -1
        nx = ox + xMax*sx
        cx = ox + slice_number*sx
    elif iaxis == 1:
        vz = -1
        ny = oy+yMax*sy
        cy = oy+slice_number*sy
    else:
        vy = 1
        nz = oz+zMax*sz
        cz = oz+slice_number*sz

    px = cx+nx*2
    py = cy+ny*2
    pz = cz+nz*3

    camera = ren.GetActiveCamera()
    camera.SetViewUp(vx, vy, vz)
    camera.SetFocalPoint(cx, cy, cz)
    camera.SetPosition(px, py, pz)
    camera.OrthogonalizeViewUp()
    ren.ResetCameraClippingRange()
    renWin.Render()

# Capture the display and place in a tiff
def CaptureImage():
    w2i = vtk.vtkWindowToImageFilter()
    writer = vtk.vtkTIFFWriter()
    w2i.SetInput(renWin)
    w2i.Update()
    writer.SetInputConnection(w2i.GetOutputPort())
    writer.SetFileName("image.tif")
    renWin.Render()
    writer.Write()


# Align the widget back into orthonormal position,
# set the slider to reflect the widget's position,
# call AlignCamera to set the camera facing the widget
def AlignXaxis():
    global xMax, xMin, current_widget, slice_number
    po = planeWidgetX.GetPlaneOrientation()
    if po == 3:
        planeWidgetX.SetPlaneOrientationToXAxes()
        slice_number = (xMax-xMin)/2
        planeWidgetX.SetSliceIndex(slice_number)
    else:
        slice_number = planeWidgetX.GetSliceIndex()

    current_widget = planeWidgetX

    slice.config(from_=xMin, to=xMax)
    slice.set(slice_number)
    AlignCamera()


def AlignYaxis():
    global yMin, yMax, current_widget, slice_number
    po = planeWidgetY.GetPlaneOrientation()
    if po == 3:
        planeWidgetY.SetPlaneOrientationToYAxes()
        slice_number = (yMax-yMin)/2
        planeWidgetY.SetSliceIndex(slice_number)
    else:
        slice_number = planeWidgetY.GetSliceIndex()

    current_widget = planeWidgetY

    slice.config(from_=yMin, to=yMax)
    slice.set(slice_number)
    AlignCamera()

def AlignZaxis():
    global yMin, yMax, current_widget, slice_number
    po = planeWidgetZ.GetPlaneOrientation()
    if po == 3:
        planeWidgetZ.SetPlaneOrientationToZAxes()
        slice_number = (zMax-zMin)/2
        planeWidgetZ.SetSliceIndex(slice_number)
    else:
        slice_number = planeWidgetZ.GetSliceIndex()

    current_widget = planeWidgetZ

    slice.config(from_=zMin, to=zMax)
    slice.set(slice_number)
    AlignCamera()


# Set the widget's reslice interpolation mode
# to the corresponding popup menu choice
def SetInterpolation():
    global mode_widget, mode
    if mode.get() == 0:
        mode_widget.TextureInterpolateOff()
    else:
        mode_widget.TextureInterpolateOn()

    mode_widget.SetResliceInterpolate(mode.get())
    renWin.Render()

# Share the popup menu among buttons, keeping track of associated
# widget's interpolation mode
def buttonEvent(event, arg=None):
    global mode, mode_widget, popm
    if arg == 0:
        mode_widget = planeWidgetX
    elif arg == 1:
        mode_widget = planeWidgetY
    elif arg == 2:
        mode_widget = planeWidgetZ
    else:
        return
    mode.set(mode_widget.GetResliceInterpolate())
    popm.entryconfigure(arg, variable=mode)
    popm.post(event.x + event.x_root, event.y + event.y_root)

def SetSlice(sl):
    global current_widget
    current_widget.SetSliceIndex(int(sl))
    ren.ResetCameraClippingRange()
    renWin.Render()


###
# Now actually create the GUI
root = Tkinter.Tk()
root.withdraw()
top = Tkinter.Toplevel(root)

# Define a quit method that exits cleanly.
def quit(obj=root):
    obj.quit()

# Popup menu
popm = Tkinter.Menu(top, tearoff=0)
mode = Tkinter.IntVar()
mode.set(1)
popm.add_radiobutton(label="nearest", variable=mode, value=0,
                     command=SetInterpolation)
popm.add_radiobutton(label="linear", variable=mode, value=1,
                     command=SetInterpolation)
popm.add_radiobutton(label="cubic", variable=mode, value=2,
                     command=SetInterpolation)

display_frame = Tkinter.Frame(top)
display_frame.pack(side="top", anchor="n", fill="both", expand="false")

# Buttons
ctrl_buttons = Tkinter.Frame(top)
ctrl_buttons.pack(side="top", anchor="n", fill="both", expand="false")

quit_button = Tkinter.Button(ctrl_buttons, text="Quit", command=quit)
capture_button = Tkinter.Button(ctrl_buttons, text="Tif",
                                command=CaptureImage)

x_button = Tkinter.Button(ctrl_buttons, text="x", command=AlignXaxis)
y_button = Tkinter.Button(ctrl_buttons, text="y", command=AlignYaxis)
z_button = Tkinter.Button(ctrl_buttons, text="z", command=AlignZaxis)
x_button.bind("<Button-3>", lambda e: buttonEvent(e, 0))
y_button.bind("<Button-3>", lambda e: buttonEvent(e, 1))
z_button.bind("<Button-3>", lambda e: buttonEvent(e, 2))

for i in (quit_button, capture_button, x_button, y_button, z_button):
    i.pack(side="left", expand="true", fill="both")


# Create the render widget
renderer_frame = Tkinter.Frame(display_frame)
renderer_frame.pack(padx=3, pady=3,side="left", anchor="n",
                    fill="both", expand="false")

render_widget = vtkTkRenderWindowInteractor(renderer_frame,
                                            rw=renWin, width=600,
                                            height=600)
for i in (render_widget, display_frame):
    i.pack(side="top", anchor="n",fill="both", expand="false")

# Add a slice scale to browse the current slice stack
slice_number = Tkinter.IntVar()
slice_number.set(current_widget.GetSliceIndex())
slice = Tkinter.Scale(top, from_=zMin, to=zMax, orient="horizontal",
                      command=SetSlice,variable=slice_number,
                      label="Slice")
slice.pack(fill="x", expand="false")

# Done with the GUI.
###

# Set the interactor for the widgets
iact = render_widget.GetRenderWindow().GetInteractor()
planeWidgetX.SetInteractor(iact)
planeWidgetX.On()
planeWidgetY.SetInteractor(iact)
planeWidgetY.On()
planeWidgetZ.SetInteractor(iact)
planeWidgetZ.On()

# Create an initial interesting view
cam1 = ren.GetActiveCamera()
cam1.Elevation(110)
cam1.SetViewUp(0, 0, -1)
cam1.Azimuth(45)
ren.ResetCameraClippingRange()

# Render it
render_widget.Render()

iact.Initialize()
renWin.Render()
iact.Start()

# Start Tkinter event loop
root.mainloop()
