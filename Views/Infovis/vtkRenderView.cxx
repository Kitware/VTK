/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderView.cxx

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

#include "vtkRenderView.h"

#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAppendPoints.h"
#include "vtkBalloonRepresentation.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkFreeTypeLabelRenderStrategy.h"
#include "vtkHardwareSelector.h"
#include "vtkHoverWidget.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderedRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkTransform.h"
#include "vtkTransformCoordinateSystems.h"
#include "vtkUnicodeString.h"
#include "vtkViewTheme.h"

#ifdef VTK_USE_QT
#include "vtkQtLabelRenderStrategy.h"
#endif

#include <sstream>

vtkStandardNewMacro(vtkRenderView);
vtkCxxSetObjectMacro(vtkRenderView, Transform, vtkAbstractTransform);
vtkCxxSetObjectMacro(vtkRenderView, IconTexture, vtkTexture);

vtkRenderView::vtkRenderView()
{
  this->RenderOnMouseMove = false;
  this->InteractionMode = -1;
  this->LabelRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->Transform = vtkTransform::New();
  this->DisplayHoverText = false;
  this->IconTexture = 0;
  this->Interacting = false;
  this->LabelRenderMode = FREETYPE;
  this->SelectionMode = SURFACE;
  this->Selector = vtkSmartPointer<vtkHardwareSelector>::New();
  this->Balloon = vtkSmartPointer<vtkBalloonRepresentation>::New();
  this->LabelPlacementMapper = vtkSmartPointer<vtkLabelPlacementMapper>::New();
  this->LabelActor = vtkSmartPointer<vtkTexturedActor2D>::New();
  this->HoverWidget = vtkSmartPointer<vtkHoverWidget>::New();
  this->InHoverTextRender = false;
  this->IconSize[0] = 16;
  this->IconSize[1] = 16;
  this->DisplaySize[0] = 0;
  this->DisplaySize[1] = 0;
  this->PickRenderNeedsUpdate = true;
  this->InPickRender = false;

  vtkTransform::SafeDownCast(this->Transform)->Identity();

  this->LabelRenderer->EraseOff();
  this->LabelRenderer->InteractiveOff();

  this->LabelRenderer->SetActiveCamera(this->Renderer->GetActiveCamera());
  this->RenderWindow->AddRenderer(this->LabelRenderer);

  // Initialize the selector and listen to render events to help Selector know
  // when to update the full-screen hardware pick.
  this->Selector->SetRenderer(this->Renderer);
  this->Selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  this->RenderWindow->AddObserver(vtkCommand::EndEvent, this->GetObserver());

  vtkRenderWindowInteractor* iren = this->RenderWindow->GetInteractor();
  // this ensure that the observer is added to the interactor correctly.
  this->SetInteractor(iren);

  // The interaction mode is -1 before calling SetInteractionMode,
  // this will force an initialization of the interaction mode/style.
  this->SetInteractionModeTo3D();

  this->HoverWidget->AddObserver(vtkCommand::TimerEvent, this->GetObserver());

  this->LabelActor->SetMapper(this->LabelPlacementMapper);
  this->LabelActor->PickableOff();
  this->LabelRenderer->AddActor(this->LabelActor);

  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->LabelRenderer->AddViewProp(this->Balloon);
  this->Balloon->SetRenderer(this->LabelRenderer);
  this->Balloon->PickableOff();
  this->Balloon->VisibilityOn();

  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
}

vtkRenderView::~vtkRenderView()
{
  if (this->Transform)
  {
    this->Transform->Delete();
  }
  if (this->IconTexture)
  {
    this->IconTexture->Delete();
  }
}

void vtkRenderView::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  if (!interactor)
  {
    vtkErrorMacro(<< "SetInteractor called with a null interactor pointer."
                  << " That can't be right.");
    return;
  }

  if (this->GetInteractor())
  {
    this->GetInteractor()->RemoveObserver(this->GetObserver());
  }

  this->Superclass::SetInteractor(interactor);
  this->HoverWidget->SetInteractor(interactor);

  interactor->EnableRenderOff();
  interactor->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
  interactor->AddObserver(vtkCommand::StartInteractionEvent, this->GetObserver());
  interactor->AddObserver(vtkCommand::EndInteractionEvent, this->GetObserver());
}

void vtkRenderView::SetInteractorStyle(vtkInteractorObserver* style)
{
  if (!style)
  {
    vtkErrorMacro("Interactor style must not be null.");
    return;
  }
  vtkInteractorObserver* oldStyle = this->GetInteractorStyle();
  if (style != oldStyle)
  {
    if (oldStyle)
    {
      oldStyle->RemoveObserver(this->GetObserver());
    }
    this->RenderWindow->GetInteractor()->SetInteractorStyle(style);
    style->AddObserver(
      vtkCommand::SelectionChangedEvent, this->GetObserver());
    vtkInteractorStyleRubberBand2D* style2D =
      vtkInteractorStyleRubberBand2D::SafeDownCast(style);
    vtkInteractorStyleRubberBand3D* style3D =
      vtkInteractorStyleRubberBand3D::SafeDownCast(style);
    if (style2D)
    {
      style2D->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      this->InteractionMode = INTERACTION_MODE_2D;
    }
    else if (style3D)
    {
      style3D->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      this->InteractionMode = INTERACTION_MODE_3D;
    }
    else
    {
      this->InteractionMode = INTERACTION_MODE_UNKNOWN;
    }
  }
}

vtkInteractorObserver* vtkRenderView::GetInteractorStyle()
{
  if (this->GetInteractor())
  {
    return this->GetInteractor()->GetInteractorStyle();
  }
  return NULL;
}

void vtkRenderView::SetRenderOnMouseMove(bool b)
{
  if (b == this->RenderOnMouseMove)
  {
    return;
  }

  vtkInteractorObserver* style = this->GetInteractor()->GetInteractorStyle();
  vtkInteractorStyleRubberBand2D* style2D =
    vtkInteractorStyleRubberBand2D::SafeDownCast(style);
  if (style2D)
  {
    style2D->SetRenderOnMouseMove(b);
  }
  vtkInteractorStyleRubberBand3D* style3D =
    vtkInteractorStyleRubberBand3D::SafeDownCast(style);
  if (style3D)
  {
    style3D->SetRenderOnMouseMove(b);
  }
  this->RenderOnMouseMove = b;
}

void vtkRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
  {
    this->InteractionMode = mode;
    vtkInteractorObserver* oldStyle = this->GetInteractor()->GetInteractorStyle();
    if (mode == INTERACTION_MODE_2D)
    {
      if (oldStyle)
      {
        oldStyle->RemoveObserver(this->GetObserver());
      }
      vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
      this->GetInteractor()->SetInteractorStyle(style);
      style->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      style->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
      this->Renderer->GetActiveCamera()->ParallelProjectionOn();
      style->Delete();
    }
    else if (mode == INTERACTION_MODE_3D)
    {
      if (oldStyle)
      {
        oldStyle->RemoveObserver(this->GetObserver());
      }
      vtkInteractorStyleRubberBand3D* style = vtkInteractorStyleRubberBand3D::New();
      this->GetInteractor()->SetInteractorStyle(style);
      style->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      style->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
      this->Renderer->GetActiveCamera()->ParallelProjectionOff();
      style->Delete();
    }
    else
    {
      vtkErrorMacro("Unknown interaction mode.");
    }
  }
}

void vtkRenderView::SetRenderWindow(vtkRenderWindow* win)
{
  vtkSmartPointer<vtkRenderWindowInteractor> irenOld = this->GetInteractor();
  this->Superclass::SetRenderWindow(win);
  vtkRenderWindowInteractor* irenNew = this->GetInteractor();
  if (irenOld != irenNew)
  {
    if (irenOld)
    {
      irenOld->RemoveObserver(this->GetObserver());
    }
    if (irenNew)
    {
      this->SetInteractor(irenNew);
    }
  }
}

void vtkRenderView::Render()
{
  // Why should we have to initialize in here at all?
  if (!this->RenderWindow->GetInteractor()->GetInitialized())
  {
    this->RenderWindow->GetInteractor()->Initialize();
  }
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  this->RenderWindow->Render();
}

void vtkRenderView::AddLabels(vtkAlgorithmOutput* conn)
{
  this->LabelPlacementMapper->AddInputConnection(0, conn);
}

void vtkRenderView::RemoveLabels(vtkAlgorithmOutput* conn)
{
  this->LabelPlacementMapper->RemoveInputConnection(0, conn);
}

void vtkRenderView::ProcessEvents(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  if (caller == this->GetInteractor() && eventId == vtkCommand::RenderEvent)
  {
    vtkDebugMacro(<< "interactor causing a render event.");
    this->Render();
  }
  if (caller == this->HoverWidget.GetPointer() && eventId == vtkCommand::TimerEvent)
  {
    vtkDebugMacro(<< "hover widget timer causing a render event.");
    this->UpdateHoverText();
    this->InHoverTextRender = true;
    this->Render();
    this->InHoverTextRender = false;
  }
  if (caller == this->GetInteractor() && eventId == vtkCommand::StartInteractionEvent)
  {
    this->Interacting = true;
    this->UpdateHoverWidgetState();
  }
  if (caller == this->GetInteractor() && eventId == vtkCommand::EndInteractionEvent)
  {
    this->Interacting = false;
    this->UpdateHoverWidgetState();
    this->PickRenderNeedsUpdate = true;
  }
  if (caller == this->RenderWindow.GetPointer()
      && eventId == vtkCommand::EndEvent)
  {
    vtkDebugMacro(<< "did a render, interacting: " << this->Interacting
         << " in pick render: " << this->InPickRender
         << " in hover text render: " << this->InHoverTextRender);
    if (!this->Interacting && !this->InPickRender && !this->InHoverTextRender)
    {
      // This will cause UpdatePickRender to create a new snapshot of the view
      // for picking with the next drag selection or hover event.
      this->PickRenderNeedsUpdate = true;
    }
  }
  if (vtkDataRepresentation::SafeDownCast(caller) &&
      eventId == vtkCommand::SelectionChangedEvent)
  {
    vtkDebugMacro("selection changed causing a render event");
    this->Render();
  }
  else if (vtkDataRepresentation::SafeDownCast(caller) &&
           eventId == vtkCommand::UpdateEvent)
  {
    // UpdateEvent is called from push pipeline executions from
    // vtkExecutionScheduler. We want to automatically render the view
    // when one of our representations is updated.
    vtkDebugMacro("push pipeline causing a render event");
    this->Render();
  }
  else if (caller == this->GetInteractorStyle() &&
           eventId == vtkCommand::SelectionChangedEvent)
  {
    vtkDebugMacro("interactor style made a selection changed event");
    vtkSmartPointer<vtkSelection> selection =
      vtkSmartPointer<vtkSelection>::New();
    this->GenerateSelection(callData, selection);

    // This enum value is the same for 2D and 3D interactor styles
    unsigned int* data = reinterpret_cast<unsigned int*>(callData);
    bool extend = (data[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION);

    // Call select on the representation(s)
    for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
      this->GetRepresentation(i)->Select(this, selection, extend);
    }
  }
  this->Superclass::ProcessEvents(caller, eventId, callData);
}

void vtkRenderView::UpdatePickRender()
{
  if (this->PickRenderNeedsUpdate)
  {
    this->InPickRender = true;
    unsigned int area[4] = {0, 0, 0, 0};
    area[2] = static_cast<unsigned int>(this->Renderer->GetSize()[0] - 1);
    area[3] = static_cast<unsigned int>(this->Renderer->GetSize()[1] - 1);
    this->Selector->SetArea(area);
    this->LabelRenderer->DrawOff();
    this->Selector->CaptureBuffers();
    this->LabelRenderer->DrawOn();
    this->InPickRender = false;
    this->PickRenderNeedsUpdate = false;
  }
}

void vtkRenderView::GenerateSelection(void* callData, vtkSelection* sel)
{
  unsigned int* rect = reinterpret_cast<unsigned int*>(callData);
  unsigned int pos1X = rect[0];
  unsigned int pos1Y = rect[1];
  unsigned int pos2X = rect[2];
  unsigned int pos2Y = rect[3];
  int stretch = 2;
  if (pos1X == pos2X && pos1Y == pos2Y)
  {
    pos1X = pos1X - stretch > 0 ? pos1X - stretch : 0;
    pos1Y = pos1Y - stretch > 0 ? pos1Y - stretch : 0;
    pos2X = pos2X + stretch;
    pos2Y = pos2Y + stretch;
  }
  unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
  unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
  unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
  unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;

  if (this->SelectionMode == FRUSTUM)
  {
    // Do a frustum selection.
    double displayRectangle[4] = {static_cast<double>(screenMinX),
                                  static_cast<double>(screenMinY),
                                  static_cast<double>(screenMaxX),
                                  static_cast<double>(screenMaxY)};
    vtkSmartPointer<vtkDoubleArray> frustcorners =
      vtkSmartPointer<vtkDoubleArray>::New();
    frustcorners->SetNumberOfComponents(4);
    frustcorners->SetNumberOfTuples(8);
    //convert screen rectangle to world frustum
    vtkRenderer *renderer = this->GetRenderer();
    double worldP[32];
    int index=0;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);

    vtkSmartPointer<vtkSelectionNode> node =
      vtkSmartPointer<vtkSelectionNode>::New();
    node->SetContentType(vtkSelectionNode::FRUSTUM);
    node->SetFieldType(vtkSelectionNode::CELL);
    node->SetSelectionList(frustcorners);
    sel->AddNode(node);
  }
  else
  {
    this->UpdatePickRender();
    vtkSelection* vsel = this->Selector->GenerateSelection(screenMinX, screenMinY, screenMaxX, screenMaxY);
    sel->ShallowCopy(vsel);
    vsel->Delete();
  }
}

void vtkRenderView::SetDisplayHoverText(bool b)
{
  this->Balloon->SetVisibility(b);
  this->DisplayHoverText = b;
}

void vtkRenderView::UpdateHoverWidgetState()
{
  // Make sure we have a context, then ensure hover widget is
  // enabled if we are displaying hover text.
  this->RenderWindow->MakeCurrent();
  if (this->RenderWindow->IsCurrent())
  {
    if (!this->Interacting && (this->HoverWidget->GetEnabled() ? true : false) != this->DisplayHoverText)
    {
      vtkDebugMacro(<< "turning " << (this->DisplayHoverText ? "on" : "off") << " hover widget");
      this->HoverWidget->SetEnabled(this->DisplayHoverText);
    }
    // Disable hover text when interacting.
    else if (this->Interacting && this->HoverWidget->GetEnabled())
    {
      vtkDebugMacro(<< "turning off hover widget");
      this->HoverWidget->SetEnabled(false);
    }
  }
  if (!this->HoverWidget->GetEnabled())
  {
    this->Balloon->SetBalloonText("");
  }
}

void vtkRenderView::PrepareForRendering()
{
  this->Update();
  this->UpdateHoverWidgetState();

  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    vtkRenderedRepresentation* rep = vtkRenderedRepresentation::SafeDownCast(
      this->GetRepresentation(i));
    if (rep)
    {
      rep->PrepareForRendering(this);
    }
  }
}

void vtkRenderView::UpdateHoverText()
{
  this->UpdatePickRender();

  int pos[2] = {0, 0};
  unsigned int upos[2] = {0, 0};
  double loc[2] = {0.0, 0.0};
  if (this->RenderWindow->GetInteractor())
  {
    this->RenderWindow->GetInteractor()->GetEventPosition(pos);
    loc[0] = pos[0];
    loc[1] = pos[1];
    upos[0] = static_cast<unsigned int>(pos[0]);
    upos[1] = static_cast<unsigned int>(pos[1]);
  }
  this->Balloon->EndWidgetInteraction(loc);

  // The number of pixels away from the pointer to search for hovered objects.
  int hoverTol = 3;

  // Retrieve the hovered cell from the saved buffer.
  vtkHardwareSelector::PixelInformation info = this->Selector->GetPixelInformation(upos, hoverTol);
  vtkIdType cell = info.AttributeID;
  vtkProp* prop = info.Prop;
  if (prop == 0 || cell == -1)
  {
    this->Balloon->SetBalloonText("");
    return;
  }

  // For debugging
  //std::ostringstream oss;
  //oss << "prop: " << prop << " cell: " << cell;
  //this->Balloon->SetBalloonText(oss.str().c_str());
  //this->Balloon->StartWidgetInteraction(loc);

  vtkUnicodeString hoverText;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    vtkRenderedRepresentation* rep = vtkRenderedRepresentation::SafeDownCast(
      this->GetRepresentation(i));
    if (rep && this->RenderWindow->GetInteractor())
    {
      hoverText = rep->GetHoverText(this, prop, cell);
      if (!hoverText.empty())
      {
        break;
      }
    }
  }
  this->Balloon->SetBalloonText(hoverText.utf8_str());
  this->Balloon->StartWidgetInteraction(loc);
  this->InvokeEvent(vtkCommand::HoverEvent, &hoverText);
}

void vtkRenderView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->SetGradientBackground(true);
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    this->GetRepresentation(i)->ApplyViewTheme(theme);
  }
}

void vtkRenderView::SetLabelPlacementMode(int mode)
{
  this->LabelPlacementMapper->SetPlaceAllLabels(mode == ALL);
}

int vtkRenderView::GetLabelPlacementMode()
{
  return this->LabelPlacementMapper->GetPlaceAllLabels() ? ALL : NO_OVERLAP;
}

int vtkRenderView::GetLabelRenderMode()
{
  return vtkFreeTypeLabelRenderStrategy::SafeDownCast(
    this->LabelPlacementMapper->GetRenderStrategy()) ? FREETYPE : QT;
}

void vtkRenderView::SetLabelRenderMode(int render_mode)
{
  //First, make sure the render mode is set on all the representations.
  // TODO: Setup global labeller render mode
  if(render_mode != this->GetLabelRenderMode())
  {
    // Set label render mode of all representations.
    for (int r = 0; r < this->GetNumberOfRepresentations(); ++r)
    {
      vtkRenderedRepresentation* rr =
        vtkRenderedRepresentation::SafeDownCast(this->GetRepresentation(r));
      if (rr)
      {
        rr->SetLabelRenderMode(render_mode);
      }
    }
  }

  switch(render_mode)
  {
    case QT:
    {
#ifdef VTK_USE_QT
      vtkSmartPointer<vtkQtLabelRenderStrategy> qts =
        vtkSmartPointer<vtkQtLabelRenderStrategy>::New();
      this->LabelPlacementMapper->SetRenderStrategy(qts);
#else
      vtkErrorMacro("Qt label rendering not supported.");
#endif
      break;
    }
    default:
    {
      vtkSmartPointer<vtkFreeTypeLabelRenderStrategy> fts =
        vtkSmartPointer<vtkFreeTypeLabelRenderStrategy>::New();
      this->LabelPlacementMapper->SetRenderStrategy(fts);
    }
  }
}

int* vtkRenderView::GetDisplaySize()
{
  if ( this->DisplaySize[0] == 0 ||
       this->DisplaySize[1] == 0 )
  {
    return this->IconSize;
  }
  else
  {
    return this->DisplaySize;
  }
}

void vtkRenderView::GetDisplaySize(int &dsx, int &dsy)
{
  if ( this->DisplaySize[0] == 0 ||
       this->DisplaySize[1] == 0 )
  {
    dsx = this->IconSize[0];
    dsy = this->IconSize[1];
  }
  else
  {
    dsx = this->DisplaySize[0];
    dsy = this->DisplaySize[1];
  }
}

void vtkRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderWindow: ";
  if (this->RenderWindow)
  {
    os << "\n";
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << "\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
  os << indent << "InteractionMode: " << this->InteractionMode << endl;
  os << indent << "DisplayHoverText: " << this->DisplayHoverText << endl;
  os << indent << "Transform: ";
  if (this->Transform)
  {
    os << "\n";
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "LabelRenderMode: " << this->LabelRenderMode << endl;
  os << indent << "IconTexture: ";
  if (this->IconTexture)
  {
    os << "\n";
    this->IconTexture->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "IconSize: " << this->IconSize[0] << "," << this->IconSize[1] << endl;
  os << indent << "DisplaySize: " << this->DisplaySize[0] << "," << this->DisplaySize[1] << endl;
  os << indent << "InteractionMode: " << this->InteractionMode << endl;
  os << indent << "RenderOnMouseMove: " << this->RenderOnMouseMove << endl;
}
