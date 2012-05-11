/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredPointsGeometryFilter.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkStructuredPointsGeometryFilter);

vtkStructuredPointsGeometryFilter::vtkStructuredPointsGeometryFilter()
{
  vtkErrorMacro("vtkStructuredPointsGeometryFilter will be deprecated in" << endl <<
                "the next release after VTK 4.0. Please use" << endl <<
                "vtkImageDataGeometryFilter instead" );
}
