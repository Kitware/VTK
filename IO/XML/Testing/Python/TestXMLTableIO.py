#!/usr/bin/env python

import os
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

file0 = VTK_TEMP_DIR + '/idFile0.vtt'
file1 = VTK_TEMP_DIR + '/idFile1.vtt'
file2 = VTK_TEMP_DIR + '/idFile2.vtt'

# read in some poly data
csvReader = vtk.vtkDelimitedTextReader()
csvReader.SetFileName(VTK_DATA_ROOT + "/Data/vehicle_data.csv")
csvReader.DetectNumericColumnsOn();
csvReader.SetHaveHeaders(True);
csvReader.Update()

# write various versions
xmlTableWriter = vtk.vtkXMLTableWriter()
xmlTableWriter.SetFileName(file0)
xmlTableWriter.SetDataModeToAscii()
xmlTableWriter.SetInputConnection(csvReader.GetOutputPort())
xmlTableWriter.Write()

xmlTableWriter.SetFileName(file1)
xmlTableWriter.SetInputConnection(csvReader.GetOutputPort())
xmlTableWriter.SetDataModeToAppended()
xmlTableWriter.Write()

# Add some arrays to the FieldData
modifiedTable = vtk.vtkTable()
modifiedTable.DeepCopy(csvReader.GetOutput())

array0 = vtk.vtkIntArray()
array0.SetName("Array 0")
array0.SetNumberOfComponents(1)
array0.SetNumberOfTuples(10)
for idArray in range(0,10):
  array0.SetTuple1(idArray, idArray+1)

modifiedTable.GetFieldData().AddArray(array0)

array1 = vtk.vtkIntArray()
array1.SetName("Array 1")
array1.SetNumberOfComponents(3)
array1.SetNumberOfTuples(10)
for idArray in range(0,10):
  array1.SetTuple3(idArray, idArray+1, idArray+2, idArray+3)

modifiedTable.GetFieldData().AddArray(array1)

array2 = vtk.vtkDoubleArray()
array2.SetName("Array 2")
array2.SetNumberOfComponents(1)
array2.SetNumberOfTuples(10)
for idArray in range(0,10):
  array2.SetTuple1(idArray, idArray+1.5)

modifiedTable.GetFieldData().AddArray(array2)

array3 = vtk.vtkFloatArray()
array3.SetName("Array 3")
array3.SetNumberOfComponents(3)
array3.SetNumberOfTuples(10)
for idArray in range(0,10):
  array3.SetTuple3(idArray, idArray+1.5, idArray+2.5, idArray+3.5)

modifiedTable.GetFieldData().AddArray(array3)

xmlTableWriter = vtk.vtkXMLTableWriter()
xmlTableWriter.SetFileName(file2)
xmlTableWriter.SetDataModeToAppended()
xmlTableWriter.SetInputData(modifiedTable)
xmlTableWriter.Write()

# read the ASCII version
reader = vtk.vtkXMLTableReader()
reader.SetFileName(file0)
reader.Update()

reference = vtk.vtkTable()
reference.DeepCopy(csvReader.GetOutput())

# Test the first table
table = vtk.vtkTable()
table.DeepCopy(reader.GetOutput())

# Checks the table dimensions
if table.GetNumberOfColumns() != reference.GetNumberOfColumns():
  sys.stderr.write("Wrong number of columns. Expected  "
    + str(reference.GetNumberOfColumns()) + " got "
    + str(table.GetNumberOfColumns()))

  sys.exit(1)

if table.GetNumberOfRows() != reference.GetNumberOfRows():
  sys.stderr.write("Wrong number of rows. Expected  "
    + str(reference.GetNumberOfRows()) + " got "
    + str(table.GetNumberOfRows()))

  sys.exit(1)

# Tests the rows' headers
for idColumn in range(0, reference.GetNumberOfColumns()):
  if table.GetColumnName(idColumn) != reference.GetColumnName(idColumn):
    sys.stderr.write("Wrong column header name. Expected "
      + reference.GetColumnName(idColumn) + " got "
      + table.GetColumnName(idColumn))

    sys.exit(1)

# Test the rows values
for idRow in range(0, reference.GetNumberOfRows()):
  for idColumn in range(0, reference.GetNumberOfColumns()):
    currentValue = table.GetValue(idRow, idColumn);
    referenceValue = reference.GetValue(idRow, idColumn);
    if currentValue != referenceValue:
      sys.stderr.write("Wrong value at (" + str(idRow) + ", " + str(idColumn)
        + "). Expected" + str(referenceValue) + " got " + str(currentValue))

      sys.exit(1)


# Test the second table
reader.SetFileName(file1)
reader.Update()

table.DeepCopy(reader.GetOutput())

# Checks the table dimensions
if table.GetNumberOfColumns() != reference.GetNumberOfColumns():
  sys.stderr.write("Wrong number of columns. Expected  "
    + str(reference.GetNumberOfColumns()) + " got "
    + str(table.GetNumberOfColumns()))
  sys.exit(1)

# There should be 2 more times rows than the reference table
if table.GetNumberOfRows() != reference.GetNumberOfRows():
  sys.stderr.write("Wrong number of rows. Expected  "
    + str(reference.GetNumberOfRows()) + " got "
    + str(table.GetNumberOfRows()))
  sys.exit(1)

# Tests the rows' headers
for idColumn in range(0, reference.GetNumberOfColumns()):
  if table.GetColumnName(idColumn) != reference.GetColumnName(idColumn):
    sys.stderr.write("Wrong column header name. Expected "
      + reference.GetColumnName(idColumn) + " got "
      + table.GetColumnName(idColumn))
    sys.exit(1)

# Test the third table
reader.SetFileName(file2)
reader.Update()

table.DeepCopy(reader.GetOutput())


fieldData = table.GetFieldData()
referenceArrays = [array0, array1, array2, array3]

# Check the number of associated arrays
if fieldData.GetNumberOfArrays() != 4:
  sys.stderr.write("Wrong number of arrays in the table FieldData. Expected 2 got "
    +str(fieldData.GetNumberOfArrays()))
  sys.exit(1)


# For each array...
for idArray in range(0, 4):
  currentArray = fieldData.GetArray(idArray)
  referenceArray = referenceArrays[idArray]

  # Check the name
  if currentArray.GetName() != referenceArray.GetName():
    sys.stderr.write("Wrong number of components for array " + str(idArray) + ". Expected"
      + referenceArray.GetName() + " got " + currentArray.GetName())
    sys.exit(1)

  # Check the number of components
  if currentArray.GetNumberOfComponents() != referenceArray.GetNumberOfComponents():
    sys.stderr.write("Wrong number of components for array " + str(idArray) + ". Expected"
      + str(referenceArray.GetNumberOfComponents()) + " got "
      + str(currentArray.GetNumberOfComponents()))
    sys.exit(1)

  # Check the number of tuples
  if currentArray.GetNumberOfTuples() != referenceArray.GetNumberOfTuples():
    sys.stderr.write("Wrong number of tuples for array " + str(idArray) + ". Expected"
      + str(referenceArray.GetNumberOfTuples()) + " got "
      + str(currentArray.GetNumberOfTuples()))
    sys.exit(1)

  # Check the values
  for idTuple in range(0, referenceArray.GetNumberOfTuples()):
    currentTuple = currentArray.GetTuple(idTuple)
    referenceTuple = referenceArray.GetTuple(idTuple)
    for idComp in range(0, referenceArray.GetNumberOfComponents()):
      if currentTuple[idComp] != referenceTuple[idComp]:
        sys.stderr.write("Wrong tuple value for tuple " + str(idTuple) + " at component "
          + str(idComp) + " of array " + referenceArray.GetName() + ". Expected "
          + str(referenceTuple[idComp]) + " got " + str(currentTuple[idComp]))
        sys.exit(1)

os.remove(file0)
os.remove(file1)
os.remove(file2)
