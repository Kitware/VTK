catch {load vtktcl}
# Threshold a volume and write it to disk.
# It then reads the new data set from disk and displays it.
# Dont forget to delete the test files after the script is finished.

# Image pipeline

vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 33
  reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader Update


vtkImageBlockWriter writer
  writer SetInput [reader GetOutput]
  writer SetFilePattern "tmp/blocks_%d_%d_%d.vtk"
  writer SetDivisions 4 4 4
  writer SetOverlap 3
  writer Write

