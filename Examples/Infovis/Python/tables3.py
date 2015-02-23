#!/usr/bin/env python
from vtk import *

if __name__ == "__main__":
    """ Main entry point of this python script """
    #
    # Load our table from a CSV file (covered in table2.py)
    #
    csv_source = vtkDelimitedTextReader()
    csv_source.SetFieldDelimiterCharacters(",")
    csv_source.SetHaveHeaders(True)
    csv_source.SetDetectNumericColumns(True)
    csv_source.SetFileName("table_data.csv")
    csv_source.Update()

    T = csv_source.GetOutput()

    print "Table loaded from CSV file:"
    T.Dump(6)

    #
    # Add a new row to the table
    #
    new_row = [8, "Luis", 68]
    for i in range( T.GetNumberOfColumns()):
        T.GetColumn(i).InsertNextValue( new_row[i] )

    print "\nTable with new row appended:"
    T.Dump(6)

    #
    # Extract row 3 out of the table into a Python list.
    #
    row = []
    row_number = 3
    for icol in range( T.GetNumberOfColumns() ):
        row.append( T.GetColumn(icol).GetValue( row_number ) )

    print "\nExtracted row 3:"
    print row