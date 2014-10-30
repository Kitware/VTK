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

#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>

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


  vtkDataObject* inputDO = vtkDataObject::GetData(inInfo);

  // verify the input, selection and ouptut
  if ( ! selInfo )
    {
    //When not given a selection, quietly select nothing.
    return 1;
    }

  vtkSelection *sel = vtkSelection::GetData(selInfo);
  vtkSelectionNode *node = 0;
  if (sel->GetNumberOfNodes() == 1)
    {
    node = sel->GetNode(0);
    }
  if (!node)
    {
    vtkErrorMacro("Selection must have a single node.");
    return 1;
    }
  if (!node->GetProperties()->Has(vtkSelectionNode::CONTENT_TYPE()) ||
      node->GetProperties()->Get(vtkSelectionNode::CONTENT_TYPE()) != vtkSelectionNode::THRESHOLDS)
    {
    vtkErrorMacro("Missing or invalid CONTENT_TYPE.");
    return 1;
    }

  if (vtkDataSet* input = vtkDataSet::SafeDownCast(inputDO))
    {
    if (input->GetNumberOfCells() == 0 && input->GetNumberOfPoints() == 0)
      {
      // empty input, nothing to do..
      return 1;
      }

    vtkDataSet *output = vtkDataSet::GetData(outInfo);
    vtkDebugMacro(<< "Extracting from dataset");

    int thresholdByPointVals = 0;
    int fieldType = vtkSelectionNode::CELL;
    if (node->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()))
      {
      fieldType = node->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
      if (fieldType == vtkSelectionNode::POINT)
        {
        if (node->GetProperties()->Has(vtkSelectionNode::CONTAINING_CELLS()))
          {
          thresholdByPointVals =
            node->GetProperties()->Get(vtkSelectionNode::CONTAINING_CELLS());
          }
        }
      }

    if (thresholdByPointVals || fieldType == vtkSelectionNode::CELL)
      {
      return this->ExtractCells(node, input, output, thresholdByPointVals);
      }
    if (fieldType == vtkSelectionNode::POINT)
      {
      return this->ExtractPoints(node, input, output);
      }
    }
  else if (vtkTable* inputTable = vtkTable::SafeDownCast(inputDO))
    {
    if (inputTable->GetNumberOfRows() == 0)
      {
      return 1;
      }
    vtkTable* output = vtkTable::GetData(outInfo);
    return this->ExtractRows(node, inputTable, output);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::ExtractCells(
  vtkSelectionNode *sel,
  vtkDataSet *input,
  vtkDataSet *output,
  int usePointScalars)
{
  //find the values to threshold within
  vtkDataArray *lims = vtkDataArray::SafeDownCast(sel->GetSelectionList());
  if (lims == NULL)
    {
    vtkErrorMacro(<<"No values to threshold with");
    return 1;
    }

  //find out what array we are supposed to threshold in
  vtkDataArray *inScalars = NULL;
  bool use_ids = false;
  if (usePointScalars)
    {
    if (sel->GetSelectionList()->GetName())
      {
      if (strcmp(sel->GetSelectionList()->GetName(), "vtkGlobalIds") == 0)
        {
        inScalars = input->GetPointData()->GetGlobalIds();
        }
      else if (strcmp(sel->GetSelectionList()->GetName(), "vtkIndices") == 0)
        {
        use_ids = true;
        }
      else
        {
        inScalars = input->GetPointData()->GetArray(
          sel->GetSelectionList()->GetName());
        }
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
      if (strcmp(sel->GetSelectionList()->GetName(), "vtkGlobalIds") == 0)
        {
        inScalars = input->GetCellData()->GetGlobalIds();
        }
      else if (strcmp(sel->GetSelectionList()->GetName(), "vtkIndices") == 0)
        {
        use_ids = true;
        }
      else
        {
        inScalars = input->GetCellData()->GetArray(
          sel->GetSelectionList()->GetName());
        }
      }
    else
      {
      inScalars = input->GetCellData()->GetScalars();
      }
    }
  if (inScalars == NULL && !use_ids)
    {
    vtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
    }

  int inverse = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
    {
    inverse = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
    }

  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  int comp_no = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::COMPONENT_NUMBER()))
    {
    comp_no = sel->GetProperties()->Get(vtkSelectionNode::COMPONENT_NUMBER());
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
        int inside = this->EvaluateValue(
          inScalars, comp_no, ptId, lims, &above, &below, NULL);
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
      keepCell = this->EvaluateValue(inScalars, comp_no, cellId, lims);
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
  vtkSelectionNode *sel,
  vtkDataSet *input,
  vtkDataSet *output)
{
  //find the values to threshold within
  vtkDataArray *lims = vtkDataArray::SafeDownCast(sel->GetSelectionList());
  if (lims == NULL)
    {
    vtkErrorMacro(<<"No values to threshold with");
    return 1;
    }

  //find out what array we are supposed to threshold in
  vtkDataArray *inScalars = NULL;
  bool use_ids = false;
  if (sel->GetSelectionList()->GetName())
    {
    if (strcmp(sel->GetSelectionList()->GetName(), "vtkGlobalIds") == 0)
      {
      inScalars = input->GetPointData()->GetGlobalIds();
      }
    else if (strcmp(sel->GetSelectionList()->GetName(), "vtkIndices") == 0)
      {
      use_ids = true;
      }
    else
      {
      inScalars = input->GetPointData()->GetArray(
        sel->GetSelectionList()->GetName());
      }
    }
  else
    {
    inScalars = input->GetPointData()->GetScalars();
    }
  if (inScalars == NULL && !use_ids)
    {
    vtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
    }

  int inverse = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
    {
    inverse = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
    }

  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  int comp_no = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::COMPONENT_NUMBER()))
    {
    comp_no = sel->GetProperties()->Get(vtkSelectionNode::COMPONENT_NUMBER());
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
    int keepPoint = this->EvaluateValue( inScalars, comp_no, ptId, lims );
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
int vtkExtractSelectedThresholds::ExtractRows(
  vtkSelectionNode* sel, vtkTable* input, vtkTable* output)
{
  //find the values to threshold within
  vtkDataArray *lims = vtkDataArray::SafeDownCast(sel->GetSelectionList());
  if (lims == NULL)
    {
    vtkErrorMacro(<<"No values to threshold with");
    return 1;
    }

  // Determine the array to threshold.
  vtkDataArray *inScalars = NULL;
  bool use_ids = false;
  if (sel->GetSelectionList()->GetName())
    {
    if (strcmp(sel->GetSelectionList()->GetName(), "vtkGlobalIds") == 0)
      {
      inScalars = input->GetRowData()->GetGlobalIds();
      }
    else if (strcmp(sel->GetSelectionList()->GetName(), "vtkIndices") == 0)
      {
      use_ids = true;
      }
    else
      {
      inScalars = input->GetRowData()->GetArray(
        sel->GetSelectionList()->GetName());
      }
    }

  if (inScalars == NULL && !use_ids)
    {
    vtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
    }

  int inverse = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
    {
    inverse = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
    }

  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  int comp_no = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::COMPONENT_NUMBER()))
    {
    comp_no = sel->GetProperties()->Get(vtkSelectionNode::COMPONENT_NUMBER());
    }

  vtkDataSetAttributes* inRD = input->GetRowData();
  vtkDataSetAttributes* outRD = output->GetRowData();
  vtkSmartPointer<vtkSignedCharArray> rowInArray;
  vtkSmartPointer<vtkIdTypeArray> originalRowIds;
  vtkIdType numRows = input->GetNumberOfRows();

  signed char flag = inverse ? 1 : -1;

  if (passThrough)
    {
    output->ShallowCopy(input);

    rowInArray = vtkSmartPointer<vtkSignedCharArray>::New();
    rowInArray->SetNumberOfComponents(1);
    rowInArray->SetNumberOfTuples(numRows);
    std::fill(rowInArray->GetPointer(0), rowInArray->GetPointer(0) + numRows, flag);
    rowInArray->SetName("vtkInsidedness");
    outRD->AddArray(rowInArray);
    }
  else
    {
    outRD->CopyGlobalIdsOn();
    outRD->CopyAllocate(inRD);

    originalRowIds = vtkSmartPointer<vtkIdTypeArray>::New();
    originalRowIds->SetNumberOfComponents(1);
    originalRowIds->SetName("vtkOriginalRowIds");
    originalRowIds->Allocate(numRows);
    outRD->AddArray(originalRowIds);
    }

  flag = -flag;

  vtkIdType outRCnt = 0;
  for (vtkIdType rowId = 0; rowId < numRows; rowId++)
    {
    int keepRow = this->EvaluateValue(inScalars, comp_no, rowId, lims);
    if (keepRow ^ inverse)
      {
      if (passThrough)
        {
        rowInArray->SetValue(rowId, flag);
        }
      else
        {
        outRD->CopyData(inRD, rowId, outRCnt);
        originalRowIds->InsertNextValue(rowId);
        outRCnt++;
        }
      }
    }
  outRD->Squeeze();
  return 1;
}


//----------------------------------------------------------------------------
void vtkExtractSelectedThresholds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

namespace
{
  template <class daT>
  bool TestItem(vtkIdType numLims, daT* limsPtr, double value)
    {
    for (int i = 0; i < numLims; i+=2)
      {
      if (value >= limsPtr[i] && value <= limsPtr[i+1])
        {
        return true;
        }
      }
    return false;
    }

  template <class daT>
  bool TestItem(vtkIdType numLims, daT* limsPtr, double value,
    int &above, int &below, int& inside)
    {
    bool keepCell = false;
    for (vtkIdType i = 0; i < numLims; i+=2)
      {
      daT low = limsPtr[i];
      daT high = limsPtr[i+1];
      if (value >= low && value <= high)
        {
        keepCell = true;
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
    return keepCell;
    }
};
//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::EvaluateValue(
  vtkDataArray *scalars, int comp_no, vtkIdType id, vtkDataArray *lims)
{
  int keepCell = 0;
  //check the value in the array against all of the thresholds in lims
  //if it is inside any, return true
  double value = 0.0;
  if (comp_no < 0 && scalars)
    {
    // use magnitude.
    int numComps = scalars->GetNumberOfComponents();
    const double *tuple = scalars->GetTuple(id);
    for (int cc=0; cc < numComps; cc++)
      {
      value += tuple[cc]*tuple[cc];
      }
    value = sqrt(value);
    }
  else
    {
    value = scalars? scalars->GetComponent(id, comp_no) :
      static_cast<double>(id); /// <=== precision loss when using id.
    }

  void* rawLimsPtr = lims->GetVoidPointer(0);
  vtkIdType numLims = lims->GetNumberOfComponents() * lims->GetNumberOfTuples();
  switch (lims->GetDataType())
    {
    vtkTemplateMacro(
      keepCell = TestItem<VTK_TT>(numLims,
        static_cast<VTK_TT*>(rawLimsPtr),
        value));
    }
  return keepCell;
}


//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::EvaluateValue(
  vtkDataArray *scalars, int comp_no, vtkIdType id, vtkDataArray *lims,
  int *AboveCount, int *BelowCount, int *InsideCount)
{
  double value = 0.0;
  if (comp_no < 0 && scalars)
    {
    // use magnitude.
    int numComps = scalars->GetNumberOfComponents();
    const double *tuple = scalars->GetTuple(id);
    for (int cc=0; cc < numComps; cc++)
      {
      value += tuple[cc]*tuple[cc];
      }
    value = sqrt(value);
    }
  else
    {
    value = scalars? scalars->GetComponent(id, comp_no) :
      static_cast<double>(id); /// <=== precision loss when using id.
    }

  int keepCell = 0;
  //check the value in the array against all of the thresholds in lims
  //if it is inside any, return true
  int above = 0;
  int below = 0;
  int inside = 0;

  void* rawLimsPtr = lims->GetVoidPointer(0);
  vtkIdType numLims = lims->GetNumberOfComponents() * lims->GetNumberOfTuples();
  switch (lims->GetDataType())
    {
    vtkTemplateMacro(
      keepCell = TestItem<VTK_TT>(numLims,
        static_cast<VTK_TT*>(rawLimsPtr),
        value,
        above, below, inside));
    }

  if (AboveCount) *AboveCount = above;
  if (BelowCount) *BelowCount = below;
  if (InsideCount) *InsideCount = inside;
  return keepCell;
}

