/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedThresholds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedThresholds.h"

#include "vtkDataSet.h"
#include "vtkThreshold.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkSignedCharArray.h"

vtkCxxRevisionMacro(vtkExtractSelectedThresholds, "1.15");
vtkStandardNewMacro(vtkExtractSelectedThresholds);

//----------------------------------------------------------------------------
vtkExtractSelectedThresholds::vtkExtractSelectedThresholds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedThresholds::~vtkExtractSelectedThresholds()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // verify the input, selection and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    vtkErrorMacro(<<"No input specified");
    return 0;
    }

  if ( ! selInfo )
    {
    //When not given a selection, quietly select nothing.
    return 1;
    }

  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::THRESHOLDS)
    {
    vtkErrorMacro("Missing or invalid CONTENT_TYPE.");
    return 1;
    }

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  vtkDebugMacro(<< "Extracting from dataset");

  int thresholdByPointVals = 0;
  int fieldType = vtkSelection::CELL;
  if (sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()))
    {
    fieldType = sel->GetProperties()->Get(vtkSelection::FIELD_TYPE());
    if (fieldType == vtkSelection::POINT)
      {
      if (sel->GetProperties()->Has(vtkSelection::CONTAINING_CELLS()))
        {
        thresholdByPointVals = 
          sel->GetProperties()->Get(vtkSelection::CONTAINING_CELLS());
        }
      }
    }

  if (thresholdByPointVals || fieldType==vtkSelection::CELL)
    {
    return this->ExtractCells(sel, input, output, thresholdByPointVals);
    }
  if (fieldType == vtkSelection::POINT)
    {
    return this->ExtractPoints(sel, input, output);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::ExtractCells(
  vtkSelection *sel, 
  vtkDataSet *input, 
  vtkDataSet *output,
  int usePointScalars)
{
  //find the values to threshold within
  vtkDoubleArray *lims = vtkDoubleArray::SafeDownCast(sel->GetSelectionList());
  if (lims == NULL)
    {
    vtkErrorMacro(<<"No values to threshold with");
    return 1;
    }

  //find out what array we are suppose to threshold in
  vtkDataArray *inScalars = NULL;
  if (usePointScalars)
    {
    if (sel->GetSelectionList()->GetName())
      {
      inScalars = input->GetPointData()->GetArray(
        sel->GetSelectionList()->GetName());
      }
    else
      {
      inScalars = input->GetPointData()->GetScalars();
      }
    }
  else
    {
    if (sel->GetSelectionList()->GetName())
      {
      inScalars = input->GetCellData()->GetArray(
        sel->GetSelectionList()->GetName());
      }
    else
      {
      inScalars = input->GetCellData()->GetScalars();
      }
    }
  if (inScalars == NULL)
    {
    vtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
    }
  
  int inverse = 0;
  if (sel->GetProperties()->Has(vtkSelection::INVERSE()))
    {
    inverse = sel->GetProperties()->Get(vtkSelection::INVERSE());
    }

  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap = NULL;
  vtkIdList *newCellPts = NULL;
  vtkCell *cell = 0;
  vtkPoints *newPoints = 0;
  vtkIdType i, ptId, newId, numPts, numCells;
  int numCellPts;
  double x[3];

  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  int keepCell;


  outPD->CopyGlobalIdsOn();
  outPD->CopyAllocate(pd);
  outCD->CopyGlobalIdsOn();
  outCD->CopyAllocate(cd);

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  vtkDataSet *outputDS = output;
  vtkSignedCharArray *pointInArray = NULL;
  vtkSignedCharArray *cellInArray = NULL;

  vtkUnstructuredGrid *outputUG = NULL;
  vtkIdTypeArray *originalCellIds = NULL;
  vtkIdTypeArray *originalPointIds = NULL;

  signed char flag = inverse ? 1 : -1;

  if (passThrough)
    {
    outputDS->ShallowCopy(input);

    pointInArray = vtkSignedCharArray::New();
    pointInArray->SetNumberOfComponents(1);
    pointInArray->SetNumberOfTuples(numPts);
    for (i=0; i < numPts; i++)
      {
      pointInArray->SetValue(i, flag);
      }
    pointInArray->SetName("vtkInsidedness");
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);

    cellInArray = vtkSignedCharArray::New();
    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i=0; i < numCells; i++)
      {
      cellInArray->SetValue(i, flag);
      }
    cellInArray->SetName("vtkInsidedness");
    outCD->AddArray(cellInArray);
    outCD->SetScalars(cellInArray);
    }
  else
    {
    outputUG = vtkUnstructuredGrid::SafeDownCast(output);
    outputUG->Allocate(input->GetNumberOfCells());
    newPoints = vtkPoints::New();
    newPoints->Allocate(numPts);

    pointMap = vtkIdList::New(); //maps old point ids into new
    pointMap->SetNumberOfIds(numPts);
    for (i=0; i < numPts; i++)
      {
      pointMap->SetId(i,-1);
      }

    newCellPts = vtkIdList::New();     

    originalCellIds = vtkIdTypeArray::New();
    originalCellIds->SetName("vtkOriginalCellIds");
    originalCellIds->SetNumberOfComponents(1);
    outCD->AddArray(originalCellIds);

    originalPointIds = vtkIdTypeArray::New();
    originalPointIds->SetName("vtkOriginalPointIds");
    originalPointIds->SetNumberOfComponents(1);
    outPD->AddArray(originalPointIds);
    originalPointIds->Delete();
    }

  flag = -flag;

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < input->GetNumberOfCells(); cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    // BUG: This code misses the case where the threshold is contained
    // completely within the cell but none of its points are inside
    // the range.  Consider as an example the threshold range [1, 2]
    // with a cell [0, 3].  
    if ( usePointScalars )
      {
      keepCell = 0;
      int totalAbove = 0;
      int totalBelow = 0;
      for ( i=0; 
            (i < numCellPts) && (passThrough || !keepCell); 
            i++)
        {
        int above = 0; 
        int below = 0;
        ptId = cellPts->GetId(i);
        int inside = this->EvaluateValue( inScalars, ptId, lims, &above, &below, NULL );
        totalAbove += above;
        totalBelow += below;
        // Have we detected a cell that straddles the threshold?
        if ((!inside) && (totalAbove && totalBelow))
          {
          inside = 1;
          }
        if (passThrough && (inside ^ inverse))
          {
          pointInArray->SetValue(ptId, flag);
          cellInArray->SetValue(cellId, flag);
          }
        keepCell |= inside;
        }
      }
    else //use cell scalars
      {
      keepCell = this->EvaluateValue( inScalars, cellId, lims );
      if (passThrough && (keepCell ^ inverse))
        {
        cellInArray->SetValue(cellId, flag);
        }
      }
    
    if (  !passThrough &&
          (numCellPts > 0) && 
          (keepCell + inverse == 1) ) // Poor man's XOR
      {
      // satisfied thresholding (also non-empty cell, i.e. not VTK_EMPTY_CELL)
      originalCellIds->InsertNextValue(cellId);

      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          input->GetPoint(ptId, x);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          originalPointIds->InsertNextValue(ptId);
          }
        newCellPts->InsertId(i,newId);
        }
      newCellId = outputUG->InsertNextCell(cell->GetCellType(),newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells
  
  // now clean up / update ourselves
  if (passThrough)
    {
    pointInArray->Delete();
    cellInArray->Delete();
    }
  else
    {
    outputUG->SetPoints(newPoints);
    newPoints->Delete();
    pointMap->Delete();
    newCellPts->Delete();
    originalCellIds->Delete();
    }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::ExtractPoints(
  vtkSelection *sel, 
  vtkDataSet *input, 
  vtkDataSet *output)
{
  //find the values to threshold within
  vtkDoubleArray *lims = vtkDoubleArray::SafeDownCast(sel->GetSelectionList());
  if (lims == NULL)
    {
    vtkErrorMacro(<<"No values to threshold with");
    return 1;
    }

  //find out what array we are suppose to threshold in
  vtkDataArray *inScalars = NULL;
  if (sel->GetSelectionList()->GetName())
    {
    inScalars = input->GetPointData()->GetArray(
      sel->GetSelectionList()->GetName());
    }
  else
    {
    inScalars = input->GetPointData()->GetScalars();
    }
  if (inScalars == NULL)
    {
    vtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
    }
  
  int inverse = 0;
  if (sel->GetProperties()->Has(vtkSelection::INVERSE()))
    {
    inverse = sel->GetProperties()->Get(vtkSelection::INVERSE());
    }

  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData *inputPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();

  vtkDataSet *outputDS = output;
  vtkSignedCharArray *pointInArray = NULL;

  vtkUnstructuredGrid * outputUG = NULL;
  vtkPoints *newPts = vtkPoints::New();

  vtkIdTypeArray* originalPointIds = 0;

  signed char flag = inverse ? 1 : -1;
  
  if (passThrough)
    {
    outputDS->ShallowCopy(input);

    pointInArray = vtkSignedCharArray::New();
    pointInArray->SetNumberOfComponents(1);
    pointInArray->SetNumberOfTuples(numPts);
    for (vtkIdType i=0; i < numPts; i++)
      {
      pointInArray->SetValue(i, flag);
      }
    pointInArray->SetName("vtkInsidedness");
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    }
  else
    {
    outputUG = vtkUnstructuredGrid::SafeDownCast(output);
    outputUG->Allocate(numPts);
    
    newPts->Allocate(numPts);
    outputUG->SetPoints(newPts);

    outPD->CopyGlobalIdsOn();
    outPD->CopyAllocate(inputPD);

    originalPointIds = vtkIdTypeArray::New();
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->SetName("vtkOriginalPointIds");
    outPD->AddArray(originalPointIds);
    originalPointIds->Delete();
    }

  flag = -flag;

  vtkIdType outPtCnt = 0;
  for (vtkIdType ptId = 0; ptId < numPts; ptId++)
    {
    int keepPoint = this->EvaluateValue( inScalars, ptId, lims );
    if (keepPoint ^ inverse)
      {
      if (passThrough)
        {
        pointInArray->SetValue(ptId, flag);
        }
      else
        {
        double X[4];
        input->GetPoint(ptId, X);
        newPts->InsertNextPoint(X);
        outPD->CopyData(inputPD, ptId, outPtCnt);
        originalPointIds->InsertNextValue(ptId);
        outputUG->InsertNextCell(VTK_VERTEX, 1, &outPtCnt);
        outPtCnt++;
        }
      }
    }

  if (passThrough)
    {
    pointInArray->Delete();
    }
  newPts->Delete();
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedThresholds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::EvaluateValue(
  vtkDataArray *scalars, vtkIdType id, vtkDoubleArray *lims)
{
  int keepCell = 0;
  //check the value in the array against all of the thresholds in lims
  //if it is inside any, return true
  for (int i = 0; i < lims->GetNumberOfTuples(); i+=2)
    {
    double value = scalars->GetComponent(id, 0);
    if (value >= lims->GetValue(i) && value <= lims->GetValue(i+1))
      {
      keepCell = 1;
      }
    }
  return keepCell;
}


//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::EvaluateValue(
  vtkDataArray *scalars, vtkIdType id, vtkDoubleArray *lims,
  int *AboveCount, int *BelowCount, int *InsideCount)
{
  int keepCell = 0;
  //check the value in the array against all of the thresholds in lims
  //if it is inside any, return true
  int above = 0;
  int below = 0;
  int inside = 0;
  for (int i = 0; i < lims->GetNumberOfTuples(); i+=2)
    {
    double value = scalars->GetComponent(id, 0);
    double low = lims->GetValue(i);
    double high = lims->GetValue(i+1);
    if (value >= low && value <= high)
      {
      keepCell = 1;
      ++inside;
      }
    else if (value < low)
      {
      ++below;
      }
    else if (value > high)
      {
      ++above;
      }
    }
  if (AboveCount) *AboveCount = above;
  if (BelowCount) *BelowCount = below;
  if (InsideCount) *InsideCount = inside;
  return keepCell;
}

