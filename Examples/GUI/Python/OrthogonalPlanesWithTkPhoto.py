#!/usr/bin/env python
import vtk
from vtk import *
import Tkinter
from Tkinter import *
import sys, os
import vtk.tk
import vtk.tk.vtkLoadPythonTkWidgets
import vtk.tk.vtkTkImageViewerWidget
from vtk.tk.vtkTkPhotoImage import *
from vtk.util.misc import *


class SampleViewer:
    def __init__ ( self ):
        self.Tk = Tk = Tkinter.Tk();
        Tk.title ( 'Python Version of vtkImageDataToTkPhoto' );

        # Image pipeline
        reader = vtkVolume16Reader ()
        reader.SetDataDimensions ( 64, 64 )
        reader.SetDataByteOrderToLittleEndian ( )
        reader.SetFilePrefix ( vtkGetDataRoot() + '/Data/headsq/quarter'  )
        reader.SetImageRange ( 1, 93 )
        reader.SetDataSpacing ( 3.2, 3.2, 1.5 )
        reader.Update ()

        self.cast = cast = vtkImageCast()
        cast.SetInputConnection( reader.GetOutputPort() )
        cast.SetOutputScalarType ( reader.GetOutput().GetScalarType() )
        cast.ClampOverflowOn()

        # Make the image a little bigger
        self.resample = resample = vtkImageResample ()
        resample.SetInputConnection( cast.GetOutputPort() )
        resample.SetAxisMagnificationFactor ( 0, 2 )
        resample.SetAxisMagnificationFactor ( 1, 2 )
        resample.SetAxisMagnificationFactor ( 2, 1 )

        l,h = reader.GetOutput().GetScalarRange()

        # Create the three orthogonal views

        tphoto = self.tphoto = self.tphoto = vtkTkPhotoImage ();
        cphoto = self.cphoto = vtkTkPhotoImage ();
        sphoto = self.sphoto = vtkTkPhotoImage ();
        reader.Update()
        d = reader.GetOutput().GetDimensions()
        self.Position = [ int(d[0]/2.0), int(d[0]/2.0), int(d[0]/2.0) ]

        # Create a popup menu
        v = IntVar()
        self.popup = popup = Menu ( Tk, tearoff=0 )
        popup.add_radiobutton ( label='unsigned char', command=self.CastToUnsignedChar, variable=v, value=-1 )
        popup.add_radiobutton ( label='unsigned short', command=self.CastToUnsignedShort, variable=v, value=0 )
        popup.add_radiobutton ( label='unsigned int', command=self.CastToFloat, variable=v, value=1 )
        popup.add_radiobutton ( label='float', command=self.CastToFloat, variable=v, value=2 )

        v.set ( 0 )

        w = self.TransverseLabelWidget = Label ( Tk, image = tphoto )
        w.grid ( row = 0, column = 0 )
        w.bind ( "<Button1-Motion>", lambda e, i=tphoto, o='transverse', s=self: s.Motion ( e, i, o ) )
        w.bind ( "<Button-3>", self.DoPopup )
        w = Label ( Tk, image = cphoto )
        w.grid ( row = 1, column = 0 )
        w.bind ( "<Button1-Motion>", lambda e, i=cphoto, o='coronal', s=self: s.Motion ( e, i, o ) )
        w.bind ( "<Button-3>", self.DoPopup )
        w = Label ( Tk, image = sphoto )
        w.grid ( row = 0, column = 1 )
        w.bind ( "<Button1-Motion>", lambda e, i=sphoto, o='sagittal', s=self: s.Motion ( e, i, o ) )
        w.bind ( "<Button-3>", self.DoPopup )
        w = self.WindowWidget = Scale ( Tk, label='Window', orient='horizontal', from_=1, to=(h-l)/2, command = self.SetWindowLevel )
        w = self.LevelWidget = Scale ( Tk, label='Level', orient='horizontal', from_=l, to=h, command=self.SetWindowLevel )
        self.WindowWidget.grid ( row=2, columnspan=2, sticky='ew' )
        self.LevelWidget.grid ( row=3, columnspan=2, sticky='ew' );
        self.WindowWidget.set ( 1370 );
        self.LevelWidget.set ( 1268 );

        w = self.LabelWidget = Label ( Tk, bd=2, relief='raised' )
        w.grid ( row=4, columnspan=2, sticky='ew' )
        w.configure ( text = "Use the right mouse button to change data type" )


    def DoPopup ( self, event ):
        self.popup.post ( event.x_root, event.y_root )

    def CastToUnsignedChar ( self ):
        self.cast.SetOutputScalarTypeToUnsignedChar()
        self.SetImages()
    def CastToUnsignedShort ( self ):
        self.cast.SetOutputScalarTypeToUnsignedShort()
        self.SetImages()
    def CastToUnsignedInt ( self ):
        self.cast.SetOutputScalarTypeToUnsignedInt()
        self.SetImages()
    def CastToFloat ( self ):
        self.cast.SetOutputScalarTypeToFloat()
        self.SetImages()


    def Motion ( self, event, image, orientation ):
        w = image.width();
        h = image.height()
        if orientation == 'transverse':
            self.Position[0] = event.x
            self.Position[1] = h - event.y - 1
        if orientation == 'coronal':
            self.Position[0] = event.x;
            self.Position[2] = event.y
        if orientation == 'sagittal':
            self.Position[1] = w - event.x - 1
            self.Position[2] = event.y
        self.LabelWidget.configure ( text = "Position: %d, %d, %d" % tuple ( self.Position ) )
        self.SetImages()

    def SetWindowLevel ( self, event ):
        self.SetImages()

    def SetImages ( self ):
        Window = self.WindowWidget.get()
        Level = self.LevelWidget.get()
        image = self.resample.GetOutput()
        self.tphoto.PutImageSlice ( self.resample.GetOutputPort(),
                                    self.Position[2],
                                    'transverse',
                                    Window,
                                    Level )
        self.sphoto.PutImageSlice ( self.resample.GetOutputPort(),
                                    self.Position[0],
                                    'sagittal',
                                    Window,
                                    Level )
        self.cphoto.PutImageSlice ( self.resample.GetOutputPort(),
                                    self.Position[1],
                                    'coronal',
                                    Window,
                                    Level )



if __name__ == '__main__':
    S = SampleViewer()
    S.Tk.mainloop()
