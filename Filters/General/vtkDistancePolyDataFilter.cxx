/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistancePolyDataFilter.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkImplicitPolyDataDistance.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"

vtkStandardNewMacro(vtkDistancePolyDataFilter);

//-----------------------------------------------------------------------------
vtkDistancePolyDataFilter::vtkDistancePolyDataFilter() : vtkPolyDataAlgorithm()
{
  this->SignedDistance = 1;
  this->NegateDistance = 0;
  this->ComputeSecondDistance = 1;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
vtkDistancePolyDataFilter::~vtkDistancePolyDataFilter()
{
}


//-----------------------------------------------------------------------------
int vtkDistancePolyDataFilter::RequestData(vtkInformation*        vtkNotUsed(request),
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector*  outputVector)
{
  vtkPolyData *input0 = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData *input1 = vtkPolyData::GetData(inputVector[1], 0);
  vtkPolyData* output0 = vtkPolyData::GetData(outputVector, 0);
  vtkPolyData* output1 = vtkPolyData::GetData(outputVector, 1);

  output0->CopyStructure(input0);
  output0->GetPointData()->PassData(input0->GetPointData());
  output0->GetCellData()->PassData(input0->GetCellData());
  output0->BuildCells();
  this->GetPolyDataDistance(output0, input1);

  if (this->ComputeSecondDistance)
  {
    output1->CopyStructure(input1);
    output1->GetPointData()->PassData(input1->GetPointData());
    output1->GetCellData()->PassData(input1->GetCellData());
    output1->BuildCells();
    this->GetPolyDataDistance(output1, input0);
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkDistancePolyDataFilter::GetPolyDataDistance(vtkPolyData* mesh, vtkPolyData* src)
{
  vtkDebugMacro(<<"Start vtkDistancePolyDataFilter::GetPolyDataDistance");

  if (mesh->GetNumberOfPolys() == 0 || mesh->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro(<<"No points/cells to operate on");
    return;
  }

  if (src->GetNumberOfPolys() == 0 || src->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro(<<"No points/cells to difference from");
    return;
  }

  vtkImplicitPolyDataDistance* imp = vtkImplicitPolyDataDistance::New();
  imp->SetInput( src );

  // Calculate distance from points.
  int numPts = mesh->GetNumberOfPoints();

  vtkDoubleArray* pointArray = vtkDoubleArray::New();
  pointArray->SetName( "Distance" );
  pointArray->SetNumberOfComponents( 1 );
  pointArray->SetNumberOfTuples( numPts );

  for (vtkIdType ptId = 0; ptId < numPts; ptId++)
  {
    double pt[3];
    mesh->GetPoint( ptId, pt );
    double val = imp->EvaluateFunction( pt );
    double dist = SignedDistance ? (NegateDistance ? -val : val) : fabs(val);
    pointArray->SetValue( ptId, dist );
  }

  mesh->GetPointData()->AddArray( pointArray );
  pointArray->Delete();
  mesh->GetPointData()->SetActiveScalars( "Distance" );

  // Calculate distance from cell centers.
  int numCells = mesh->GetNumberOfCells();

  vtkDoubleArray* cellArray = vtkDoubleArray::New();
  cellArray->SetName( "Distance" );
  cellArray->SetNumberOfComponents( 1 );
  cellArray->SetNumberOfTuples( numCells );

  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    vtkCell *cell = mesh->GetCell( cellId );
    int subId;
    double pcoords[3], x[3], weights[256];

    cell->GetParametricCenter( pcoords );
    cell->EvaluateLocation( subId, pcoords, x, weights );

    double val = imp->EvaluateFunction( x );
    double dist = SignedDistance ? (NegateDistance ? -val : val) : fabs(val);
    cellArray->SetValue( cellId, dist );
  }

  mesh->GetCellData()->AddArray( cellArray );
  cellArray->Delete();
  mesh->GetCellData()->SetActiveScalars("Distance");

  imp->Delete();

  vtkDebugMacro(<<"End vtkDistancePolyDataFilter::GetPolyDataDistance");
}

//-----------------------------------------------------------------------------
vtkPolyData* vtkDistancePolyDataFilter::GetSecondDistanceOutput()
{
  if (!this->ComputeSecondDistance)
  {
    return 0;
  }
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(1));
}

//-----------------------------------------------------------------------------
void vtkDistancePolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SignedDistance: " << this->SignedDistance << "\n";
  os << indent << "NegateDistance: " << this->NegateDistance << "\n";
  os << indent << "ComputeSecondDistance: " << this->ComputeSecondDistance << "\n";
}
