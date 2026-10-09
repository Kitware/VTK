# Shared layout for the widget examples: a sidebar of readouts beside the view.
from imgui_bundle import imgui
from vtk_viewer import VtkViewer


def run(view, title, sidebar, hint=None, sidebar_width=280):
    """Show *view* with *sidebar* drawn beside it.

    sidebar is called each frame to draw the readouts; hint is a line of grey
    text under them saying what to do with the mouse.
    """
    viewer = VtkViewer(view=view)

    def custom_gui():
        vp = imgui.get_main_viewport()
        imgui.set_next_window_pos(vp.work_pos)
        imgui.set_next_window_size(vp.work_size)
        imgui.begin("App", flags=(imgui.WindowFlags_.no_decoration
                                  | imgui.WindowFlags_.no_move
                                  | imgui.WindowFlags_.no_saved_settings))
        avail = imgui.get_content_region_avail()

        imgui.begin_child("Sidebar", imgui.ImVec2(sidebar_width, avail.y),
                          child_flags=imgui.ChildFlags_.borders)
        sidebar()
        if hint:
            imgui.spacing()
            for line in hint.split("\n"):
                imgui.text_colored(imgui.ImVec4(0.5, 0.5, 0.5, 1.0), line)
        imgui.end_child()

        imgui.same_line()
        imgui.begin_child("Viewport", imgui.ImVec2(0, avail.y))
        viewer.draw_viewport()
        imgui.end_child()
        imgui.end()

    viewer.run(custom_gui=custom_gui, title=title)
