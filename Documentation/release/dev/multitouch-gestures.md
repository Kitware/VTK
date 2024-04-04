## Multi-touch gestures in VTK

Multiple improvements have been introduced to the multi-touch event based gesture handling.

- Panning was compounding the translation causing a pan that would scale exponentially as it
  progresses.
- Pinch gesture was being handled by printing an error message.
- There was no "end" to the multi-touch events.
- The [Set/Get]CurrentGesture methods are moved to the vtkRenderWindowInteractor from its derived
  class vtkVRRenderWindowInteractor. This would allow client applications with custom interaction
  styles to redefine these APIs.
