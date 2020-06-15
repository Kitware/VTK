## Capturing rendering results at arbitrary resolution

The current vtkWindowToImageFilter can only save screenshots with the same
aspect ratio as the input window.

VTK now provides vtkResizingWindowToImageFilter that can create screenshots of
any size using `this->SetSize(width,height)`, independent of the size of the
input window.

This approach renders the image without tilling using offscreen buffers and as
such uses more memory than vtkWindowToImageFilter. To avoid excessive memory
use you can set a size limit using `this->SetSizeLimit(width,height)`. The
default limit is 4000x4000. For images bigger that the limit the filter falls
back to using image tilling.
