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
#include "vtkImageData.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderedRepresentation.h"
#include "vtkRenderer.h"
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
#include "vtkViewTheme.h"

#ifdef VTK_USE_QT
#include "vtkQtLabelRenderStrategy.h"
#endif

vtkCxxRevisionMacro(vtkRenderView, "1.26");
vtkStandardNewMacro(vtkRenderView);
vtkCxxSetObjectMacro(vtkRenderView, Transform, vtkAbstractTransform);
vtkCxxSetObjectMacro(vtkRenderView, IconTexture, vtkTexture);

vtkRenderView::vtkRenderView()
{
  this->Renderer = vtkRenderer::New();
  this->LabelRenderer = vtkRenderer::New();
  this->RenderWindow = vtkRenderWindow::New();
  vtkTransform* t = vtkTransform::New();
  t->Identity();
  this->Transform = t;
  this->DisplayHoverText = true;

  this->IconTexture = 0;
  this->IconSize[0] = 16;
  this->IconSize[1] = 16;

  this->RenderWindow->AddRenderer(this->Renderer);
  this->LabelRenderer->EraseOff();
  this->LabelRenderer->InteractiveOff();
  this->LabelRenderer->SetActiveCamera(this->Renderer->GetActiveCamera());
  this->RenderWindow->AddRenderer(this->LabelRenderer);

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->EnableRenderOff();
  iren->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
  this->RenderWindow->SetInteractor(iren);
  //iren->Initialize();

  // Set interaction mode to -1 before calling SetInteractionMode,
  // this will force an initialization of the interaction mode/style.
  this->InteractionMode = -1;
  this->SetInteractionModeTo3D();
  this->LabelRenderMode = FREETYPE;

  this->Balloon = vtkSmartPointer<vtkBalloonRepresentation>::New();

  this->LabelPlacementMapper = vtkSmartPointer<vtkLabelPlacementMapper>::New();
  this->LabelActor = vtkSmartPointer<vtkTexturedActor2D>::New();

  this->LabelActor->SetMapper(this->LabelPlacementMapper);
  this->LabelActor->PickableOff();
  this->LabelRenderer->AddActor(this->LabelActor);

  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->LabelRenderer->AddViewProp(this->Balloon);
  this->Balloon->SetRenderer(this->LabelRenderer);
  this->Balloon->VisibilityOn();

  this->SelectionMode = SURFACE;

  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();  
}

vtkRenderView::~vtkRenderView()
{
  if (this->Renderer)
    {
    this->Renderer->Delete();
    }
  if (this->LabelRenderer)
    {
    this->LabelRenderer->Delete();
    }
  if (this->RenderWindow)
    {
    this->RenderWindow->Delete();
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    }
  if (this->IconTexture)
    {
    this->IconTexture->Delete();
    }
}

vtkRenderWindowInteractor* vtkRenderView::GetInteractor()
{
  return this->RenderWindow->GetInteractor();
}

void vtkRenderView::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  if (!interactor)
    {
    vtkErrorMacro(<< "SetInteractor called with a null interactor pointer."
                  << " That can't be right.");
    return;
    }
  
  // get rid of the render observer on any current interactor
  if (this->RenderWindow->GetInteractor())
    {
    this->RenderWindow->GetInteractor()->RemoveObserver(this->GetObserver());
    }

  // We need to preserve the interactor style currently present on the
  // interactor.
  vtkInteractorObserver *oldStyle = this->GetInteractorStyle();
  oldStyle->Register(this);

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  interactor->EnableRenderOff();
  interactor->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
  this->RenderWindow->SetInteractor(interactor);

  interactor->SetInteractorStyle(oldStyle);
  oldStyle->UnRegister(this);
}

void vtkRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
    {
    this->InteractionMode = mode;
    vtkInteractorObserver* oldStyle = this->GetInteractorStyle();
    if (mode == INTERACTION_MODE_2D)
      {
      if (oldStyle)
        {
        oldStyle->RemoveObserver(this->GetObserver());
        }
      vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
      this->RenderWindow->GetInteractor()->SetInteractorStyle(style);
      style->SetRenderOnMouseMove(this->GetDisplayHoverText());
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
      this->RenderWindow->GetInteractor()->SetInteractorStyle(style);
      style->SetRenderOnMouseMove(this->GetDisplayHoverText());
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
      style2D->SetRenderOnMouseMove(this->GetDisplayHoverText());
      this->InteractionMode = INTERACTION_MODE_2D;
      }
    else if (style3D)
      {
      style3D->SetRenderOnMouseMove(this->GetDisplayHoverText());
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
  return this->GetInteractor()->GetInteractorStyle();
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
    this->Render();
    }
  if (vtkDataRepresentation::SafeDownCast(caller) &&
      eventId == vtkCommand::SelectionChangedEvent)
    {
    this->Render();
    }
  else if (caller == this->GetInteractorStyle() &&
           eventId == vtkCommand::SelectionChangedEvent)
    {
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
    int displayRectangle[4] = {screenMinX, screenMinY, screenMaxX, screenMaxY};
    vtkSmartPointer<vtkDoubleArray> frustcorners =
      vtkSmartPointer<vtkDoubleArray>::New();
    frustcorners->SetNumberOfComponents(4);
    frustcorners->SetNumberOfTuples(8);
    //convert screen rectangle to world frustum
    vtkRenderer *renderer = this->GetRenderer();
    double worldP[32];
    int index=0;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index*4]);
    frustcorners->SetTuple4(index,  worldP[index*4], worldP[index*4+1],
      worldP[index*4+2], worldP[index*4+3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1);
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
    // Do a visible cell selection.
    vtkSmartPointer<vtkHardwareSelector> vcs =
      vtkSmartPointer<vtkHardwareSelector>::New();
    vcs->SetRenderer(this->Renderer);
    vcs->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
    vtkSelection* vsel = vcs->Select();
    sel->ShallowCopy(vsel);
    vsel->Delete();
    }
}

void vtkRenderView::Render()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  this->RenderWindow->Render();
}

void vtkRenderView::ResetCamera()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCamera();
}

void vtkRenderView::ResetCameraClippingRange()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
}

void vtkRenderView::SetDisplayHoverText(bool b)
{
  this->Balloon->SetVisibility(b);
  vtkInteractorStyleRubberBand2D* style2D =
    vtkInteractorStyleRubberBand2D::SafeDownCast(this->GetInteractorStyle());
  vtkInteractorStyleRubberBand3D* style3D =
    vtkInteractorStyleRubberBand3D::SafeDownCast(this->GetInteractorStyle());
  if (style2D)
    {
    style2D->SetRenderOnMouseMove(b);
    }
  if (style3D)
    {
    style3D->SetRenderOnMouseMove(b);
    }
  this->DisplayHoverText = b;
}

void vtkRenderView::PrepareForRendering()
{
  this->Update();

  if (this->GetDisplayHoverText())
    {
    this->UpdateHoverText();
    }

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
  bool foundHoverText = false;
  int pos[2] = {0, 0};
  double loc[2] = {0.0, 0.0};
  if (this->RenderWindow->GetInteractor())
    {
    this->RenderWindow->GetInteractor()->GetEventPosition(pos);
    loc[0] = pos[0];
    loc[1] = pos[1];
    }
  this->Balloon->EndWidgetInteraction(loc);
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkRenderedRepresentation* rep = vtkRenderedRepresentation::SafeDownCast(
      this->GetRepresentation(i));
    if (rep)
      {
      if (!foundHoverText)
        {
        if (this->RenderWindow->GetInteractor())
          {
          const char* hoverText = rep->GetHoverText(this, pos[0], pos[1]);
          if (hoverText)
            {
            foundHoverText = true;
            this->Balloon->SetBalloonText(hoverText);
            this->Balloon->StartWidgetInteraction(loc);
            }
          }
        }
      rep->PrepareForRendering(this);
      }
    }
  if (!foundHoverText)
    {
    this->Balloon->SetBalloonText("");
    }
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
}
