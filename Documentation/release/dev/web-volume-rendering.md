## Add serializers to support VolumeRendering when using vue-vtk-js::VtkSyncView

Extend the `render_window_serializer.py` to support serialization of a
vtkRenderWindow that contains vtkVolume/vtkVolumeProperty/vtkVolumeMapper.
