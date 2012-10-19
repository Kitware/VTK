#!/usr/bin/env python

import vtk


class TestStyleBase(object):

    def __init__(self, ren):
        self.ren1 = ren

        # Get random numbers
        self.math = vtk.vtkMath()
        self.math.RandomSeed(1)


    def randint(self, min, max):
        f = self.math.Random(min, max)
        return int(f)


    # Main testing proc

    def test_style(self, style):

        # Event state to test

        buttons = ["Left", "Middle", "Right"]
        ctrls = [0, 1]
        shifts = [0, 1]

        # I do not trust timers while testing (since they trigger asynchronous
        # rendering/interaction)

        use_timers = style.GetUseTimers()
        style.UseTimersOff()
        style.AutoAdjustCameraClippingRangeOn()

        print "Testing: " + style.GetClassName()

        iren = style.GetInteractor()
        renwin = iren.GetRenderWindow()

        renwin.Render()

        # Get renwin size and center

        win_size = renwin.GetSize()
        win_center_x = win_size[0] / (2.0)
        win_center_y = win_size[1] / (2.0)

        pick = vtk.vtkPropPicker()

        radius = 5 * (1 + use_timers)

        for ctrl in ctrls:
            for shift in shifts:
                print " - ctrl: " + str(ctrl) + " shift: " + str(shift) + " " + "button: ",
                for button in buttons:
                    print button,
                    # First try to find a starting position where an actor
                    # can be picked (not mandatory for trackball modes).
                    # Search in increasingly big area, until we reach win size
                    # in that case actors might not be on screen, so reset cam

                    search = radius
                    while True:
                        start_x = self.randint(win_center_x - search, win_center_x + search)
                        start_y = self.randint(win_center_x - search, win_center_x + search)
                        if pick.PickProp(start_x, start_y, self.ren1):
                            break
                        else:
                            if search > win_center_x or search > win_center_y:
                                print "   (resetting camera)",
                                self.ren1.ResetCamera()
                                search = radius
                            else:
                                search += 5

                    # Start by pressing the button

                    iren.SetEventInformationFlipY(start_x, start_y, ctrl, shift, '', 0, '')
                    eval('iren.InvokeEvent("' + button + 'ButtonPressEvent")')
                    pos = iren.GetEventPosition()
                    #print " - Starting: " + str(pos)

                    # Now move around (alternating left and right around
                    # the window center in order to compensate somehow).

                    sign = 1
                    for i in range(0, 2 + use_timers):
                        sign *= -1
                        x = self.randint(win_center_x + radius * 2 * sign, win_center_y + radius * sign)
                        y = self.randint(win_center_y + radius * 2 * sign, win_center_y + radius * sign)
                        iren.SetEventInformationFlipY(x, y, ctrl, shift, '', 0, '')
                        #pos = iren.GetEventPosition()
                        #lastPos = iren.GetLastEventPosition()
                        #print " - Moving:   " + str(pos) + " " + str(ctrl) + " " + str(shift) + " (was " + str(lastPos) + ")"
                        iren.InvokeEvent("MouseMoveEvent")

                        # If this style uses timers, run OnTimer multiple times

                        if use_timers:
                            for j in range(0, 10):
                                iren.InvokeEvent("TimerEvent")

                        renwin.Render()


                    # End by releasing the button

                    iren.SetEventInformationFlipY(x, y, ctrl, shift, '', 0, '')
                    eval('iren.InvokeEvent("' + button + 'ButtonReleaseEvent")')

                print "."

        style.SetUseTimers(use_timers)

        renwin.Render()
