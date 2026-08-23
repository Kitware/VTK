// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisDataRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkScalarsToColors.h"
#include "vtkScivisView.h"
#include "vtkSelection.h"
#include "vtkTrivialProducer.h"

#include <map>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
class vtkScivisDataRepresentation::Internals
{
public:
  // Shallow copies of the inputs, so that internal filters can be connected to
  // an input without holding this representation's own pipeline open. Keyed by
  // (port, connection); the input port pointer is compared, never dereferenced.
  std::map<std::pair<int, int>, std::pair<vtkAlgorithmOutput*, vtkSmartPointer<vtkTrivialProducer>>>
    InputInternal;
};

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkScivisDataRepresentation::GetInternalOutputPort(int port, int conn)
{
  if (port >= this->GetNumberOfInputPorts() || conn >= this->GetNumberOfInputConnections(port))
  {
    vtkErrorMacro(
      "Port " << port << ", connection " << conn << " is not defined on this representation.");
    return nullptr;
  }

  const std::pair<int, int> key(port, conn);
  vtkAlgorithmOutput* input = this->GetInputConnection(port, conn);
  vtkDataObject* inputData = this->GetInputDataObject(port, conn);
  if (!inputData)
  {
    vtkErrorMacro("Port " << port << ", connection " << conn << " has produced no data yet.");
    return nullptr;
  }

  auto& cached = this->Implementation->InputInternal[key];
  if (!cached.second)
  {
    cached.second = vtkSmartPointer<vtkTrivialProducer>::New();
  }

  // The copy is stale when the input connection changed, or when the input has
  // been modified since the copy was taken.
  if (cached.first != input || cached.second->GetMTime() < inputData->GetMTime())
  {
    cached.first = input;
    auto copy = vtkSmartPointer<vtkDataObject>::Take(inputData->NewInstance());
    copy->ShallowCopy(inputData);
    cached.second->SetOutput(copy);
  }

  vtkTrivialProducer* producer = cached.second;
  producer->GetOutputPortInformation(0)->Set(
    vtkDataObject::DATA_TYPE_NAME(), producer->GetOutputDataObject(0)->GetClassName());
  return producer->GetOutputPort();
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::Select(vtkScivisView* view, vtkSelection* selection, bool extend)
{
  if (!this->Selectable)
  {
    return;
  }
  vtkSelection* converted = this->ConvertSelection(view, selection);
  if (converted)
  {
    this->UpdateSelection(converted, extend);
    if (converted != selection)
    {
      converted->Delete();
    }
  }
}

//------------------------------------------------------------------------------
vtkSelection* vtkScivisDataRepresentation::ConvertSelection(
  vtkScivisView* vtkNotUsed(view), vtkSelection* selection)
{
  return selection;
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::UpdateSelection(vtkSelection* selection, bool extend)
{
  if (extend && this->CurrentSelection)
  {
    selection->Union(this->CurrentSelection);
  }
  this->CurrentSelection = selection;
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(selection));
}

//------------------------------------------------------------------------------
vtkSelection* vtkScivisDataRepresentation::GetCurrentSelection()
{
  return this->CurrentSelection;
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::AddClippingPlane(vtkPlane* plane)
{
  if (!plane || this->ClippingPlanes->IsItemPresent(plane))
  {
    return;
  }
  this->ClippingPlanes->AddItem(plane);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::RemoveClippingPlane(vtkPlane* plane)
{
  if (!plane || !this->ClippingPlanes->IsItemPresent(plane))
  {
    return;
  }
  this->ClippingPlanes->RemoveItem(plane);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::RemoveAllClippingPlanes()
{
  if (this->ClippingPlanes->GetNumberOfItems() == 0)
  {
    return;
  }
  this->ClippingPlanes->RemoveAllItems();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::SetClippingPlanes(vtkPlaneCollection* planes)
{
  // Copy rather than adopt: a subclass has already handed this collection to
  // its mapper, and replacing it here would leave the mapper following the old
  // one.
  this->ClippingPlanes->RemoveAllItems();
  if (planes)
  {
    planes->InitTraversal();
    while (vtkPlane* plane = planes->GetNextItem())
    {
      this->ClippingPlanes->AddItem(plane);
    }
  }
  this->Modified();
}

//------------------------------------------------------------------------------
vtkPlaneCollection* vtkScivisDataRepresentation::GetClippingPlanes()
{
  return this->ClippingPlanes;
}

//------------------------------------------------------------------------------
int vtkScivisDataRepresentation::GetNumberOfClippingPlanes()
{
  return this->ClippingPlanes->GetNumberOfItems();
}

//------------------------------------------------------------------------------
vtkScivisDataRepresentation::vtkScivisDataRepresentation()
{
  this->Implementation = new Internals();
  this->Selectable = true;
}

//------------------------------------------------------------------------------
vtkScivisDataRepresentation::~vtkScivisDataRepresentation()
{
  delete this->Implementation;
}

//------------------------------------------------------------------------------
void vtkScivisDataRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Selectable: " << this->Selectable << "\n";
  os << indent << "CurrentSelection: " << this->CurrentSelection << "\n";
  os << indent << "NumberOfClippingPlanes: " << this->GetNumberOfClippingPlanes() << "\n";
}

VTK_ABI_NAMESPACE_END
