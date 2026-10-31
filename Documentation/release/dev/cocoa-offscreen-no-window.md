# Offscreen rendering on any thread on macOS

An offscreen `vtkCocoaRenderWindow` (hidden, rendering into offscreen buffers) no
longer creates an `NSWindow` or `NSView`. AppKit allows creating those only on the
main thread, so you can now render offscreen on macOS from any thread, for example an
in situ worker thread. If you later show the window or turn off offscreen buffers,
the window and view are created then, or at the next render if that call was not
on the main thread.

A window that needs an `NSWindow` and `NSView` but is used on another thread now
reports an error and keeps rendering offscreen, where it previously let AppKit
abort the process.
