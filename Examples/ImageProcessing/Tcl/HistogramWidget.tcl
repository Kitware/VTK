# Creates a meta object which clips a region of the input, and
# draws a histogram for the data.

# Create a histogram object

proc vtkHistogramWidget {widget {width 512} {height 192}} {
    
    set clip [::vtk::new_widget_object $widget vtkImageClip Clip]

    set accumulate \
            [::vtk::new_widget_object $widget vtkImageAccumulate Accumulate]
    $accumulate SetInputConnection [$clip GetOutputPort]

    set canvas [::vtk::new_widget_object $widget vtkImageCanvasSource2D Canvas]
    $canvas SetScalarTypeToUnsignedChar
    $canvas SetNumberOfScalarComponents 3
    $canvas SetExtent 0 [expr $width - 1] 0 [expr $height - 1] 0 0

    set viewer [::vtk::new_widget_object $widget vtkImageViewer Viewer]
    $viewer SetInputConnection [$canvas GetOutputPort]
    $viewer SetColorWindow 256
    $viewer SetColorLevel 127

    vtkTkImageViewerWidget $widget \
            -width $width \
            -height $height \
            -iv $viewer 

    return $widget
}

# Set the input

proc HistogramWidgetSetInput {widget input} {
    set clip [::vtk::get_widget_variable_value $widget Clip]
    $clip SetInput $input
}

# Set the extent

proc HistogramWidgetSetExtent {widget x1 x2 y1 y2 z1 z2} {
    set clip [::vtk::get_widget_variable_value $widget Clip]
    $clip SetOutputWholeExtent $x1 $x2 $y1 $y2 $z1 $z2
}

# Render

proc HistogramWidgetRender {widget} {

    # Get the size of the histogram window
    
    set width [lindex [$widget configure -width] 4]
    set height [lindex [$widget configure -height] 4]
    
    # Setup the bins of the accumulate filter from the range of input data
    
    set accumulate [::vtk::get_widget_variable_value $widget Accumulate]
    set numBins [expr $width / 2]

    set data [$accumulate GetInput]
    $data Update

    set inputRange [[[$data GetPointData] GetScalars] GetRange]
    set origin [lindex $inputRange 0]
    set spacing [expr 1.0 * ([lindex $inputRange 1] - $origin) / $numBins]

    $accumulate SetComponentExtent 0 [expr $numBins - 1] 0 0 0 0
    $accumulate SetComponentOrigin $origin 0.0 0.0
    $accumulate SetComponentSpacing $spacing 1.0 1.0
    
    # Initialize the canvas

    set canvas [::vtk::get_widget_variable_value $widget Canvas]
    $canvas SetExtent 0 [expr $width - 1] 0 [expr $height - 1] 0 0
    $canvas SetDrawColor 255
    $canvas SetDrawColor 172 174 241
    $canvas FillBox 0 [expr $width - 1] 0 [expr $height - 1]
    $canvas SetDrawColor 0
    $canvas SetDrawColor 137 28 28

    # Get the histogram data

    set data [$accumulate GetOutput]
    $data Update

    # Scale the histogram max to fit the window

    set histRange [[[$data GetPointData] GetScalars] GetRange]
    set scale [expr 0.9 * $height / [lindex $histRange 1]]

    for {set idx 0} {$idx < $numBins} {incr idx} {
        set y [$data GetScalarComponentAsDouble $idx 0 0 0]
        set y1 [expr $y * $scale]
        set y2 [lindex [split $y1 .] 0]
        set x [expr $idx * 2]
        $canvas DrawSegment $x 0 $x $y2
    }

    $widget Render
}

# Set the bindings

proc HistogramWidgetBind {widget} {

    # The usual vtkTkImageRenderWidget bindings

    ::vtk::bind_tk_imageviewer_widget $widget

    set iren [[[$widget GetImageViewer] GetRenderWindow] GetInteractor]

    # Remove the usual ConfigureEvent and ExposeEvent observers and
    # use ours

    $iren RemoveObservers ConfigureEvent
    $iren RemoveObservers ExposeEventTag
    $iren AddObserver ExposeEvent \
            [list HistogramWidgetRender $widget]

    # Remove the usual PickEvent and use ours for probing

    set istyle [$iren GetInteractorStyle]

    $istyle RemoveObservers PickEvent
    $istyle AddObserver PickEvent \
            [list HistogramWidgetUpdateInteraction $widget]

    # Bind the left button so that it acts like the right button

    $istyle RemoveObservers LeftButtonPressEvent
    $istyle AddObserver LeftButtonPressEvent \
            "::vtk::cb_istyleimg_right_button_press_event $istyle"

    $istyle RemoveObservers LeftButtonReleaseEvent
    $istyle AddObserver LeftButtonReleaseEvent \
            "::vtk::cb_istyleimg_right_button_release_event $istyle"
}

# Probe the histogram

proc HistogramWidgetUpdateInteraction {widget} {

    set pos [[[[$widget GetImageViewer] GetRenderWindow] GetInteractor] GetEventPosition]
    set x [lindex $pos 0]
    set y [lindex $pos 1]

    # Compute the bin selected by the mouse

    set x [expr $x / 2]
    set accumulate [::vtk::get_widget_variable_value $widget Accumulate]
    set origin [lindex [$accumulate GetComponentOrigin] 0]
    set spacing [lindex [$accumulate GetComponentSpacing] 0]
    set binMin [expr $origin + $spacing * $x]
    set binMax [expr $binMin + $spacing]

    # Now get the height of the histogram

    set data [$accumulate GetOutput]
    $data Update

    # Make sure value is in extent

    set max [lindex [$data GetExtent] 1]
    if {$x < 0 || $x > $max} {
        return
    }
    set y [$data GetScalarComponentAsDouble $x 0 0 0]

    # Display the value
    
    set mapper [::vtk::get_widget_variable_value $widget text1_mapper]
    $mapper SetInput [format "\[%.1f, %.1f): %d" $binMin $binMax $y]

    $widget Render
}
