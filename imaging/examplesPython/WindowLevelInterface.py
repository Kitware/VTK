# a simple user interface that manipulates window level.
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

# places in the Tkinter top window.  

from Tkinter import *
import sys, string


VTK_VOID           =  0
VTK_BIT            =  1
VTK_CHAR           =  2
VTK_UNSIGNED_CHAR  =  3
VTK_SHORT          =  4
VTK_UNSIGNED_SHORT =  5
VTK_INT            =  6
VTK_UNSIGNED_INT   =  7
VTK_LONG           =  8
VTK_UNSIGNED_LONG  =  9
VTK_FLOAT          = 10
VTK_DOUBLE         = 11

VTK_IMAGE_X_AXIS            =  0
VTK_IMAGE_Y_AXIS            =  1
VTK_IMAGE_Z_AXIS            =  2

VTK_CLAW_NEAREST_NETWORK    =  0
VTK_CLAW_NEAREST_MINIMUM    =  1
VTK_CLAW_NEAREST_GLOBAL     =  2
VTK_CLAW_PIONEER_LOCAL      =  3
VTK_CLAW_PIONEER_GLOBAL     =  4
VTK_CLAW_WELL_NOISE         =  6
VTK_CLAW_WELL_DIRECTED_NOISE=  7
VTK_CLAW_MINIMUM_WELL       =  8
VTK_CLAW_INSERT             =  9
VTK_CLAW_NARROW_WEL         =  10



class WindowLevelInterface(Frame):
    def __init__(self,viewer=None,**kw):
        apply(Frame.__init__,(self,),kw)

        if (viewer == None):
            print "Must specify a vtkImageViewer"
            
        self.viewer = viewer        

        w = viewer.GetColorWindow()
        l = viewer.GetColorLevel()
        sliceNumber = viewer.GetZSlice()
        zMin = viewer.GetWholeZMin()
        zMax = viewer.GetWholeZMax()

        self.slice = Frame(self)
        self.slice_label = Label(self.slice,text="Slice")
        self.slice_scale = Scale(self.slice,from_=zMin,to=zMax, \
                                 orient='horizontal', \
                                 command=self.SetSlice, \
                                 variable='sliceNumber')
        
        self.wl = Frame(self)
        self.wl_f1 = Frame(self.wl)
        self.wl_f1_windowLabel = Label(self.wl_f1,text="Window")
        self.wl_f1_window = Scale(self.wl_f1,from_=1,to=w*2, \
                                  orient='horizontal', \
                                  command=self.SetWindow, \
                                  variable='window')
        
        self.wl_f2 = Frame(self.wl)
        self.wl_f2_levelLabel = Label(self.wl_f2,text="Level")
        self.wl_f2_level = Scale(self.wl_f2,from_=l-w,to=l+w, \
                                  orient='horizontal', \
                                  command=self.SetLevel, \
                                  variable='level')
        
        self.wl_video = Checkbutton(self.wl,text="Inverse Video", \
                                    command=self.SetInverseVideo,\
                                    variable='video')

        # resolutions less than 1.0
        if (w < 20):
            res = 0.05*w
            self.wl_f1_window.configure(resolution=res,\
                                        from_=res,to=2.0*2)
            self.wl_f2_level.configure(resolution=res,\
                                       from_=1.0*(l-w),to=1.0*(l+w))

        self.slice_scale.set(sliceNumber)
        self.wl_f1_window.set(w)
        self.wl_f2_level.set(l)

        self.ex = Frame(self)
        self.ex_exit = Button(self.ex,text="Exit",\
                              command=sys.exit)


        self.slice.pack(side='top')
        self.wl.pack(side='top')
        self.ex.pack(side='top')
        self.slice_label.pack(side='left')
        self.slice_scale.pack(side='left')
        self.wl_f1.pack(side='top')
        self.wl_f2.pack(side='top')
        self.wl_video.pack(side='top')
        self.wl_f1_windowLabel.pack(side='left')
        self.wl_f1_window.pack(side='left')
        self.wl_f2_levelLabel.pack(side='left')
        self.wl_f2_level.pack(side='left')
        self.ex_exit.pack(side='left')

        self.pack()
        self.mainloop()

    def SetSlice(self,slice):
        self.viewer.SetZSlice(string.atoi(slice))
        self.viewer.Render()

    def SetWindow(self,window):
        if string.atoi(self.getvar('video')):
            self.viewer.SetColorWindow(-string.atof(window))
        else:
            self.viewer.SetColorWindow(string.atof(window))
        self.viewer.Render()

    def SetLevel(self,level):
        self.viewer.SetColorLevel(string.atof(level))
        self.viewer.Render()

    def SetInverseVideo(self):
        if string.atoi(self.getvar('video')):
            self.viewer.SetColorWindow(-string.atof(self.getvar('window')))
        else:
            self.viewer.SetColorWindow(string.atof(self.getvar('window')))
        self.viewer.Render()


