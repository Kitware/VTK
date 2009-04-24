/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionLink.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSelectionLink.h"

#include "vtkCommand.h"
#include "vtkDataObjectCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkSelectionLink, "1.5");
vtkStandardNewMacro(vtkSelectionLink);
//----------------------------------------------------------------------------
vtkSelectionLink::vtkSelectionLink()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
  this->DomainMaps = vtkDataObjectCollection::New();
  
  // Start with an empty index selection
  this->Selection = vtkSelection::New();
  vtkSmartPointer<vtkSelectionNode> node =
    vtkSmartPointer<vtkSelectionNode>::New();
  node->SetContentType(vtkSelectionNode::INDICES);
  vtkSmartPointer<vtkIdTypeArray> ids =
    vtkSmartPointer<vtkIdTypeArray>::New();
  node->SetSelectionList(ids);
  this->Selection->AddNode(node);
}

//----------------------------------------------------------------------------
vtkSelectionLink::~vtkSelectionLink()
{
  if (this->Selection)
    {
    this->Selection->Delete();
    }
  this->DomainMaps->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionLink::SetSelection(vtkSelection* selection)
{
  if (!selection)
    {
    vtkErrorMacro("Cannot set a null selection.");
    return;
    }
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Selection to " << selection );
  if (this->Selection != selection)
    {
    vtkSelection* tempSGMacroVar = this->Selection;
    this->Selection = selection;
    if (this->Selection != NULL) { this->Selection->Register(this); }
    if (tempSGMacroVar != NULL)
      {
      tempSGMacroVar->UnRegister(this);
      }
    this->Modified();
    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    }
}

//----------------------------------------------------------------------------
void vtkSelectionLink::AddDomainMap(vtkTable* map)
{
  if (!this->DomainMaps->IsItemPresent(map))
    {
    this->DomainMaps->AddItem(map);
    }
}

//----------------------------------------------------------------------------
void vtkSelectionLink::RemoveDomainMap(vtkTable* map)
{
  this->DomainMaps->RemoveItem(map);
}

//----------------------------------------------------------------------------
void vtkSelectionLink::RemoveAllDomainMaps()
{
  this->DomainMaps->RemoveAllItems();
}

//----------------------------------------------------------------------------
int vtkSelectionLink::GetNumberOfDomainMaps()
{
  return this->DomainMaps->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkTable* vtkSelectionLink::GetDomainMap(int i)
{
  return vtkTable::SafeDownCast(this->DomainMaps->GetItem(i));
}

//----------------------------------------------------------------------------
int vtkSelectionLink::RequestData(
  vtkInformation *vtkNotUsed(info),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  this->InvokeEvent(vtkCommand::StartEvent);
  
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkSelection* output = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation *mapInfo = outVector->GetInformationObject(1);
  vtkMultiBlockDataSet* maps = vtkMultiBlockDataSet::SafeDownCast(
    mapInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (this->Selection)
    {
    output->ShallowCopy(this->Selection);
    }

  unsigned int numMaps = static_cast<unsigned int>(this->DomainMaps->GetNumberOfItems());
  maps->SetNumberOfBlocks(numMaps);
  for (unsigned int i = 0; i < numMaps; ++i)
    {
    vtkSmartPointer<vtkTable> map = vtkSmartPointer<vtkTable>::New();
    map->ShallowCopy(this->DomainMaps->GetItem(i));
    maps->SetBlock(i, map);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectionLink::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    }
  else
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSelectionLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Selection: " << (this->Selection ? "" : "null") << endl;
  if (this->Selection)
    {
    this->Selection->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "DomainMaps: " << (this->DomainMaps ? "" : "null") << endl;
  if (this->DomainMaps)
    {
    this->DomainMaps->PrintSelf(os, indent.GetNextIndent());
    }
}

