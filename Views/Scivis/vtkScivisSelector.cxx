// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisSelector.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkHardwareSelector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkScivisDataRepresentation.h"
#include "vtkScivisView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkScivisSelector);

//------------------------------------------------------------------------------
vtkScivisSelector::vtkScivisSelector()
{
  this->Mode = SURFACE;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
}

//------------------------------------------------------------------------------
vtkScivisSelector::~vtkScivisSelector() = default;

//------------------------------------------------------------------------------
void vtkScivisSelector::SetView(vtkScivisView* view)
{
  if (this->View == view)
  {
    return;
  }
  this->View = view;
  // The hardware selector picks in a renderer, and the renderer is the view's.
  this->HardwareSelector->SetRenderer(view ? view->GetRenderer() : nullptr);
  this->HardwareSelector->SetFieldAssociation(this->FieldAssociation);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkScivisView* vtkScivisSelector::GetView()
{
  return this->View;
}

//------------------------------------------------------------------------------
void vtkScivisSelector::SetMode(int mode)
{
  if (this->Mode == mode)
  {
    return;
  }
  if (mode != SURFACE && mode != FRUSTUM)
  {
    vtkWarningMacro("Unknown selection mode: " << mode);
    return;
  }
  this->Mode = mode;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkScivisSelector::GetMode()
{
  return this->Mode;
}

//------------------------------------------------------------------------------
void vtkScivisSelector::SetFieldAssociation(int assoc)
{
  if (this->FieldAssociation == assoc)
  {
    return;
  }
  this->FieldAssociation = assoc;
  this->HardwareSelector->SetFieldAssociation(assoc);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkScivisSelector::GetFieldAssociation()
{
  return this->FieldAssociation;
}

//------------------------------------------------------------------------------
void vtkScivisSelector::SelectCells()
{
  this->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

//------------------------------------------------------------------------------
void vtkScivisSelector::SelectPoints()
{
  this->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
}

//------------------------------------------------------------------------------
vtkSelection* vtkScivisSelector::GetCurrentSelection()
{
  return this->CurrentSelection;
}

//------------------------------------------------------------------------------
void vtkScivisSelector::SelectRegion(int x0, int y0, int x1, int y1, bool extend)
{
  if (!this->View)
  {
    return;
  }

  // Normalize and clamp in signed arithmetic.  The interactor reports positions
  // outside the window as negative numbers, and those must not be allowed to
  // reach the unsigned display coordinates the selectors work in.
  int minX = std::min(x0, x1);
  int maxX = std::max(x0, x1);
  int minY = std::min(y0, y1);
  int maxY = std::max(y0, y1);

  // A click selects nothing at all unless the region is given some area.
  const int stretch = 2;
  if (minX == maxX && minY == maxY)
  {
    minX -= stretch;
    minY -= stretch;
    maxX += stretch;
    maxY += stretch;
  }

  vtkRenderer* renderer = this->View->GetRenderer();
  const int* size = renderer->GetSize();
  const int limitX = size[0] > 0 ? size[0] - 1 : 0;
  const int limitY = size[1] > 0 ? size[1] - 1 : 0;
  const int region[4] = { std::clamp(minX, 0, limitX), std::clamp(minY, 0, limitY),
    std::clamp(maxX, 0, limitX), std::clamp(maxY, 0, limitY) };

  vtkNew<vtkSelection> selection;
  this->GenerateSelection(region, selection);

  for (int i = 0; i < this->View->GetNumberOfRepresentations(); ++i)
  {
    // Only a representation with data behind it has anything to select.
    if (auto* rep = vtkScivisDataRepresentation::SafeDownCast(this->View->GetRepresentation(i)))
    {
      rep->Select(this->View, selection, extend);
    }
  }

  this->CurrentSelection = selection;

  // Fired here and on the view: selection moved out of the view, but observers
  // of the view predate the move and there is no reason to break them.
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, selection.GetPointer());
  this->View->InvokeEvent(vtkCommand::SelectionChangedEvent, selection.GetPointer());
  this->View->Render();
}

//------------------------------------------------------------------------------
void vtkScivisSelector::Clear()
{
  if (!this->View)
  {
    return;
  }

  vtkNew<vtkSelection> empty;
  for (int i = 0; i < this->View->GetNumberOfRepresentations(); ++i)
  {
    if (auto* rep = vtkScivisDataRepresentation::SafeDownCast(this->View->GetRepresentation(i)))
    {
      rep->Select(this->View, empty, false);
    }
  }

  this->CurrentSelection = nullptr;
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, empty.GetPointer());
  this->View->InvokeEvent(vtkCommand::SelectionChangedEvent, empty.GetPointer());
  this->View->Render();
}

//------------------------------------------------------------------------------
void vtkScivisSelector::GenerateSelection(const int region[4], vtkSelection* sel)
{
  const unsigned int screenMinX = static_cast<unsigned int>(region[0]);
  const unsigned int screenMinY = static_cast<unsigned int>(region[1]);
  const unsigned int screenMaxX = static_cast<unsigned int>(region[2]);
  const unsigned int screenMaxY = static_cast<unsigned int>(region[3]);

  vtkRenderer* renderer = this->View->GetRenderer();

  if (this->Mode == FRUSTUM)
  {
    double displayRect[4] = { static_cast<double>(screenMinX), static_cast<double>(screenMinY),
      static_cast<double>(screenMaxX), static_cast<double>(screenMaxY) };
    vtkDoubleArray* frustcorners = vtkDoubleArray::New();
    frustcorners->SetNumberOfComponents(4);
    frustcorners->SetNumberOfTuples(8);

    double worldP[4];
    int index = 0;

    // 4 screen corners x 2 depth values (near=0, far=1) = 8 frustum corners
    double corners[4][2] = { { displayRect[0], displayRect[1] }, { displayRect[0], displayRect[3] },
      { displayRect[2], displayRect[1] }, { displayRect[2], displayRect[3] } };
    for (int c = 0; c < 4; ++c)
    {
      for (double z = 0.0; z <= 1.0; z += 1.0)
      {
        renderer->SetDisplayPoint(corners[c][0], corners[c][1], z);
        renderer->DisplayToWorld();
        renderer->GetWorldPoint(worldP);
        frustcorners->SetTuple4(index, worldP[0], worldP[1], worldP[2], worldP[3]);
        index++;
      }
    }

    vtkSelectionNode* node = vtkSelectionNode::New();
    node->SetContentType(vtkSelectionNode::FRUSTUM);
    node->SetFieldType(this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS
        ? vtkSelectionNode::POINT
        : vtkSelectionNode::CELL);
    node->SetSelectionList(frustcorners);
    sel->AddNode(node);
    node->Delete();
    frustcorners->Delete();
  }
  else
  {
    // Surface selection via hardware picking
    unsigned int area[4] = { 0, 0, 0, 0 };
    area[2] = static_cast<unsigned int>(renderer->GetSize()[0] - 1);
    area[3] = static_cast<unsigned int>(renderer->GetSize()[1] - 1);
    this->HardwareSelector->SetArea(area);
    this->HardwareSelector->CaptureBuffers();

    vtkSelection* vsel =
      this->HardwareSelector->GenerateSelection(screenMinX, screenMinY, screenMaxX, screenMaxY);
    sel->ShallowCopy(vsel);
    vsel->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkScivisSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << (this->Mode == FRUSTUM ? "Frustum" : "Surface") << "\n";
  os << indent << "FieldAssociation: " << this->FieldAssociation << "\n";
  os << indent << "CurrentSelection: " << this->CurrentSelection << "\n";
  os << indent << "View: " << this->View.GetPointer() << "\n";
}

VTK_ABI_NAMESPACE_END
