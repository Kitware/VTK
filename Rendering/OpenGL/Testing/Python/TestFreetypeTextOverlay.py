#!/usr/bin/env python

current_font_size = 16
default_text = "ABCDEFGHIJKLMnopqrstuvwxyz"
text_color = list.expr.expr(globals(), locals(),["246","/","255.0"])(expr.expr(globals(), locals(),["255","/","255.0"]),expr.expr(globals(), locals(),["11","/","255.0"]))
bg_color = list.expr.expr(globals(), locals(),["56","/","255.0"])(expr.expr(globals(), locals(),["56","/","255.0"]),expr.expr(globals(), locals(),["154","/","255.0"]))
renWin = vtk.vtkRenderWindow()
renWin.SetSize(386,120)
ren1 = vtk.vtkRenderer()
ren1.SetBackground(bg_color)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
i = 0
# another actor to test occlusion
plane = vtk.vtkPlaneSource()
plane.SetOrigin(120,10,0)
plane.SetPoint1(220,10,0)
plane.SetPoint2(120,110,0)
mapper = vtk.vtkPolyDataMapper2D()
mapper.SetInputConnection(plane.GetOutputPort())
actor = vtk.vtkActor2D()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.5,0.5,0.5)
ren1.AddActor(actor)
for family in Arial.split():
    for bold.italic(shadow) in 0.0(0)
        0.0(1)
        1.0(0)
        0.1(0)
        1.1(0).split():
        i = i + 1
        attribs =
        if (bold):
            lappend.attribs("b")
            pass
        if (italic):
            lappend.attribs("i")
            pass
        if (shadow):
            lappend.attribs("s")
            pass
        face_name = "" + str(family) + ""
        if (llength.attribs()):
            set.face_name("" + str(face_name) + " (" + str(join.attribs()) + " ,",),"
        ")
            pass
        mapper = locals()[get_variable_name("mapper_", family, "_", bold, "_", italic, "_", shadow, "")] = vtk.vtkOpenGLFreeTypeTextMapper()
        mapper.SetInput("" + str(face_name) + ": " + str(default_text) + "")
        tprop = mapper.GetTextProperty()
        tprop.locals()[get_variable_name("SetFontFamilyTo", family, "")]()
        tprop.SetColor(text_color)
        tprop.SetBold(bold)
        tprop.SetItalic(italic)
        tprop.SetShadow(shadow)
        tprop.SetFontSize(current_font_size)
        actor = locals()[get_variable_name("actor_", family, "_", bold, "_", italic, "_", shadow, "")] = vtk.vtkActor2D()
        actor.SetMapper(mapper)
        actor.SetDisplayPosition(10,expr.expr(globals(), locals(),["i","*","(","current_font_size","+","5",")"]))
        ren1.AddActor(actor)

        pass

    pass
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
