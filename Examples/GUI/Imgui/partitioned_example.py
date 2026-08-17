# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "imgui-bundle",
#     # VTK::ViewsScivis is not part of a released wheel yet.
#     "vtk>=9.7",
# ]
# ///
# partitioned_example.py
# Tests vtkPartitionedDataSetCollection with vtkSurfaceRepresentation.
# Creates a tetrahedral mesh, redistributes into 4 partitions,
# wraps in a PDSC, colors by vtkCompositeIndex, and tests selection.
from imgui_bundle import imgui
from vtk_viewer import VtkViewer

from vtkmodules.vtkCommonCore import vtkCommand, vtkLookupTable
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkPartitionedDataSet,
    vtkPartitionedDataSetCollection,
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkFiltersGeneral import vtkDataSetTriangleFilter
from vtkmodules.vtkFiltersParallelDIY2 import vtkRedistributeDataSetFilter
from vtkmodules.vtkViewsScivis import vtkScivisSelector, vtkScivisView

# --- Build the partitioned dataset collection ---
source = vtkRTAnalyticSource(whole_extent=(-10, 10, -10, 10, -10, 10))
tet_mesh = (source >> vtkDataSetTriangleFilter())()
print(f"Tetrahedral mesh: {tet_mesh.number_of_cells} cells, {tet_mesh.number_of_points} points")

pds = vtkPartitionedDataSet()
pds.append(tet_mesh)

rpds = vtkRedistributeDataSetFilter(number_of_partitions=4)(pds)
print(f"Redistributed: {rpds.class_name}, {rpds.number_of_partitions} partitions")
for i, p in enumerate(rpds):
    print(f"  Partition {i}: {p.number_of_cells} cells, {p.number_of_points} points")

# Wrap in a PartitionedDataSetCollection
pdsc = vtkPartitionedDataSetCollection()
pdsc.append(rpds)
print(f"PDSC: {pdsc.number_of_partitioned_data_sets} partitioned dataset(s)")

# --- VTK View Setup ---
view = vtkScivisView(use_light_kit=True)

rep = view.show(pdsc, specular=0.3, specular_power=20)
# Color by composite index to distinguish partitions.
rep.ColorByCellArray("vtkCompositeIndex")

# The lookup table carries the range that scalars are mapped through.
lut = vtkLookupTable(number_of_table_values=4, range=(2, 5))
lut.SetTableValue(0, 0.23, 0.30, 0.75, 1.0)  # blue  (index 2)
lut.SetTableValue(1, 0.87, 0.40, 0.20, 1.0)  # orange (index 3)
lut.SetTableValue(2, 0.17, 0.63, 0.17, 1.0)  # green  (index 4)
lut.SetTableValue(3, 0.84, 0.15, 0.16, 1.0)  # red    (index 5)
rep.color_map = lut

viewer = VtkViewer(view=view)

# --- GUI State ---
class GUIState:
    """Mutable state driving the imgui controls."""

    interaction_mode = 0  # 0=Camera, 1=Selection
    selection_mode = vtkScivisSelector.SURFACE
    field_assoc = vtkDataObject.FIELD_ASSOCIATION_CELLS
    drag_start = None
    selection_info = ""
    rep_opacity = 1.0
    show_edges = False
    color_by_composite = True
    rep_color = [0.6, 0.75, 0.9]


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
        changed, state.color_by_composite = imgui.checkbox("Color by Partition", state.color_by_composite)
        if changed:
            if state.color_by_composite:
                rep.ColorByCellArray("vtkCompositeIndex")
                rep.color_map = lut
            else:
                rep.scalar_visibility = False
                rep.color = tuple(state.rep_color)

        if not state.color_by_composite:
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


viewer.run(custom_gui=custom_gui, title="PDSC Selection Demo")
