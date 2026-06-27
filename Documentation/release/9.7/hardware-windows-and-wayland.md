# Architectural Update: Hardware Window, Wayland, and WebGPU

## ✨ New Wayland Support

### Native Wayland Interactor (vtkWaylandRenderWindowInteractor):
A new, dedicated interactor has been implemented for robust event handling on
Wayland compositors. It processes pointer (mouse motion, buttons, scrolling)
and keyboard inputs directly, using xkbcommon for precise keymap and modifier
state management.

### Native Wayland Display Protocol Support
This version introduces comprehensive, native support for the Wayland display
protocol, providing a modern alternative to X11 on Linux systems. Windows can
request standard decorations (title bars, borders) from the Wayland compositor
by leveraging the xdg-decoration-unstable-v1 protocol. This ensures
applications integrate visually with the user's desktop environment.

### WebGPU on Wayland:
The WebGPU rendering backend is now fully integrated with Wayland surfaces,
allowing high-performance graphics in modern Linux environments.

## 🚀 Platform-Specific Enhancements & WebGPU Integration
Significant advancements have been made to standardize and enhance the
rendering experience across diverse operating systems, including Windows,
macOS, and Linux. Historically, VTK's rendering architecture has been
characterized by a tightly coupled integration between the windowing system and
the graphics stack. However, with the advent of modern graphics APIs, such as
WebGPU, which offer more sophisticated and explicit surface descriptors, VTK
now implements a clear logical separation between its graphics system and the
underlying windowing system.

This architectural shift is embodied by the introduction of the new
`vtkHardwareWindow` class and its specialized sub-classes. These components are
meticulously designed to manage platform-specific window system integrations,
critically, without incorporating any direct calls to the rendering layer. This
distinct separation not only facilitates superior management and independent
evolution of these two crucial codebases but also significantly streamlines the
integration of future graphics and windowing APIs, thereby enhancing
flexibility and long-term maintainability.


### Windows: Improved Interactor and Hardware Window Support
The `vtkWin32RenderWindowInteractor` has been updated to seamlessly support
both the legacy OpenGL render window and the new vtkHardwareWindow abstraction.

The `vtkWin32HardwareWindow` also supports native Windows cursor shapes and
visibility controls.

### Apple (macOS/Cocoa) WebGPU Rendering via Metal
The `vtkCocoaHardwareWindow` now utilizes the WebGPU API with the Metal
backend, bringing modern, high-performance rendering to macOS applications.

The `vtkCocoaHardwareView` is set as the "first responder," allowing it to
properly intercept and process system keyboard and mouse events for reliable
interaction.

### Linux (X11)
Unified Hardware Window: The specialized `vtkXWebGPURenderWindow` has been
removed in favor of the more generic `vtkXlibHardwareWindow`. This simplifies
the architecture by using a single, unified component for X11-based windowing.

## 🔧 Refactoring & Build System Improvements

### Consolidated Win32 Build Logic
A new `VTK_USE_WIN32` CMake option has been introduced to replace disparate
WIN32 checks as well as the opengl dependent variable `VTK_USE_WIN32_OPENGL`.
This unifies conditional compilation and linking for all Win32 components,
simplifying the build configuration for Windows.

### CMake Support for Wayland
With the introduction of `FindWAYLAND.cmake` CMake script, the build system
finds and links against required Wayland libraries and generates necessary
protocol files, making it straightforward to build with Wayland support
enabled. To enable Wayland support, set `VTK_USE_Wayland:BOOL=ON`.

## 🐛 Bug Fixes

### Fixed Xlib Macro Collision
Resolved a critical compilation error on Linux systems where the Xlib Success
macro conflicted with the wgpu::MapAsyncStatus::Success enum, allowing WebGPU
code to build correctly with X11 headers. (Commit d8a4c55)
