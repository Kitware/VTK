# vtk_viewer.py
from imgui_bundle import imgui, immapp, hello_imgui

# --- Granular VTK Imports for Sub-second Startup ---
from vtkmodules.vtkRenderingCore import vtkRenderer
from vtkmodules.vtkRenderingOpenGL2 import vtkGenericOpenGLRenderWindow
from vtkmodules.vtkRenderingUI import vtkGenericRenderWindowInteractor
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleTrackballCamera

# (Optional but safe: Ensures OpenGL object factories are registered)
import vtkmodules.vtkRenderingOpenGL2

class VtkViewer:
    """A generic VTK + Dear ImGui application framework.

    Can be used in two modes:

    1. Standalone (no view): creates its own renderer, render window,
       and interactor.  Add actors directly to ``viewer.renderer``.

    2. With a vtkScivisView: pass ``view=`` to the constructor.
       The view's renderer (with its background, lights, etc.) is
       migrated to the imgui-compatible render window.  Add
       representations to the view as usual.
    """

    def __init__(self, view=None, background_color=(0.15, 0.15, 0.2)):
        self.view = view

        # Create the imgui-compatible render window.
        self.render_window = vtkGenericOpenGLRenderWindow(
            own_context=False, off_screen_rendering=True)

        if view is not None:
            # Set up our interactor on the window *before* SetRenderWindow
            # so the view finds it and preserves its interactor style.
            self.interactor = vtkGenericRenderWindowInteractor()
            self.render_window.interactor = self.interactor

            # Migrate the view's renderer and interactor style to our window.
            view.render_window = self.render_window
            self.renderer = view.renderer

            # Rebind the orientation widget to the new interactor.
            # SetInteractor disables the widget, so re-enable it after.
            widget = view.orientation_marker_widget
            if widget:
                widget.interactor = self.interactor
                widget.enabled = 1
        else:
            self.renderer = vtkRenderer(background=background_color, use_fxaa=True)
            self.render_window.AddRenderer(self.renderer)

            self.interactor = vtkGenericRenderWindowInteractor(render_window=self.render_window)
            self.interactor.interactor_style = vtkInteractorStyleTrackballCamera()

        self.is_interacting = False

    def _init_context(self):
        """Internal hook: fires when ImGui creates the OpenGL context."""
        self.render_window.mapped = True
        self.render_window.SetIsCurrent(True)
        self.render_window.OpenGLInitContext()

    def draw_viewport(self):
        """Draws the fully interactive VTK texture into the current ImGui layout."""
        size = imgui.get_content_region_avail()
        if size.x <= 0 or size.y <= 0: return

        pos = imgui.get_cursor_screen_pos()
        self._handle_input(pos, size)

        dpi_scale = imgui.get_io().display_framebuffer_scale
        phys_w, phys_h = int(size.x * dpi_scale.x), int(size.y * dpi_scale.y)

        self.render_window.size = (phys_w, phys_h)
        if self.view is not None:
            self.view.Render()
        else:
            self.render_window.Render()

        fbo = self.render_window.render_framebuffer
        if not fbo: return
        tex = fbo.GetColorAttachmentAsTextureObject(0)
        if not tex: return

        raw_tex_id = tex.handle
        tex_obj = imgui.ImTextureRef(raw_tex_id) if hasattr(imgui, "ImTextureRef") else imgui.ImTextureID(raw_tex_id)

        # Draw with Y-flip correction
        imgui.image(tex_obj, size, uv0=imgui.ImVec2(0, 1), uv1=imgui.ImVec2(1, 0))

    def _handle_input(self, pos, logical_size):
        io = imgui.get_io()
        is_hovered = imgui.is_window_hovered()

        if is_hovered and (imgui.is_mouse_clicked(0) or imgui.is_mouse_clicked(1)):
            self.is_interacting = True

        if not is_hovered and not self.is_interacting:
            return

        dpi_scale = io.display_framebuffer_scale
        rel_x, rel_y = io.mouse_pos.x - pos.x, logical_size.y - (io.mouse_pos.y - pos.y)
        phys_x, phys_y = int(rel_x * dpi_scale.x), int(rel_y * dpi_scale.y)

        self.interactor.SetEventInformation(phys_x, phys_y, int(io.key_ctrl), int(io.key_shift))

        if imgui.is_mouse_clicked(0): self.interactor.LeftButtonPressEvent()
        elif imgui.is_mouse_released(0): self.interactor.LeftButtonReleaseEvent()
        if imgui.is_mouse_clicked(1): self.interactor.RightButtonPressEvent()
        elif imgui.is_mouse_released(1): self.interactor.RightButtonReleaseEvent()

        if io.mouse_wheel > 0: self.interactor.MouseWheelForwardEvent()
        elif io.mouse_wheel < 0: self.interactor.MouseWheelBackwardEvent()

        self.interactor.MouseMoveEvent()

        if imgui.is_mouse_released(0) or imgui.is_mouse_released(1):
            self.is_interacting = False

    def _default_gui(self):
        """The fallback fullscreen GUI if the user doesn't provide one."""
        vp = imgui.get_main_viewport()
        imgui.set_next_window_pos(vp.work_pos)
        imgui.set_next_window_size(vp.work_size)
        flags = imgui.WindowFlags_.no_decoration | imgui.WindowFlags_.no_move | imgui.WindowFlags_.no_saved_settings

        imgui.begin("Main Application", flags=flags)
        self.draw_viewport()
        imgui.end()

    def run(self, custom_gui=None, title="VTK ImGui App", width=1000, height=800):
        """Starts the ImGui application loop."""
        runner_params = hello_imgui.RunnerParams()

        runner_params.callbacks.show_gui = custom_gui if custom_gui else self._default_gui
        runner_params.callbacks.post_init = self._init_context

        runner_params.app_window_params.window_title = title
        runner_params.app_window_params.window_geometry.size = (width, height)

        immapp.run(runner_params)
