#!/bin/bash
#
#  Author: Darren Weber
#
#  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
#  All rights reserved.
#  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notices for more information.
#
#  Copy this script to somewhere on your computer and edit it,
#  setting the paths in the variables
#  vtkExamplePath, vtkTestingPath and vtkSourcePath
#  to wherever you have installed VTK.

if [ $# -lt 1 ]; then
    echo "$0 'search term' ['search term' ...]"
    exit 1
fi

#
# Search the CXX, TCL and Python files
#
# You may need to set the paths for these variables.
vtkExamplePath="/opt/local/share/vtk/examples"
vtkTestingPath="/opt/local/share/vtk/testing"
vtkSourcePath="/opt/local/share/vtk"

for term in $@; do
    echo
    echo "Search term: ${term}"
    for vtkPath in "${vtkExamplePath}" "${vtkTestingPath}" "${vtkSourcePath}" ; do
        if [ ! -d ${vtkPath} ]; then
            echo "Path not found: ${vtkPath}"
        else
            echo "Searching VTK files in: ${vtkPath}"
            cxxFiles=$(find ${vtkPath} -name "*.cxx")
            grep -l -E -e ${term} ${cxxFiles}
            tclFiles=$(find ${vtkPath} -name "*.tcl")
            grep -l -E -e ${term} ${tclFiles}
            pyFiles=$(find ${vtkPath} -name "*.py")
            grep -l -E -e ${term} ${pyFiles}
      fi
    done
done