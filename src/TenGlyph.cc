/*=========================================================================

  Program:   Visualization Library
  Module:    TenGlyph.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TenGlyph.hh"
#include "Trans.hh"
#include "FVectors.hh"
#include "FNormals.hh"
#include "vtkMath.hh"

// Description
// Construct object with scaling on and scale factor 1.0.
vtkTensorGlyph::vtkTensorGlyph()
{
  this->Source = NULL;
  this->Scaling = 1;
  this->ScaleFactor = 1.0;
  this->ExtractEigenvalues = 1;
}

vtkTensorGlyph::~vtkTensorGlyph()
{
}

void vtkTensorGlyph::Execute()
{
  vtkPointData *pd;
  vtkTensors *inTensors;
  int numPts, numSourcePts, numSourceCells;
  int inPtId, i;
  vtkPoints *sourcePts;
  vtkCellArray *sourceCells;  
  vtkFloatPoints *newPts;
  float *x;
  vtkTransform trans;
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts, pts[MAX_CELL_SIZE];
  int ptIncr, cellId;
  vtkMath math;

  vtkDebugMacro(<<"Generating tensor glyphs");
  this->Initialize();

  pd = this->Input->GetPointData();
  inTensors = pd->GetTensors();
  numPts = this->Input->GetNumberOfPoints();

  if ( !inTensors || numPts < 1 )
    {
    vtkErrorMacro(<<"No data to glyph!");
    return;
    }
//
// Allocate storage for output PolyData
//
  sourcePts = this->Source->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = this->Source->GetNumberOfCells();

  newPts = new vtkFloatPoints(numPts*numSourcePts);

  // Setting up for calls to PolyData::InsertNextCell()
  if ( (sourceCells=this->Source->GetVerts())->GetNumberOfCells() > 0 )
    {
    this->SetVerts(new vtkCellArray(numPts*sourceCells->GetSize()));
    }
  if ( (sourceCells=this->Source->GetLines())->GetNumberOfCells() > 0 )
    {
    this->SetLines(new vtkCellArray(numPts*sourceCells->GetSize()));
    }
  if ( (sourceCells=this->Source->GetPolys())->GetNumberOfCells() > 0 )
    {
    this->SetPolys(new vtkCellArray(numPts*sourceCells->GetSize()));
    }
  if ( (sourceCells=this->Source->GetStrips())->GetNumberOfCells() > 0 )
    {
    this->SetStrips(new vtkCellArray(numPts*sourceCells->GetSize()));
    }

  // only copy scalar data through
  pd = this->Source->GetPointData();
  this->PointData.CopyAllOff();
  this->PointData.CopyScalarsOn();
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
// Traverse all Input points, transforming glyph at Source points
//
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = inPtId * numSourcePts;
    
    trans.Identity();

    // translate Source to Input point
    x = this->Input->GetPoint(inPtId);
    trans.Translate(x[0], x[1], x[2]);

    // extract appropriate eigenfunctions

    // eigenvectors (assumed normalized) rotate object

    // if scaling modify matrix to scale according to eigenvalues

    // multiply points by resulting matrix
    trans.MultiplyPoints(sourcePts,newPts);

    // Copy point data from source
    for (i=0; i < numSourcePts; i++) 
      this->PointData.CopyData(pd,i,ptIncr+i);
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->Squeeze();
}

// Description:
// Override update method because execution can branch two ways (Input 
// and Source)
void vtkTensorGlyph::Update()
{
  // make sure input is available
  if ( this->Input == NULL || this->Source == NULL )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || 
  this->Source->GetMTime() > this->GetMTime() || 
  this->GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Source->ShouldIReleaseData() ) this->Source->ReleaseData();
}

void vtkTensorGlyph::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->Source << "\n";
  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}

