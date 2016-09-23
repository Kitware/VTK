/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCursor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCursor3D.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include <cmath>

vtkStandardNewMacro(vtkCursor3D);

// Construct with model bounds = (-1,1,-1,1,-1,1), focal point = (0,0,0),
// all parts of cursor visible, and wrapping off.
vtkCursor3D::vtkCursor3D()
{
  vtkPoints *pts;

  this->Focus = vtkPolyData::New();
  pts = vtkPoints::New();
  pts->vtkPoints::InsertPoint(0, 0.0, 0.0, 0.0);
  this->Focus->SetPoints(pts);
  pts->Delete();
  vtkCellArray* vert = vtkCellArray::New();
  vert->InsertNextCell(1);
  vert->InsertCellPoint(0);
  this->Focus->SetVerts(vert);
  vert->Delete();


  this->ModelBounds[0] = -1.0;
  this->ModelBounds[1] = 1.0;
  this->ModelBounds[2] = -1.0;
  this->ModelBounds[3] = 1.0;
  this->ModelBounds[4] = -1.0;
  this->ModelBounds[5] = 1.0;

  this->FocalPoint[0] = 0.0;
  this->FocalPoint[1] = 0.0;
  this->FocalPoint[2] = 0.0;

  this->Outline = 1;
  this->Axes = 1;
  this->XShadows = 1;
  this->YShadows = 1;
  this->ZShadows = 1;
  this->Wrap = 0;
  this->TranslationMode = 0;

  this->SetNumberOfInputPorts(0);
}

vtkCursor3D::~vtkCursor3D()
{
  this->Focus->Delete();
}

int vtkCursor3D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int i;
  int numPts=0, numLines=0;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  double x[3];
  vtkIdType ptIds[2];

  // Check bounding box and origin
  //
  if ( this->Wrap )
  {
    for (i=0; i<3; i++)
    {
      this->FocalPoint[i] = this->ModelBounds[2*i] +
             fmod(static_cast<double>(
                    this->FocalPoint[i]-this->ModelBounds[2*i]),
                  static_cast<double>(
                    this->ModelBounds[2*i+1]-this->ModelBounds[2*i]));
    }
  }
  else
  {
    for (i=0; i<3; i++)
    {
      if ( this->FocalPoint[i] < this->ModelBounds[2*i] )
      {
        this->FocalPoint[i] = this->ModelBounds[2*i];
      }
      if ( this->FocalPoint[i] > this->ModelBounds[2*i+1] )
      {
        this->FocalPoint[i] = this->ModelBounds[2*i+1];
      }
    }
  }

  // Allocate storage
  //
  if (this->Axes)
  {
    numPts += 6;
    numLines += 3;
  }

  if (this->Outline)
  {
    numPts += 8;
    numLines += 12;
  }

  if (this->XShadows)
  {
    numPts += 8;
    numLines += 4;
  }

  if (this->YShadows)
  {
    numPts += 8;
    numLines += 4;
  }

  if (this->ZShadows)
  {
    numPts += 8;
    numLines += 4;
  }

  if ( numPts )
  {
    newPts = vtkPoints::New();
    newPts->Allocate(numPts);
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numLines,2));
  }
  else
  {
    return 1;
  }

  // Create axes
  //
  if ( this->Axes )
  {
    x[0] = this->ModelBounds[0];
    x[1] = this->FocalPoint[1];
    x[2] = this->FocalPoint[2];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1];
    x[1] = this->FocalPoint[1];
    x[2] = this->FocalPoint[2];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->FocalPoint[0];
    x[1] = this->ModelBounds[2];
    x[2] = this->FocalPoint[2];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->FocalPoint[0];
    x[1] = this->ModelBounds[3];
    x[2] = this->FocalPoint[2];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->FocalPoint[0];
    x[1] = this->FocalPoint[1];
    x[2] = this->ModelBounds[4];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->FocalPoint[0];
    x[1] = this->FocalPoint[1];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);
  }

  // Create outline
  //
  if ( this->Outline )
  {
    // First triad
    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[4];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[4];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[4];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    // Second triad
    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[5];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[4];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    // Fill in remaining lines
    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[4];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[4];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[4];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[4];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);


    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[5];
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1];
    x[1] = this->ModelBounds[2];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);

    x[0] = this->ModelBounds[0];
    x[1] = this->ModelBounds[3];
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);
  }

  // Create x-shadows
  //
  if ( this->XShadows )
  {
    for (i=0; i<2; i++)
    {
      x[0] = this->ModelBounds[i];
      x[1] = this->ModelBounds[2];
      x[2] = this->FocalPoint[2];
      ptIds[0] = newPts->InsertNextPoint(x);

      x[0] = this->ModelBounds[i];
      x[1] = this->ModelBounds[3];
      x[2] = this->FocalPoint[2];
      ptIds[1] = newPts->InsertNextPoint(x);
      newLines->InsertNextCell(2,ptIds);

      x[0] = this->ModelBounds[i];
      x[1] = this->FocalPoint[1];
      x[2] = this->ModelBounds[4];
      ptIds[0] = newPts->InsertNextPoint(x);

      x[0] = this->ModelBounds[i];
      x[1] = this->FocalPoint[1];
      x[2] = this->ModelBounds[5];
      ptIds[1] = newPts->InsertNextPoint(x);
      newLines->InsertNextCell(2,ptIds);
    }
  }

  //  Create y-shadows
  //
  if ( this->YShadows )
  {
    for (i=0; i<2; i++)
    {
      x[0] = this->ModelBounds[0];
      x[1] = this->ModelBounds[i+2];
      x[2] = this->FocalPoint[2];
      ptIds[0] = newPts->InsertNextPoint(x);

      x[0] = this->ModelBounds[1];
      x[1] = this->ModelBounds[i+2];
      x[2] = this->FocalPoint[2];
      ptIds[1] = newPts->InsertNextPoint(x);
      newLines->InsertNextCell(2,ptIds);

      x[0] = this->FocalPoint[0];
      x[1] = this->ModelBounds[i+2];
      x[2] = this->ModelBounds[4];
      ptIds[0] = newPts->InsertNextPoint(x);

      x[0] = this->FocalPoint[0];
      x[1] = this->ModelBounds[i+2];
      x[2] = this->ModelBounds[5];
      ptIds[1] = newPts->InsertNextPoint(x);
      newLines->InsertNextCell(2,ptIds);
    }
  }

  //  Create z-shadows
  //
  if ( this->ZShadows )
  {
    for (i=0; i<2; i++)
    {
      x[0] = this->ModelBounds[0];
      x[1] = this->FocalPoint[1];
      x[2] = this->ModelBounds[i+4];
      ptIds[0] = newPts->InsertNextPoint(x);

      x[0] = this->ModelBounds[1];
      x[1] = this->FocalPoint[1];
      x[2] = this->ModelBounds[i+4];
      ptIds[1] = newPts->InsertNextPoint(x);
      newLines->InsertNextCell(2,ptIds);

      x[0] = this->FocalPoint[0];
      x[1] = this->ModelBounds[2];
      x[2] = this->ModelBounds[i+4];
      ptIds[0] = newPts->InsertNextPoint(x);

      x[0] = this->FocalPoint[0];
      x[1] = this->ModelBounds[3];
      x[2] = this->ModelBounds[i+4];
      ptIds[1] = newPts->InsertNextPoint(x);
      newLines->InsertNextCell(2,ptIds);
    }
  }

  // Update ourselves and release memory
  //
  this->Focus->GetPoints()->SetPoint(0,this->FocalPoint);

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

// Set the boundary of the 3D cursor.
void vtkCursor3D::SetModelBounds(double xmin, double xmax,
                                 double ymin, double ymax,
                                 double zmin, double zmax)
{
  if ( xmin != this->ModelBounds[0] || xmax != this->ModelBounds[1] ||
       ymin != this->ModelBounds[2] || ymax != this->ModelBounds[3] ||
       zmin != this->ModelBounds[4] || zmax != this->ModelBounds[5] )
  {
    this->Modified();

    this->ModelBounds[0] = xmin; this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin; this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin; this->ModelBounds[5] = zmax;

    for (int i=0; i<3; i++)
    {
      if ( this->ModelBounds[2*i] > this->ModelBounds[2*i+1] )
      {
        this->ModelBounds[2*i] = this->ModelBounds[2*i+1];
      }
    }
  }
}

void vtkCursor3D::SetFocalPoint(double x[3])
{
  if ( x[0] == this->FocalPoint[0] && x[1] == this->FocalPoint[1] &&
       x[2] == this->FocalPoint[2] )
  {
    return;
  }

  this->Modified();

  double v[3];
  for (int i=0; i<3; i++)
  {
    v[i] = x[i] - this->FocalPoint[i];
    this->FocalPoint[i] = x[i];

    if ( this->TranslationMode )
    {
      this->ModelBounds[2*i] += v[i];
      this->ModelBounds[2*i+1] += v[i];
    }
    else if ( this->Wrap ) //wrap
    {
      this->FocalPoint[i] = this->ModelBounds[2*i] +
             fmod(static_cast<double>(
                    this->FocalPoint[i]-this->ModelBounds[2*i]),
                  static_cast<double>(
                    this->ModelBounds[2*i+1]-this->ModelBounds[2*i]));
    }
    else //clamp
    {
      if ( x[i] < this->ModelBounds[2*i] )
      {
        this->FocalPoint[i] = this->ModelBounds[2*i];
      }
      if ( x[i] > this->ModelBounds[2*i+1] )
      {
        this->FocalPoint[i] = this->ModelBounds[2*i+1];
      }
    }
  }
}

void vtkCursor3D::SetModelBounds(const double bounds[6])
{
  this->SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4],
                       bounds[5]);
}

// Turn every part of the 3D cursor on.
void vtkCursor3D::AllOn()
{
  this->OutlineOn();
  this->AxesOn();
  this->XShadowsOn();
  this->YShadowsOn();
  this->ZShadowsOn();
}

// Turn every part of the 3D cursor off.
void vtkCursor3D::AllOff()
{
  this->OutlineOff();
  this->AxesOff();
  this->XShadowsOff();
  this->YShadowsOff();
  this->ZShadowsOff();
}

void vtkCursor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Focal Point: (" << this->FocalPoint[0] << ", "
               << this->FocalPoint[1] << ", "
               << this->FocalPoint[2] << ")\n";

  os << indent << "Outline: " << (this->Outline ? "On\n" : "Off\n");
  os << indent << "Axes: " << (this->Axes ? "On\n" : "Off\n");
  os << indent << "XShadows: " << (this->XShadows ? "On\n" : "Off\n");
  os << indent << "YShadows: " << (this->YShadows ? "On\n" : "Off\n");
  os << indent << "ZShadows: " << (this->ZShadows ? "On\n" : "Off\n");
  os << indent << "Wrap: " << (this->Wrap ? "On\n" : "Off\n");
  os << indent << "Translation Mode: "
     << (this->TranslationMode ? "On\n" : "Off\n");
}
