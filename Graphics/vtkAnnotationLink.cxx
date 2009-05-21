/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotationLink.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAnnotationLink.h"

#include "vtkCommand.h"
#include "vtkDataObjectCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkAnnotationLayers.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkAnnotationLink, "1.4");
vtkStandardNewMacro(vtkAnnotationLink);
vtkCxxSetObjectMacro(vtkAnnotationLink, AnnotationLayers, vtkAnnotationLayers);
//----------------------------------------------------------------------------
vtkAnnotationLink::vtkAnnotationLink()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(3);
  this->AnnotationLayers = vtkAnnotationLayers::New();
  this->DomainMaps = vtkDataObjectCollection::New();  
}

//----------------------------------------------------------------------------
vtkAnnotationLink::~vtkAnnotationLink()
{
  if (this->AnnotationLayers)
    {
    this->AnnotationLayers->Delete();
    }
  if (this->DomainMaps)
    {
    this->DomainMaps->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::AddDomainMap(vtkTable* map)
{
  if (!this->DomainMaps->IsItemPresent(map))
    {
    this->DomainMaps->AddItem(map);
    }
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::RemoveDomainMap(vtkTable* map)
{
  this->DomainMaps->RemoveItem(map);
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::RemoveAllDomainMaps()
{
  this->DomainMaps->RemoveAllItems();
}

//----------------------------------------------------------------------------
int vtkAnnotationLink::GetNumberOfDomainMaps()
{
  return this->DomainMaps->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkTable* vtkAnnotationLink::GetDomainMap(int i)
{
  return vtkTable::SafeDownCast(this->DomainMaps->GetItem(i));
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::SetCurrentSelection(vtkSelection* sel)
{
  if (this->AnnotationLayers)
    {
    this->AnnotationLayers->SetCurrentSelection(sel);
    }
}

//----------------------------------------------------------------------------
vtkSelection* vtkAnnotationLink::GetCurrentSelection()
{
  if (this->AnnotationLayers)
    {
    return this->AnnotationLayers->GetCurrentSelection();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkAnnotationLink::RequestData(
  vtkInformation *vtkNotUsed(info),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkAnnotationLayers* output = vtkAnnotationLayers::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkInformation *mapInfo = outVector->GetInformationObject(1);
  vtkMultiBlockDataSet* maps = vtkMultiBlockDataSet::SafeDownCast(
    mapInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkInformation *selInfo = outVector->GetInformationObject(2);
  vtkSelection* sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (this->AnnotationLayers)
    {
    output->ShallowCopy(this->AnnotationLayers);

    if (this->AnnotationLayers->GetCurrentSelection())
      {
      sel->ShallowCopy(this->AnnotationLayers->GetCurrentSelection());
      }
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
int vtkAnnotationLink::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkAnnotationLayers");
    }
  else if (port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    }
  else
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    }
  return 1;
}

//----------------------------------------------------------------------------
unsigned long vtkAnnotationLink::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  if (this->AnnotationLayers)
    {
    unsigned long atime = this->AnnotationLayers->GetMTime();
    if (atime > mtime)
      {
      mtime = atime;
      }
    }
  if (this->DomainMaps)
    {
    unsigned long dtime = this->DomainMaps->GetMTime();
    if (dtime > mtime)
      {
      mtime = dtime;
      }
    }
  return mtime;
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AnnotationLayers: ";
  if (this->AnnotationLayers)
    {
    os << "\n";
    this->AnnotationLayers->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "DomainMaps: ";
  if (this->DomainMaps)
    {
    os << "\n";
    this->DomainMaps->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}

