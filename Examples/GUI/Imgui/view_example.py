# view_example.py
# Demonstrates vtkStandardRenderView + representations in imgui-bundle
# with interactive GUI controls for lighting, materials, and selection.
from imgui_bundle import imgui
from vtk_viewer import VtkViewer

from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkFiltersSources import (
    vtkSphereSource,
    vtkConeSource,
    vtkCylinderSource,
)

# vtkDataObject provides the field association constants; importing
# vtkCommonDataModel also registers the pythonic vtkSelection API used below.
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkViewsRendering import vtkStandardRenderView

# --- VTK Setup ---
view = vtkStandardRenderView(use_light_kit=True)

# show() creates, connects, configures, and adds a surface representation,
# returning it so it can be tweaked interactively below.
sphere_rep = view.show(vtkSphereSource(theta_resolution=48, phi_resolution=48),
                       color=(0.2, 0.6, 0.9), specular=0.4, specular_power=30)
cone_rep = view.show(vtkConeSource(center=(2.5, 0, 0), resolution=48),
                     color=(0.9, 0.3, 0.2), specular=0.4, specular_power=30)
cyl_rep = view.show(vtkCylinderSource(center=(-2.5, 0, 0), resolution=48),
                    color=(0.2, 0.8, 0.3), specular=0.4, specular_power=30)

viewer = VtkViewer(view=view)

# --- GUI State ---
class GUIState:
    """Mutable state driving the imgui controls."""

    use_light_kit = True
    key_intensity = 0.75
    key_to_fill = 3.0
    key_to_head = 3.0
    key_to_back = 3.5
    key_warmth = 0.6
    fill_warmth = 0.4
    gradient_bg = True
    bg_color = [0.32, 0.34, 0.43]
    bg_color2 = [0.0, 0.0, 0.17]
    show_axes = True
    interaction_mode = 0  # 0=Camera, 1=Selection
    selection_mode = vtkStandardRenderView.SELECTION_MODE_SURFACE
    field_assoc = vtkDataObject.FIELD_ASSOCIATION_CELLS
    drag_start = None  # Screen coords (x, y) when drag starts
    selection_info = ""  # Text summary of last selection


state = GUIState()


class RepState:
    """Per-representation GUI state."""

    def __init__(self, rep, color):
        self.rep = rep
        self.color = list(color)
        self.opacity = 1.0
        self.specular = 0.4
        self.spec_power = 30.0
        self.style = 0  # index into STYLE_NAMES


STYLE_NAMES = ["Surface", "Wireframe", "Points", "Outline", "FeatureEdges"]

rep_states = {
    "Sphere": RepState(sphere_rep, (0.2, 0.6, 0.9)),
    "Cone": RepState(cone_rep, (0.9, 0.3, 0.2)),
    "Cylinder": RepState(cyl_rep, (0.2, 0.8, 0.3)),
}


def on_view_selection(caller, event):
    """Observer for SelectionChangedEvent on the view.

    Queries each representation's AnnotationLink to get per-actor
    selection counts, which works for both surface and frustum modes.
    """
    lines = []
    for name, rs in rep_states.items():
        sel = rs.rep.annotation_link.current_selection
        if not sel or len(sel) == 0:
            continue
        for node in sel:
            field = "points" if node.field_type == "POINT" else "cells"
            ids = node.selection_list
            if node.content_type == "FRUSTUM":
                lines.append(f"{name}: {field} (frustum)")
            elif ids:
                lines.append(f"{name}: {ids.number_of_tuples} {field}")
    state.selection_info = "\n".join(lines) if lines else "No selection"


view.AddObserver(vtkCommand.SelectionChangedEvent, on_view_selection)


def set_interaction_mode(mode):
    """Switch interaction mode and update both VTK view and GUI state."""
    state.interaction_mode = mode
    if mode == 0:
        view.interaction_mode = "3d"
    else:
        view.interaction_mode = "selection"


def custom_gui():
    # Global keybinding: 's' toggles selection mode.
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

    # Sidebar + viewport layout.
    sidebar_width = 280
    avail = imgui.get_content_region_avail()

    # --- Sidebar ---
    imgui.begin_child("Sidebar", imgui.ImVec2(sidebar_width, avail.y), child_flags=imgui.ChildFlags_.borders)

    # Lighting section.
    if imgui.collapsing_header("Lighting", imgui.TreeNodeFlags_.default_open):
        changed, state.use_light_kit = imgui.checkbox("Use Light Kit", state.use_light_kit)
        if changed:
            view.use_light_kit = state.use_light_kit

        if state.use_light_kit:
            changed, state.key_intensity = imgui.slider_float(
                "Key Intensity", state.key_intensity, 0.0, 2.0
            )
            if changed:
                view.key_light_intensity = state.key_intensity

            changed, state.key_to_fill = imgui.slider_float(
                "Key/Fill Ratio", state.key_to_fill, 1.0, 15.0
            )
            if changed:
                view.key_to_fill_ratio = state.key_to_fill

            changed, state.key_to_head = imgui.slider_float(
                "Key/Head Ratio", state.key_to_head, 1.0, 15.0
            )
            if changed:
                view.key_to_head_ratio = state.key_to_head

            changed, state.key_to_back = imgui.slider_float(
                "Key/Back Ratio", state.key_to_back, 1.0, 15.0
            )
            if changed:
                view.key_to_back_ratio = state.key_to_back

            imgui.spacing()
            imgui.text("Warmth (0=cool, 1=warm)")

            changed, state.key_warmth = imgui.slider_float(
                "Key Warmth", state.key_warmth, 0.0, 1.0
            )
            if changed:
                view.key_light_warmth = state.key_warmth

            changed, state.fill_warmth = imgui.slider_float(
                "Fill Warmth", state.fill_warmth, 0.0, 1.0
            )
            if changed:
                view.fill_light_warmth = state.fill_warmth

    imgui.spacing()

    # Background section.
    if imgui.collapsing_header("Background", imgui.TreeNodeFlags_.default_open):
        changed, state.gradient_bg = imgui.checkbox("Gradient", state.gradient_bg)
        if changed:
            view.gradient_background = state.gradient_bg

        changed, state.bg_color = imgui.color_edit3("BG Color", state.bg_color)
        if changed:
            view.background = tuple(state.bg_color)

        if state.gradient_bg:
            changed, state.bg_color2 = imgui.color_edit3("BG Color 2", state.bg_color2)
            if changed:
                view.background2 = tuple(state.bg_color2)

        changed, state.show_axes = imgui.checkbox("Orientation Axes", state.show_axes)
        if changed:
            view.orientation_axes_visibility = state.show_axes

    imgui.spacing()

    # Selection section.
    if imgui.collapsing_header("Selection", imgui.TreeNodeFlags_.default_open):
        # Interaction mode toggle.
        if imgui.radio_button("Camera", state.interaction_mode == 0):
            set_interaction_mode(0)
        imgui.same_line()
        if imgui.radio_button("Select", state.interaction_mode == 1):
            set_interaction_mode(1)
        imgui.text_colored(imgui.ImVec4(0.5, 0.5, 0.5, 1.0), "(press 's' to toggle)")

        if state.interaction_mode == 1:
            # Selection mode.
            changed, state.selection_mode = imgui.combo(
                "Mode", state.selection_mode, ["Surface", "Frustum"]
            )
            if changed:
                view.selection_mode = state.selection_mode

            # Field association. Combo order matches vtkDataObject:
            # FIELD_ASSOCIATION_POINTS=0, FIELD_ASSOCIATION_CELLS=1.
            changed, state.field_assoc = imgui.combo(
                "Field", state.field_assoc, ["Points", "Cells"]
            )
            if changed:
                if state.field_assoc == vtkDataObject.FIELD_ASSOCIATION_POINTS:
                    view.SelectPoints()
                else:
                    view.SelectCells()

        if imgui.button("Clear Selection"):
            view.ClearSelection()

    # Selection info — always visible when there's a selection.
    if state.selection_info:
        imgui.spacing()
        imgui.separator()
        imgui.text("Selection info:")
        imgui.text_wrapped(state.selection_info)
        imgui.separator()

    imgui.spacing()

    # Representation section.
    if imgui.collapsing_header("Representations", imgui.TreeNodeFlags_.default_open):
        for name, rs in rep_states.items():
            imgui.push_id(name)
            if imgui.tree_node(name):
                changed, rs.color = imgui.color_edit3("Color", rs.color)
                if changed:
                    rs.rep.color = tuple(rs.color)

                changed, rs.opacity = imgui.slider_float(
                    "Opacity", rs.opacity, 0.0, 1.0
                )
                if changed:
                    rs.rep.opacity = rs.opacity

                changed, rs.specular = imgui.slider_float(
                    "Specular", rs.specular, 0.0, 1.0
                )
                if changed:
                    rs.rep.specular = rs.specular

                changed, rs.spec_power = imgui.slider_float(
                    "Spec Power", rs.spec_power, 1.0, 100.0
                )
                if changed:
                    rs.rep.specular_power = rs.spec_power

                changed, rs.style = imgui.combo(
                    "Style", rs.style, STYLE_NAMES
                )
                if changed:
                    rs.rep.representation = STYLE_NAMES[rs.style].lower()

                imgui.tree_pop()
            imgui.pop_id()

    imgui.end_child()

    # --- Viewport ---
    imgui.same_line()
    imgui.begin_child("Viewport", imgui.ImVec2(0, avail.y))

    # Capture viewport position before drawing.
    vp_pos = imgui.get_cursor_screen_pos()

    viewer.draw_viewport()

    # Draw rubber band overlay when in selection mode.
    if state.interaction_mode == 1:
        io = imgui.get_io()
        if imgui.is_mouse_clicked(0) and imgui.is_window_hovered():
            state.drag_start = (io.mouse_pos.x, io.mouse_pos.y)

        if state.drag_start is not None and imgui.is_mouse_down(0):
            draw_list = imgui.get_foreground_draw_list()
            p1 = imgui.ImVec2(state.drag_start[0], state.drag_start[1])
            p2 = io.mouse_pos
            # Semi-transparent fill.
            draw_list.add_rect_filled(p1, p2, imgui.IM_COL32(77, 128, 255, 40))
            # Border.
            draw_list.add_rect(p1, p2, imgui.IM_COL32(255, 255, 255, 200), 0.0, 0, 1.5)

        if imgui.is_mouse_released(0):
            # Switch back to camera mode after selection completes.
            if state.drag_start is not None:
                set_interaction_mode(0)
            state.drag_start = None

    imgui.end_child()

    imgui.end()


viewer.run(custom_gui=custom_gui, title="Light Kit Demo")
