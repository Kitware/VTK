#!/usr/bin/env python
"""
A simple image viewer.

Left Button:    Pan
Middle Button:  Slice
Right Button:   Zoom

with 'Shift' held down:

Left Button:    Slice
Middle Button:  Slice
Right Button:   Window/Level (vert/horiz)

special keys:

r:              Reset View


See the end of this file for an example of how to use
the SliceViewer class.
"""

try:
    from vtkpython import *
except:
    from libVTKCommonPython import *
    from libVTKGraphicsPython import *
    from libVTKImagingPython import *
    from libVTKContribPython import *
    
import math

class SliceViewer:
    def __init__(self):

        self._ImageReslice = vtkImageReslice()
        self._ImageReslice.SetInterpolationModeToCubic()
        
        self._ImageMapper = vtkImageMapper()        
        self._ImageMapper.SetInput(self._ImageReslice.GetOutput())
        self._OriginalColorWindow = self._ImageMapper.GetColorWindow()
        self._OriginalColorLevel = self._ImageMapper.GetColorLevel()

        self._Actor = vtkActor2D()
        self._Actor.SetMapper(self._ImageMapper)

        self._Renderer = vtkRenderer()
        self._Renderer.AddActor2D(self._Actor)

        self._RenderWindow = vtkRenderWindow()
        self._RenderWindow.AddRenderer(self._Renderer)
        self._RenderWindow.SetWindowName("SliceViewer")

        self._InteractorStyle = vtkInteractorStyleUser() 
        
        self._Interactor = vtkRenderWindowInteractor()
        self._Interactor.SetRenderWindow(self._RenderWindow)
        self._Interactor.SetInteractorStyle(self._InteractorStyle)

        self.BindSliceViewer()

    def SetSliceAxes(self,*args):
        apply(self._ImageReslice.SetResliceAxesDirectionCosines,args)

    def BindSliceViewer(self):
        self._LastX = 0
        self._LastY = 0

        self._InteractorStyle.SetButtonPressMethod(
            self.OnButtonPress)

        self._InteractorStyle.SetButtonReleaseMethod(
            self.OnButtonRelease)

        self._InteractorStyle.SetMouseMoveMethod(
            self.OnMotion)

        self._InteractorStyle.SetKeyPressMethod(
            self.OnKeyPress)

        self._InteractorStyle.SetKeyReleaseMethod(
            self.OnKeyRelease)

        self._InteractorStyle.SetConfigureMethod(
            self.OnConfigure)

    def OnConfigure(self):
        width, height = self._RenderWindow.GetSize()
        
        reslice = self._ImageReslice
        extent = reslice.GetOutputExtent()
        origin = reslice.GetOutputOrigin()
        spacing = reslice.GetOutputSpacing()

        old_width = extent[1]-extent[0]+1
        old_height = extent[3]-extent[2]+1
        
        extent = list(extent)
        extent[1] = extent[0] + width - 1 
        extent[3] = extent[2] + height - 1
        extent = tuple(extent)

        origin = list(origin)
        origin[0] = origin[0]-(width-old_width)/2*spacing[0] 
        origin[1] = origin[1]-(height-old_height)/2*spacing[1]
        origin = tuple(origin)
        
        reslice.SetOutputExtent(extent)
        reslice.SetOutputOrigin(origin)

        self.Scale(1.0*(old_width-1)/(width-1))
    
    def OnKeyPress(self):
        keysym = self._InteractorStyle.GetKeySym()
        if keysym == "r" or keysym == "R":
            self.Reset()

    def OnKeyRelease(self):
        keysym = self._InteractorStyle.GetKeySym()        

    def OnButtonPress(self):
        x,y = self._InteractorStyle.GetLastPos()
        self._ImageReslice.SetInterpolationModeToLinear()
        self._LastX = x
        self._LastY = y

    def OnButtonRelease(self):
        x,y = self._InteractorStyle.GetLastPos()
        self._ImageReslice.SetInterpolationModeToCubic()
        self.Render()

    def OnMotion(self):
        button = self._InteractorStyle.GetButton()
        x,y = self._InteractorStyle.GetLastPos()
        shift = self._InteractorStyle.GetShiftKey()
        
        if button == 1:
            if shift:
                self.Slice(x,y)
            else:
                self.Slide(x,y)
        elif button == 2:
            self.Slice(x,y)
        elif button == 3:
            if shift:
                self.WindowLevel(x,y)
            else:
                self.Scale(x,y)

    def Slide(self,x,y):
        reslice = self._ImageReslice
        spacing = reslice.GetOutputSpacing()
        origin = reslice.GetOutputOrigin()
        delta = (self._LastX-x,self._LastY-y)

        origin = list(origin)
        for i in xrange(2):
            origin[i] = origin[i]+delta[i]*spacing[i]
        origin = tuple(origin)

        reslice.SetOutputOrigin(origin)
            
        self._LastX = x
        self._LastY = y
        self.Render()

    def Slice(self,x,y):
        reslice = self._ImageReslice
        input = reslice.GetInput()
        extent = reslice.GetOutputExtent()
        origin = reslice.GetOutputOrigin()
        spacing = reslice.GetOutputSpacing()

        reslice.SetOutputExtentToDefault()
        reslice.SetOutputOriginToDefault()
        reslice.SetOutputSpacingToDefault()
        output = reslice.GetOutput()
        output.UpdateInformation()
        lo,hi = output.GetWholeExtent()[4:6]
        s = output.GetSpacing()[2]
        o = output.GetOrigin()[2]
        lo = o + lo*s
        hi = o + hi*s
        orig_lo = min((lo,hi))
        orig_hi = max((lo,hi))

        delta = self._LastY-y
        origin = list(origin)
        origin[2] = origin[2]+delta*spacing[2]
        if (origin[2] > orig_hi):
            origin[2] = orig_hi
        elif (origin[2] < orig_lo):
            origin[2] = orig_lo
        origin = tuple(origin)

        reslice.SetOutputSpacing(spacing)
        reslice.SetOutputOrigin(origin)
        reslice.SetOutputExtent(extent)

        self._LastX = x
        self._LastY = y
        self.Render()        

    def Scale(self,x,y=None):
        if (y == None):
            zoomFactor = x
        else:
            zoomFactor = math.pow(1.02,(0.5*(self._LastY-y)))
        reslice = self._ImageReslice
        extent = reslice.GetOutputExtent()
        spacing = reslice.GetOutputSpacing()
        origin = reslice.GetOutputOrigin()
        center = (0.5*(extent[0]+extent[1])*spacing[0]+origin[0],
                  0.5*(extent[2]+extent[3])*spacing[1]+origin[1],
                  0.5*(extent[4]+extent[5])*spacing[2]+origin[2])

        spacing = list(spacing)
        for i in xrange(3):
            spacing[i] = zoomFactor*spacing[i]
        spacing = tuple(spacing)

        origin = list(origin)
        for i in xrange(2):
            origin[i] = center[i] + zoomFactor*(origin[i]-center[i])
        origin = tuple(origin)

        reslice.SetOutputSpacing(spacing)
        reslice.SetOutputOrigin(origin)

        self._LastX = x
        self._LastY = y
        self.Render()

    def WindowLevel(self,x,y):
        window = self._ImageMapper.GetColorWindow()
        level = self._ImageMapper.GetColorLevel()

        owin = self._OriginalColorWindow
        olev = self._OriginalColorLevel

        level = level + (x - self._LastX)*owin/500.0
        window = window + (y - self._LastY)*owin/250.0

        if window <= 0:
            window = 1e-3

        self._ImageMapper.SetColorWindow(window)
        self._ImageMapper.SetColorLevel(level)

        self._LastX = x
        self._LastY = y
        self.Render()        

    def ChangeSlice(self,inc):
        reslice = self._ImageReslice
        input = reslice.GetInput()
        orig_lo = input.GetOrigin()[2]
        orig_hi = input.GetOrigin()[2]+input.GetSpacing()[2]*\
                  (input.GetWholeExtent()[5]-input.GetWholeExtent()[4])
        if (orig_lo > orig_hi):
            tmp = orig_hi
            orig_hi = orig_lo
            orig_lo = tmp
        origin = reslice.GetOutputOrigin()
        spacing = reslice.GetOutputSpacing()
        origin = list(origin)
        origin[2] = origin[2]+inc*spacing[2]
        if (origin[2] > orig_hi):
            origin[2] = orig_hi
        elif (origin[2] < orig_lo):
            origin[2] = orig_lo
        origin = tuple(origin)
        reslice.SetOutputOrigin(origin)
        
    def Reset(self):
        reslice = self._ImageReslice
        wextent = reslice.GetOutputExtent()
        zorigin = reslice.GetOutputOrigin()[2]

        input = reslice.GetInput()
        input.UpdateInformation()
        newspacing = min(map(abs,input.GetSpacing()))

        reslice.SetOutputExtentToDefault()
        reslice.SetOutputOriginToDefault()
        reslice.SetOutputSpacingToDefault()

        output = reslice.GetOutput()
        output.UpdateInformation()

        # note: don't convert the /2 into *0.5 because we want
        # the result of the division to be an integer
        extent = output.GetWholeExtent()
        spacing = output.GetSpacing()
        zspacing = spacing[2]
        origin = output.GetOrigin()
        center = [(extent[0]+extent[1])/2*spacing[0]+origin[0],
                  (extent[2]+extent[3])/2*spacing[1]+origin[1],
                  (extent[4]+extent[5])/2*spacing[2]+origin[2]]
        
        origin = list(origin)
        for i in xrange(2):
            origin[i] = center[i]-(wextent[2*i+1]+wextent[2*i])/2* \
                        newspacing
        
        # go to nearest slice
        slice = math.floor((center[2] - zorigin)/zspacing+0.5)
        origin[2] = center[2] - slice*zspacing        
        origin = tuple(origin)

        reslice.SetOutputSpacing(newspacing,newspacing,newspacing)
        reslice.SetOutputExtent(wextent)
        reslice.SetOutputOrigin(origin)
        self.Render()

    def SetInput(self,input):
        input.UpdateInformation()
        spacing = min(map(abs,input.GetSpacing()))

        reslice = self._ImageReslice
        reslice.SetInput(input)
        reslice.SetOutputSpacing(spacing,spacing,spacing)
        reslice.SetOutputOriginToDefault()
        reslice.SetOutputExtentToDefault()

        output = reslice.GetOutput()
        output.UpdateInformation()

        extent = output.GetWholeExtent()
        origin = output.GetOrigin()
        origin = list(origin)
        origin[2] = origin[2] + (extent[4] + extent[5])/2*spacing
        origin = tuple(origin)

        reslice.SetOutputOrigin(origin)
        reslice.SetOutputExtent(extent)
        
        self._RenderWindow.SetSize(extent[1]-extent[0]+1,
                                   extent[3]-extent[2]+1)

    def SetColorWindow(self,window):
        self._ImageMapper.SetColorWindow(window)
        self._OriginalColorWindow = window

    def SetColorLevel(self,level):
        self._ImageMapper.SetColorLevel(level)
        self._OriginalColorLevel = level

    def GetInput(self):
        return self._ImageReslice.GetInput()

    def Start(self):
        self._Interactor.Initialize()
        # this next line allows 'ctrl-C' to work
        self._InteractorStyle.SetTimerMethod(lambda: None)
        self._Interactor.Start()

    def Render(self):
        self._Interactor.Render()

if __name__ == '__main__':
    # short how-to-use example

    import os
    try:
        VTK_DATA = os.environ['VTK_DATA']
    except KeyError:
        VTK_DATA = '../../../vtkdata/'

    reader = vtkImageReader()
    reader.ReleaseDataFlagOff()
    reader.SetDataByteOrderToLittleEndian()
    reader.SetDataSpacing(1.0,1.0,-2.0)
    reader.SetDataExtent(0,255,0,255,1,93)
    #reader.SetDataOrigin(-127.5,-127.5,-94.0)
    reader.SetFilePrefix(os.path.join(VTK_DATA,'fullHead/headsq'))
    reader.SetDataMask(0x7fff)
    reader.UpdateWholeExtent()

    # set the slice orientation to view
    sagittal = ( 0, 1, 0,
                 0, 0,-1,
                -1, 0, 0)
    coronal =  ( 1, 0, 0,
                 0, 0,-1,
                 0, 1, 0)
    axial =    ( 1, 0, 0,
                 0, 1, 0,
                 0, 0, 1)
        
    # you _must_ set the initial Window and Level, these
    # will not be set automatically
    viewer = SliceViewer()
    viewer.SetSliceAxes(sagittal)
    viewer.SetInput(reader.GetOutput())
    viewer.SetColorWindow(2000) 
    viewer.SetColorLevel(1000)

    # this method never returns
    viewer.Start()



