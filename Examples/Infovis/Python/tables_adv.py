#!/usr/bin/env python
"""
This file contains Python code illustrating the creation and manipulation of
vtkTable objects.
"""

from __future__ import print_function
from vtk import *

#------------------------------------------------------------------------------
# Some Helper Functions
#------------------------------------------------------------------------------

def add_row_to_vtkTable(vtk_table, new_row=None):
    """ Python helper function to add a new row of data to a vtkTable object. """

    # Just a couple of sanity checks.
    if new_row == None:
        print("ERROR: No data provided for new table row.")
        return False
    if len(new_row) != vtk_table.GetNumberOfColumns():
        print("ERROR: Number of entries in new row does not match # of columns in table.")
        return False

    for i in range(vtk_table.GetNumberOfColumns()):
        vtk_table.GetColumn(i).InsertNextValue( new_row[i] )
    return True


def get_vtkTableHeaders(vtk_table):
    """ Returns the vtkTable headers (column names) as a Python list """
    headers = []
    for icol in range( vtk_table.GetNumberOfColumns() ):
        headers.append( vtk_table.GetColumn(icol).GetName() )
    return headers


def get_vtkTableRow(vtk_table, row_number):
    """ Returns a row from a vtkTable object as a Python list. """
    row = []
    for icol in range( vtk_table.GetNumberOfColumns() ):
        row.append( vtk_table.GetColumn(icol).GetValue(row_number) )
    return row


def get_vtkTableAsDelimitedText(vtk_table, sep="\t"):
    """ return a nicely formatted string version of a vtkTable """
    s    = ""
    hdrs = get_vtkTableHeaders(vtk_table)

    for i in hdrs:
        s += "%s%s"%(i,sep)
    s = s.rstrip(sep)
    s += "\n"

    for irow in range(vtk_table.GetNumberOfRows()):
        rowdata = get_vtkTableRow(vtk_table, irow)
        for i in rowdata:
            s += "%s%s"%(str(i),sep)
        s = s.rstrip(sep)
        s += "\n"
    return s



#------------------------------------------------------------------------------
# Script Entry Point (i.e., main() )
#------------------------------------------------------------------------------

if __name__ == "__main__":
    """ Main entry point of this python script """

    #----------------------------------------------------------
    # Create an empty table
    T = vtkTable()

    #----------------------------------------------------------
    # Create Column 1 (IDs)
    col1 = vtkIntArray()
    col1.SetName("ID")
    for i in range(1, 8):
        col1.InsertNextValue(i)
    T.AddColumn(col1)

    #----------------------------------------------------------
    # Create Column 2 (Names)
    namesList = ['Bob', 'Ann', 'Sue', 'Bill', 'Joe', 'Jill', 'Rick']
    col2 = vtkStringArray()
    col2.SetName("Name")
    for val in namesList:
        col2.InsertNextValue(val)
    T.AddColumn(col2)

    #----------------------------------------------------------
    # Create Column 3 (Ages)
    agesList = [12, 25, 72, 11, 31, 36, 32]
    col3 = vtkIntArray()
    col3.SetName("Age")
    for val in agesList:
        col3.InsertNextValue(val)
    T.AddColumn(col3)

    #----------------------------------------------------------
    # Add a row to the table
    new_row = [8, "Luis", 68]

    # we can't really use vtkTable.InsertNextRow() since it takes a vtkVariantArray
    # as its argument (and the SetValue, etc. methods on that are not wrapped into
    # Python) We can just append to each of the column arrays.

    if not add_row_to_vtkTable(T, new_row):
        print("Whoops!")

    #----------------------------------------------------------
    # Call PrintSelf() on a VTK object is done simply by printing the object
    print(25*"=")
    print("Calling PrintSelf():")
    print(T)

    #----------------------------------------------------------

    # Here are a couple of ways to print out our table in Python using
    # the helper functions that appear earlier in this script.
    # The accessor methods used here can be adapted to do more interesting
    # things with a vtkTable from within Python.

    # print out our table
    print(25*"=")
    print("Rows as lists:")
    print(get_vtkTableHeaders(T))
    for i in range(T.GetNumberOfRows()):
        print(get_vtkTableRow(T,i))

    print("")
    print(25*"=")
    print("Delimited text:")
    print(get_vtkTableAsDelimitedText(T))

    print("vtkTable Python Example Completed.")
