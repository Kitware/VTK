/*=========================================================================

  Program:   Visualization Library
  Module:    Cursor3D.cc
  Language:  C++
  Date:      5/16/94
  Version:   1.1

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>

#include "Cursor3D.hh"

vlCursor3D::vlCursor3D()
{
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
}

void vlCursor3D::Execute()
{
  int i;
  int numPts=0, numLines=0;
  vlFloatPoints *newPts;
  vlCellArray *newLines;
  float x[3];
  int ptIds[2];

  this->Initialize();
//
// Check bounding box and origin
//
  for (i=0; i<3; i++)
    if ( ModelBounds[2*i] > ModelBounds[2*i+1] )
       ModelBounds[2*i] = ModelBounds[2*i+1];

  if ( this->Wrap ) 
    {
    for (i=0; i<3; i++)
      {
      this->FocalPoint[i] = ModelBounds[2*i] + 
             fmod((double)(this->FocalPoint[i]-ModelBounds[2*i]), 
                  (double)(ModelBounds[2*i+1]-ModelBounds[2*i]));
      }
    } 
  else 
    {
    for (i=0; i<3; i++)
      {
      if ( this->FocalPoint[i] < this->ModelBounds[2*i] )
        this->FocalPoint[i] = this->ModelBounds[2*i];
      if ( this->FocalPoint[i] > this->ModelBounds[2*i+1] )
        this->FocalPoint[i] = this->ModelBounds[2*i+1];
      }
    }
//
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
    newPts = new vlFloatPoints(numPts);
    newLines = new vlCellArray();
    newLines->Allocate(newLines->EstimateSize(numLines,2));
    }
  else return;
//
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
//
// Create outline
//
  if ( this->Outline ) 
    {
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
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1]; 
    x[1] = this->ModelBounds[3]; 
    x[2] = this->ModelBounds[4];
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
    ptIds[0] = newPts->InsertNextPoint(x);

    x[0] = this->ModelBounds[1]; 
    x[1] = this->ModelBounds[3]; 
    x[2] = this->ModelBounds[5];
    ptIds[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,ptIds);
    }
//
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
//
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
//
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
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->SetLines(newLines);
}

void vlCursor3D::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlCursor3D::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

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
    }
}
