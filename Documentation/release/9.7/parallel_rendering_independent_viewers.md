## Support independent off-axis viewers

VTK's parallel rendering module can now provide separate EyeTransformMatrix and
EyeSeparation for any number of independent viewers. This feature will allow ParaView in
CAVE to independently track multiple viewers so that each viewer sees the scene from a
distinct viewpoint.

If `vtkIndependentViewerCollection` is not provided to the `vtkSynchronizedRenderers`
instance, or if the collection is empty, only the number of viewers (which, in these
cases, is zero) is sent from client to server. Otherwise, an additional 17 double
values are sent from client to server for each independent viewer.
