package provide vtkinteraction 4.0

namespace eval ::vtk {

    namespace export *

    variable gvars

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
    #    key1: keycode field
    #    key2: keysym field

    proc cb_vtkw_key_binding {vtkw renwin x y ctrl shift event key1 key2} {
        set iren [$renwin GetInteractor]
        $iren SetEventInformationFlipY $x $y $ctrl $shift $key1 0 $key2
        $iren Key${event}Event
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

        $renwin AddObserver AbortCheckEvent \
                [list ::vtk::cb_renwin_abort_check_event $renwin]
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
        puts "I'm handling ConfigureEvent"
        variable gvars
        # Cancel the previous timer if any
        if {[info exists gvars($iren,ConfigureEvent,timer)]} {
            puts " => Cancelling $gvars($iren,ConfigureEvent,timer)"
            after cancel $gvars($iren,ConfigureEvent,timer)
        }
        set gvars($iren,ConfigureEvent,timer) \
                [after 300 [list ::vtk::cb_iren_expose_event $iren]]
        puts " => Setting $gvars($iren,ConfigureEvent,timer)"
    }

    # ExposeEvent obverser.
    # This event is triggered when a part (or all) of the widget is exposed,
    # i.e. a new area is visible. It usually happens when the widget window
    # is brought to the front, or when the widget is resized.
    # See above for explanations about the update rate tricks.

    proc cb_iren_expose_event {iren} {
        puts "I'm handling ExposeEvent"
        variable gvars
        set renwin [$iren GetRenderWindow]
        # Check if a ConfigureEvent timer is pending
        if {[info exists gvars($iren,ConfigureEvent,timer)]} {
            puts " => In test $gvars($iren,ConfigureEvent,timer)"
            if {[catch {after info $gvars($iren,ConfigureEvent,timer)}]} {
                unset gvars($iren,ConfigureEvent,timer)
                $renwin SetDesiredUpdateRate [$iren GetStillUpdateRate]
                puts " => Still update"
            } else {
                puts " => Desired update"
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

        $iren AddObserver CreateTimerEvent \
                [list ::vtk::cb_iren_create_timer_event $iren]

        # User Tk interactor

        $iren AddObserver UserEvent \
                [list ::vtk::cb_iren_user_event]

        # Expose and Configure

        $iren AddObserver ConfigureEvent \
                [list ::vtk::cb_iren_configure_event $iren]

        $iren AddObserver ExposeEvent \
                [list ::vtk::cb_iren_expose_event $iren]

        # Exit
        # Since the callback is likely to delete all VTK objects
        # using vtkCommand::DeleteAllObject, let's try not to call it from
        # the object itself. Use a timer.

        $iren AddObserver ExitEvent \
                "after 100 [list ::vtk::cb_iren_exit_event]"
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

    proc bind_tk_imageviewer_widget {vtkiw} {
        bind_tk_widget $vtkiw [[$vtkiw GetImageViewer] GetRenderWindow]
    }
}

