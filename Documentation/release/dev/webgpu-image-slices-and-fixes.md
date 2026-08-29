# WebGPU: image slices, overlays and pixel readback

## Image slices are drawn by WebGPU

`vtkImageSlice` and its subclasses - `vtkImageActor` among them - are now drawn
by `vtkWebGPUImageSliceMapper` when the WebGPU backend is in use. Until now the
object factory had no WebGPU implementation to offer and handed back the OpenGL
slice mapper, which issued GL calls into a window that has no GL context and
crashed the process.

The mapper reuses the backend-independent work `vtkImageMapper3D` already does -
applying the image property's colour window/level and lookup table, and building
the textured quad for the current slice - and draws the result through an
ordinary actor, mapper and texture.

## Overlay props no longer occlude what follows them

Overlay geometry is drawn in the order the props were added and must not hide
anything drawn after it. The 2D pipeline was writing depth and comparing with
`Less`, which had two consequences: props with a background display location sit
at the far plane, which is also what the depth attachment is cleared to, so they
were discarded entirely; and a label drawn before a background image punched a
hole in that image the shape of the label's own quad, transparent pixels
included. Overlay geometry now compares `LessEqual` and does not write depth.

## Pixel readback waits for the pixels

`GetPixelData`, `GetZbufferData` and the ids readback used to wait only for the
queue. The map callback is delivered from `ProcessEvents`, and the queue's
work-done callback can arrive in an earlier `ProcessEvents` than the map, so the
call could return a buffer that had not been written yet - and the callback would
then write into memory the caller may already have freed. The readback now waits
for the copy itself. This also removes a long-standing source of intermittent
image-test failures.

## Windows

The WebGPU CI jobs run on Windows again. Destroying a render window there was
crashing the process with `STATUS_FATAL_USER_CALLBACK_EXCEPTION`: the interactor
stores the hardware window in the HWND's extra data and hooks the window
procedure, and `DestroyWindow` dispatches `WM_DESTROY` synchronously, so the
handler ran against a window being torn down. `vtkWin32HardwareWindow::Destroy`
now clears that pointer first, as `vtkWin32OpenGLRenderWindow` already did.
