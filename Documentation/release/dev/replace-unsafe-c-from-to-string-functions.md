## Replace unsafe c from/to string functions

VTK has been using a set of either unsafe or slow C/C++ functions to convert numbers to string or vice versa.
The exhaustive list of functions is given below. And all of them have been replaced with safer alternatives/faster
alternatives provided by scnlib, fmt, and fast_float libraries and exposed though the vtk:: namespace.

C/C++ has the following functions to convert one/many char or string to a number.

1. atof, atoi, atol, atoll,
2. std::stof, std::stod, std::stold, std::stoi, std::stol, std::stoll, std::stoul, std::stoull
3. std::strtof, std::strtod, std::strtold, std::strtol, std::strtoll/_strtoi64, std::strtoul,
   std::strtoull
4. sscanf, sscanf_s, vsscanf, vsscanf_s
5. std::from_chars (This is slow because it does not use fast_float under the hood)

These functions should be replaced by:

1. vtk::from_chars, vtk::scan_int, vtk::scan_value, if one number needs to be converted
2. vtk::scan, if one/many numbers need to be converted (optionally with a specific format)

C/C++ has the following functions to scan one/many numbers from a stdin/file.

1. scanf, scanf_s, vscanf, vscanf_s,
2. fscanf, fscanf_s, vfscanf, vfscanf_s,

These functions should be replaced by:

1. vtk::scan_value, if one number needs to be converted
2. vtk::input, vtk::scan, if one/many numbers need to be converted (optionally with a specific
   format)

C/C++ has the following functions to convert one/many numbers to a char or string.

1. itoa/_itoa, ltoa/_ltoa, lltoa/_i64toa, ultoa/_ultoa, ulltoa/_ulltoa/_ui64toa
2. sprintf, sprintf_s, vsprintf, vsprintf_s,
3. snprintf, snprintf_s, vsnprintf, vsnprintf_s,
4. strftime
5. std::to_chars, std::to_string

These functions should be replaced by:

1. vtk::to_chars or vtk::to_string, if one number needs to be converted
2. vtk::format, vtk::format_to, or vtk::format_to_n, if one/many
   numbers need to be converted with a specific format

C/C++ has the following functions to print one/many numbers to stdout/file.

1. printf, printf_s, vprintf, vprintf_s,
2. fprintf, fprintf_s, vfprintf, vfprintf_s,

These functions should be replaced by:

1. vtk::print, vtk::println

It should also be noted that the following functions (including subclasses that use them)
need to be provided with strings that use the std::format style format instead of the printf one:

1. `void vtkControlPointsItem::SetLabelFormat(const char* formatArg)`
2. `void vtkTimerLog::FormatAndMarkEvent(const char* formatArg, T&&... args)`
3. `void vtkAngleRepresentation::SetLabelFormat(const char* formatArg)`
4. `void vtkBiDimensionalRepresentation::SetLabelFormat(const char* formatArg)`
5. `void vtkDistanceRepresentation::SetLabelFormat(const char* formatArg)`
6. `void vtkLineRepresentation::SetDistanceAnnotationFormat(const char* formatArg)`
7. `void vtkResliceCursorRepresentation::SetThicknessLabelFormat(const char* formatArg)`
8. `void vtkSliderRepresentation::SetLabelFormat(const char* formatArg)`
9. `void vtkImageReader2::SetFilePattern(const char* formatArg)`
10. `void vtkImageWriter::SetFilePattern(const char* formatArg)`
11. `void vtkVolumeReader::SetFilePattern(const char* formatArg)`
12. `void vtkPDataSetWriter::SetFilePattern(const char* formatArg)`
13. `void vtkPExodusIIReader::SetFilePattern(const char* formatArg)`
14. `void vtkAxisActor::SetLabelFormat(const char* formatArg)`
15. `void vtkAxisActor2D::SetLabelFormat(const char* formatArg)`
16. `void vtkCubeAxesActor::SetXLabelFormat(const char* formatArg)`
17. `void vtkCubeAxesActor::SetYLabelFormat(const char* formatArg)`
18. `void vtkCubeAxesActor::SetZLabelFormat(const char* formatArg)`
19. `void vtkCubeAxesActor2D::SetLabelFormat(const char* formatArg)`
20. `void vtkLeaderActor2D::SetLabelFormat(const char* formatArg)`
21. `void vtkParallelCoordinatesActor::SetLabelFormat(const char* formatArg)`
22. `void vtkPolarAxesActor::SetPolarLabelFormat(const char* formatArg)`
23. `void vtkScalarBarActor::SetLabelFormat(const char* formatArg)`
24. `void vtkXYPlotActor::SetXLabelFormat(const char* formatArg)`
25. `void vtkXYPlotActor::SetYLabelFormat(const char* formatArg)`
26. `void vtkLabeledDataMapper::SetLabelFormat(const char* formatArg)`
27. `void vtkFastLabeledDataMapper::SetLabelFormat(const char* formatArg)`
28. `void vtkDateToNumeric::SetDateFormat(const char* formatArg)`
29. `void vtkPlot::SetTooltipLabelFormat(const vtkStdString& labelFormat)`
