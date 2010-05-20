/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedIds.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkSortDataArray.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractSelectedIds);

//----------------------------------------------------------------------------
vtkExtractSelectedIds::vtkExtractSelectedIds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedIds::~vtkExtractSelectedIds()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::FillInputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port==0)
    {
    // this filter can only work with datasets.
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()); 
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet"); 
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // verify the input selection and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! input )
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
  vtkSelectionNode *node = 0;
  if (sel->GetNumberOfNodes() == 1)
    {
    node = sel->GetNode(0);
    }
  if (!node)
    {
    vtkErrorMacro("Selection must have a single node.");
    return 0;
    }
  if (node->GetContentType() != vtkSelectionNode::GLOBALIDS &&
      node->GetContentType() != vtkSelectionNode::PEDIGREEIDS &&
      node->GetContentType() != vtkSelectionNode::VALUES &&  
      node->GetContentType() != vtkSelectionNode::INDICES
      )
    {
    vtkErrorMacro("Incompatible CONTENT_TYPE.");
    return 0;
    }

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Extracting from dataset");
  
  int fieldType = vtkSelectionNode::CELL;
  if (node->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()))
    {
    fieldType = node->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
    }
  switch (fieldType)
    {
    case vtkSelectionNode::CELL:
      return this->ExtractCells(node, input, output);
      break;
    case vtkSelectionNode::POINT:
      return this->ExtractPoints(node, input, output);
    }
  return 1;
}

// Copy the points marked as "in" and build a pointmap
void vtkExtractSelectedIdsCopyPoints(vtkDataSet* input, 
  vtkDataSet* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkPoints* newPts = vtkPoints::New();

  vtkIdType i, numPts = input->GetNumberOfPoints();

  vtkIdTypeArray* originalPtIds = vtkIdTypeArray::New();
  originalPtIds->SetNumberOfComponents(1);
  originalPtIds->SetName("vtkOriginalPointIds");

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->SetCopyGlobalIds(1);
  outPD->CopyAllocate(inPD);

  for (i = 0; i < numPts; i++)
    {
    if (inArray[i] > 0)
      {
      pointMap[i] = newPts->InsertNextPoint(input->GetPoint(i));
      outPD->CopyData(inPD, i, pointMap[i]);
      originalPtIds->InsertNextValue(i);
      }
    else
      {
      pointMap[i] = -1;
      }
    }

  outPD->AddArray(originalPtIds);
  originalPtIds->Delete();

  // outputDS must be either vtkPolyData or vtkUnstructuredGrid
  vtkPointSet::SafeDownCast(output)->SetPoints(newPts);
  newPts->Delete();
}

// Copy the cells marked as "in" using the given pointmap
template <class T>
void vtkExtractSelectedIdsCopyCells(vtkDataSet* input, T* output, 
  signed char* inArray, vtkIdType* pointMap)
{
  vtkIdType numCells = input->GetNumberOfCells();
  output->Allocate(numCells / 4);

  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->SetCopyGlobalIds(1);
  outCD->CopyAllocate(inCD);

  vtkIdTypeArray* originalIds = vtkIdTypeArray::New();
  originalIds->SetNumberOfComponents(1);
  originalIds->SetName("vtkOriginalCellIds");

  vtkIdType i, j, newId = 0;
  vtkIdList* ptIds = vtkIdList::New();
  for (i = 0; i < numCells; i++)
    {
    if (inArray[i] > 0)
      {
      // special handling for polyhedron cells
      if (vtkUnstructuredGrid::SafeDownCast(input) &&
          vtkUnstructuredGrid::SafeDownCast(output) &&
          input->GetCellType(i) == VTK_POLYHEDRON)
        {
        ptIds->Reset();
        vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, ptIds);
        vtkUnstructuredGrid::ConvertFaceStreamPointIds(ptIds, pointMap);
        }
      else
        {
        input->GetCellPoints(i, ptIds);
        for (j = 0; j < ptIds->GetNumberOfIds(); j++)
          {
          ptIds->SetId(j, pointMap[ptIds->GetId(j)]);
          }
        }
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, newId++);
      originalIds->InsertNextValue(i);
      }
    }

  outCD->AddArray(originalIds);
  originalIds->Delete();
  ptIds->Delete();
}

#define vtkESI_ExtendedTemplateMacro(t1, t2, call)\
  switch (t1)\
    {\
    vtkTemplateMacro(\
      typedef VTK_TT VTK_TT1;\
      switch (t2)\
        {\
        vtkTemplateMacro(\
          typedef VTK_TT VTK_TT2;\
          call;);\
        }\
    );\
    \
  case VTK_STRING:\
     {\
     typedef vtkStdString VTK_TT1;\
     typedef vtkStdString VTK_TT2;\
     call;\
     }\
    }

namespace
{
  template <class T>
  void vtkESIDeepCopyImpl(T* out, T* in, int compno, int num_comps,
    vtkIdType numTuples)
    {
    if (compno < 0)
      {
      for (vtkIdType cc=0; cc < numTuples; cc++)
        {
        double mag = 0;
        for (int comp=0; comp < num_comps; comp++)
          {
          mag += in[comp]*in[comp];
          }
        mag = sqrt(mag);
        *out = static_cast<T>(mag);
        out++;
        in += num_comps;
        }
      }
    else
      {
      for (vtkIdType cc=0; cc < numTuples; cc++)
        {
        *out = in[compno];
        out++;
        in += num_comps;
        }
      }
    }

  void vtkESIDeepCopyImpl(vtkStdString* out, vtkStdString* in,
    int compno, int num_comps, vtkIdType numTuples)
    {
    if (compno < 0)
      {
      // we cannot compute magnitudes for string arrays!
      compno = 0;
      }
    for (vtkIdType cc=0; cc < numTuples; cc++)
      {
      *out = in[compno];
      out++;
      in += num_comps;
      }
    }

  // Deep copies a specified component (or magnitude of compno < 0).
  static void vtkESIDeepCopy(vtkAbstractArray* out, vtkAbstractArray* in, int compno)
    {
    if (in->GetNumberOfComponents() == 1)
      {
      //trivial case.
      out->DeepCopy(in);
      return;
      }

    vtkIdType numTuples = in->GetNumberOfTuples();
    out->SetNumberOfComponents(1);
    out->SetNumberOfTuples(numTuples);
    switch (in->GetDataType())
      {
      vtkTemplateMacro(
        vtkESIDeepCopyImpl<VTK_TT>(
          static_cast<VTK_TT*>(out->GetVoidPointer(0)),
          static_cast<VTK_TT*>(in->GetVoidPointer(0)),
          compno, in->GetNumberOfComponents(), numTuples));
    case VTK_STRING:
      vtkESIDeepCopyImpl(
        static_cast<vtkStdString*>(out->GetVoidPointer(0)),
        static_cast<vtkStdString*>(in->GetVoidPointer(0)),
        compno, in->GetNumberOfComponents(), numTuples);
      break;
      }
    }
};
//----------------------------------------------------------------------------
int vtkExtractSelectedIds::ExtractCells(
  vtkSelectionNode *sel,  vtkDataSet *input,
  vtkDataSet *output)
{
  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
    {
    invert = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
    }

  vtkIdType i, numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkSignedCharArray> pointInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i=0; i < numPts; i++)
    {
    pointInArray->SetValue(i, flag);
    }

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkSignedCharArray> cellInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  cellInArray->SetNumberOfComponents(1);
  cellInArray->SetNumberOfTuples(numCells);
  for (i=0; i < numCells; i++)
    {
    cellInArray->SetValue(i, flag);
    }

  if (passThrough)
    {
    output->ShallowCopy(input);
    pointInArray->SetName("vtkInsidedness");
    vtkPointData *outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    cellInArray->SetName("vtkInsidedness");
    vtkCellData *outCD = output->GetCellData();
    outCD->AddArray(cellInArray);
    outCD->SetScalars(cellInArray);
    }

  //decide what the IDS mean
  vtkAbstractArray *labelArray = NULL;
  int selType = sel->GetProperties()->Get(vtkSelectionNode::CONTENT_TYPE());
  if (selType == vtkSelectionNode::GLOBALIDS)
    {
    labelArray = vtkIdTypeArray::SafeDownCast(
      input->GetCellData()->GetGlobalIds());
    }
  else if (selType == vtkSelectionNode::PEDIGREEIDS)
    {
    labelArray = input->GetCellData()->GetPedigreeIds();
    }
  else if (selType == vtkSelectionNode::VALUES &&
           sel->GetSelectionList()->GetName())
    {
    //user chose a specific label array
    labelArray = input->GetCellData()->GetAbstractArray(
        sel->GetSelectionList()->GetName());
    }    
  
  if (labelArray == NULL && selType != vtkSelectionNode::INDICES)
    {
    return 1;
    }

  vtkIdTypeArray *idxArray = vtkIdTypeArray::New();
  idxArray->SetNumberOfComponents(1);
  idxArray->SetNumberOfTuples(numCells);
  for (i=0; i < numCells; i++)
    {
    idxArray->SetValue(i, i);
    }

  if (labelArray)
    {    
    int component_no = 0;
    if (sel->GetProperties()->Has(vtkSelectionNode::COMPONENT_NUMBER()))
      {
      component_no =
        sel->GetProperties()->Get(vtkSelectionNode::COMPONENT_NUMBER());
      if (component_no >= labelArray->GetNumberOfComponents())
        {
        component_no = 0;
        }
      }

    vtkAbstractArray* sortedArray = 
      vtkAbstractArray::CreateArray(labelArray->GetDataType());
    vtkESIDeepCopy(sortedArray, labelArray, component_no);
    vtkSortDataArray::Sort(sortedArray, idxArray);
    labelArray = sortedArray;
    }
  else
    {
    //no global array, so just use the input cell index
    labelArray = idxArray;
    labelArray->Register(NULL);
    }

  // Reverse the "in" flag
  flag = -flag;

  vtkIdList *ptIds = NULL;
  char* cellCounter = NULL;
  if (invert)
    {
    ptIds = vtkIdList::New();
    cellCounter = new char[numPts];
    for (i = 0; i < numPts; ++i)
      {
      cellCounter[i] = 0;
      }
    }
  vtkIdList *idList = vtkIdList::New();
  vtkIdType numIds = 0, ptId, cellId, idArrayIndex = 0, labelArrayIndex = 0;
  vtkAbstractArray* idArray = sel->GetSelectionList();
  if (idArray)
    {
    numIds = idArray->GetNumberOfTuples();
    vtkAbstractArray* sortedArray = 
      vtkAbstractArray::CreateArray(idArray->GetDataType());
    sortedArray->DeepCopy(idArray);
    vtkSortDataArray::SortArrayByComponent(sortedArray, 0);
    idArray = sortedArray;
    }

  if (idArray == NULL)
    {
    labelArray->Delete();
    idxArray->Delete();
    idList->Delete();
    if (ptIds)
      {
      ptIds->Delete();
      }
    if (cellCounter)
      {
      delete[] cellCounter;
      }
    return 1;
    }
  
  // Array types must match if they are string arrays.
  if (vtkStringArray::SafeDownCast(labelArray) &&
    vtkStringArray::SafeDownCast(idArray) == NULL)
    {
    labelArray->Delete();
    idxArray->Delete();
    idList->Delete();
    if (ptIds)
      {
      ptIds->Delete();
      }
    if (cellCounter)
      {
      delete[] cellCounter;
      }
    vtkWarningMacro(
      "Array types don't match. They must match for vtkStringArray.");
    return 0;
    }

  void *idVoid = idArray->GetVoidPointer(0);
  void *labelVoid = labelArray->GetVoidPointer(0);
  int idArrayType = idArray->GetDataType();
  int labelArrayType = labelArray->GetDataType();

  // Check each cell to see if it's selected
  while (labelArrayIndex < numCells)
    {
    // Advance through the selection ids until we find
    // one that's NOT LESS THAN the current cell label.
    bool idLessThanLabel = false;
    if (idArrayIndex < numIds)
      {
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idLessThanLabel = 
        (static_cast<VTK_TT1*>(idVoid)[idArrayIndex]) <
        static_cast<VTK_TT1>(
          (static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex])));
      }
    while ((idArrayIndex < numIds) && idLessThanLabel)
      {
      ++idArrayIndex;
      if (idArrayIndex >= numIds)
        {
        break;
        }
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idLessThanLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }

    if (idArrayIndex >= numIds)
      {
      // We're out of selection ids, so we're done.
      break;
      }
    this->UpdateProgress(static_cast<double>(idArrayIndex) / (numIds * (passThrough + 1)));

    // Advance through and mark all cells with a label EQUAL TO the
    // current selection id, as well as their points.
    bool idEqualToLabel = false;
    if (labelArrayIndex < numCells)
      {
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idEqualToLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] ==
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }
    while ((labelArrayIndex < numCells) && idEqualToLabel)
      {
      cellId = idxArray->GetValue(labelArrayIndex);
      cellInArray->SetValue(cellId, flag);
      input->GetCellPoints(cellId, idList);
      if (!invert)
        {
        for (i = 0; i < idList->GetNumberOfIds(); ++i)
          {
          pointInArray->SetValue(idList->GetId(i), flag);
          }
        }
      else
        {
        for (i = 0; i < idList->GetNumberOfIds(); ++i)
          {
          ptId = idList->GetId(i);
          ptIds->InsertUniqueId(ptId);
          cellCounter[ptId]++;
          }
        }
      ++labelArrayIndex;
      if (labelArrayIndex >= numCells)
        {
        break;
        }
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idEqualToLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] ==
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }
      

    // Advance through cell labels until we find
    // one that's NOT LESS THAN the current selection id.
    bool labelLessThanId = false;
    if (labelArrayIndex < numCells)
      {
      vtkESI_ExtendedTemplateMacro(
        labelArrayType, idArrayType,
        labelLessThanId = 
        static_cast<VTK_TT1*>(labelVoid)[labelArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(idVoid)[idArrayIndex]));
      }
    while ((labelArrayIndex < numCells) && labelLessThanId)
      {
      ++labelArrayIndex;
      if (labelArrayIndex >= numCells)
        {
        break;
        }
      vtkESI_ExtendedTemplateMacro(labelArrayType, idArrayType,
        labelLessThanId = 
        static_cast<VTK_TT1*>(labelVoid)[labelArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(idVoid)[idArrayIndex]));
      }
    }

  idArray->Delete();

  if (invert)
    {
    for (i = 0; i < ptIds->GetNumberOfIds(); ++i)
      {
      ptId = ptIds->GetId(i);
      input->GetPointCells(ptId, idList);
      if (cellCounter[ptId] == idList->GetNumberOfIds())
        {
        pointInArray->SetValue(ptId, flag);
        }
      }
  
    ptIds->Delete();
    delete [] cellCounter;
    }

  idList->Delete();
  idxArray->Delete();
  labelArray->Delete();

  if (!passThrough)
    {
    vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedIdsCopyPoints(input, output, 
      pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);    
    if (output->GetDataObjectType() == VTK_POLY_DATA)
      {
      vtkExtractSelectedIdsCopyCells<vtkPolyData>(input, 
        vtkPolyData::SafeDownCast(output), 
        cellInArray->GetPointer(0), pointMap);
      }
    else
      {
      vtkExtractSelectedIdsCopyCells<vtkUnstructuredGrid>(input, 
        vtkUnstructuredGrid::SafeDownCast(output), 
        cellInArray->GetPointer(0), pointMap);
      }
    delete [] pointMap;
    this->UpdateProgress(1.0);
    }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::ExtractPoints(
  vtkSelectionNode *sel,  vtkDataSet *input,
  vtkDataSet *output)
{
  int passThrough = 0;
  if (this->PreserveTopology)
    {
    passThrough = 1;
    }

  int containingCells = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::CONTAINING_CELLS()))
    {
    containingCells = sel->GetProperties()->Get(vtkSelectionNode::CONTAINING_CELLS());
    }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
    {
    invert = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
    }

  vtkIdType i, numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkSignedCharArray> pointInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i=0; i < numPts; i++)
    {
    pointInArray->SetValue(i, flag);
    }

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkSignedCharArray> cellInArray;
  if (containingCells)
    {
    cellInArray = vtkSmartPointer<vtkSignedCharArray>::New();
    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i=0; i < numCells; i++)
     {
     cellInArray->SetValue(i, flag);
     }
   }

 if (passThrough)
    {
    output->ShallowCopy(input);
    pointInArray->SetName("vtkInsidedness");
    vtkPointData *outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    if (containingCells)
      {
      cellInArray->SetName("vtkInsidedness");
      vtkCellData *outCD = output->GetCellData();
      outCD->AddArray(cellInArray);
      outCD->SetScalars(cellInArray);
      }
    }

  //decide what the IDS mean
  vtkAbstractArray *labelArray = NULL;
  int selType = sel->GetProperties()->Get(vtkSelectionNode::CONTENT_TYPE());
  if (selType == vtkSelectionNode::GLOBALIDS)
    {
    labelArray = vtkIdTypeArray::SafeDownCast(
      input->GetPointData()->GetGlobalIds());
    }
  else if (selType == vtkSelectionNode::PEDIGREEIDS)
    {
    labelArray = input->GetPointData()->GetPedigreeIds();
    }
  else if (selType == vtkSelectionNode::VALUES &&
           sel->GetSelectionList()->GetName())
    {
    //user chose a specific label array
    labelArray = input->GetPointData()->GetAbstractArray(
      sel->GetSelectionList()->GetName());
    }
  if (labelArray == NULL && selType != vtkSelectionNode::INDICES)
    {
    return 1;
    }
  
  vtkIdTypeArray *idxArray = vtkIdTypeArray::New();
  idxArray->SetNumberOfComponents(1);
  idxArray->SetNumberOfTuples(numPts);
  for (i=0; i < numPts; i++)
    {
    idxArray->SetValue(i, i);
    }

  if (labelArray)
    {
    int component_no = 0;
    if (sel->GetProperties()->Has(vtkSelectionNode::COMPONENT_NUMBER()))
      {
      component_no =
        sel->GetProperties()->Get(vtkSelectionNode::COMPONENT_NUMBER());
      if (component_no >= labelArray->GetNumberOfComponents())
        {
        component_no = 0;
        }
      }

    vtkAbstractArray* sortedArray = 
      vtkAbstractArray::CreateArray(labelArray->GetDataType());
    vtkESIDeepCopy(sortedArray, labelArray, component_no);
    vtkSortDataArray::Sort(sortedArray, idxArray);
    labelArray = sortedArray;
    }
  else
    {
    //no global array, so just use the input cell index
    labelArray = idxArray;
    labelArray->Register(NULL);
    }

  // Reverse the "in" flag
  flag = -flag;

  vtkIdList *ptCells = vtkIdList::New();
  vtkIdList *cellPts = vtkIdList::New();
  vtkIdType numIds = 0, ptId, cellId, idArrayIndex = 0, labelArrayIndex = 0;
  vtkAbstractArray* idArray = sel->GetSelectionList();
  if (idArray == NULL)
    {
    labelArray->Delete();
    idxArray->Delete();
    ptCells->Delete();
    cellPts->Delete();
    return 1;
    }

  // Array types must match if they are string arrays.
  if (vtkStringArray::SafeDownCast(labelArray) &&
    vtkStringArray::SafeDownCast(idArray) == NULL)
    {
    vtkWarningMacro(
      "Array types don't match. They must match for vtkStringArray.");
    labelArray->Delete();
    idxArray->Delete();
    ptCells->Delete();
    cellPts->Delete();
    return 0;
    }

  numIds = idArray->GetNumberOfTuples();
  vtkAbstractArray* sortedArray = 
    vtkAbstractArray::CreateArray(idArray->GetDataType());
  sortedArray->DeepCopy(idArray);
  vtkSortDataArray::SortArrayByComponent(sortedArray, 0);
  idArray = sortedArray;

  void *idVoid = idArray->GetVoidPointer(0);
  void *labelVoid = labelArray->GetVoidPointer(0);
  int idArrayType = idArray->GetDataType();
  int labelArrayType = labelArray->GetDataType();

  // Check each point to see if it's selected
  while (labelArrayIndex < numPts)
    {
    // Advance through the selection ids until we find
    // one that's NOT LESS THAN the current point label.
    bool idLessThanLabel = false;
    if (idArrayIndex < numIds)
      {
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idLessThanLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }
    while ((idArrayIndex < numIds) && idLessThanLabel)
      {
      ++idArrayIndex;
      if (idArrayIndex >= numIds)
        {
        break;
        }
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idLessThanLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }

    this->UpdateProgress(static_cast<double>(idArrayIndex) / (numIds * (passThrough + 1)));
    if (idArrayIndex >= numIds)
      {
      // We're out of selection ids, so we're done.
      break;
      }

    // Advance through and mark all points with a label EQUAL TO the
    // current selection id, as well as their cells.
    bool idEqualToLabel = false;
    if (labelArrayIndex < numPts)
      {
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idEqualToLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] ==
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }
    while ((labelArrayIndex < numPts) && idEqualToLabel)
      {
      ptId = idxArray->GetValue(labelArrayIndex);
      pointInArray->SetValue(ptId, flag);
      if (containingCells)
        {
        for (vtkIdType j = 0; j < input->GetNumberOfPoints(); j++)
          {
          input->GetPointCells(ptId, ptCells);
          for (i = 0; i < ptCells->GetNumberOfIds(); ++i)
            {
            cellId = ptCells->GetId(i);
            if (!passThrough && !invert && cellInArray->GetValue(cellId) != flag)
              {
              input->GetCellPoints(cellId, cellPts);
              for (j = 0; j < cellPts->GetNumberOfIds(); ++j)
                {
                pointInArray->SetValue(cellPts->GetId(j), flag);
                }
              }
            cellInArray->SetValue(cellId, flag);
            }
          }
        }
      ++labelArrayIndex;
      if (labelArrayIndex >= numPts)
        {
        break;
        }
      vtkESI_ExtendedTemplateMacro(
        idArrayType, labelArrayType,
        idEqualToLabel = 
        static_cast<VTK_TT1*>(idVoid)[idArrayIndex] ==
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(labelVoid)[labelArrayIndex]));
      }

    // Advance through point labels until we find
    // one that's NOT LESS THAN the current selection id.
    bool labelLessThanId = false;
    if (labelArrayIndex < numPts)
      {
      vtkESI_ExtendedTemplateMacro(
        labelArrayType, idArrayType,
        labelLessThanId = 
        static_cast<VTK_TT1*>(labelVoid)[labelArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(idVoid)[idArrayIndex]));
      }
    while ((labelArrayIndex < numPts) && labelLessThanId)
      {
      ++labelArrayIndex;
      if (labelArrayIndex >= numPts)
        {
        break;
        }
      vtkESI_ExtendedTemplateMacro(labelArrayType, idArrayType,
        labelLessThanId = 
        static_cast<VTK_TT1*>(labelVoid)[labelArrayIndex] <
        static_cast<VTK_TT1>(
          static_cast<VTK_TT2*>(idVoid)[idArrayIndex]));
      }
    }

  idArray->Delete();

  ptCells->Delete();
  cellPts->Delete();
  idxArray->Delete();
  labelArray->Delete();

  if (!passThrough)
    {
    vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedIdsCopyPoints(input, output, 
      pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (containingCells)
      {
      if (output->GetDataObjectType() == VTK_POLY_DATA)
        {
        vtkExtractSelectedIdsCopyCells<vtkPolyData>(input, 
          vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), 
          pointMap);
        }
      else
        {
        vtkExtractSelectedIdsCopyCells<vtkUnstructuredGrid>(input, 
          vtkUnstructuredGrid::SafeDownCast(output), 
          cellInArray->GetPointer(0), pointMap);
        }
      }
    else
      {
      numPts = output->GetNumberOfPoints();
      vtkUnstructuredGrid* outputUG = vtkUnstructuredGrid::SafeDownCast(output);
      outputUG->Allocate(numPts);
      for (i = 0; i < numPts; ++i)
        {
        outputUG->InsertNextCell(VTK_VERTEX, 1, &i);
        }
      }
      this->UpdateProgress(1.0);
      delete [] pointMap;
    }
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

