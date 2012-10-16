#!/usr/bin/env python

current_font_size = 55
default_text = "MmNnKk @"
text_color = list.expr.expr(globals(), locals(),["246","/","255.0"])(expr.expr(globals(), locals(),["255","/","255.0"]),expr.expr(globals(), locals(),["11","/","255.0"]))
bg_color = list.expr.expr(globals(), locals(),["56","/","255.0"])(expr.expr(globals(), locals(),["56","/","255.0"]),expr.expr(globals(), locals(),["154","/","255.0"]))
renWin = vtk.vtkRenderWindow()
renWin.SetSize(790,450)
ren1 = vtk.vtkRenderer()
ren1.SetBackground(bg_color)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
i = 0
for family in Arial
    Courier
    Times.split():
    for bold.italic() in 0.0()
        1.1().split():
        i = i + 1
        attribs =
        if (bold):
            lappend.attribs("b")
            pass
        if (italic):
            lappend.attribs("i")
            pass
        face_name = "" + str(family) + ""
        if (llength.attribs()):
            set.face_name("" + str(face_name) + " (" + str(join.attribs()) + " ,",),"
        ")
            pass
        mapper = locals()[get_variable_name("mapper_", family, "_", bold, "_", italic, "")] = vtk.vtkOpenGLFreeTypeTextMapper()
        mapper.SetInput("" + str(face_name) + ": " + str(default_text) + "")
        tprop = mapper.GetTextProperty()
        tprop.locals()[get_variable_name("SetFontFamilyTo", family, "")]()
        tprop.SetColor(text_color)
        tprop.SetBold(bold)
        tprop.SetItalic(italic)
        tprop.SetShadow(1)
        tprop.SetFontSize(current_font_size)
        actor = locals()[get_variable_name("actor_", family, "_", bold, "_", italic, "")] = vtk.vtkActor2D()
        actor.SetMapper(mapper)
        actor.SetDisplayPosition(10,expr.expr(globals(), locals(),["i","*","(","current_font_size","+","5",")"]))
        ren1.AddActor(actor)

        pass

    pass
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
