/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEarthSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEarthSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkEarthSource);

// Description:
// Construct an Earth with radius = 1.0 and OnRatio set at 10. The outlines are drawn
// in wireframe as default.
vtkEarthSource::vtkEarthSource()
{
  this->Radius = 1.0;
  this->OnRatio = 10;
  this->Outline = 1;

  this->SetNumberOfInputPorts(0);
}

void vtkEarthSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "OnRatio: " << this->OnRatio << "\n";
  os << indent << "Outline: " << (this->Outline ? "On\n" : "Off\n");
}

#include "vtkEarthSourceData.cxx"

int vtkEarthSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int i;
  int maxPts;
  int maxPolys;
  vtkPoints* newPoints;
  vtkFloatArray* newNormals;
  vtkCellArray* newPolys;
  double x[3], base[3];
  vtkIdType Pts[4000];
  int npts, land, offset;
  int actualpts, actualpolys;
  double scale = 1.0 / 30000.0;

  //
  // Set things up; allocate memory
  //
  maxPts = 12000 / this->OnRatio;
  maxPolys = 16;
  actualpts = actualpolys = 0;

  newPoints = vtkPoints::New();
  newPoints->Allocate(maxPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3 * maxPts);
  newPolys = vtkCellArray::New();
  newPolys->AllocateEstimate(maxPolys, 4000 / this->OnRatio);

  //
  // Create points
  //
  offset = 0;
  while (1)
  {
    // read a polygon
    npts = vtkEarthData[offset++];
    if ((npts == 0) || (actualpolys > maxPolys))
    {
      break;
    }

    land = vtkEarthData[offset++];

    base[0] = 0;
    base[1] = 0;
    base[2] = 0;

    for (i = 1; i <= npts; i++)
    {
      base[0] += vtkEarthData[offset++] * scale;
      base[1] += vtkEarthData[offset++] * scale;
      base[2] += vtkEarthData[offset++] * scale;

      x[0] = base[2] * this->Radius;
      x[1] = base[0] * this->Radius;
      x[2] = base[1] * this->Radius;

      if ((land == 1) && (npts > this->OnRatio * 3))
      {
        // use only every OnRatioth point in the polygon
        if ((i % this->OnRatio) == 0)
        {
          newPoints->InsertNextPoint(x);
          vtkMath::Normalize(x);
          newNormals->InsertNextTuple(x);
          actualpts++;
        }
      }
    }

    if ((land == 1) && (npts > this->OnRatio * 3))
    {
      //
      // Generate mesh connectivity for this polygon
      //

      for (i = 0; i < (npts / this->OnRatio); i++)
      {
        Pts[i] = (actualpts - npts / this->OnRatio) + i;
      }

      if (this->Outline) // close the loop in the line
      {
        Pts[i] = (actualpts - npts / this->OnRatio);
        newPolys->InsertNextCell(i + 1, Pts);
      }
      else
      {
        newPolys->InsertNextCell(i, Pts);
      }

      actualpolys++;
    }
  }

  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  if (this->Outline) // lines or polygons
  {
    output->SetLines(newPolys);
  }
  else
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  output->Squeeze();

  return 1;
}
