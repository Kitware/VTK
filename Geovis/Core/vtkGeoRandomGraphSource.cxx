/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoRandomGraphSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGeoRandomGraphSource.h"

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkGeoRandomGraphSource);

// ----------------------------------------------------------------------

vtkGeoRandomGraphSource::vtkGeoRandomGraphSource()
{
}

// ----------------------------------------------------------------------

vtkGeoRandomGraphSource::~vtkGeoRandomGraphSource()
{
}

// ----------------------------------------------------------------------

void vtkGeoRandomGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

int vtkGeoRandomGraphSource::RequestData(
  vtkInformation* info,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkRandomGraphSource::RequestData(info, inputVector, outputVector);

  // Set random lat/long points into the output
  vtkGraph* output = vtkGraph::GetData(outputVector);
  vtkDoubleArray* latArr = vtkDoubleArray::New();
  vtkDoubleArray* lonArr = vtkDoubleArray::New();
  latArr->SetNumberOfTuples(output->GetNumberOfVertices());
  lonArr->SetNumberOfTuples(output->GetNumberOfVertices());
  latArr->SetName("latitude");
  lonArr->SetName("longitude");
  for (vtkIdType v = 0; v < output->GetNumberOfVertices(); ++v)
  {
    double lat = vtkMath::Random()*180.0 - 90.0;
    double lon = vtkMath::Random()*360.0 - 180.0;
    latArr->SetValue(v, lat);
    lonArr->SetValue(v, lon);
  }
  output->GetVertexData()->AddArray(latArr);
  output->GetVertexData()->AddArray(lonArr);
  latArr->Delete();
  lonArr->Delete();

  return 1;
}

