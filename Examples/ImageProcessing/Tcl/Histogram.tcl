package require vtk
package require vtkinteraction

source HistogramWidget.tcl

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#source TkInteractor.tcl

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

# named viewer2 so regresion test will find the histogram window
vtkImageViewer viewer2
viewer2 SetInput [reader GetOutput]
viewer2 SetZSlice 14
viewer2 SetColorWindow 2000
viewer2 SetColorLevel 1000


# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 64 -height 64 -iv viewer2

set hist [vtkHistogramWidget .top.f1.r2]
HistogramWidgetSetInput $hist [reader GetOutput]
HistogramWidgetSetExtent $hist 0 63 0 63 14 14

# let the regression test find the histogram window
set viewer [$hist GetImageViewer]


scan [[reader GetOutput] GetWholeExtent] "%d %d %d %d %d %d" \
  xMin xMax yMin yMax zMin zMax

button .top.btn  -text Quit -command exit
scale .top.slice -from $zMin -to $zMax -orient horizontal \
     -command SetSlice -variable sliceNumber -label "Z Slice"
set sliceNumber 14

proc SetSlice {slice} {
   global hist xMin xMax yMin yMax

   viewer2 SetZSlice $slice
   viewer2 Render

   HistogramWidgetSetExtent $hist $xMin $xMax $yMin $yMax $slice $slice
   HistogramWidgetRender $hist
}

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand f
pack $hist -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.slice .top.btn -fill x -expand f


BindTkImageViewer .top.f1.r1 
HistogramWidgetBind .top.f1.r2


