/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApplyIcons.cxx

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
#include "vtkApplyIcons.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <vtksys/stl/map>

vtkStandardNewMacro(vtkApplyIcons);

class vtkApplyIcons::Internals {
  public:
    vtksys_stl::map<vtkVariant, int> LookupTable;
};

vtkApplyIcons::vtkApplyIcons()
{
  this->Implementation = new Internals();
  this->DefaultIcon = -1;
  this->SelectedIcon = 0;
  this->SetNumberOfInputPorts(2);
  this->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_VERTICES,
    vtkDataSetAttributes::SCALARS);
  this->UseLookupTable = false;
  this->IconOutputArrayName = 0;
  this->SetIconOutputArrayName("vtkApplyIcons icon");
  this->SelectionMode = IGNORE_SELECTION;
  this->AttributeType = vtkDataObject::VERTEX;
}

vtkApplyIcons::~vtkApplyIcons()
{
  delete Implementation;
  this->SetIconOutputArrayName(0);
}

void vtkApplyIcons::SetIconType(vtkVariant v, int icon)
{
  this->Implementation->LookupTable[v] = icon;
}

void vtkApplyIcons::ClearAllIconTypes()
{
  this->Implementation->LookupTable.clear();
}

int vtkApplyIcons::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAnnotationLayers");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

int vtkApplyIcons::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Get the info objects.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* layersInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->IconOutputArrayName)
    {
    vtkErrorMacro("Output array name must be valid");
    return 0;
    }

  // Get the input and output.
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkAnnotationLayers* layers = 0;
  if (layersInfo)
    {
    layers = vtkAnnotationLayers::SafeDownCast(
      layersInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  // Initialize icon array.
  vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  vtkSmartPointer<vtkIntArray> iconArr =
    vtkSmartPointer<vtkIntArray>::New();
  iconArr->SetName(this->IconOutputArrayName);

  // If we have an input array, use its attribute type, otherwise use the
  // AttributeType ivar.
  int attribType = this->AttributeType;
  if (arr)
    {
    attribType = output->GetAttributeTypeForArray(arr);
    }

  // Error if the attribute type is not defined on the data.
  if (!output->GetAttributes(attribType))
    {
    vtkErrorMacro("The input array is not found, and the AttributeType parameter is not valid for this data object.");
    return 1;
    }

  // Size the array and add it to the correct attributes.
  vtkIdType numTuples = input->GetNumberOfElements(attribType);
  iconArr->SetNumberOfTuples(numTuples);
  output->GetAttributes(attribType)->AddArray(iconArr);

  // Process the icon array.
  if (this->UseLookupTable && arr)
    {
    // Map the data values through the lookup table.
    vtksys_stl::map<vtkVariant, int>::iterator itEnd = this->Implementation->LookupTable.end();
    for (vtkIdType i = 0; i < iconArr->GetNumberOfTuples(); ++i)
      {
      vtkVariant val = arr->GetVariantValue(i);
      int mappedIcon = this->DefaultIcon;
      if (this->Implementation->LookupTable.find(val) != itEnd)
        {
        mappedIcon = this->Implementation->LookupTable[val];
        }
      iconArr->SetValue(i, mappedIcon);
      }
    }
  else if (arr)
    {
    // If no lookup table, pass the input array values.
    for (vtkIdType i = 0; i < iconArr->GetNumberOfTuples(); ++i)
      {
      iconArr->SetValue(i, arr->GetVariantValue(i).ToInt());
      }
    }
  else
    {
    // If no lookup table or array, use default icon.
    for (vtkIdType i = 0; i < iconArr->GetNumberOfTuples(); ++i)
      {
      iconArr->SetValue(i, this->DefaultIcon);
      }
    }

  // Convert to a selection attribute type.
  int attribTypeSel = -1;
  switch (attribType)
    {
    case vtkDataObject::POINT:
      attribTypeSel = vtkSelectionNode::POINT;
      break;
    case vtkDataObject::CELL:
      attribTypeSel = vtkSelectionNode::CELL;
      break;
    case vtkDataObject::VERTEX:
      attribTypeSel = vtkSelectionNode::VERTEX;
      break;
    case vtkDataObject::EDGE:
      attribTypeSel = vtkSelectionNode::EDGE;
      break;
    case vtkDataObject::ROW:
      attribTypeSel = vtkSelectionNode::ROW;
      break;
    case vtkDataObject::FIELD:
      attribTypeSel = vtkSelectionNode::FIELD;
      break;
    }

  if (layers)
    {
    // Set annotated icons.
    vtkSmartPointer<vtkIdTypeArray> list1 =
      vtkSmartPointer<vtkIdTypeArray>::New();
    unsigned int numAnnotations = layers->GetNumberOfAnnotations();
    for (unsigned int a = 0; a < numAnnotations; ++a)
      {
      vtkAnnotation* ann = layers->GetAnnotation(a);
      if (ann->GetInformation()->Has(vtkAnnotation::ENABLE()) && 
          ann->GetInformation()->Get(vtkAnnotation::ENABLE())==0)
        {
        continue;
        }
      list1->Initialize();
      vtkSelection* sel = ann->GetSelection();
      int curIcon = -1;
      if (ann->GetInformation()->Has(vtkAnnotation::ICON_INDEX()))
        {
        curIcon = ann->GetInformation()->Get(vtkAnnotation::ICON_INDEX());
        }
      else
        {
        continue;
        }
      vtkConvertSelection::GetSelectedItems(sel, input, attribTypeSel, list1);
      vtkIdType numIds = list1->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numIds; ++i)
        {
        if (list1->GetValue(i) >= iconArr->GetNumberOfTuples())
          {
          continue;
          }
        iconArr->SetValue(list1->GetValue(i), curIcon);
        }
      }

    // Set selected icons.
    if (vtkAnnotation* ann = layers->GetCurrentAnnotation())
      {
      vtkSelection* selection = ann->GetSelection();
      list1 = vtkSmartPointer<vtkIdTypeArray>::New();
      int selectedIcon = -1;
      bool changeSelected = false;
      switch (this->SelectionMode)
        {
        case SELECTED_ICON:
        case SELECTED_OFFSET:
          selectedIcon = this->SelectedIcon;
          changeSelected = true;
          break;
        case ANNOTATION_ICON:
          if (ann->GetInformation()->Has(vtkAnnotation::ICON_INDEX()))
            {
            selectedIcon = ann->GetInformation()->Get(vtkAnnotation::ICON_INDEX());
            changeSelected = true;
            }
          break;
        }
      if (changeSelected)
        {
        vtkConvertSelection::GetSelectedItems(selection, input, attribTypeSel, list1);
        vtkIdType numIds = list1->GetNumberOfTuples();
        for (vtkIdType i = 0; i < numIds; ++i)
          {
          if (list1->GetValue(i) >= iconArr->GetNumberOfTuples())
            {
            continue;
            }
          if (this->SelectionMode == SELECTED_OFFSET)
            {
            // Use selected icon as an offset into the icon sheet.
            selectedIcon = iconArr->GetValue(list1->GetValue(i)) + this->SelectedIcon;
            }
          iconArr->SetValue(list1->GetValue(i), selectedIcon);
          }
        }
      } // if changeSelected
    } // if current ann not NULL

  return 1;
}

void vtkApplyIcons::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DefaultIcon: " << this->DefaultIcon << endl;
  os << indent << "SelectedIcon: " << this->SelectedIcon << endl;
  os << indent << "UseLookupTable: "
    << (this->UseLookupTable ? "on" : "off") << endl;
  os << indent << "IconOutputArrayName: "
    << (this->IconOutputArrayName ? this->IconOutputArrayName : "(none)") << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}

