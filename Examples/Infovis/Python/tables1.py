#!/usr/bin/env python
"""
This file contains Python code illustrating the creation and manipulation of
vtkTable objects.
"""

from __future__ import print_function
from vtk import *

#------------------------------------------------------------------------------
# Script Entry Point (i.e., main() )
#------------------------------------------------------------------------------

if __name__ == "__main__":
    """ Main entry point of this python script """
    print("vtkTable Example 1: Building a vtkTable from scratch.")

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

    T.Dump(6)

    print("vtkTable Example 1: Finished.")
