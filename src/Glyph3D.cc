/*=========================================================================

  Program:   Visualization Library
  Module:    Glyph3D.cc
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
#include "Glyph3D.hh"
#include "Trans.hh"
#include "FVectors.hh"
#include "FNormals.hh"
#include "vlMath.hh"

// Description
// Construct object with scaling on, scaling mode is by scalar value, 
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector.
vlGlyph3D::vlGlyph3D()
{
  this->Source = NULL;
  this->Scaling = 1;
  this->ScaleMode = SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = USE_VECTOR;
}

vlGlyph3D::~vlGlyph3D()
{
  if (this->Source)
    {
    this->Source->UnRegister(this);
    }
}

void vlGlyph3D::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlGlyph3D::GetClassName()))
    {
    vlDataSetToPolyFilter::PrintSelf(os,indent);

    os << indent << "Source: " << this->Source << "\n";
    os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
    os << indent << "Scale Mode: " << (this->ScaleMode == SCALE_BY_SCALAR ? "Scale by scalar\n" : "Scale by vector\n");
    os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
    os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
    os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");



    os << indent << "Orient Mode: " << (this->VectorMode == USE_VECTOR ? "Orient by vector\n" : "Orient by normal\n");
    }
}

void vlGlyph3D::Execute()
{
  vlPointData *pd;
  vlScalars *inScalars;
  vlVectors *inVectors;
  vlNormals *inNormals, *sourceNormals;
  int numPts, numSourcePts, numSourceCells;
  int inPtId, i;
  vlPoints *sourcePts;
  vlCellArray *sourceCells;  
  vlFloatPoints *newPts;
  vlFloatScalars *newScalars=NULL;
  vlFloatVectors *newVectors=NULL;
  vlFloatNormals *newNormals=NULL;
  float *x, s, *v, vNew[3];
  vlTransform trans;
  vlCell *cell;
  vlIdList *cellPts;
  int npts, pts[MAX_CELL_SIZE];
  int orient, scaleSource, ptIncr, cellId;
  float scale, den;
  vlMath math;
//
// Initialize
//
  this->Initialize();

  pd = this->Input->GetPointData();
  inScalars = pd->GetScalars();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  numPts = this->Input->GetNumberOfPoints();
//
// Allocate storage for output PolyData
//
  sourcePts = this->Source->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = this->Source->GetNumberOfCells();
  sourceNormals = this->Source->GetPointData()->GetNormals();

  newPts = new vlFloatPoints(numPts*numSourcePts);
  if (inScalars != NULL) 
    newScalars = new vlFloatScalars(numPts*numSourcePts);
  if (inVectors != NULL || inNormals != NULL ) 
    newVectors = new vlFloatVectors(numPts*numSourcePts);
  if (sourceNormals != NULL) 
    newNormals = new vlFloatNormals(numPts*numSourcePts);

  // Setting up for calls to PolyData::InsertNextCell()
  if ( (sourceCells=this->Source->GetVerts())->GetNumberOfCells() > 0 )
    {
    this->SetVerts(new vlCellArray(numPts*sourceCells->GetSize()));
    }
  if ( (sourceCells=this->Source->GetLines())->GetNumberOfCells() > 0 )
    {
    this->SetLines(new vlCellArray(numPts*sourceCells->GetSize()));
    }
  if ( (sourceCells=this->Source->GetPolys())->GetNumberOfCells() > 0 )
    {
    this->SetPolys(new vlCellArray(numPts*sourceCells->GetSize()));
    }
  if ( (sourceCells=this->Source->GetStrips())->GetNumberOfCells() > 0 )
    {
    this->SetStrips(new vlCellArray(numPts*sourceCells->GetSize()));
    }
//
// Copy (input scalars) to (output scalars) and either (input vectors or
// normals) to (output vectors). All other point attributes are copied 
// from Source.
//
  pd = this->Source->GetPointData();
  this->PointData.CopyScalarsOff();
  this->PointData.CopyVectorsOff();
  this->PointData.CopyNormalsOff();
  this->PointData.CopyAllocate(pd,numPts*numSourcePts);
//
// First copy all topology (transformation independent)
//
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->Source->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (i=0; i < npts; i++) pts[i] = cellPts->GetId(i) + ptIncr;
      this->InsertNextCell(cell->GetCellType(),npts,pts);
      }
    }
//
// Traverse all Input points, transforming Source points and copying 
// point attributes.
//
  if ( this->VectorMode == USE_VECTOR && inVectors != NULL ||
  this->VectorMode == USE_NORMAL && inNormals != NULL )
    orient = 1;
  else
    orient = 0;
    
  if ( this->Scaling && 
  ((this->ScaleMode == SCALE_BY_SCALAR && inScalars != NULL) ||
  (this->ScaleMode == SCALE_BY_VECTOR && (inVectors || inNormals))) )
    scaleSource = 1;
  else
    scaleSource = 0;

  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    
    trans.Identity();

    // translate Source to Input point
    x = this->Input->GetPoint(inPtId);
    trans.Translate(x[0], x[1], x[2]);

    if ( this->VectorMode == USE_NORMAL )
      v = inNormals->GetNormal(inPtId);
    else
      v = inVectors->GetVector(inPtId);
    scale = math.Norm(v);

    if ( orient )
      {
      // Copy Input vector
      for (i=0; i < numSourcePts; i++) 
        newVectors->InsertVector(ptIncr+i,v);
          
      if ( this->Orient ) 
        {
        vNew[0] = (v[0]+scale) / 2.0;
        vNew[1] = v[1] / 2.0;
        vNew[2] = v[2] / 2.0;
        trans.RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
        }
      }

    // determine scale factor from scalars if appropriate
    if ( inScalars != NULL )
      {
      // Copy Input scalar
      for (i=0; i < numSourcePts; i++) 
        newScalars->InsertScalar(ptIncr+i,scale);

      if ( this->ScaleMode == SCALE_BY_SCALAR )
        {
        if ( (den = this->Range[1] - this->Range[0]) == 0.0 ) den = 1.0;
        scale = inScalars->GetScalar(inPtId);


        scale = (scale < this->Range[0] ? this->Range[0] :
                 (scale > this->Range[1] ? this->Range[1] : scale));
        scale = (scale - this->Range[0]) / den;

        }
      }

    // scale data if appropriate
    if ( scaleSource )
      {
      scale *= this->ScaleFactor;
      if ( scale == 0.0 ) scale = 1.0e-10;
      trans.Scale(scale,scale,scale);
      }

    // multiply points and normals by resulting matrix
    trans.MultiplyPoints(sourcePts,newPts);
    if ( sourceNormals != NULL ) 
      trans.MultiplyNormals(sourceNormals,newNormals);

    // Copy point data from source
    for (i=0; i < numSourcePts; i++) 
      this->PointData.CopyData(pd,i,ptIncr+i);
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->PointData.SetScalars(newScalars);
  this->PointData.SetVectors(newVectors);
  this->PointData.SetNormals(newNormals);
  this->Squeeze();
}

// Description:
// Override update method because execution can branch two ways (Input 
// and Source)

void vlGlyph3D::Update()
{
  // make sure input is available
  if ( this->Input == NULL )
    {
    vlErrorMacro(<< "No input!");
    return;
    }

  if ( this->Source == NULL )
    {
    vlErrorMacro(<< "No data to copy");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}
