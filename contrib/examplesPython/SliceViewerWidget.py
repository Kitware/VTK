from VTK import *
from Tkinter import *
import math

class SliceViewerWidget(vtkTkImageWindowWidget):
    def __init__(self,*args,**kw):
        self.__ImageViewer = vtkImageViewer()
        kw['iw'] = self.__ImageViewer.GetImageWindow()
        kw['double'] = 1
        apply(vtkTkImageWindowWidget.__init__,(self,)+args,kw)

        self.__ImageReslice = vtkImageReslice()
        self.__ImageReslice.SetInterpolationModeToCubic()
        self.__ImageReslice.SetOptimization(2)        
        self.__ImageViewer.SetInput(self.__ImageReslice.GetOutput())

        self.BindSliceViewerWidget()

    def BindSliceViewerWidget(self):
        self.__LastX = 0
        self.__LastY = 0
        
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

        self.bind('<KeyPress-r>',
                  lambda e,s=self: s.Reset())

        self.bind('<Configure>',
                  lambda e,s=self: s.Configure(e.width,e.height))
        self.bind("<Enter>",
                  lambda e,s=self: s.Enter())


    def Configure(self,width,height):
        reslice = self.__ImageReslice
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

    def Enter(self):
        self.focus()

    def Slide(self,x,y):
        reslice = self.__ImageReslice
        spacing = reslice.GetOutputSpacing()
        origin = reslice.GetOutputOrigin()
        delta = (self.__LastX-x,y-self.__LastY)

        origin = list(origin)
        for i in xrange(2):
            origin[i] = origin[i]+delta[i]*spacing[i]
        origin = tuple(origin)

        reslice.SetOutputOrigin(origin)
            
        self.__LastX = x
        self.__LastY = y
        self.Render()

    def Slice(self,x,y):
        reslice = self.__ImageReslice
        input = reslice.GetInput()
        spacing = reslice.GetOutputSpacing()
        origin = reslice.GetOutputOrigin()
        delta = y-self.__LastY

        origin = list(origin)
        origin[2] = origin[2]+delta*spacing[2]
        orig_lo = input.GetOrigin()[2]
        orig_hi = input.GetOrigin()[2]+input.GetSpacing()[2]*\
                  (input.GetWholeExtent()[5]-input.GetWholeExtent()[4])
        if (orig_lo > orig_hi):
            tmp = orig_hi
            orig_hi = orig_lo
            orig_lo = tmp
        if (origin[2] > orig_hi):
            origin[2] = orig_hi
        elif (origin[2] < orig_lo):
            origin[2] = orig_lo
        origin = tuple(origin)

        reslice.SetOutputOrigin(origin)

        self.__LastX = x
        self.__LastY = y
        self.Render()        

    def Scale(self,x,y=None):
        if (y == None):
            zoomFactor = x
        else:
            zoomFactor = math.pow(1.02,(0.5*(y - self.__LastY)))
        reslice = self.__ImageReslice
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
        for i in xrange(3):
            origin[i] = center[i] + zoomFactor*(origin[i]-center[i])
        origin = tuple(origin)

        reslice.SetOutputSpacing(spacing)
        reslice.SetOutputOrigin(origin)

        self.__LastX = x
        self.__LastY = y
        self.Render()

    def StartInteraction(self,x,y):
        self.__ImageReslice.SetInterpolationModeToLinear()
        self.__LastX = x
        self.__LastY = y

    def EndInteraction(self,x,y):
        self.__ImageReslice.SetInterpolationModeToCubic()
        self.Render()

    def ChangeSlice(self,inc):
        reslice = self.__ImageReslice
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
        
    def GetImageViewer(self):
        return self.__ImageViewer

    def Reset(self):
        reslice = self.__ImageReslice
        input = reslice.GetInput()
        extent = input.GetWholeExtent()
        spacing = input.GetSpacing()
        origin = input.GetOrigin()
        center = [(extent[0]+extent[1])/2*spacing[0]+origin[0],
                  (extent[2]+extent[3])/2*spacing[1]+origin[1],
                  0]
        
        zspacing = spacing[2]
        zorigin = origin[2]
        
        newspacing = min(spacing)

        extent = reslice.GetOutputExtent()
        spacing = reslice.GetOutputSpacing()
        origin = reslice.GetOutputOrigin()
        center[2] = (extent[4]+extent[5])/2*spacing[2]+origin[2]

        spacing = list(spacing)
        spacing[0] = newspacing
        spacing[1] = newspacing
        spacing[2] = newspacing
        spacing = tuple(spacing)

        origin = list(origin)
        for i in xrange(2):
            origin[i] = center[i]-(extent[2*i+1]+extent[2*i])/2*spacing[i]
        # go to nearest slice, i.e. don't interpolate between slices
        slice = int((center[2]-zorigin)/zspacing+0.5)
        origin[2] = zorigin+slice*zspacing        
        origin = tuple(origin)
        
        reslice.SetOutputSpacing(spacing)
        reslice.SetOutputOrigin(origin)
        self.Render()

    def SetInput(self,input):
        reslice = self.__ImageReslice
        input.UpdateInformation()

        spacing = list(input.GetSpacing())
        extent = list(input.GetWholeExtent())
        newspacing = min(spacing)

        extent[1] = extent[0]+int((extent[1]-extent[0])*\
                                  spacing[0]/newspacing)
        extent[3] = extent[2]+int((extent[3]-extent[2])*\
                                  spacing[1]/newspacing)
        extent[5] = extent[4]
        extent = tuple(extent)

        spacing[0] = newspacing
        spacing[1] = newspacing
        spacing[2] = newspacing
        spacing = tuple(spacing)

        reslice.SetInput(input)
        reslice.SetOutputOrigin(input.GetOrigin())
        reslice.SetOutputExtent(extent)
        reslice.SetOutputSpacing(spacing)

        extent = input.GetWholeExtent()
        self.configure(width=extent[1]-extent[0]+1,
                       height=extent[3]-extent[2]+1)
        self.update()

    def GetInput(self):
        return self.__ImageViewer.GetInput()

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
    
    top = Toplevel()
    
    ren = SliceViewerWidget(top,width=256,height=256)
    ren.SetInput(reader.GetOutput())
    
    top.bind("<Destroy>",lambda e: e.widget.quit())
    
    top.tk.call('wm','withdraw','.')
    ren.pack(side='left',fill='both',expand='t')
    
    top.mainloop()


