#!/usr/bin/env python

# Get random numbers
math = vtk.vtkMath()
math.RandomSeed(1)
def randint (min,max,__vtk__temp0=0,__vtk__temp1=0):
    f = math.Random(min,max)
    return expr.expr(globals(), locals(),["int","(","f",")"])

# Main testing proc
def test_style (style,__vtk__temp0=0,__vtk__temp1=0):
    # Event state to test
    buttons =
        Left
        Middle
        Right

    ctrls =
        0 1

    shifts =
        0 1

    # I do not trust timers while testing (since they trigger asynchronous
    # rendering/interaction)
    use_timers = style.GetUseTimers()
    style.UseTimersOff()
    style.AutoAdjustCameraClippingRangeOn()
    puts."Testing: " + str(style.GetClassName()) + ""()
    iren = style.GetInteractor()
    renwin = iren.GetRenderWindow()
    renwin.Render()
    # Get renwin size and center
    win_size = renwin.GetSize()
    win_center_x = expr.expr(globals(), locals(),["lindex(win_size,0)","/","2.0"])
    win_center_y = expr.expr(globals(), locals(),["lindex(win_size,1)","/","2.0"])
    pick = vtk.vtkPropPicker()
    radius = expr.expr(globals(), locals(),["5","*","(","1","+","use_timers",")"])
    for ctrl in ctrls.split():
        for shift in shifts.split():
            puts.-nonewline(" - ctrl: " + str(ctrl) + " shift: " + str(shift) + " button:")
            for button in buttons.split():
                puts.-nonewline(" " + str(button) + "")
                flush.stdout()
                # First try to find a starting position where an actor
                # can be picked (not mandatory for trackball modes).
                # Search in increasingly big area, until we reach win size
                # in that case actors might not be on screen, so reset cam
                search = radius
                while 1:
                    start_x = randint(expr.expr(globals(), locals(),["win_center_x","-","search"]),expr.expr(globals(), locals(),["win_center_x","+","search"]))
                    start_y = randint(expr.expr(globals(), locals(),["win_center_x","-","search"]),expr.expr(globals(), locals(),["win_center_x","+","search"]))
                    if (pick.PickProp(start_x,start_y,ren1)):
                        break
                        pass
                    else:
                        if (search > win_center_x or
                        search > win_center_y):
                            puts."   (resetting camera)"()
                            ren1.ResetCamera()
                            search = radius
                            pass
                        else:
                            search = search + 5
                            pass
                        pass

                    pass
                # Start by pressing the button
                iren.SetEventInformationFlipY(start_x,start_y,ctrl,shift,0,0,0)
                iren.InvokeEvent("" + str(locals()[get_variable_name("", button, "ButtonPressEvent")]) + "")
                # puts " - Starting: [$iren GetEventPosition]"
                # Now move around (alternating left and right around
                # the window center in order to compensate somehow).
                sign = 1
                i = 0
                while i < 2 + use_timers:
                    sign = expr.expr(globals(), locals(),["sign","*","-1"])
                    x = randint(expr.expr(globals(), locals(),["win_center_x","+","radius","*","2","*","sign"]),expr.expr(globals(), locals(),["win_center_y","+","radius","*","sign"]))
                    y = randint(expr.expr(globals(), locals(),["win_center_y","+","radius","*","2","*","sign"]),expr.expr(globals(), locals(),["win_center_y","+","radius","*","sign"]))
                    iren.SetEventInformationFlipY(x,y,ctrl,shift,0,0,0)
                    # puts " - Moving:   [$iren GetEventPosition] $ctrl $shift (was [$iren GetLastEventPosition])"
                    iren.InvokeEvent("MouseMoveEvent")
                    # If this style use timers, run OnTimer multiple times
                    if (use_timers):
                        j = 0
                        while j < 10:
                            iren.InvokeEvent("TimerEvent")
                            j = j + 1

                        pass
                    renwin.Render()
                    i = i + 1

                # End by releasing the button
                iren.SetEventInformationFlipY(x,y,ctrl,shift,0,0,0)
                iren.InvokeEvent("" + str(locals()[get_variable_name("", button, "ButtonReleaseEvent")]) + "")

                pass
            puts."."()

            pass

        pass
    style.SetUseTimers(use_timers)
    renwin.Render()

# --- end of script --
