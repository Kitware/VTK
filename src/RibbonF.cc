/*=========================================================================

  Program:   Visualization Library
  Module:    RibbonF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlMath.hh"
#include "RibbonF.hh"

vlRibbonFilter::vlRibbonFilter()
{
  this->Radius = 0.5;
  this->Angle = 180.0;
}

void vlRibbonFilter::Execute()
{
  int i, j, k;
  float center[3], *p;
  vlPoints *inPts;
  vlPointData *pd;
  vlCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewPts, numNewLines, numNewPolys, poly_alloc_size;
  int npts, *pts;
  vlFloatPoints *newPoints;
  vlNormals *inNormals;
  vlCellArray *newVerts, *newLines, *newPolys;
  int newIds[MAX_CELL_SIZE];
  float *p1, *p2, *p3, pt[3];
//
// Initialize
//
  this->Initialize();

  inPts = this->Input->GetPoints();
  pd = this->Input->GetPointData();

  if ( !(inLines = this->Input->GetLines()) || inLines->GetNumberOfCells() < 1 )
    {
    cerr << this->GetClassName() << ": No input data!\n";
    return;
    }

  if (pd)
    {
    inNormals = pd->GetNormals();
    }
  else 
    {
    ; //for now
    }
}

void vlRibbonFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlRibbonFilter::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Radius: " << this->Radius << "\n";
    os << indent << "Angle: " << this->Angle << "\n";
    }
}

