# Fixing an impacting issue in vtkImageDifference

A impacting issue in vtkImageDifference has been fixed.
Between vtk 9.1 and 9.2, an change has been made to vtkImageDifference
that causes different image to be identified as not different
depending on how the vtkImageDifference was configured.

This fix may causes some testing to start failing.
