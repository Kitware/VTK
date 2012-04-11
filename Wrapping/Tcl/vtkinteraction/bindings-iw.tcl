namespace eval ::vtk {

    namespace export *

    # -------------------------------------------------------------------
    # Specific vtkTkImageViewerWidget bindings

    # Create a 2d text actor that can be used to display infos
    # like window/level, pixel picking, etc

    proc cb_vtkiw_create_text1 {vtkiw} {
        set mapper [::vtk::get_widget_variable_value $vtkiw text1_mapper]
        if {$mapper == ""} {
            set mapper \
                  [::vtk::new_widget_object $vtkiw vtkTextMapper text1_mapper]
            $mapper SetInput "none"
            set tprop [$mapper GetTextProperty]
            $tprop SetFontFamilyToArial
            $tprop SetFontSize 12
            $tprop BoldOn
            $tprop ShadowOn
            $tprop SetColor 1 1 0.5
        }
        set actor [::vtk::get_widget_variable_value $vtkiw text1_actor]
        if {$actor == ""} {
            set actor \
                    [::vtk::new_widget_object $vtkiw vtkActor2D text1_actor]
            $actor SetMapper $mapper
            $actor SetLayerNumber 1
            [$actor GetPositionCoordinate] SetValue 5 4 0
            $actor SetVisibility 0
            [[$vtkiw GetImageViewer] GetRenderer] AddActor2D $actor
        }
    }

    # Show/Hide the 2d text actor
    # Ensure that it stays in the upper left corner of the window

    proc cb_vtkiw_show_text1 {vtkiw} {
        set actor [::vtk::get_widget_variable_value $vtkiw text1_actor]
        if {![$actor GetVisibility]} {
            set height [lindex [$vtkiw configure -height] 4]
            set pos [$actor GetPositionCoordinate]
            set value [$pos GetValue]
            $pos SetValue \
                    [lindex $value 0] [expr $height - 15] [lindex $value 2]
            $actor VisibilityOn
        }
    }

    proc cb_vtkiw_hide_text1 {vtkiw} {
        set actor [::vtk::get_widget_variable_value $vtkiw text1_actor]
        if {[$actor GetVisibility]} {
            $actor VisibilityOff
        }
    }

    # -------------------------------------------------------------------
    # vtkInteractorStyleImage callbacks/observers
    #   istyle: interactor style
    #   vtkiw: vtkTkImageRenderWindget

    # StartWindowLevelEvent observer.
    # Create the text actor, show it

    proc cb_istyleimg_start_window_level_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_create_text1 $vtkiw
        ::vtk::cb_vtkiw_show_text1 $vtkiw
    }

    # EndWindowLevelEvent observer.
    # Hide the text actor.

    proc cb_istyleimg_end_window_level_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_hide_text1 $vtkiw
        $vtkiw Render
    }

    # WindowLevelEvent observer.
    # Update the text actor with the current window/level values.

    proc cb_istyleimg_window_level_event {istyle vtkiw} {
        set mapper [::vtk::get_widget_variable_value $vtkiw text1_mapper]
        set viewer [$vtkiw GetImageViewer]
        $mapper SetInput [format "W/L: %.0f/%.0f" \
                [$viewer GetColorWindow] [$viewer GetColorLevel]]
    }

    # RightButtonPressEvent observer.
    # Invert the 'shift' key. The usual vtkInteractorStyleImage
    # behaviour is to enable picking mode with "Shift+Right button",
    # whereas we want picking mode to be "Right button" (for backward
    # compatibility).

    proc cb_istyleimg_right_button_press_event {istyle} {
        set iren [$istyle GetInteractor]
        $iren SetShiftKey [expr [$iren GetShiftKey] ? 0 : 1]
        $istyle OnRightButtonDown
    }

    proc cb_istyleimg_right_button_release_event {istyle} {
        set iren [$istyle GetInteractor]
        $iren SetShiftKey [expr [$iren GetShiftKey] ? 0 : 1]
        $istyle OnRightButtonUp
    }

    # StartPickEvent observer.
    # Create the text actor, show it

    proc cb_istyleimg_start_pick_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_create_text1 $vtkiw
        ::vtk::cb_vtkiw_show_text1 $vtkiw
    }

    # EndPickEvent observer.
    # Hide the text actor.

    proc cb_istyleimg_end_pick_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_hide_text1 $vtkiw
        $vtkiw Render
    }

    # PickEvent observer.
    # Update the text actor with the current value of the picked pixel.

    proc cb_istyleimg_pick_event {istyle vtkiw} {

        set viewer [$vtkiw GetImageViewer]
        set input [$viewer GetInput]
        set pos [[$istyle  GetInteractor] GetEventPosition]
        set x [lindex $pos 0]
        set y [lindex $pos 1]
        set z [$viewer GetZSlice]

        # Y is flipped upside down

        set height [lindex [$vtkiw configure -height] 4]
        set y [expr $height - $y]

        # Make sure point is in the whole extent of the image.

        scan [$input GetWholeExtent] "%d %d %d %d %d %d" \
                xMin xMax yMin yMax zMin zMax
        if {$x < $xMin || $x > $xMax || \
            $y < $yMin || $y > $yMax || \
            $z < $zMin || $z > $zMax} {
           return
        }

        $input SetUpdateExtent $x $x $y $y $z $z
        $input Update

        set num_comps [$input GetNumberOfScalarComponents]
        set str "($x, $y):"
        for {set idx 0} {$idx < $num_comps} {incr idx} {
            set str [format "%s %.0f" $str \
                    [$input GetScalarComponentAsDouble $x $y $z $idx]]
        }

        set mapper [::vtk::get_widget_variable_value $vtkiw text1_mapper]
        $mapper SetInput "$str"
        $vtkiw Render
    }

    # -------------------------------------------------------------------
    # Create vtkTkImageViewerWidget bindings, setup observers

    proc bind_tk_imageviewer_widget {vtkiw} {

        bind_tk_widget $vtkiw [[$vtkiw GetImageViewer] GetRenderWindow]

        set viewer [$vtkiw GetImageViewer]
        set iren [[$viewer GetRenderWindow] GetInteractor]

        # Ask the viewer to setup an image style interactor

        $viewer SetupInteractor $iren
        set istyle [$iren GetInteractorStyle]

        # Window/Level observers

        ::vtk::set_widget_variable_value $istyle StartWindowLevelEventTag \
                [$istyle AddObserver StartWindowLevelEvent \
                "::vtk::cb_istyleimg_start_window_level_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle WindowLevelEventTag \
                [$istyle AddObserver WindowLevelEvent \
                "::vtk::cb_istyleimg_window_level_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle EndWindowLevelEventTag \
                [$istyle AddObserver EndWindowLevelEvent \
                "::vtk::cb_istyleimg_end_window_level_event $istyle $vtkiw"]

        # Picking observers

        ::vtk::set_widget_variable_value $istyle RightButtonPressEventTag \
                [$istyle AddObserver RightButtonPressEvent \
                "::vtk::cb_istyleimg_right_button_press_event $istyle"]

        ::vtk::set_widget_variable_value $istyle RightButtonReleaseEventTag \
                [$istyle AddObserver RightButtonReleaseEvent \
                "::vtk::cb_istyleimg_right_button_release_event $istyle"]

        ::vtk::set_widget_variable_value $istyle StartPickEventTag \
                [$istyle AddObserver StartPickEvent \
                "::vtk::cb_istyleimg_start_pick_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle EndPickEventTag \
                [$istyle AddObserver EndPickEvent \
                "::vtk::cb_istyleimg_end_pick_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle PickEventTag \
                [$istyle AddObserver PickEvent \
                "::vtk::cb_istyleimg_pick_event $istyle $vtkiw"]
    }
}

# Handle deprecated calls

proc BindTkImageViewer {widget} {
    puts stderr "BindTkImageViewer is deprecated. Please use ::vtk::bind_tk_imageviewer_widget instead"
    ::vtk::bind_tk_imageviewer_widget $widget
}
