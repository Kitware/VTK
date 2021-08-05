## Fully serialize a dataset with render-window-serializer.py

The Render Window serializer used to serialize only the arrays used for rendering in the serialized render window. This change adds an option for the PolyData, ImageData, and mergeToPolyData serializers to instead serialize all arrays. This can be used to individually serialize vtkDataSet objects, without needing a vtkRenderWindow to serialize.
This option is activated by setting the parameter "requested_fields" to ["*"] for those serializers.
For example, to serialize a polydata "pdata" :

vtk_serializer.polydataSerializer(None, pdata, 0, self.context, 1, ["*"])
