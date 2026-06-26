## vtkPTSReader: bugfix to read all points

The PTS Reader now reads all points in the .pts provided without accidentally
skipping any points. The reader previously didn't handle the parser and buffer
correctly and the conditional if statement in the for loop had an "off by 1"
error which made it consistently skip the first two points in the file. The
parser and buffer are now reset before the points loading for loop and the if
condition has been corrected. Also while determining file type, points were
unnecessarily stored when just reading them using vtk::scn was sufficient; hence
the redundant code was refactored.
