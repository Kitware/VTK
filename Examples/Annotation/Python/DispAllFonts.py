#!/usr/bin/env python

# This example displays all possible combinations of font families and
# styles.  This example also shows how to create a simple Tkinter
# based GUI for VTK-Python.

import vtk
import Tkinter
from vtk.tk.vtkTkRenderWindowInteractor import \
     vtkTkRenderWindowInteractor
import string

# We set the font size constraints, default text and colors
current_font_size = 16
min_font_size = 3
max_font_size = 50

default_text = "ABCDEFGHIJKLMnopqrstuvwxyz(0123456789, !@#%()-=_+.:,./<>?"

# set default_text "The quick red fox"

text_color = [246/255.0, 255/255.0, 11/255.0]
bg_color = [56/255.0, 56/255.0, 154/255.0]

# We create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer.
# Do not set the size of the window here.
renWin = vtk.vtkRenderWindow()
ren = vtk.vtkRenderer()
ren.SetBackground(bg_color)
renWin.AddRenderer(ren)

# We create text actors for each font family and several combinations
# of bold, italic and shadowed style.
text_actors = []
for family in ("Arial", "Courier", "Times"):
    for (bold, italic, shadow) in ((0, 0, 0), (0, 0, 1), (1, 0, 0),
                                   (0, 1, 0), (1, 1, 0)):
        mapper = vtk.vtkTextMapper()
        attribs = []
        if bold:
            attribs.append("b")
        if italic:
            attribs.append("i")
        if shadow:
            attribs.append("s")

        face_name = family
        if len(attribs):
            face_name = face_name + "(" + \
                        string.join(attribs, ",") + ")"

        mapper.SetInput(face_name + ": " + default_text)
        tprop = mapper.GetTextProperty()
        eval("tprop.SetFontFamilyTo%s()"%family)
        tprop.SetColor(text_color)
        tprop.SetBold(bold)
        tprop.SetItalic(italic)
        tprop.SetShadow(shadow)

        actor = vtk.vtkActor2D()
        actor.SetMapper(mapper)
        text_actors.append(actor)
        ren.AddActor(actor)


# Now setup the Tkinter GUI.

# Create the root window.
root = Tkinter.Tk()

# vtkTkRenderWindowInteractor is a Tk widget that we can render into.
# It has a GetRenderWindow method that returns a vtkRenderWindow.
# This can then be used to create a vtkRenderer and etc. We can also
# specify a vtkRenderWindow to be used when creating the widget by
# using the rw keyword argument, which is what we do here by using
# renWin. It also takes width and height options that can be used to
# specify the widget size, hence the render window size.
vtkw = vtkTkRenderWindowInteractor(root, rw=renWin, width=800)


# Once the VTK widget has been created it can be inserted into a whole
# Tk GUI as well as any other standard Tk widgets.

# This function is called by the slider-widget handler whenever the
# slider value changes (either through user interaction or
# programmatically). It receives the slider value as parameter. We
# update the corresponding VTK objects by calling the SetFontSize
# method using this parameter and we render the scene to update the
# pipeline.
def set_font_size(sz):
    global text_actors, renWin
    size = int(sz)
    i = 0
    for actor in text_actors:
        i += 1
        actor.GetMapper().GetTextProperty().SetFontSize(size)
        actor.SetDisplayPosition(10, i*(size+5))

    renWin.SetSize(800, 20+i*(size+5))
    renWin.Render()

# We create a size slider controlling the font size.  The orientation
# of this widget is horizontal (orient option). We label it using the
# label option. Finally, we bind the scale to Python code by assigning
# the command option to the name of a Python function.  Whenever the
# slider value changes this function will be called, enabling us to
# propagate this GUI setting to the corresponding VTK object.
size_slider = Tkinter.Scale(root, from_=min_font_size,
                            to=max_font_size, res=1,
                            orient='horizontal', label="Font size:",
                            command=set_font_size)
size_slider.set(current_font_size)

# Finally we pack the VTK widget and the sliders on top of each other
# (side='top') inside the main root widget.
vtkw.Initialize()
size_slider.pack(side="top", fill="both")
vtkw.pack(side="top", fill='both', expand=1)


# Define a quit method that exits cleanly.
def quit(obj=root):
    obj.quit()

# We handle the WM_DELETE_WINDOW protocal request. This request is
# triggered when the widget is closed using the standard window
# manager icons or buttons. In this case the quit function will be
# called and it will free up any objects we created then exit the
# application.
root.protocol("WM_DELETE_WINDOW", quit)

renWin.Render()
vtkw.Start()

# start the Tkinter event loop.
root.mainloop()
