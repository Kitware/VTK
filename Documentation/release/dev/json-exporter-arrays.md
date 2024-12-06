## JSONSceneExporter: Support exporting a list of named actors and arrays selections

vtkJSONSceneExporter is now able to export a list of named actors instead of extracting them from a renderer. With these named actors, you can select which arrays are to be exported for each actor. vtkJSONDataSetWriter now also support point and cell array selection. The protected methods `vtkJSONSceneExporter::WriteDataObject` and `vtkJSONSceneExporter::WriteDataSet` are now private.
