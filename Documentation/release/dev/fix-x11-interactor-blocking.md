## Make the X11 ProcessEvents return immediately.

The `ProcessEvents` method now returns immediately after all user interaction and timer events
are dispatched. Earlier, `ProcessEvents` method used to wait for a short period of time for new events.
The behavior of `vtkRenderWindowInteractor::ProcessEvents` is now consistent
across all platforms (macOS, Windows, Linux and WebAssembly).
