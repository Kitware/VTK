## Generate ConfigureEvent for Qt QResizeEvent

The QVTKInteractorAdapter now translates QResizeEvent into
a VTK ConfigureEvent.  This brings its behavior in line with
the native X11/Windows/macOS event handling, which generate
ConfigureEvent in response to the native UI resize events.
