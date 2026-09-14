# scalar_bar_example.py
# Two representations coloring by the same array over different ranges.  A
# scalar bar describes the scene rather than either of them, so the view keeps
# one bar for "RTData", spanning both, and it goes away when nothing is drawing
# that array any more.
from imgui_bundle import imgui
from vtk_viewer import VtkViewer

from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkFiltersGeneral import vtkWarpScalar
from vtkmodules.vtkViewsScivis import vtkScivisView

# --- Two sources with the same array name over different ranges ---

# Small extent, centered at the origin.
source1 = vtkRTAnalyticSource(whole_extent=(-10, 10, -10, 10, 0, 0))
source1.Update()
r1 = source1.output.point_data.scalars.range
print(f"Source 1 RTData range: [{r1[0]:.1f}, {r1[1]:.1f}]")

# Larger extent, offset, so it reaches further at both ends.
source2 = vtkRTAnalyticSource(whole_extent=(-20, 20, -20, 20, 0, 0), center=(5, 5, 0))
source2.Update()
r2 = source2.output.point_data.scalars.range
print(f"Source 2 RTData range: [{r2[0]:.1f}, {r2[1]:.1f}]")

# Warp by scalar to give the flat slices some shape.
warp1 = vtkWarpScalar(scale_factor=0.05)
warp1.SetInputConnection(source1.GetOutputPort())

warp2 = vtkWarpScalar(scale_factor=0.05)
warp2.SetInputConnection(source2.GetOutputPort())

# --- View setup ---
view = vtkScivisView(use_light_kit=True)

# position is the actor's, reached through the representation in Python.
rep1 = view.show(warp1, specular=0.3, specular_power=20, position=(-12, 0, 0))
rep1.ColorByPointArray("RTData")

rep2 = view.show(warp2, specular=0.3, specular_power=20, position=(12, 0, 0))
rep2.ColorByPointArray("RTData")

viewer = VtkViewer(view=view)


# --- GUI State ---
class GUIState:
    """Mutable state driving the imgui controls."""

    scalar_bar_auto = True
    draggable = False
    color_by_scalar = [True, True]
    show_edges = [False, False]
    opacity = [1.0, 1.0]


state = GUIState()


def custom_gui():
    vp = imgui.get_main_viewport()
    imgui.set_next_window_pos(vp.work_pos)
    imgui.set_next_window_size(vp.work_size)
    flags = (
        imgui.WindowFlags_.no_decoration
        | imgui.WindowFlags_.no_move
        | imgui.WindowFlags_.no_saved_settings
    )
    imgui.begin("App", flags=flags)

    sidebar_width = 300
    avail = imgui.get_content_region_avail()

    imgui.begin_child(
        "Sidebar", imgui.ImVec2(sidebar_width, avail.y),
        child_flags=imgui.ChildFlags_.borders,
    )

    if imgui.collapsing_header("Scalar Bars", imgui.TreeNodeFlags_.default_open):
        changed, state.scalar_bar_auto = imgui.checkbox(
            "Auto Scalar Bars", state.scalar_bar_auto
        )
        if changed:
            view.scalar_bars.auto_visibility = state.scalar_bar_auto

        changed, state.draggable = imgui.checkbox("Draggable", state.draggable)
        if changed:
            view.scalar_bars.draggable = state.draggable

        # What the view is maintaining right now.  Uncheck both surfaces below
        # and the bar goes with them.
        count = view.scalar_bars.GetNumberOfBars()
        imgui.text(f"{count} bar(s)")
        for i in range(count):
            bar = view.scalar_bars.GetActor(i)
            lo, hi = bar.lookup_table.range
            imgui.text(f"  {view.scalar_bars.GetArrayName(i)}: [{lo:.1f}, {hi:.1f}]")

    imgui.spacing()

    for i, (rep, label) in enumerate(
        [(rep1, "Source 1 (small)"), (rep2, "Source 2 (large)")]
    ):
        if imgui.collapsing_header(label, imgui.TreeNodeFlags_.default_open):
            changed, state.color_by_scalar[i] = imgui.checkbox(
                f"Color by RTData##{i}", state.color_by_scalar[i]
            )
            if changed:
                if state.color_by_scalar[i]:
                    rep.ColorByPointArray("RTData")
                else:
                    rep.scalar_visibility = False
                    rep.color = (0.8, 0.8, 0.8)

            changed, state.opacity[i] = imgui.slider_float(
                f"Opacity##{i}", state.opacity[i], 0.0, 1.0
            )
            if changed:
                rep.opacity = state.opacity[i]

            changed, state.show_edges[i] = imgui.checkbox(
                f"Edges##{i}", state.show_edges[i]
            )
            if changed:
                rep.representation = (
                    "surfacewithedges" if state.show_edges[i] else "surface"
                )

        imgui.spacing()

    imgui.end_child()

    imgui.same_line()
    imgui.begin_child("Viewport", imgui.ImVec2(0, avail.y))
    viewer.draw_viewport()
    imgui.end_child()

    imgui.end()


viewer.run(custom_gui=custom_gui, title="Scalar Bar Demo")
