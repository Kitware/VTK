// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceInteractorStyle.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkZSpaceHardwarePicker.h"
#include "vtkZSpaceRayActor.h"
#include "vtkZSpaceRenderWindowInteractor.h"
#include "vtkZSpaceSDKManager.h"

#include <cmath>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkZSpaceInteractorStyle);

//----------------------------------------------------------------------------
vtkZSpaceInteractorStyle::vtkZSpaceInteractorStyle()
{
  // This is to ensure our events are processed before the other widgets events
  // For example to hide the ray when moving a widget with the right button
  this->SetPriority(1.0);

  vtkNew<vtkPolyDataMapper> pdm;
  this->PickActor->SetMapper(pdm);
  this->PickActor->GetProperty()->SetLineWidth(4);
  this->PickActor->GetProperty()->RenderLinesAsTubesOn();
  this->PickActor->GetProperty()->SetRepresentationToWireframe();
  this->PickActor->DragableOff();

  this->TextActor->GetTextProperty()->SetFontSize(17);

  // This picker is used to do interactive picking (i.e. compute the intersection
  // of the ray with the actors).
  // XXX As we have to use a ray-cast based picker to do this stuff,
  // the performances are very bad, especially if the data have a lot of cells.
  // Consider to implement another way to render interactivity, like just
  // changing the color of the ray if something is hit (without computing
  // the intersection) and for example do fast pre-selection.
  vtkNew<vtkCellPicker> exactPicker;
  this->SetInteractionPicker(exactPicker);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "HoverPick: " << this->HoverPick << endl;

  this->PickActor->PrintSelf(os, indent.GetNextIndent());
  this->PickedInteractionProp->PrintSelf(os, indent.GetNextIndent());
  this->TextActor->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// Generic events binding
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::OnMove3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd || !this->CurrentRenderer)
  {
    return;
  }

  switch (this->State)
  {
    case VTKIS_POSITION_PROP:
      this->PositionProp(edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
  }

  this->UpdateRay(edd);
  this->UpdatePickActor();
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::OnPick3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd || !this->CurrentRenderer)
  {
    return;
  }

  this->State = VTKIS_PICK;

  switch (edd->GetAction())
  {
    case vtkEventDataAction::Press:
      this->StartAction(this->State, edd);
      break;
    case vtkEventDataAction::Release:
      this->EndAction(this->State, edd);
      break;
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::OnPositionProp3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd || !this->CurrentRenderer)
  {
    return;
  }

  this->State = VTKIS_POSITION_PROP;

  switch (edd->GetAction())
  {
    case vtkEventDataAction::Press:
      this->StartAction(this->State, edd);
      break;
    case vtkEventDataAction::Release:
      this->EndAction(this->State, edd);
      break;
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::OnSelect3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd || !this->CurrentRenderer)
  {
    return;
  }

  // This event is handled in some various widgets to move them
  // But we want to disable the ray visibility during the interaction
  switch (edd->GetAction())
  {
    case vtkEventDataAction::Press:
      this->ZSpaceRayActor->SetVisibility(false);
      break;
    case vtkEventDataAction::Release:
      this->ZSpaceRayActor->SetVisibility(true);
      break;
  }
}

//----------------------------------------------------------------------------
// Interaction entry points
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::StartPick(vtkEventDataDevice3D* edata)
{
  this->RemovePickActor();
  this->State = VTKIS_PICK;
  this->UpdateRay(edata);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndPick(vtkEventDataDevice3D* edata)
{
  // Perform probing
  this->ProbeData(edata);
  this->State = VTKIS_NONE;
  this->UpdateRay(edata);
}

//----------------------------------------------------------------------------
bool vtkZSpaceInteractorStyle::HardwareSelect(vtkEventDataDevice3D* edd, bool actorPassOnly)
{
  vtkDebugMacro("Hardware Select");

  vtkRenderer* ren = this->CurrentRenderer;
  vtkRenderWindow* renWin = this->Interactor->GetRenderWindow();
  vtkZSpaceRenderWindowInteractor* iren =
    static_cast<vtkZSpaceRenderWindowInteractor*>(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return false;
  }

  double pos[3];
  edd->GetWorldPosition(pos);
  double wxyz[4];
  edd->GetWorldOrientation(wxyz);

  if (this->HardwarePicker->PickProp(pos, wxyz, this->PickingFieldAssociation, ren, actorPassOnly))
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::StartPositionProp(vtkEventDataDevice3D* edata)
{
  vtkDebugMacro("Start Position Prop");

  // Do not position another prop if one is already selected
  if (this->InteractionProp != nullptr)
  {
    return;
  }

  if (!this->HardwareSelect(edata, true))
  {
    return;
  }

  vtkSelection* selection = this->HardwarePicker->GetSelection();
  if (!selection || selection->GetNumberOfNodes() == 0)
  {
    return;
  }

  vtkSelectionNode* node = selection->GetNode(0);
  this->InteractionProp =
    vtkProp3D::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndPositionProp(vtkEventDataDevice3D* vtkNotUsed(edata))
{
  vtkDebugMacro("End Position Prop");

  this->State = VTKIS_NONE;
  this->InteractionProp = nullptr;
}

//----------------------------------------------------------------------------
// Interaction methods
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::ProbeData(vtkEventDataDevice3D* edata)
{
  vtkDebugMacro("Probe Data");

  if (!edata)
  {
    return;
  }

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, edata);

  if (!this->HardwareSelect(edata, false))
  {
    return;
  }

  // Invoke end pick method if defined
  if (this->HandleObservers && this->HasObserver(vtkCommand::EndPickEvent))
  {
    this->InvokeEvent(vtkCommand::EndPickEvent, this->HardwarePicker->GetSelection());
  }
  else
  {
    this->EndPickCallback(this->HardwarePicker->GetSelection());
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndPickCallback(vtkSelection* sel)
{
  vtkDebugMacro("End Pick Callback");

  // XXX Consider to use rendering-based selection in the future, that is much more efficient.
  // Problem is, currently, this mode doesn't seems to work with composite datasets.
  // The following commented code was an attempt to use it.

  // if (!sel)
  // {
  //   return;
  // }

  // vtkSelectionNode* node = sel->GetNode(0);
  // if (!node || !node->GetProperties()->Has(vtkSelectionNode::PROP()))
  // {
  //   return;
  // }

  // vtkActor* actor = vtkActor::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
  // if (!actor)
  // {
  //   cout << "Cannot downcast prop to actor !" << endl;
  //   return;
  // }

  // actor->GetMapper()->SetSelection(sel);
  // actor->GetProperty()->SetSelectionColor(0.0, 0.0, 1.0, 1.0);
  // actor->GetProperty()->SetSelectionLineWidth(3.0);

  vtkSmartPointer<vtkDataSet> ds;
  vtkIdType aid;
  if (!this->FindDataSet(sel, ds, aid))
  {
    return;
  }

  // Create the corresponding pick actor
  if (this->GetPickingFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkCell* cell = ds->GetCell(aid);
    this->CreatePickCell(cell);
  }
  else
  {
    double* point = ds->GetPoint(aid);
    this->CreatePickPoint(point);
  }

  if (this->PickedInteractionProp)
  {
    this->PickActor->SetPosition(this->PickedInteractionProp->GetPosition());
    this->PickActor->SetScale(this->PickedInteractionProp->GetScale());
    this->PickActor->SetUserMatrix(this->PickedInteractionProp->GetUserMatrix());
    this->PickActor->SetOrientation(this->PickedInteractionProp->GetOrientation());
  }
  else
  {
    this->PickActor->SetPosition(0.0, 0.0, 0.0);
    this->PickActor->SetScale(1.0, 1.0, 1.0);
  }
  this->CurrentRenderer->AddActor(this->PickActor);

  // Compute the text info about cell or point
  std::string pickedText = this->GetPickedText(ds, aid);

  this->TextActor->SetDisplayPosition(50, 50);
  this->TextActor->SetInput(pickedText.c_str());
  this->CurrentRenderer->AddActor2D(this->TextActor);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::PositionProp(
  vtkEventData* ed, double* vtkNotUsed(lwpos), double* vtkNotUsed(lwori))
{
  if (this->InteractionProp == nullptr || !this->InteractionProp->GetDragable())
  {
    return;
  }
  this->Superclass::PositionProp(ed);
}

//----------------------------------------------------------------------------
// Utility routines
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::StartAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->StartPositionProp(edata);
      break;
    case VTKIS_PICK:
      this->StartPick(edata);
      break;
  }
}
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->EndPositionProp(edata);
      break;
    case VTKIS_PICK:
      this->EndPick(edata);
      break;
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::UpdateRay(vtkEventDataDevice3D* edata)
{
  if (!this->Interactor)
  {
    return;
  }

  double p0[3];
  double wxyz[4];
  edata->GetWorldPosition(p0);
  edata->GetWorldOrientation(wxyz);

  // Create the appropriate ray user transform from event position and orientation
  vtkNew<vtkTransform> stylusT;
  stylusT->Identity();
  stylusT->Translate(p0);
  stylusT->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);

  // The maximum ray length is the camera zfar
  const double rayMaxLength = this->CurrentRenderer->GetActiveCamera()->GetClippingRange()[1];

  double rayLength = rayMaxLength;
  if (this->State == VTKIS_POSITION_PROP)
  {
    rayLength = this->ZSpaceRayActor->GetLength();
  }
  // Make sure that the ray length is updated in case of a pick
  else if (this->HoverPick || this->State == VTKIS_PICK)
  {
    this->FindPickedActor(p0, wxyz);
    // If something is picked, set the length accordingly
    if (this->InteractionProp)
    {
      // Compute the length of the ray
      double p1[3];
      this->InteractionPicker->GetPickPosition(p1);
      rayLength = sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
    }
  }

  if (rayLength == rayMaxLength)
  {
    this->ZSpaceRayActor->SetNoPick();
  }
  else
  {
    this->ZSpaceRayActor->SetPick();
  }

  this->ZSpaceRayActor->SetLength(rayLength);
  stylusT->Scale(rayLength, rayLength, rayLength);
  this->ZSpaceRayActor->SetUserTransform(stylusT);

  return;
}

//----------------------------------------------------------------------------
bool vtkZSpaceInteractorStyle::FindDataSet(
  vtkSelection* sel, vtkSmartPointer<vtkDataSet>& ds, vtkIdType& aid)
{
  if (!sel)
  {
    vtkWarningMacro("Unable to retrieve the selection !");
    return false;
  }

  vtkSelectionNode* node = sel->GetNode(0);
  if (!node || !node->GetProperties()->Has(vtkSelectionNode::PROP()))
  {
    vtkWarningMacro("Unable to retrieve the picked prop !");
    return false;
  }

  vtkActor* pickedActor =
    vtkActor::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
  if (!pickedActor)
  {
    vtkWarningMacro("Unable to retrieve the picked actor !");
    return false;
  }

  this->PickedInteractionProp = pickedActor;

  vtkMapper* mapper = pickedActor->GetMapper();
  if (!mapper)
  {
    vtkWarningMacro("Unable to retrieve the mapper !");
    return false;
  }

  vtkDataObject* pickedDataObject = mapper->GetExecutive()->GetInputData(0, 0);
  if (!pickedDataObject)
  {
    vtkWarningMacro("Unable to retrieve the picked data object !");
    return false;
  }

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(pickedDataObject);

  // Handle composite datasets
  if (cds)
  {
    vtkIdType cid = node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX());
    vtkNew<vtkDataObjectTreeIterator> iter;
    iter->SetDataSet(cds);
    iter->SkipEmptyNodesOn();
    iter->SetVisitOnlyLeaves(1);
    iter->InitTraversal();
    while (iter->GetCurrentFlatIndex() != cid && !iter->IsDoneWithTraversal())
    {
      iter->GoToNextItem();
    }
    if (iter->GetCurrentFlatIndex() == cid)
    {
      ds.TakeReference(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()));
    }
  }
  else
  {
    ds.TakeReference(vtkDataSet::SafeDownCast(pickedDataObject));
  }
  if (!ds)
  {
    vtkWarningMacro("Unable to retrieve the picked dataset !");
    return false;
  }

  // Get the picked cell
  vtkIdTypeArray* ids = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
  if (ids == 0)
  {
    vtkWarningMacro("Unable to retrieve the picked cell !");
    return false;
  }
  aid = ids->GetComponent(0, 0);

  ds->Register(this);

  return true;
}

//----------------------------------------------------------------------------
std::string vtkZSpaceInteractorStyle::GetPickedText(vtkDataSet* ds, const vtkIdType& aid)
{
  // Compute the text from the selected point or cell
  // It would be nice to be able to factorize this code
  // with the vtkSMTooltipSelectionPipeline code
  std::stringstream ssPickedText;

  vtkFieldData* fieldData = nullptr;
  vtkDataArray* originalIds = nullptr;

  // We selected a cell
  if (this->GetPickingFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    ssPickedText << "Cell id : " << aid << "\n";
    vtkCellData* cellData = ds->GetCellData();
    fieldData = cellData;

    originalIds = cellData->GetArray("vtkOriginalCellIds");
    if (originalIds)
    {
      ssPickedText << "Id: " << originalIds->GetTuple1(0) << "\n";
    }

    // Cell type
    vtkCell* cell = ds->GetCell(aid);

    // XXX Can be inproved by printing the type of the cell as a string
    // (see vtkSMCoreUtilities::GetStringForCellType in ParaView)
    ssPickedText << "Cell type: " << cell->GetCellType() << "\n";
  }
  else // We selected a point
  {
    ssPickedText << "Point id : " << aid << "\n";
    vtkPointData* pointData = ds->GetPointData();
    fieldData = pointData;

    originalIds = pointData->GetArray("vtkOriginalPointIds");
    if (originalIds)
    {
      ssPickedText << "Id: " << originalIds->GetTuple1(0) << "\n";
    }

    // Point coords
    double* point = ds->GetPoint(aid);
    ssPickedText << "Coords: (" << point[0] << ", " << point[1] << ", " << point[2] << ")\n";
  }

  if (fieldData)
  {
    // Point attributes
    vtkIdType nbArrays = fieldData->GetNumberOfArrays();
    for (vtkIdType i_arr = 0; i_arr < nbArrays; i_arr++)
    {
      vtkDataArray* array = fieldData->GetArray(i_arr);
      if (!array || originalIds == array)
      {
        continue;
      }
      ssPickedText << array->GetName() << ": ";
      if (array->GetNumberOfComponents() > 1)
      {
        ssPickedText << "(";
      }
      vtkIdType nbComps = array->GetNumberOfComponents();
      for (vtkIdType i_comp = 0; i_comp < nbComps; i_comp++)
      {
        ssPickedText << array->GetTuple(0)[i_comp];
        if (i_comp + 1 < nbComps)
        {
          ssPickedText << ", ";
        }
      }
      if (array->GetNumberOfComponents() > 1)
      {
        ssPickedText << ")\n";
      }
      else
      {
        ssPickedText << "\n";
      }
    }
  }

  return std::move(ssPickedText.str());
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::CreatePickCell(vtkCell* cell)
{
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkPoints> pdpts;
  pdpts->SetDataTypeToDouble();
  vtkNew<vtkCellArray> lines;

  this->PickActor->GetProperty()->SetColor(this->PickColor);

  int nedges = cell->GetNumberOfEdges();

  if (nedges)
  {
    for (int edgeNum = 0; edgeNum < nedges; ++edgeNum)
    {
      vtkCell* edge = cell->GetEdge(edgeNum);
      vtkPoints* pts = edge->GetPoints();
      int npts = edge->GetNumberOfPoints();
      lines->InsertNextCell(npts);
      for (int ep = 0; ep < npts; ++ep)
      {
        vtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
        lines->InsertCellPoint(newpt);
      }
    }
  }
  else if (cell->GetCellType() == VTK_LINE || cell->GetCellType() == VTK_POLY_LINE)
  {
    vtkPoints* pts = cell->GetPoints();
    int npts = cell->GetNumberOfPoints();
    lines->InsertNextCell(npts);
    for (int ep = 0; ep < npts; ++ep)
    {
      vtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
      lines->InsertCellPoint(newpt);
    }
  }
  else
  {
    return;
  }

  pd->SetPoints(pdpts.Get());
  pd->SetLines(lines.Get());

  static_cast<vtkPolyDataMapper*>(this->PickActor->GetMapper())->SetInputData(pd);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::CreatePickPoint(double* point)
{
  this->PickActor->GetProperty()->SetColor(this->PickColor);
  this->PickActor->GetProperty()->SetPointSize(8.0);

  vtkNew<vtkPointSource> pointSource;
  pointSource->SetCenter(point);
  pointSource->SetNumberOfPoints(1);
  pointSource->SetRadius(0);

  pointSource->Update();
  static_cast<vtkPolyDataMapper*>(this->PickActor->GetMapper())
    ->SetInputData(pointSource->GetOutput());
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::UpdatePickActor()
{

  if (this->PickedInteractionProp)
  {
    // Remove the pick actor if it has been deleted
    if (!this->CurrentRenderer->HasViewProp(this->PickedInteractionProp))
    {
      this->RemovePickActor();
      return;
    }

    // Update the visibility
    this->PickActor->SetVisibility(this->PickedInteractionProp->GetVisibility());
    this->TextActor->SetVisibility(this->PickedInteractionProp->GetVisibility());

    // Move the point/cell picked with the prop
    this->PickActor->SetPosition(this->PickedInteractionProp->GetPosition());
    this->PickActor->SetScale(this->PickedInteractionProp->GetScale());
    this->PickActor->SetUserMatrix(this->PickedInteractionProp->GetUserMatrix());
    this->PickActor->SetOrientation(this->PickedInteractionProp->GetOrientation());
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::RemovePickActor()
{
  if (this->CurrentRenderer)
  {
    this->CurrentRenderer->RemoveActor(this->PickActor);
    this->CurrentRenderer->RemoveActor2D(this->TextActor);
    this->PickedInteractionProp = nullptr;
  }
}

VTK_ABI_NAMESPACE_END
