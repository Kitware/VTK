import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from VTK import *
from Tkinter import *

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.

reader = vtkPNMReader()
reader.ReleaseDataFlagOff()
reader.SetFileName(VTK_DATA + "/earth.ppm")

viewer = vtkImageViewer()
viewer.SetInput(reader.GetOutput())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
viewer.GetImageWindow().DoubleBufferOn()

# Create the GUI: two renderer widgets and a quit button
#
top = Toplevel()

frame = Frame(top)

ren = vtkTkImageViewerWidget(frame,width=512,height=256,iv=viewer)

button = Button(top,text="Quit",command=top.quit)

top.tk.call('wm','withdraw','.')
ren.pack(side='left',padx=3,pady=3,fill='both',expand='t')
frame.pack(fill='both',expand='t')
button.pack(fill='x')

top.mainloop()

