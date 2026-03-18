# Adding support for reading/writing RGB/RGBA color arrays to OBJ file format

This change affects vtkOBJReader&vtkOBJWriter.
The vertex node looks like:
 > v <x> <y> <z> [<r> <g> <b> [a]]

For vtkOBJReader, now it can read RGB/RGBA color arrays and write to vtkPolyData automatically with "RGB"/"RGBA" array name.

For vtkOBJWriter, if you manually set WriteColorArray and ColorArrayName for the writer, you can write the color array to OBJ file.
 > vtkNew<vtkOBJWriter> writer;
 writer->SetFileName(filename);
 writer->SetColorArrayName(arrayName);
 writer->WriteColorArrayOn();
 writer->SetInputData(0, polydata);
 writer->Write();
