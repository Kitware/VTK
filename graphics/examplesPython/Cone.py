from VTK import *

# Make a root window
root = Tk() 

# Add a vtkTkRenderWidget
rw = vtkTkRenderWidget(root,width=200,height=200)
rw.pack(expand='true',fill='both')

# Make a quit button
def quit():
    root.destroy()
    
button = Button(text="Quit",command=quit)
button.pack(expand='true',fill='x')


# Get the render window 
renWin = rw.GetRenderWindow()

# Next, do the VTK stuff
ren = vtkRenderer()
renWin.AddRenderer(ren)
cone = vtkConeSource()
cone.SetResolution(16)
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)
ren.AddActor(coneActor)
coneMapper.GetLookupTable().Build()

# Create a scalar bar
scalarBar = vtkScalarBarActor()
scalarBar.SetLookupTable(coneMapper.GetLookupTable())
scalarBar.SetTitle("Temperature")
scalarBar.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
scalarBar.GetPositionCoordinate().SetValue(0.1, 0.01)
scalarBar.SetOrientationToHorizontal()
scalarBar.SetWidth(0.8)
scalarBar.SetHeight(0.17)
ren.AddActor2D(scalarBar)

# Finally, start up the event loop
root.mainloop()
