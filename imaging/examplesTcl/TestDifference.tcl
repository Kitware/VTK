# append multiple displaced spheres into an RGB image.
catch {load vtktcl}
source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# Image pipeline
vtkImageCanvasSource2D canvas1
canvas1 SetNumberOfScalarComponents 3
canvas1 SetScalarType $VTK_UNSIGNED_CHAR
canvas1 SetExtent 0 511 0 511 0 0
canvas1 SetDrawColor 100 100 0
canvas1 FillBox 0 511 0 511
canvas1 SetDrawColor 200 0 200
canvas1 FillBox 32 511 100 500
canvas1 SetDrawColor 100 0 0
canvas1 FillTube 550 20 30 400 5

vtkImageCanvasSource2D canvas2
canvas2 SetNumberOfScalarComponents 3
canvas2 SetScalarType $VTK_UNSIGNED_CHAR
canvas2 SetExtent 0 511 0 511 0 0
canvas2 SetDrawColor 100 100 0
canvas2 FillBox 0 511 0 511
canvas2 SetDrawColor 200 0 200
canvas2 FillBox 32 511 100 500
canvas2 SetDrawColor 100 0 0
#canvas2 FillTube 550 20 30 400 5





vtkImageDifference diff
diff SetInput [canvas1 GetOutput]
diff SetImage [canvas2 GetOutput]

vtkImageViewer viewer
viewer SetInput [diff GetOutput]
viewer SetColorWindow 25
viewer SetColorLevel 1

# make interface
# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 512 -height 512 -iv viewer
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 







