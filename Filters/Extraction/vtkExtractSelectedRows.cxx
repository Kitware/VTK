/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedRows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkExtractSelectedRows.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkEdgeListIterator.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"
#include "vtkVertexListIterator.h"

#include <map>
#include <vector>

vtkStandardNewMacro(vtkExtractSelectedRows);
//----------------------------------------------------------------------------
vtkExtractSelectedRows::vtkExtractSelectedRows()
{
  this->AddOriginalRowIdsArray = false;
  this->SetNumberOfInputPorts(3);
}

//----------------------------------------------------------------------------
vtkExtractSelectedRows::~vtkExtractSelectedRows() = default;

//----------------------------------------------------------------------------
int vtkExtractSelectedRows::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
  }
  else if (port == 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAnnotationLayers");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedRows::SetSelectionConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//----------------------------------------------------------------------------
void vtkExtractSelectedRows::SetAnnotationLayersConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(2, in);
}

namespace
{
struct vtkCopySelectedRows
{
  template <class ArrayT>
  void operator()(ArrayT* list, vtkTable* input, vtkTable* output, vtkIdTypeArray* originalRowIds,
    bool addOriginalRowIdsArray) const
  {
    for (auto value : vtk::DataArrayValueRange(list))
    {
      vtkIdType val = static_cast<vtkIdType>(value);
      output->InsertNextRow(input->GetRow(val));
      if (addOriginalRowIdsArray)
      {
        originalRowIds->InsertNextValue(val);
      }
    }
  }
};
} // namespace

//----------------------------------------------------------------------------
int vtkExtractSelectedRows::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* input = vtkTable::GetData(inputVector[0]);
  vtkSelection* inputSelection = vtkSelection::GetData(inputVector[1]);
  vtkAnnotationLayers* inputAnnotations = vtkAnnotationLayers::GetData(inputVector[2]);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!inputSelection && !inputAnnotations)
  {
    vtkErrorMacro("No vtkSelection or vtkAnnotationLayers provided as input.");
    return 0;
  }

  vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();
  int numSelections = 0;
  if (inputSelection)
  {
    selection->DeepCopy(inputSelection);
    numSelections++;
  }

  // If input annotations are provided, extract their selections only if
  // they are enabled and not hidden.
  if (inputAnnotations)
  {
    for (unsigned int i = 0; i < inputAnnotations->GetNumberOfAnnotations(); ++i)
    {
      vtkAnnotation* a = inputAnnotations->GetAnnotation(i);
      if ((a->GetInformation()->Has(vtkAnnotation::ENABLE()) &&
            a->GetInformation()->Get(vtkAnnotation::ENABLE()) == 0) ||
        (a->GetInformation()->Has(vtkAnnotation::ENABLE()) &&
          a->GetInformation()->Get(vtkAnnotation::ENABLE()) == 1 &&
          a->GetInformation()->Has(vtkAnnotation::HIDE()) &&
          a->GetInformation()->Get(vtkAnnotation::HIDE()) == 1))
      {
        continue;
      }
      selection->Union(a->GetSelection());
      numSelections++;
    }
  }

  // Handle case where there was no input selection and no enabled, non-hidden
  // annotations
  if (numSelections == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Convert the selection to an INDICES selection
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    selection, input, vtkSelectionNode::INDICES, nullptr, vtkSelectionNode::ROW));
  if (!converted)
  {
    vtkErrorMacro("Selection conversion to INDICES failed.");
    return 0;
  }

  vtkIdTypeArray* originalRowIds = vtkIdTypeArray::New();
  originalRowIds->SetName("vtkOriginalRowIds");

  output->GetRowData()->CopyStructure(input->GetRowData());

  for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* node = converted->GetNode(i);
    if (node->GetFieldType() == vtkSelectionNode::ROW)
    {
      vtkDataArray* list = vtkDataArray::SafeDownCast(node->GetSelectionList());
      if (list)
      {
        int inverse = node->GetProperties()->Get(vtkSelectionNode::INVERSE());
        if (inverse)
        {
          vtkIdType numRows = input->GetNumberOfRows(); // How many rows are in the whole dataset
          for (vtkIdType j = 0; j < numRows; ++j)
          {
            if (list->LookupValue(j) < 0)
            {
              output->InsertNextRow(input->GetRow(j));
              if (this->AddOriginalRowIdsArray)
              {
                originalRowIds->InsertNextValue(j);
              }
            }
          }
        }
        else
        {
          if (list->GetNumberOfComponents() != 1)
          {
            vtkGenericWarningMacro("NumberOfComponents expected to be 1.");
          }

          using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>;
          if (!Dispatcher::Execute(list, vtkCopySelectedRows{}, input, output, originalRowIds,
                this->AddOriginalRowIdsArray))
          { // fallback for unsupported array types and non-integral value types:
            vtkCopySelectedRows{}(
              list, input, output, originalRowIds, this->AddOriginalRowIdsArray);
          }
        }
      }
    }
  }
  if (this->AddOriginalRowIdsArray)
  {
    output->AddColumn(originalRowIds);
  }
  originalRowIds->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedRows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AddOriginalRowIdsArray: " << this->AddOriginalRowIdsArray << endl;
}
