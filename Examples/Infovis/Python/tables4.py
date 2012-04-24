"""
This file provides a more advanced example of vtkTable access and
manipulation methods.
"""

from vtk import *

#------------------------------------------------------------------------------
# Script Entry Point (i.e., main() )
#------------------------------------------------------------------------------

if __name__ == "__main__":
    """ Main entry point of this python script """
    print "vtkTable Example 4: Accessing vtkTable data elements"

    # Load our table from a CSV file (covered in table2.py)
    csv_source = vtkDelimitedTextReader()
    csv_source.SetFieldDelimiterCharacters(",")
    csv_source.SetHaveHeaders(True)
    csv_source.SetFileName("table_data.csv")
    csv_source.Update()
    csv_source.GetOutput().Dump(6)

    T = csv_source.GetOutput()

    # Print some information about the table
    print "Number of Columns =", T.GetNumberOfColumns()
    print "Number of Rows    =", T.GetNumberOfRows()
    print "Get column 1, row 4 data: ", T.GetColumn(1).GetValue(4)

    # Add a new row to the table
    new_row = [8, "Luis", 68]
    for i in range( T.GetNumberOfColumns()):
        T.GetColumn(i).InsertNextValue( str(new_row[i]) )

    print "Table after new row appenended:"
    T.Dump(6)

    print "vtkTable Example 4: Finished."