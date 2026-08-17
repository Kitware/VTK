# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "imgui-bundle",
#     # VTK::ViewsScivis is not part of a released wheel yet.
#     "vtk>=9.7",
# ]
# ///
# tet_example.py
# Renders vtkRTAnalyticSource -> vtkDataSetTriangleFilter (tetrahedral mesh)
# as a surface with selection support.
from imgui_bundle import imgui
from vtk_viewer import VtkViewer

from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkFiltersGeneral import vtkDataSetTriangleFilter

# vtkDataObject provides the field association constants; importing
# vtkCommonDataModel also registers the pythonic vtkSelection API used below.
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkViewsScivis import vtkScivisSelector, vtkScivisView

# --- VTK Setup ---
view = vtkScivisView(use_light_kit=True)

source = vtkRTAnalyticSource(whole_extent=(-10, 10, -10, 10, -10, 10))
rep = view.show(source >> vtkDataSetTriangleFilter(),
                color=(0.6, 0.75, 0.9), specular=0.3, specular_power=20)

viewer = VtkViewer(view=view)

# --- GUI State ---
class GUIState:
    """Mutable state driving the imgui controls."""

    interaction_mode = 0  # 0=Camera, 1=Selection
    selection_mode = vtkScivisSelector.SURFACE
    field_assoc = vtkDataObject.FIELD_ASSOCIATION_CELLS
    drag_start = None
    selection_info = ""
    rep_color = [0.6, 0.75, 0.9]
    rep_opacity = 1.0
    show_edges = False


state = GUIState()


def on_view_selection(caller, event):
    sel = rep.current_selection
    if not sel or len(sel) == 0:
        state.selection_info = "No selection"
        return
    lines = []
    for node in sel:
        field = "points" if node.field_type == "POINT" else "cells"
        ids = node.selection_list
        if node.content_type == "FRUSTUM":
            lines.append(f"{field} (frustum)")
        elif ids:
            lines.append(f"{ids.number_of_tuples} {field}")
    state.selection_info = "\n".join(lines) if lines else "No selection"


# Selection is the selector's concern, so that is what to listen to.
view.selector.AddObserver(vtkCommand.SelectionChangedEvent, on_view_selection)


def set_interaction_mode(mode):
    state.interaction_mode = mode
    if mode == 0:
        view.interaction_mode = "3d"
    else:
        view.interaction_mode = "selection"


def custom_gui():
    if imgui.is_key_pressed(imgui.Key.s) and not imgui.get_io().want_text_input:
        set_interaction_mode(1 if state.interaction_mode == 0 else 0)

    vp = imgui.get_main_viewport()
    imgui.set_next_window_pos(vp.work_pos)
    imgui.set_next_window_size(vp.work_size)
    flags = (
        imgui.WindowFlags_.no_decoration
        | imgui.WindowFlags_.no_move
        | imgui.WindowFlags_.no_saved_settings
    )
    imgui.begin("App", flags=flags)

    sidebar_width = 280
    avail = imgui.get_content_region_avail()

    # --- Sidebar ---
    imgui.begin_child("Sidebar", imgui.ImVec2(sidebar_width, avail.y), child_flags=imgui.ChildFlags_.borders)

    if imgui.collapsing_header("Selection", imgui.TreeNodeFlags_.default_open):
        if imgui.radio_button("Camera", state.interaction_mode == 0):
            set_interaction_mode(0)
        imgui.same_line()
        if imgui.radio_button("Select", state.interaction_mode == 1):
            set_interaction_mode(1)
        imgui.text_colored(imgui.ImVec4(0.5, 0.5, 0.5, 1.0), "(press 's' to toggle)")

        if state.interaction_mode == 1:
            changed, state.selection_mode = imgui.combo(
                "Mode", state.selection_mode, ["Surface", "Frustum"]
            )
            if changed:
                view.selector.mode = state.selection_mode

            # Combo order matches vtkDataObject: FIELD_ASSOCIATION_POINTS=0,
            # FIELD_ASSOCIATION_CELLS=1.
            changed, state.field_assoc = imgui.combo(
                "Field", state.field_assoc, ["Points", "Cells"]
            )
            if changed:
                if state.field_assoc == vtkDataObject.FIELD_ASSOCIATION_POINTS:
                    view.selector.SelectPoints()
                else:
                    view.selector.SelectCells()

        if imgui.button("Clear Selection"):
            view.selector.Clear()

    if state.selection_info:
        imgui.spacing()
        imgui.separator()
        imgui.text("Selection info:")
        imgui.text_wrapped(state.selection_info)
        imgui.separator()

    imgui.spacing()

    if imgui.collapsing_header("Appearance", imgui.TreeNodeFlags_.default_open):
        changed, state.rep_color = imgui.color_edit3("Color", state.rep_color)
        if changed:
            rep.color = tuple(state.rep_color)

        changed, state.rep_opacity = imgui.slider_float("Opacity", state.rep_opacity, 0.0, 1.0)
        if changed:
            rep.opacity = state.rep_opacity

        changed, state.show_edges = imgui.checkbox("Show Edges", state.show_edges)
        if changed:
            rep.representation = "surfacewithedges" if state.show_edges else "surface"

    imgui.end_child()

    # --- Viewport ---
    imgui.same_line()
    imgui.begin_child("Viewport", imgui.ImVec2(0, avail.y))

    viewer.draw_viewport()

    if state.interaction_mode == 1:
        io = imgui.get_io()
        if imgui.is_mouse_clicked(0) and imgui.is_window_hovered():
            state.drag_start = (io.mouse_pos.x, io.mouse_pos.y)

        if state.drag_start is not None and imgui.is_mouse_down(0):
            draw_list = imgui.get_foreground_draw_list()
            p1 = imgui.ImVec2(state.drag_start[0], state.drag_start[1])
            p2 = io.mouse_pos
            draw_list.add_rect_filled(p1, p2, imgui.IM_COL32(77, 128, 255, 40))
            draw_list.add_rect(p1, p2, imgui.IM_COL32(255, 255, 255, 200), 0.0, 0, 1.5)

        if imgui.is_mouse_released(0):
            if state.drag_start is not None:
                set_interaction_mode(0)
            state.drag_start = None

    imgui.end_child()

    imgui.end()


viewer.run(custom_gui=custom_gui, title="Tetrahedral Mesh Selection Demo")
