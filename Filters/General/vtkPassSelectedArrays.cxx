/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassSelectedArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPassSelectedArrays.h"

#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPassSelectedArrays);
//----------------------------------------------------------------------------
vtkPassSelectedArrays::vtkPassSelectedArrays()
  : Enabled(true)
{
  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ASSOCIATIONS; ++cc)
  {
    if (cc != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      this->ArraySelections[cc] = vtkSmartPointer<vtkDataArraySelection>::New();
      this->ArraySelections[cc]->AddObserver(
        vtkCommand::ModifiedEvent, this, &vtkPassSelectedArrays::Modified);
    }
    else
    {
      this->ArraySelections[cc] = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
vtkPassSelectedArrays::~vtkPassSelectedArrays() {}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkPassSelectedArrays::GetArraySelection(int association)
{
  if (association >= 0 && association < vtkDataObject::NUMBER_OF_ASSOCIATIONS)
  {
    return this->ArraySelections[association];
  }

  return nullptr;
}

//----------------------------------------------------------------------------
int vtkPassSelectedArrays::FillInputPortInformation(int, vtkInformation* info)
{
  // Skip composite data sets so that executives will treat this as a simple filter
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPassSelectedArrays::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkDataObject::GetData(outputVector, 0);
  output->ShallowCopy(input);

  if (!this->Enabled)
  {
    return 1;
  }

  // now filter arrays for each of the associations.
  for (int association = 0; association < vtkDataObject::NUMBER_OF_ASSOCIATIONS; ++association)
  {
    if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      continue;
    }

    auto inFD = input->GetAttributesAsFieldData(association);
    auto outFD = output->GetAttributesAsFieldData(association);
    auto selection = this->GetArraySelection(association);
    if (!inFD || !outFD || !selection)
    {
      continue;
    }

    auto inDSA = vtkDataSetAttributes::SafeDownCast(inFD);
    auto outDSA = vtkDataSetAttributes::SafeDownCast(outFD);

    outFD->Initialize();
    for (int idx = 0, max = inFD->GetNumberOfArrays(); idx < max; ++idx)
    {
      auto inarray = inFD->GetAbstractArray(idx);
      if (inarray && inarray->GetName())
      {
        if (selection->ArrayIsEnabled(inarray->GetName()) ||
          /** ghost array is passed unless explicitly disabled **/
          (strcmp(inarray->GetName(), vtkDataSetAttributes::GhostArrayName()) == 0 &&
            selection->ArrayExists(vtkDataSetAttributes::GhostArrayName()) == 0))
        {
          outFD->AddArray(inarray);

          // preserve attribute type flags.
          for (int attr = 0; inDSA && outDSA && (attr < vtkDataSetAttributes::NUM_ATTRIBUTES);
               ++attr)
          {
            if (inDSA->GetAbstractAttribute(attr) == inarray)
            {
              outDSA->SetAttribute(inarray, attr);
            }
          }
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPassSelectedArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "PointDataArraySelection: " << endl;
  this->GetPointDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CellDataArraySelection: " << endl;
  this->GetCellDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FieldDataArraySelection: " << endl;
  this->GetFieldDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexDataArraySelection: " << endl;
  this->GetVertexDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeDataArraySelection: " << endl;
  this->GetEdgeDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RowDataArraySelection: " << endl;
  this->GetRowDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
}
