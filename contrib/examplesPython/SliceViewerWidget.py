#!/usr/bin/env python

"""
A moderately advanced image viewer.

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
the SliceViewerWidget.
"""

try:
    from vtkpython import *
except:
    from libVTKCommonPython import *
    from libVTKGraphicsPython import *
    from libVTKImagingPython import *
    from libVTKContribPython import *

from Tkinter import *
import Tkinter
import math, os

class SliceViewerWidget(Tkinter.Widget):
    def __init__(self, master, cnf={}, **kw):
        try: # check for VTK_TK_WIDGET_PATH environment variable
	    tkWidgetPath = os.environ['VTK_TK_WIDGET_PATH']
        except KeyError:
            tkWidgetPath = "."

        try: # try specified path or current directory
            master.tk.call('load',os.path.join(tkWidgetPath, \
                                               'vtkTkRenderWidget'))
        except: # try tcl/tk load path
            master.tk.call('load','vtkTkRenderWidget')

        try: # check to see if a render window was specified
            renderWindow = kw['rw']
        except KeyError:
            renderWindow = vtkRenderWindow()

        kw['rw'] = renderWindow.GetAddressAsString("vtkRenderWindow")
        Tkinter.Widget.__init__(self, master, 'vtkTkRenderWidget', cnf, kw)
        
        self._ImageReslice = vtkImageReslice()
        self._ImageReslice.SetInterpolationModeToCubic()

        self._LookupTable = vtkLookupTable()
        self._LookupTable.SetSaturationRange(0,0)
        self._LookupTable.SetValueRange(0,1)
        self._LookupTable.SetTableRange(0,2000)
        self._LookupTable.Build()

        self._OriginalColorWindow = 2000
        self._OriginalColorLevel = 1000

        self._ImageColor = vtkImageMapToColors()
        self._ImageColor.SetInput(self._ImageReslice.GetOutput())
        self._ImageColor.SetOutputFormatToRGB()
        self._ImageColor.SetLookupTable(self._LookupTable)
        
        self._ImageMapper = vtkImageMapper()        
        self._ImageMapper.SetInput(self._ImageColor.GetOutput())
        self._ImageMapper.SetColorWindow(255.0)
        self._ImageMapper.SetColorLevel(127.5)

        self._Actor = vtkActor2D()
        self._Actor.SetMapper(self._ImageMapper)

        renderer = vtkRenderer()
        renderer.AddActor2D(self._Actor)
        self._RenderWindow.AddRenderer(renderer)

        self.BindSliceViewerWidget()

    def __getattr__(self,attr):
        # because the tk part of vtkTkRenderWidget must have
        # the only remaining reference to the RenderWindow when
        # it is destroyed, we can't actually store the RenderWindow
        # as an attribute but instead have to get it from the tk-side
        if attr == '_RenderWindow':
            return self.GetRenderWindow()
        if attr == '_Renderer':
            rens = self.GetRenderWindow().GetRenderers()
            rens.InitTraversal()
            return rens.GetNextItem()

    def SetResliceAxes(self,*args):
        try:
            apply(self._ImageReslice.SetResliceAxes,args)
        except:
            apply(self._ImageReslice.SetResliceAxesDirectionCosines,args)

    def GetResliceAxes(self):
        return self._ImageReslice.GetResliceAxes()

    def SetResliceTransform(self,transform): 
        self._ImageReslice.SetResliceTransform(transform)

    def GetResliceTransform(self):
        return self._ImageReslice.GetResliceTransform()

    def SetLookupTable(self,table):
        if self._Table == table:
            return
        self._LookupTable = table
        self._ImageColor.SetLookupTable(table)
        range = table.GetTableRange()
        self._OriginalColorWindow = range[1]-range[0]
        self._OriginalColorLevel = 0.5*(range[0]+range[1])

    def GetLookupTable(self):
        return self._LookupTable

    def GetRenderWindow(self):
        addr = self.tk.call(self._w, 'GetRenderWindow')[5:]
        return vtkRenderWindow('_%s_vtkRenderWindow_p' % addr)
    
    def BindSliceViewerWidget(self):
        self._LastX = 0
        self._LastY = 0
        
        self.bind('<Any-ButtonPress>',
                  lambda e,s=self: s.StartInteraction(e.x,e.y))
        self.bind('<Any-ButtonRelease>',
                  lambda e,s=self: s.EndInteraction(e.x,e.y))
        
        self.bind('<ButtonPress-4>',
                  lambda e,s=self: s.ChangeSlice(-1))
        self.bind('<ButtonPress-5>',
                  lambda e,s=self: s.ChangeSlice(+1))

        self.bind('<B1-Motion>',
                  lambda e,s=self: s.Slide(e.x,e.y))
        self.bind('<Shift-B1-Motion>',
                  lambda e,s=self: s.Slice(e.x,e.y))
        self.bind('<B2-Motion>',
                  lambda e,s=self: s.Slice(e.x,e.y))
        self.bind('<B3-Motion>',
                  lambda e,s=self: s.Scale(e.x,e.y))

        self.bind('<Shift-B3-Motion>',
                  lambda e,s=self: s.WindowLevel(e.x,e.y))

        self.bind('<KeyPress-r>',
                  lambda e,s=self: s.Reset())

        self.bind('<Configure>',
                  lambda e,s=self: s.Configure(e.width,e.height))
        self.bind("<Enter>",
                  lambda e,s=self: s.Enter())

    def Enter(self):
        self.focus()

    def Configure(self, width, height):

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
    
    def StartInteraction(self,x,y):
        self._ImageReslice.SetInterpolationModeToLinear()
        self._LastX = x
        self._LastY = y

    def EndInteraction(self,x,y):
        self._ImageReslice.SetInterpolationModeToCubic()
        self.Render()

    def Slide(self,x,y):
        reslice = self._ImageReslice
        spacing = reslice.GetOutputSpacing()
        origin = reslice.GetOutputOrigin()
        delta = (self._LastX-x,y-self._LastY)

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

        delta = y-self._LastY
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
            zoomFactor = math.pow(1.02,(0.5*(y-self._LastY)))
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
        range = self._LookupTable.GetTableRange()
        window = range[1]-range[0]
        level = 0.5*(range[0]+range[1])

        owin = self._OriginalColorWindow
        olev = self._OriginalColorLevel

        level = level + (x - self._LastX)*owin/500.0
        window = window + (self._LastY - y)*owin/250.0

        if window <= 0:
            window = 1e-3

        self._LookupTable.SetTableRange(level-window*0.5,
                                        level+window*0.5)

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
        width, height = self._RenderWindow.GetSize() 

        reslice = self._ImageReslice
        reslice.SetInput(input)
        reslice.SetOutputSpacing(spacing,spacing,spacing)
        reslice.SetOutputOriginToDefault()
        if (width > 0 and height > 0):
            reslice.SetOutputExtent(0,width-1,0,height-1,0,0)
        else:
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

    def Render(self):
        self._RenderWindow.Render()


if __name__ == '__main__':
    # short how-to-use example

    reader = vtkImageReader()
    reader.ReleaseDataFlagOff()
    reader.SetDataByteOrderToLittleEndian()
    reader.SetDataSpacing(1.0,1.0,2.0)
    reader.SetDataExtent(0,255,0,255,1,93)
    reader.SetDataOrigin(-127.5,-127.5,-94.0)
    reader.SetFilePrefix('../../../vtkdata/fullHead/headsq')
    reader.SetDataMask(0x7fff)
    reader.UpdateWholeExtent()
    
    # set the slice orientation to view
    sagittal = ( 0, 1, 0,
                 0, 0,-1,
                -1, 0, 0 )
    
    coronal =  ( 1, 0, 0,
                 0, 0,-1,
                 0, 1, 0 )
    
    axial =    ( 1, 0, 0,
                 0, 1, 0,
                 0, 0, 1 )    

    oblique = vtkTransform()
    oblique.RotateWXYZ(-50,1,0,0)

    root = Tk()

    def RenderAll():
        viewer1.Render()
        viewer2.Render()
        viewer3.Render()
        viewer4.Render()

    frame = Frame(root)
    
    viewer1 = SliceViewerWidget(frame,width=256,height=256)
    viewer1.SetResliceAxes(coronal)
    viewer1.SetInput(reader.GetOutput())

    viewer2 = SliceViewerWidget(frame,width=256,height=256)
    viewer2.SetResliceAxes(sagittal)
    viewer2.SetInput(reader.GetOutput())

    viewer3 = SliceViewerWidget(frame,width=256,height=256)
    viewer3.SetResliceAxes(axial)
    viewer3.SetInput(reader.GetOutput())

    viewer4 = SliceViewerWidget(frame,width=256,height=256)
    viewer4.SetResliceAxes(oblique.GetMatrix())
    viewer4.SetInput(reader.GetOutput())

    # uncomment the following to make window/level sync across windows
    """
    table = vtkLookupTable()
    table.SetSaturationRange(0,0)
    table.SetValueRange(0,1)
    table.SetTableRange(0,2000)
    table.Build()

    viewers = [viewer1, viewer2, viewer3, viewer4]

    for viewer in viewers:
        viewer.SetLookupTable(table)

    def RenderAll():
        for viewer in viewers:
            viewer.GetRenderWindow().SwapBuffersOff()
        for viewer in viewers:
            viewer.GetRenderWindow().Render()
        for viewer in viewers:
            viewer.GetRenderWindow().SwapBuffersOn()
        for viewer in viewers:
            viewer.GetRenderWindow().Frame()

    for viewer in viewers:
        viewer.Render = RenderAll
    """

    viewer1.grid(row=1,column=1,sticky='nsew')
    viewer2.grid(row=1,column=2,sticky='nsew')
    viewer3.grid(row=2,column=1,sticky='nsew')
    viewer4.grid(row=2,column=2,sticky='nsew')

    frame.grid_rowconfigure(1,weight=1)
    frame.grid_rowconfigure(2,weight=1)
    frame.grid_columnconfigure(1,weight=1)
    frame.grid_columnconfigure(2,weight=1)

    frame.pack(expand='true',fill='both')
    
    root.bind("<Destroy>",lambda e: e.widget.quit())    
    root.mainloop()


