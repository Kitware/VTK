namespace eval ::vtk {

    namespace export *

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

    # Called when a Tk mouse motion event is triggered.
    # Helper binding: propagate the event as a VTK event.

    proc cb_vtkw_motion_binding {vtkw renwin x y} {
        set iren [$renwin GetInteractor]
        $iren SetEventPositionFlipY $x $y
        $iren MouseMoveEvent 
    }

    # Called when a Tk button mouse event is triggered.
    # Helper binding: propagate the event as a VTK event.
    #    event:  button state, Press or Release
    #    pos:    position of the button, Left, Middle or Right

    proc cb_vtkw_button_binding {vtkw renwin x y ctrl shift event pos} {
        set iren [$renwin GetInteractor]
        $iren SetEventPositionFlipY $x $y
        $iren SetControlKey $ctrl
        $iren SetShiftKey $shift
        $iren ${pos}Button${event}Event
    }

    # Called when a Tk key event is triggered.
    # Helper binding: propagate the event as a VTK event (indeed, two).
    #    event:   key state, Press or Release
    #    keycode: keycode field
    #    keysym:  keysym field

    proc cb_vtkw_key_binding {vtkw renwin x y ctrl shift event keycode keysym} {
        set iren [$renwin GetInteractor]
        # Not a bug, two times keysym, since 5th param expect a char, and
        # $keycode is %k, which is a number
        $iren SetEventInformationFlipY $x $y $ctrl $shift $keysym 0 $keysym
        $iren Key${event}Event
        $iren SetEventInformationFlipY $x $y $ctrl $shift $keysym 0 $keysym
        $iren CharEvent
    }

    # Called when a Tk Expose/Configure event is triggered.
    # Helper binding: propagate the event as a VTK event.

    proc cb_vtkw_configure_binding {vtkw renwin w h} {
        set iren [$renwin GetInteractor]
        $iren UpdateSize $w $h
        $iren ConfigureEvent 
    }
        
    proc cb_vtkw_expose_binding {vtkw renwin x y w h} {
        set iren [$renwin GetInteractor]
        $iren SetEventPositionFlipY $x $y
        $iren SetEventSize $w $h
        $iren ExposeEvent 
    }

    # Called when a Tk Enter/Leave event is triggered.
    # Helper binding: propagate the event as a VTK event.
    # Note that entering the widget automatically grabs the focus so
    # that key events can be processed.

    proc cb_vtkw_enter_binding {vtkw renwin x y} {
        focus $vtkw
        set iren [$renwin GetInteractor]
        $iren SetEventPositionFlipY $x $y
        $iren EnterEvent 
    }

    proc cb_vtkw_leave_binding {vtkw renwin x y} {
        set iren [$renwin GetInteractor]
        $iren SetEventPositionFlipY $x $y
        $iren LeaveEvent
    }

    # Set the above bindings for a vtkTkRenderWidget widget.

    proc create_vtkw_bindings {vtkw renwin} {
    
        # Find the render window (which creates it if it was not set).
        # Find the interactor, create a generic one if needed.

        if {[$renwin GetInteractor] == ""} {
            # the duh is critical in the follwing line, it causes 
            # vtkTclUtil.cxx to know that the object was created in
            # a Tcl script, otherwise if ${renwin} was a return value
            # from a C++ function it would be called vtkTemp### and
            # the interactor instance would have the same name causeing Tcl
            # to think it also was a C++ return value. 
            set iren [vtkGenericRenderWindowInteractor duh_${renwin}_iren]
            $iren SetRenderWindow $renwin
            $iren Initialize
        }

        # Mouse motion

        bind $vtkw <Motion> "::vtk::cb_vtkw_motion_binding $vtkw $renwin %x %y"

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

        bind $vtkw <Configure> \
                "::vtk::cb_vtkw_configure_binding $vtkw $renwin %w %h"

        bind $vtkw <Expose> \
                "::vtk::cb_vtkw_expose_binding $vtkw $renwin %x %y %w %h"

        # Enter/Leave

        bind $vtkw <Enter> \
                "::vtk::cb_vtkw_enter_binding $vtkw $renwin %x %y"

        bind $vtkw <Leave> \
                "::vtk::cb_vtkw_leave_binding $vtkw $renwin %x %y"
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

    # CreateTimerEvent/DestroyTimerEvent obversers.
    # Handle the creation of a timer event (10 ms)
    
    proc cb_iren_create_timer_event {iren} {
        set timer [after 10 "$iren TimerEvent"]
        ::vtk::set_widget_variable_value $iren CreateTimerEventTimer $timer
    }

    proc cb_iren_destroy_timer_event {iren} {
        set timer \
                [::vtk::get_widget_variable_value $iren CreateTimerEventTimer]
        if {$timer != ""} {
            after cancel $timer
        }
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

        ::vtk::set_widget_variable_value $iren DestroyTimerEventTag \
                [$iren AddObserver DestroyTimerEvent \
                [list ::vtk::cb_iren_destroy_timer_event $iren]]

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
}
