package provide vtkinteraction 4.0

namespace eval ::vtk {

    namespace export *

    # -------------------------------------------------------------------
    # Some functions that can be used to associate variables to
    # widgets without polluting the global space

    variable gvars

    # Generate a "unique" name for a widget variable

    proc get_widget_variable {widget var_name} {
        variable gvars
        return "gvars($widget,vars,$var_name)"
    }

    # Set the value of a widget variable

    proc set_widget_variable_value {widget var_name value} {
        variable gvars
        set var [get_widget_variable $widget $var_name]
        set $var $value
    }

    proc unset_widget_variable {widget var_name} {
        variable gvars
        set var [get_widget_variable $widget $var_name]
        if {[info exists $var]} {
            unset $var
        }
    }

    # Get the value of a widget variable ("" if undef)

    proc get_widget_variable_value {widget var_name} {
        variable gvars
        set var [get_widget_variable $widget $var_name]
        if {[info exists $var]} {
            return [expr $$var]
        } else {
            return ""
        }
    }

    # Return an object which will be associated with a widget

    proc new_widget_object {widget type var_name} {
        variable gvars
        set var [get_widget_variable $widget "${var_name}_obj"]
        $type $var
        set_widget_variable_value $widget $var_name $var
        return $var
    }

    # -------------------------------------------------------------------
    # vtkTk(ImageViewer/Render)Widget callbacks.
    #    vtkw: Tk pathname of the widget
    #    renwin: render window embeded in the widget
    #    x:      X coord, widget relative
    #    y:      X coord, widget relative
    #    w:      width of an event or update
    #    h:      height of an event or update
    #    ctrl:   1 if the Control key modifier was pressed, 0 otherwise
    #    shift:  1 if the Control key modifier was pressed, 0 otherwise

    # Called when a mouse motion event is triggered.
    # Helper binding: propagate the event as a VTK event.

    proc cb_vtkw_motion {vtkw renwin x y} {
        set iren [$renwin GetInteractor]
        $iren SetEventInformationFlipY $x $y 0 0 0 0 0
        $iren MouseMoveEvent 
    }

    # Called when a button mouse Tk event is triggered.
    # Helper binding: propagate the event as a VTK event.
    #    event:  button state, Press or Release
    #    pos:    position of the button, Left, Middle or Right

    proc cb_vtkw_button_binding {vtkw renwin x y ctrl shift event pos} {
        set iren [$renwin GetInteractor]
        $iren SetEventInformationFlipY $x $y $ctrl $shift 0 0 0
        $iren ${pos}Button${event}Event
    }

    # Called when a key event is triggered.
    # Helper binding: propagate the event as a VTK event.
    #    keycode: keycode field
    #    keysym: keysym field

    proc cb_vtkw_key_binding {vtkw renwin x y ctrl shift event keycode keysym} {
        set iren [$renwin GetInteractor]
        # Not a bug, two times keysym, since 5th param expect a char, and
        # $keycode is %k, which is a number
        $iren SetEventInformationFlipY $x $y $ctrl $shift $keysym 0 $keysym
        $iren Key${event}Event
        $iren SetEventInformationFlipY $x $y $ctrl $shift $keysym 0 $keysym
        $iren CharEvent
    }

    # Called when a Expose/Configure event is triggered.
    # Helper binding: propagate the event as a VTK event.

    proc cb_vtkw_configure {vtkw renwin w h} {
        set iren [$renwin GetInteractor]
        $iren UpdateSize $w $h
        $iren ConfigureEvent 
    }
        
    proc cb_vtkw_expose {vtkw renwin x y w h} {
        set iren [$renwin GetInteractor]
        $iren SetEventInformationFlipY $x $y 0 0 0 0 0
        $iren SetEventSize $w $h
        $iren ExposeEvent 
    }

    # Called when a Enter/Leave event is triggered.
    # Helper binding: propagate the event as a VTK event.
    # Note that entering the widget automatically grabs the focus so
    # that key events can be processed.

    proc cb_vtkw_enter {vtkw renwin x y} {
        focus $vtkw
        set iren [$renwin GetInteractor]
        $iren SetEventInformationFlipY $x $y 0 0 0 0 0
        $iren EnterEvent 
    }

    proc cb_vtkw_leave {vtkw renwin x y} {
        set iren [$renwin GetInteractor]
        $iren SetEventInformationFlipY $x $y 0 0 0 0 0
        $iren LeaveEvent
    }

    # Set the above bindings for a vtkTkRenderWidget widget.

    proc create_vtkw_bindings {vtkw renwin} {
    
        # Find the render window (which creates it if it was not set).
        # Find the interactor, create a generic one if needed.

        if {[$renwin GetInteractor] == ""} {
            set iren [vtkGenericRenderWindowInteractor ${renwin}_iren]
            $iren SetRenderWindow $renwin
        }

        # Mouse motion

        bind $vtkw <Motion> "::vtk::cb_vtkw_motion $vtkw $renwin %x %y"

        # Mouse buttons and key events

        foreach {modifier ctrl shift} {
            ""               0 0
            "Control-"       1 0
            "Shift-"         0 1
            "Control-Shift-" 1 1
        } {
            foreach event {
                Press
                Release
            } {
                foreach {pos number} {
                    Left 1
                    Middle 2
                    Right 3
                } {
                    bind $vtkw <${modifier}Button${event}-${number}> \
                   "::vtk::cb_vtkw_button_binding $vtkw $renwin %x %y $ctrl $shift $event $pos"
                }

                bind $vtkw <${modifier}Key${event}> \
                  "::vtk::cb_vtkw_key_binding $vtkw $renwin %x %y $ctrl $shift $event %k %K"
            }
        }
        
        # Expose/Configure

        bind $vtkw <Configure> "::vtk::cb_vtkw_configure $vtkw $renwin %w %h"
        bind $vtkw <Expose> "::vtk::cb_vtkw_expose $vtkw $renwin %x %y %w %h"

        # Enter/Leave

        bind $vtkw <Enter> "::vtk::cb_vtkw_enter $vtkw $renwin %x %y"
        bind $vtkw <Leave> "::vtk::cb_vtkw_leave $vtkw $renwin %x %y"
    }

    # -------------------------------------------------------------------
    # vtkRenderWindow callbacks/observers
    #   renwin: render window object

    # AbortCheckEvent observer.
    # Check if some events are pending, and abort render in that case

    proc cb_renwin_abort_check_event {renwin} {
        if {[$renwin GetEventPending] != 0} {    
            $renwin SetAbortRender 1        
        }                                   
    }

    # Add the above observers to a vtkRenderWindow

    proc add_renwin_observers {renwin} {
        
        # Check for aborting rendering

        ::vtk::set_widget_variable_value $renwin AbortCheckEventTag \
                [$renwin AddObserver AbortCheckEvent \
                [list ::vtk::cb_renwin_abort_check_event $renwin]]
    }

    # -------------------------------------------------------------------
    # vtk(Generic)RenderWindowInteractor callbacks/observers
    #   iren: interactor object

    # CreateTimerEvent obverser.
    # Handle the creation of a timer event (10 ms)
    
    proc cb_iren_create_timer_event {iren} {
        after 10 "$iren TimerEvent"
    }

    # UserEvent obverser.
    # Popup the vtkInteract widget (simple wish-like console)
    
    proc cb_iren_user_event {} {
        wm deiconify .vtkInteract
    }

    # ConfigureEvent obverser.
    # This event is triggered when the widget is re-configured, i.e. its
    # size is changed. Note that for every ConfigureEvent an ExposeEvent is
    # triggered too and the corresponding observer will re-render the window.
    # It might be nice to switch the frame update rate to an interactive
    # update rate while the window is getting resized, so that the user can
    # experience a decent feedback. This is achieved by launching a timer
    # each time the ConfigureEvent is triggered. This timer lasts 300 ms. When
    # the ExposeEvent observer is called, it checks if this timer still exists.
    # If it is the case, it implies that the user is configuring/resizing the
    # window and that an interactive frame rate may be used. If it is not, 
    # it uses the still update rate to render the scene with full details. 
    # The timer itself is a call to the ExposeEvent observer, which will 
    # finaly render the window using a still update rate.

    proc cb_iren_configure_event {iren} {
        # Cancel the previous timer if any
        set timer [::vtk::get_widget_variable_value $iren ConfigureEventTimer]
        if {$timer != ""} {
            after cancel $timer
        }
        ::vtk::set_widget_variable_value $iren ConfigureEventTimer \
                [after 300 [list ::vtk::cb_iren_expose_event $iren]]
    }

    # ExposeEvent obverser.
    # This event is triggered when a part (or all) of the widget is exposed,
    # i.e. a new area is visible. It usually happens when the widget window
    # is brought to the front, or when the widget is resized.
    # See above for explanations about the update rate tricks.

    proc cb_iren_expose_event {iren} {
        set renwin [$iren GetRenderWindow]
        # Check if a ConfigureEvent timer is pending
        set timer [::vtk::get_widget_variable_value $iren ConfigureEventTimer]
        if {$timer != ""} {
            if {[catch {after info $timer}]} {
                ::vtk::unset_widget_variable $iren ConfigureEventTimer
                $renwin SetDesiredUpdateRate [$iren GetStillUpdateRate]
            } else {
                $renwin SetDesiredUpdateRate [$iren GetDesiredUpdateRate]
            }
        }
        update
        $renwin Render
    }

    # ExitEvent obverser.
    # Destroy all VTK objects (therefore, try not to call this function
    # directly from a VTK object), then exit.

    proc cb_exit {} {
        vtkCommand DeleteAllObjects
        exit
    }

    proc cb_iren_exit_event {} {
        ::vtk::cb_exit
    }

    # Add the above observers to a vtk(Generic)RenderWindowInteractor

    proc add_iren_observers {iren} {

        # Timer events

        ::vtk::set_widget_variable_value $iren CreateTimerEventTag \
                [$iren AddObserver CreateTimerEvent \
                [list ::vtk::cb_iren_create_timer_event $iren]]

        # User Tk interactor

        ::vtk::set_widget_variable_value $iren UserEventTag \
                [$iren AddObserver UserEvent \
                [list ::vtk::cb_iren_user_event]]

        # Expose and Configure

        ::vtk::set_widget_variable_value $iren ConfigureEventTag \
                [$iren AddObserver ConfigureEvent \
                [list ::vtk::cb_iren_configure_event $iren]]

        ::vtk::set_widget_variable_value $iren ExposeEventTag \
                [$iren AddObserver ExposeEvent \
                [list ::vtk::cb_iren_expose_event $iren]]

        # Exit
        # Since the callback is likely to delete all VTK objects
        # using vtkCommand::DeleteAllObject, let's try not to call it from
        # the object itself. Use a timer.

        ::vtk::set_widget_variable_value $iren ExitEventTag \
                [$iren AddObserver ExitEvent \
                "after 100 [list ::vtk::cb_iren_exit_event]"]
    }

    # -------------------------------------------------------------------
    # Create vtkTk(ImageViewer/Render)Widget bindings, setup observers

    proc bind_tk_widget {vtkw renwin} {
        create_vtkw_bindings $vtkw $renwin
        add_renwin_observers $renwin
        add_iren_observers [$renwin GetInteractor]
    }

    proc bind_tk_render_widget {vtkrw} {
        bind_tk_widget $vtkrw [$vtkrw GetRenderWindow]
    }

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
            $mapper SetFontFamilyToArial
            $mapper SetFontSize 12
            $mapper BoldOn
            $mapper ShadowOn
        }
        set actor [::vtk::get_widget_variable_value $vtkiw text1_actor]
        if {$actor == ""} {
            set actor \
                    [::vtk::new_widget_object $vtkiw vtkActor2D text1_actor]
            $actor SetMapper $mapper
            $actor SetLayerNumber 1
            [$actor GetPositionCoordinate] SetValue 5 4 0
            [$actor GetProperty] SetColor 1 1 0.5
            $actor SetVisibility 0
            [[$vtkiw GetImageViewer] GetRenderer] AddActor2D $actor
        }
    }

    # Show/Hide the 2d text actor
    
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

    proc cb_istyle_start_window_level_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_create_text1 $vtkiw
        ::vtk::cb_vtkiw_show_text1 $vtkiw
    }

    # EndWindowLevelEvent observer.
    # Hide the text actor.

    proc cb_istyle_end_window_level_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_hide_text1 $vtkiw
        $vtkiw Render
    }

    # WindowLevelEvent observer.
    # Update the text actor with the current window/level values.

    proc cb_istyle_window_level_event {istyle vtkiw} {
        set mapper [::vtk::get_widget_variable_value $vtkiw text1_mapper]
        set viewer [$vtkiw GetImageViewer]
        $mapper SetInput [format "W/L: %.0f/%.0f" \
                [$viewer GetColorWindow] [$viewer GetColorLevel]]
    }

    # RightButtonPressEvent observer.
    # Invert the 'shift' key. The usual vtkInteractorStyleImage
    # behaviour is to enable picking mode with "Shift+Right button", 
    # whereas we want picking mode to be "Right button" only for backward
    # compatibility.

    proc cb_istyle_right_button_press_event {istyle} {
        set iren [$istyle GetInteractor]
        eval $istyle OnRightButtonDown \
                [$iren GetControlKey] \
                [expr [$iren GetShiftKey] ? 0 : 1]  \
                [$iren GetEventPosition]
    }

    proc cb_istyle_right_button_release_event {istyle} {
        set iren [$istyle GetInteractor]
        eval $istyle OnRightButtonUp \
                [$iren GetControlKey] \
                [expr [$iren GetShiftKey] ? 0 : 1]  \
                [$iren GetEventPosition]
    }

    # StartPickEvent observer.
    # Create the text actor, show it

    proc cb_istyle_start_pick_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_create_text1 $vtkiw
        ::vtk::cb_vtkiw_show_text1 $vtkiw
    }

    # EndPickEvent observer.
    # Hide the text actor.

    proc cb_istyle_end_pick_event {istyle vtkiw} {
        ::vtk::cb_vtkiw_hide_text1 $vtkiw
        $vtkiw Render
    }

    # PickEvent observer.
    # Update the text actor with the current value of the picked pixel.

    proc cb_istyle_pick_event {istyle vtkiw} {

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
                    [$input GetScalarComponentAsFloat $x $y $z $idx]]
        }

        set mapper [::vtk::get_widget_variable_value $vtkiw text1_mapper]
        $mapper SetInput "$str"
        $vtkiw Render
    }

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
                "::vtk::cb_istyle_start_window_level_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle WindowLevelEventTag \
                [$istyle AddObserver WindowLevelEvent \
                "::vtk::cb_istyle_window_level_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle EndWindowLevelEventTag \
                [$istyle AddObserver EndWindowLevelEvent \
                "::vtk::cb_istyle_end_window_level_event $istyle $vtkiw"]

        # Picking observers

        ::vtk::set_widget_variable_value $istyle RightButtonPressEventTag \
                [$istyle AddObserver RightButtonPressEvent \
                "::vtk::cb_istyle_right_button_press_event $istyle"]

        ::vtk::set_widget_variable_value $istyle StartPickEventTag \
                [$istyle AddObserver StartPickEvent \
                "::vtk::cb_istyle_start_pick_event $istyle $vtkiw"]

        ::vtk::set_widget_variable_value $istyle EndPickEventTag \
                [$istyle AddObserver EndPickEvent \
                "::vtk::cb_istyle_end_pick_event $istyle $vtkiw"]
        
        ::vtk::set_widget_variable_value $istyle PickEventTag \
                [$istyle AddObserver PickEvent \
                "::vtk::cb_istyle_pick_event $istyle $vtkiw"]
    }

}

