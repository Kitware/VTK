/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearToQuadraticCellsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLinearToQuadraticCellsFilter.h"

#include "vtkAlgorithm.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMergePoints.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolygon.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkTetra.h"
#include "vtkWedge.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticPolygon.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkLinearToQuadraticCellsFilter);

namespace
{
void DegreeElevate(vtkCell* lowerOrderCell,
                   vtkIncrementalPointLocator* pointLocator,
                   vtkUnsignedCharArray *types, vtkIdTypeArray *locations,
                   vtkCellArray* cells, vtkPointData *inPd, vtkPointData *outPd,
                   vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd)
{
  double lowerOrderCoeffs[VTK_CELL_SIZE];

  vtkCell* higherOrderCell = NULL;

  switch (lowerOrderCell->GetCellType())
  {

#define DegreeElevateCase(LowerOrderCellType, HigherOrderCell)          \
  case LowerOrderCellType:                                              \
    higherOrderCell = HigherOrderCell::New();                           \
    break

  DegreeElevateCase(VTK_LINE, vtkQuadraticEdge);
  DegreeElevateCase(VTK_TRIANGLE, vtkQuadraticTriangle);
  DegreeElevateCase(VTK_QUAD, vtkQuadraticQuad);
  DegreeElevateCase(VTK_POLYGON, vtkQuadraticPolygon);
  DegreeElevateCase(VTK_TETRA, vtkQuadraticTetra);
  DegreeElevateCase(VTK_HEXAHEDRON, vtkQuadraticHexahedron);
  DegreeElevateCase(VTK_WEDGE, vtkQuadraticWedge);
  DegreeElevateCase(VTK_PYRAMID, vtkQuadraticPyramid);

#undef DegreeElevateMacro

    default:
      vtkGenericWarningMacro(
        << "vtkLinearToQuadraticCellsFilter does not currently support degree elevating cell type " << lowerOrderCell->GetCellType() << ".");
      break;
  }

  if (higherOrderCell == NULL)
  {
    return;
  }

  double *higherOrderPCoords = higherOrderCell->GetParametricCoords();
  for (vtkIdType hp = 0; hp < higherOrderCell->GetNumberOfPoints(); hp++)
  {
    lowerOrderCell->InterpolateFunctions(higherOrderPCoords + (hp*3),
                                         lowerOrderCoeffs);

    double higherOrderPoint[3] = {0.,0.,0.};
    double lowerOrderPoint[3];
    for (vtkIdType lp=0; lp<lowerOrderCell->GetNumberOfPoints(); lp++)
    {
      // NB: vtkGenericCell creates a local copy of the cell's points, so we
      //     must use local indexing here (i.e. <lp> instead of
      //     <lowerOrderCell->GetPointIds()->GetId(lp)>).
      lowerOrderCell->GetPoints()->GetPoint(lp, lowerOrderPoint);
;
      for (int i = 0; i < 3; i++)
      {
        higherOrderPoint[i] += lowerOrderPoint[i] * lowerOrderCoeffs[lp];
      }
    }

    vtkIdType pId;
    pointLocator->InsertUniquePoint(higherOrderPoint, pId);
    higherOrderCell->GetPointIds()->SetId(hp,pId);

    outPd->InterpolatePoint(inPd, pId, lowerOrderCell->GetPointIds(),
                            lowerOrderCoeffs);
  }

  vtkIdType newCellId = cells->InsertNextCell(higherOrderCell);
  locations->InsertNextValue(cells->GetTraversalLocation());
  types->InsertNextValue(higherOrderCell->GetCellType());
  outCd->CopyData(inCd,cellId,newCellId);

  higherOrderCell->Delete();
}

}

//----------------------------------------------------------------------------
vtkLinearToQuadraticCellsFilter::vtkLinearToQuadraticCellsFilter()
{
  this->Locator = NULL;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
vtkLinearToQuadraticCellsFilter::~vtkLinearToQuadraticCellsFilter()
{
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkLinearToQuadraticCellsFilter::SetLocator(
  vtkIncrementalPointLocator *locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLinearToQuadraticCellsFilter::CreateDefaultLocator()
{
  if (this->Locator == NULL)
  {
    this->Locator = vtkMergePoints::New();
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function.
vtkMTimeType vtkLinearToQuadraticCellsFilter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Locator != NULL)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkLinearToQuadraticCellsFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast(
                                 inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
                                  outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkNew<vtkUnsignedCharArray> outputCellTypes;
  vtkNew<vtkIdTypeArray> outputCellLocations;
  vtkNew<vtkCellArray> outputCellConnectivities;

  output->SetPoints(vtkNew<vtkPoints>().GetPointer());

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    output->GetPoints()->SetDataType(input->GetPoints()->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    output->GetPoints()->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    output->GetPoints()->SetDataType(VTK_DOUBLE);
  }

  // locator used to merge potentially duplicate points
  if (this->Locator == NULL)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(output->GetPoints(), input->GetBounds());

  vtkIdType estimatedSize = input->GetNumberOfCells();
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  output->GetPointData()->InterpolateAllocate(
    input->GetPointData(),estimatedSize,estimatedSize/2);
  output->GetCellData()->CopyAllocate(
    input->GetCellData(),estimatedSize,estimatedSize/2);

  vtkGenericCell *cell = vtkGenericCell::New();
  vtkCellIterator *it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCell(cell);
    DegreeElevate(cell, this->Locator, outputCellTypes.GetPointer(),
                  outputCellLocations.GetPointer(),
                  outputCellConnectivities.GetPointer(),
                  input->GetPointData(), output->GetPointData(),
                  input->GetCellData(), it->GetCellId(), output->GetCellData());
  }
  it->Delete();
  cell->Delete();

  output->SetCells(outputCellTypes.GetPointer(),
                   outputCellLocations.GetPointer(),
                   outputCellConnectivities.GetPointer());

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkLinearToQuadraticCellsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
