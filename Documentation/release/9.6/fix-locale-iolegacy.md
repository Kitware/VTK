## IOLegacy: Locale handling for stream objects

VTK now sets the locale on stream objects in IOLegacy, ensuring consistent behavior across different system configurations. This fixes locale-related failures in vtkDataWriter and vtkDataReader.
