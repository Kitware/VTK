# creates a meta object which clips a region of the input, and
# draws a histogram for the data.

# create a histogram object
proc vtkHistogramWidget {widget} {
   set clip [NewWidgetObject $widget vtkImageClip Clip]
   set accumulate [NewWidgetObject $widget vtkImageAccumulate Accumulate]
   set canvas [NewWidgetObject $widget vtkImageCanvasSource2D Canvas]
   $canvas SetNumberOfScalarComponents 1
   $canvas SetScalarTypeToUnsignedChar
   set viewer [NewWidgetObject $widget vtkImageViewer Viewer]
   $viewer SetColorWindow 256
   $viewer SetColorLevel 127
   vtkTkImageViewerWidget $widget -width 512 -height 200 -iv $viewer 
 
   $accumulate SetInput [$clip GetOutput]
   $viewer SetInput [$canvas GetOutput]

   # create text actor for value display
   set mapper [NewWidgetObject $widget vtkTextMapper Mapper1]
     $mapper SetInput "none"
     $mapper SetFontFamilyToTimes
     $mapper SetFontSize 18
     $mapper BoldOn
     $mapper ShadowOn
   set actor [NewWidgetObject $widget vtkActor2D Actor1]
     $actor SetMapper $mapper
     $actor SetLayerNumber 1
     [$actor GetPositionCoordinate] SetValue 4 4
     [$actor GetProperty] SetColor 0 0.8 0
     $actor SetVisibility 0
   set imager [[$widget GetImageViewer] GetRenderer]
     $imager AddActor2D $actor
   # line 2
   set mapper [NewWidgetObject $widget vtkTextMapper Mapper2]
     $mapper SetInput "none"
     $mapper SetFontFamilyToTimes
     $mapper SetFontSize 18
     $mapper BoldOn
     $mapper ShadowOn
   set actor [NewWidgetObject $widget vtkActor2D Actor2]
     $actor SetMapper $mapper
     $actor SetLayerNumber 1
     [$actor GetPositionCoordinate] SetValue 4 4
     [$actor GetProperty] SetColor 0 0.8 0
     $actor SetVisibility 0
   set imager [[$widget GetImageViewer] GetRenderer]
     $imager AddActor2D $actor

   return $widget
}


# Sets the input
proc HistogramWidgetSetInput {widget input} {
   set clip [GetWidgetVariableValue $widget Clip]
   $clip SetInput $input
}

# Render
proc HistogramWidgetRender {widget} {
   # get the size of the histogram window
   set width [lindex [$widget configure -width] 4]
   set height [lindex [$widget configure -height] 4]
   # setup the bins of the accumulate filter from the range of input data
   set accumulate [GetWidgetVariableValue $widget Accumulate]
   set numBins [expr $width / 2]
   set data [$accumulate GetInput]
   $data Update
   set inputRange [[[$data GetPointData] GetScalars] GetRange]
   set origin [lindex $inputRange 0]
   set spacing [expr 1.0 * ([lindex $inputRange 1] - $origin) / $numBins]
   $accumulate SetComponentExtent 0 [expr $numBins - 1] 0 0 0 0
   $accumulate SetComponentOrigin $origin 0.0 0.0
   $accumulate SetComponentSpacing $spacing 1.0 1.0
     
   # initialize the canvas
   set canvas [GetWidgetVariableValue $widget Canvas]
   $canvas SetExtent 0 $width 0 $height 0 0
   $canvas SetDrawColor 255
   $canvas FillBox 0 $width 0 $height
   $canvas SetDrawColor 0

   # get the histogram data
   set data [$accumulate GetOutput]
   $data Update
   
   # scale the histogram max to fit the window
   set histRange [[[$data GetPointData] GetScalars] GetRange]
   set scale [expr 0.9 * $height / [lindex $histRange 1]]

   for {set idx 0} {$idx < $numBins} {incr idx} {
      set y [$data GetScalarComponentAsFloat $idx 0 0 0]
      set y1 [expr $y * $scale]
      set y2 [lindex [split $y1 .] 0]
      set x [expr $idx * 2]
      $canvas DrawSegment $x 0 $x $y2
   }

   $widget Render
}


proc HistogramWidgetSetExtent {widget x1 x2 y1 y2 z1 z2} {
   set clip [GetWidgetVariableValue $widget Clip]
   $clip SetOutputWholeExtent $x1 $x2 $y1 $y2 $z1 $z2
}


# ---- Bindings and interaction procedures ----

proc HistogramWidgetBind {widget} {
   bind $widget <Expose> {HistogramWidgetRender %W}
   
   # probe value
   bind $widget <ButtonPress-1> {HistogramWidgetStartInteraction %W %x %y}
   bind $widget <B1-Motion> {HistogramWidgetUpdateInteraction %W %x %y}
   bind $widget <ButtonRelease-1> {HistogramWidgetEndInteraction %W}  
}


proc HistogramWidgetStartInteraction {widget x y} {
   # make the text visible
   set actor1 [GetWidgetVariableValue $widget Actor1]
   set actor2 [GetWidgetVariableValue $widget Actor2]
   $actor1 SetVisibility 1
   $actor2 SetVisibility 1

   # in case the window has been resized, place at the top of the window.
   set height [lindex [$widget configure -height] 4]
   [$actor1 GetPositionCoordinate] SetValue 4 [expr $height - 22]
   [$actor2 GetPositionCoordinate] SetValue 4 [expr $height - 40]

   HistogramWidgetUpdateInteraction $widget $x $y
}

proc HistogramWidgetEndInteraction {widget} {
   set actor [GetWidgetVariableValue $widget Actor1]
   $actor SetVisibility 0
   set actor [GetWidgetVariableValue $widget Actor2]
   $actor SetVisibility 0
   $widget Render
}

proc HistogramWidgetUpdateInteraction {widget x y} {
   # compute the bin selected by the mouse
   set x [expr $x / 2]
   set accumulate [GetWidgetVariableValue $widget Accumulate]
   set origin [lindex [$accumulate GetComponentOrigin] 0]
   set spacing [lindex [$accumulate GetComponentSpacing] 0]
   set binMin [expr $origin + $spacing * $x]
   set binMax [expr $binMin + $spacing]
   # now get the height of the histogram
   set data [$accumulate GetOutput]
   $data Update
   # make sure value is in extent
   set max [lindex [$data GetExtent] 1]
   if {$x < 0 || $x > $max} {
      return
   }
   set y [$data GetScalarComponentAsFloat $x 0 0 0]
   # format the string to display
   set str1 [format "Bin: \[%.1f, %.1f)" $binMin $binMax]
   set str2 [format "Count: %d" $y]
   # display the value
   set mapper [GetWidgetVariableValue $widget Mapper1]
   $mapper SetInput $str1
   set mapper [GetWidgetVariableValue $widget Mapper2]
   $mapper SetInput $str2
   $widget Render
}
