/*=========================================================================

  Program:   Visualization Library
  Module:    TriF.cc
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
#include "TriF.hh"
#include "Polygon.hh"

void vlTriangleFilter::Execute()
{
  vlCellArray *inPolys=this->Input->GetPolys();
  vlCellArray *inStrips=this->Input->GetStrips();;
  int npts, *pts;
  vlCellArray *newPolys=NULL;
  int numCells;
  int p1, p2, p3;
  vlPolygon poly;
  int i, j;
  vlIdList outVerts(3*MAX_CELL_SIZE);
  vlPoints *inPoints=this->Input->GetPoints();
  vlPointData *pd;

  vlDebugMacro(<<"Executing triangle filter");

  this->Initialize();

  newPolys = new vlCellArray();
  // approximation
  numCells = this->GetNumberOfPolys() + this->GetNumberOfStrips();
  newPolys->Allocate(newPolys->EstimateSize(numCells,3),3*numCells);

  // pass through triangles; triangulate polygons if necessary
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    if ( npts == 3 )
      {
      newPolys->InsertNextCell(npts,pts);
      }
    else if ( npts > 3 ) // triangulate poly
      {
      poly.Initialize(npts,pts,inPoints);
      poly.Triangulate(outVerts);
      for (i=0; i<outVerts.GetNumberOfIds()/3; i++)
        {
        newPolys->InsertNextCell(3);
        for (j=0; j<3; j++)
          newPolys->InsertCellPoint(outVerts.GetId(3*i+j));
        }
      }
    }

  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    p1 = pts[0];
    p2 = pts[1];
    p3 = pts[2];
    for (i=0; i<(npts-2); i++)
      {
      newPolys->InsertNextCell(3);
      if ( (i % 2) ) // flip ordering to preserve consistency
        {
        newPolys->InsertCellPoint(p2);
        newPolys->InsertCellPoint(p1);
        newPolys->InsertCellPoint(p3);
        }
      else
        {
        newPolys->InsertCellPoint(p1);
        newPolys->InsertCellPoint(p2);
        newPolys->InsertCellPoint(p3);
        }
      p1 = p2;
      p2 = p3;
      p3 = pts[3+i];
      }
    }
//
// Update ourselves
//
  newPolys->Squeeze();
  this->SetPolys(newPolys);

  // pass through points and point data
  this->SetPoints(this->Input->GetPoints());
  pd = this->Input->GetPointData();
  this->PointData = *pd;

  // pass through other stuff if requested
  if ( this->PassVerts ) this->SetVerts(this->Input->GetVerts());
  if ( this->PassLines ) this->SetLines(this->Input->GetLines());

  vlDebugMacro(<<"Converted " << inPolys->GetNumberOfCells() <<
               " polygons and " << inStrips->GetNumberOfCells() <<
               " strips to " << newPolys->GetNumberOfCells() <<
               " triangles");
}


void vlTriangleFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlTriangleFilter::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Pass Verts: " << (this->PassVerts ? "On\n" : "Off\n");
    os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");
    }

}

