# First we include the VTK Tcl packages which will make available 
# all of the vtk commands to Tcl

package require vtk
package require vtkinteraction

# Create the image reader

vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 63 0 63 1 93
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataMask 0x7fff

reader Update
scan [[reader GetOutput] GetWholeExtent] "%d %d %d %d %d %d" \
        xMin xMax yMin yMax zMin zMax

# Magnify the image

set mag_factor 3
vtkImageMagnify magnify
  magnify SetInput [reader GetOutput]
  magnify SetMagnificationFactors $mag_factor $mag_factor 1

# Create the image viewer

vtkImageViewer viewer2
  viewer2 SetInput [magnify GetOutput]
  viewer2 SetZSlice 14
  viewer2 SetColorWindow 2000
  viewer2 SetColorLevel 1000

# Create the GUI, i.e. two Tk image viewer, one for the image
# the other for the histogram, and a slice slider

wm withdraw .
toplevel .top 

# Set the window manager (wm command) so that it registers a
# command to handle the WM_DELETE_WINDOW protocal request. This
# request is triggered when the widget is closed using the standard
# window manager icons or buttons. In this case the exit callback
# will be called and it will free up any objects we created then exit
# the application.

wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

# Create the vtkTkImageViewerWidget

frame .top.f1 

set vtkiw [vtkTkImageViewerWidget .top.f1.r1 \
        -width [expr ($xMax - $xMin + 1) * $mag_factor] \
        -height [expr ($yMax - $yMin + 1) * $mag_factor] \
        -iv viewer2]

# Setup some Tk bindings, a generic renwin interactor and VTK observers 
# for that widget

::vtk::bind_tk_imageviewer_widget $vtkiw

# Create the histogram widget

source HistogramWidget.tcl

set hist [vtkHistogramWidget .top.f1.r2 512 192]

set slice_number [viewer2 GetZSlice]

HistogramWidgetSetInput $hist [reader GetOutput]
HistogramWidgetSetExtent $hist $xMin $xMax $yMin $yMax $slice_number $slice_number

HistogramWidgetBind .top.f1.r2

# Add a 'Quit' button that will call the usual cb_exit callback and destroy
# all VTK objects

button .top.btn \
        -text Quit \
        -command ::vtk::cb_exit

# Add a slice scale to browse the whole stack

scale .top.slice \
        -from $zMin \
        -to $zMax \
        -orient horizontal \
        -command SetSlice \
        -variable slice_number \
        -label "Z Slice"

proc SetSlice {slice} {
    global hist xMin xMax yMin yMax

    viewer2 SetZSlice $slice
    viewer2 Render

    HistogramWidgetSetExtent $hist $xMin $xMax $yMin $yMax $slice $slice
    HistogramWidgetRender $hist
}

# Pack all gui elements

pack $vtkiw \
        -side left -anchor n \
        -padx 3 -pady 3 \
        -fill x -expand f

pack $hist \
        -side left \
        -padx 3 -pady 3 \
        -fill both -expand t

pack .top.f1 \
        -fill both -expand t

pack .top.slice .top.btn \
        -fill x -expand f

# You only need this line if you run this script from a Tcl shell
# (tclsh) instead of a Tk shell (wish) 

tkwait window .
