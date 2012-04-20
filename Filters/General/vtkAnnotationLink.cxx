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

vtkStandardNewMacro(vtkAnnotationLink);
//vtkCxxSetObjectMacro(vtkAnnotationLink, AnnotationLayers, vtkAnnotationLayers);


//---------------------------------------------------------------------------
// vtkAnnotationLink::Command
//----------------------------------------------------------------------------

class vtkAnnotationLink::Command : public vtkCommand
{
public:
  static Command* New() {  return new Command(); }
  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ProcessEvents(caller, eventId, callData);
      }
    }
  void SetTarget(vtkAnnotationLink* t)
    {
    this->Target = t;
    }
private:
  Command() { this->Target = 0; }
  vtkAnnotationLink* Target;
};

//----------------------------------------------------------------------------
vtkAnnotationLink::vtkAnnotationLink()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(3);
  this->AnnotationLayers = vtkAnnotationLayers::New();
  this->DomainMaps = vtkDataObjectCollection::New();

  this->Observer = Command::New();
  this->Observer->SetTarget(this);
  this->AnnotationLayers->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
}

//----------------------------------------------------------------------------
vtkAnnotationLink::~vtkAnnotationLink()
{
  this->Observer->Delete();

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
void vtkAnnotationLink::ProcessEvents(vtkObject *caller, unsigned long eventId, void *vtkNotUsed(callData))
{
  if(this->AnnotationLayers)
    {
    vtkAnnotationLayers* caller_annotations = vtkAnnotationLayers::SafeDownCast( caller );
    if (caller_annotations == this->AnnotationLayers && eventId == vtkCommand::ModifiedEvent)
      {
      this->InvokeEvent(vtkCommand::AnnotationChangedEvent, this->AnnotationLayers);
      }
    }
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::SetAnnotationLayers(vtkAnnotationLayers* layers)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except that we listen for modified events from the annotations layers
  if (layers != this->AnnotationLayers)
    {
    vtkAnnotationLayers *tmp = this->AnnotationLayers;
    if (tmp)
      {
      tmp->RemoveObserver(this->Observer);
      }
    this->AnnotationLayers = layers;
    if (this->AnnotationLayers != NULL)
      {
      this->AnnotationLayers->Register(this);
      this->AnnotationLayers->AddObserver(vtkCommand::ModifiedEvent,
                                        this->Observer);
      }
    if (tmp != NULL)
      {
      tmp->UnRegister(this);
      }
    this->Modified();
    this->InvokeEvent(vtkCommand::AnnotationChangedEvent, this->AnnotationLayers);
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
  if(this->DomainMaps->GetNumberOfItems() > 0)
    {
    this->DomainMaps->RemoveAllItems();
    }
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
  vtkInformationVector **inVector,
  vtkInformationVector *outVector)
{
  vtkInformation *inInfo = inVector[0]->GetInformationObject(0);
  vtkTable* inputMap = vtkTable::GetData(inVector[1]);
  vtkAnnotationLayers* input = 0;
  vtkSelection* inputSelection = 0;
  if (inInfo)
    {
    input = vtkAnnotationLayers::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    inputSelection = vtkSelection::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }

  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkAnnotationLayers* output = vtkAnnotationLayers::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *mapInfo = outVector->GetInformationObject(1);
  vtkMultiBlockDataSet* maps = vtkMultiBlockDataSet::SafeDownCast(
    mapInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *selInfo = outVector->GetInformationObject(2);
  vtkSelection* sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Give preference to input annotations
  if (input)
    {
    this->ShallowCopyToOutput(input, output, sel);
    }
  else if (this->AnnotationLayers)
    {
    this->ShallowCopyToOutput(this->AnnotationLayers, output, sel);
    }

  // If there is an input selection, set it on the annotation layers
  if (inputSelection)
    {
    sel->ShallowCopy(inputSelection);
    output->SetCurrentSelection(sel);
    }

  // If there are input domain maps, give preference to them
  if(inputMap)
    {
    vtkSmartPointer<vtkTable> outMap = vtkSmartPointer<vtkTable>::New();
    outMap->ShallowCopy(inputMap);
    maps->SetBlock(0, outMap);
    }
  else
    {
    unsigned int numMaps = static_cast<unsigned int>(this->DomainMaps->GetNumberOfItems());
    maps->SetNumberOfBlocks(numMaps);
    for (unsigned int i = 0; i < numMaps; ++i)
      {
      vtkSmartPointer<vtkTable> map = vtkSmartPointer<vtkTable>::New();
      map->ShallowCopy(this->DomainMaps->GetItem(i));
      maps->SetBlock(i, map);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::ShallowCopyToOutput(
  vtkAnnotationLayers* input,
  vtkAnnotationLayers* output,
  vtkSelection* sel)
{
  output->ShallowCopy(input);

  if (input->GetCurrentSelection())
    {
    sel->ShallowCopy(input->GetCurrentSelection());
    }
}

//----------------------------------------------------------------------------
int vtkAnnotationLink::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAnnotationLayers");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    //info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkAnnotationLink::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkAnnotationLayers");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    return 1;
    }
  else if (port == 2)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    return 1;
    }
  return 0;
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

