import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from VTK import *
from Tkinter import *

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93) 
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

mapper2 = vtkImageMapper()
mapper2.SetInput(reader.GetOutput())
mapper2.SetColorWindow(2000)
mapper2.SetColorLevel(1000)
mapper2.SetZSlice(50) 

actor2 = vtkActor2D()
actor2.SetMapper(mapper2)

vtext = vtkVectorText()
vtext.SetText("Imagine!")

trans = vtkTransform()
trans.Scale(25,25,25)

tpd = vtkTransformPolyDataFilter()
tpd.SetTransform(trans)
tpd.SetInput(vtext.GetOutput())

textMapper = vtkPolyDataMapper2D()
textMapper.SetInput(tpd.GetOutput())

coord = vtkCoordinate()
coord.SetCoordinateSystemToNormalizedViewport()
coord.SetValue(0.5,0.5)

textActor = vtkActor2D()
textActor.SetMapper(textMapper)
textActor.GetProperty().SetColor(0.7,1.0,1.0)
textActor.GetPositionCoordinate().SetReferenceCoordinate(coord)
textActor.GetPositionCoordinate().SetCoordinateSystemToViewport()
textActor.GetPositionCoordinate().SetValue(-80,-20)

imager1 = vtkImager() 
imager1.AddActor2D(textActor)

imgWin = vtkImageWindow()
imgWin.AddImager(imager1)

top = Toplevel()
top_f1 = Frame(top)

top_f1_r1 = vtkTkImageWindowWidget(top_f1,width=256,height=256,iw=imgWin)

top_btn = Button(top,text="Quit",command=top.tk.quit)

top.tk.call('wm','withdraw','.')
top_f1_r1.pack(side='left',padx=3,pady=3,fill='both',expand='t')
top_f1.pack(fill='both',expand='t')
top_btn.pack(fill='x')

imager1.SetBackground(0.1,0.0,0.6)

top.mainloop()
