#!/usr/bin/env python
"""
This file contains Python code illustrating the creation and manipulation of
vtkTable objects.
"""

from vtk import *

#------------------------------------------------------------------------------
# Script Entry Point (i.e., main() )
#------------------------------------------------------------------------------

if __name__ == "__main__":
    """ Main entry point of this python script """
    print "vtkTable Example 2: Loading table data from a comma-separated value file."

    # Create a Delimited Text Reader object
    csv_source = vtkDelimitedTextReader()

    # Tell it we want the field-separator to be a comma
    csv_source.SetFieldDelimiterCharacters(",")

    # Tell the filter that the first row in the data file are headers.
    csv_source.SetHaveHeaders(True)

    # Provide the filename that we want to load.
    csv_source.SetFileName("table_data.csv")

    # Update forces the filter to execute and get the data.
    csv_source.Update()

    # Print out the table data to the screen
    csv_source.GetOutput().Dump(6)

    # Note: tables loaded from DelimitedTextReader have all
    #       types defaulted to string data.

    print "vtkTable Example 2: Finished."