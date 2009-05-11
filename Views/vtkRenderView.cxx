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
#include "vtkArrayCalculator.h"
#include "vtkBalloonRepresentation.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkHardwareSelector.h"
#include "vtkIconGlyphFilter.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelPlacer.h"
#include "vtkLabelSizeCalculator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSetToLabelHierarchy.h"
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
#include "vtkQtLabelSizeCalculator.h"
#include "vtkQtLabelMapper.h"
#endif

vtkCxxRevisionMacro(vtkRenderView, "1.19");
vtkStandardNewMacro(vtkRenderView);
vtkCxxSetObjectMacro(vtkRenderView, Transform, vtkAbstractTransform);

vtkRenderView::vtkRenderView()
{
  this->Renderer = vtkRenderer::New();
  this->Renderer->AddObserver(vtkCommand::StartEvent, this->GetObserver());
  this->Renderer->AddObserver(vtkCommand::ComputeVisiblePropBoundsEvent,
    this->GetObserver());

  vtkTransform* t = vtkTransform::New();
  t->Identity();
  this->Transform = t;
  this->InteractorStyle = 0;

  // Set interaction mode to -1 before calling SetInteractionMode,
  // this will force an initialization of the interaction mode/style.
  this->InteractionMode = -1;
  this->SetInteractionModeTo3D();
  this->LabelRenderMode = FREETYPE;

  this->Balloon = vtkSmartPointer<vtkBalloonRepresentation>::New();
  this->DisplayHoverText = true;

  this->LabelAppend = vtkSmartPointer<vtkAppendPoints>::New();
  this->LabelSize = vtkSmartPointer<vtkLabelSizeCalculator>::New();
  this->LabelHierarchy = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->LabelPlacer = vtkSmartPointer<vtkLabelPlacer>::New();
  this->LabelMapper = vtkSmartPointer<vtkLabeledDataMapper>::New();
  this->LabelMapper2D = vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
  this->LabelActor = vtkSmartPointer<vtkTexturedActor2D>::New();

  this->IconAppend = vtkSmartPointer<vtkAppendPoints>::New();
  this->IconSize = vtkSmartPointer<vtkArrayCalculator>::New();
  this->IconGlyph = vtkSmartPointer<vtkIconGlyphFilter>::New();
  this->IconTransform = vtkSmartPointer<vtkTransformCoordinateSystems>::New();
  this->IconMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  this->IconActor = vtkSmartPointer<vtkTexturedActor2D>::New();

  this->LabelSize->SetInputConnection(this->LabelAppend->GetOutputPort());
  this->LabelHierarchy->AddInputConnection(0, this->LabelSize->GetOutputPort());
  this->LabelPlacer->SetInputConnection(this->LabelHierarchy->GetOutputPort());
  this->LabelMapper->SetInputConnection(this->LabelPlacer->GetOutputPort(0));
  this->LabelMapper2D->SetInputConnection(this->LabelAppend->GetOutputPort());
  this->LabelActor->SetMapper(this->LabelMapper);

  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->Renderer->AddViewProp(this->Balloon);
  this->Balloon->SetRenderer(this->Renderer);
  this->Balloon->VisibilityOn();

  this->SelectionMode = SURFACE;
  this->LabelAppend->SetInputIdArrayName("ID");
  this->LabelSize->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText");
  this->LabelSize->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
  this->LabelSize->SetLabelSizeArrayName("LabelSize");
  this->LabelHierarchy->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority");
  this->LabelHierarchy->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize");
  this->LabelHierarchy->SetInputArrayToProcess(
    2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText");
  this->LabelHierarchy->SetInputArrayToProcess(
    3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
  this->LabelPlacer->SetRenderer(this->Renderer);
  this->LabelMapper->SetLabelMode(VTK_LABEL_FIELD_DATA);
  this->LabelMapper->SetFieldDataName("LabelText");
  this->LabelMapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
  this->LabelMapper2D->SetLabelMode(VTK_LABEL_FIELD_DATA);
  this->LabelMapper2D->SetFieldDataName("LabelText");
  this->LabelMapper2D->SetPriorityArrayName("Priority");
  this->LabelMapper2D->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");

#ifdef VTK_USE_QT
  this->QtLabelSize = vtkSmartPointer<vtkQtLabelSizeCalculator>::New();
  this->QtLabelHierarchy = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->QtLabelPlacer = vtkSmartPointer<vtkLabelPlacer>::New();
  this->QtLabelMapper = vtkSmartPointer<vtkQtLabelMapper>::New();

  this->QtLabelSize->SetInputConnection(this->LabelAppend->GetOutputPort());
  this->QtLabelHierarchy->AddInputConnection(0, this->QtLabelSize->GetOutputPort());
  this->QtLabelPlacer->SetInputConnection(this->QtLabelHierarchy->GetOutputPort());
  this->QtLabelMapper->SetInputConnection(this->QtLabelPlacer->GetOutputPort(0));

  this->QtLabelSize->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText");
  this->QtLabelSize->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
  this->QtLabelSize->SetLabelSizeArrayName("LabelSize");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
  this->QtLabelPlacer->SetRenderer(this->Renderer);

  this->QtLabelHierarchy->SetUseUnicodeStrings( true );
  this->QtLabelPlacer->SetUseUnicodeStrings( true );

  this->QtLabelMapper->SetLabelMode(VTK_LABEL_FIELD_DATA);
  this->QtLabelMapper->SetFieldDataName("LabelText");
  this->QtLabelMapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");

  this->QtLabelHierarchy->AddInputConnection(0, this->IconSize->GetOutputPort());
  this->QtLabelHierarchy->SetInputArrayToProcess(
    4, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconIndex");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    5, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconSize");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    6, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconIndex");
  this->QtLabelHierarchy->SetInputArrayToProcess(
    7, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
#endif

  this->LabelActor->PickableOff();
  this->Renderer->AddActor(this->LabelActor);

  this->IconSize->SetInputConnection(this->IconAppend->GetOutputPort());
  this->LabelHierarchy->AddInputConnection(0, this->IconSize->GetOutputPort());
  this->IconTransform->SetInputConnection(this->LabelPlacer->GetOutputPort(1));
  this->IconGlyph->SetInputConnection(this->IconTransform->GetOutputPort());
  this->IconMapper->SetInputConnection(this->IconGlyph->GetOutputPort());
  this->IconActor->SetMapper(this->IconMapper);

  this->IconAppend->SetInputIdArrayName("ID");
  this->IconSize->SetFunction("iHat + jHat");
  this->IconSize->SetResultArrayName("IconSize");
  this->IconSize->SetResultArrayType(VTK_INT);
  this->LabelHierarchy->SetInputArrayToProcess(
    4, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconIndex");
  this->LabelHierarchy->SetInputArrayToProcess(
    5, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconSize");
  this->LabelHierarchy->SetInputArrayToProcess(
    6, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconIndex");
  this->LabelHierarchy->SetInputArrayToProcess(
    7, 0, 1, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID");
  this->IconTransform->SetInputCoordinateSystemToWorld();
  this->IconTransform->SetOutputCoordinateSystemToDisplay();
  this->IconTransform->SetViewport(this->Renderer);
  this->IconGlyph->SetUseIconSize(true);
  this->IconGlyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "IconIndex");
  this->IconMapper->ScalarVisibilityOff();
  this->IconActor->PickableOff();
  this->Renderer->AddActor(this->IconActor);

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
  if (this->Transform)
    {
    this->Transform->Delete();
    }
  if (this->InteractorStyle)
    {
    this->InteractorStyle->RemoveObserver(this->GetObserver());
    this->InteractorStyle->Delete();
    }
}

void vtkRenderView::SetupRenderWindow(vtkRenderWindow* win)
{
  win->AddRenderer(this->Renderer);
  if (!win->GetInteractor())
    {
    vtkSmartPointer<vtkRenderWindowInteractor> iren =
      vtkSmartPointer<vtkRenderWindowInteractor>::New();
    win->SetInteractor(iren);
    iren->Initialize();
    }
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
}

vtkRenderWindow* vtkRenderView::GetRenderWindow()
{
  return this->Renderer->GetRenderWindow();
}

void vtkRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
    {
    this->InteractionMode = mode;
    if (mode == INTERACTION_MODE_2D)
      {
      if (this->InteractorStyle)
        {
        this->InteractorStyle->RemoveObserver(this->GetObserver());
        this->InteractorStyle->Delete();
        }
      vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
      this->InteractorStyle = style;
      style->SetRenderOnMouseMove(this->GetDisplayHoverText());
      this->InteractorStyle->AddObserver(
        vtkCommand::SelectionChangedEvent, this->GetObserver());
      this->Renderer->GetActiveCamera()->ParallelProjectionOn();
      }
    else if (mode == INTERACTION_MODE_3D)
      {
      if (this->InteractorStyle)
        {
        this->InteractorStyle->RemoveObserver(this->GetObserver());
        this->InteractorStyle->Delete();
        }
      vtkInteractorStyleRubberBand3D* style = vtkInteractorStyleRubberBand3D::New();
      this->InteractorStyle = style;
      this->InteractorStyle->AddObserver(
        vtkCommand::SelectionChangedEvent, this->GetObserver());
      this->Renderer->GetActiveCamera()->ParallelProjectionOff();
      }
    else
      {
      vtkErrorMacro("Unknown interaction mode.");
      }
    if (this->Renderer->GetRenderWindow())
      {
      this->Renderer->GetRenderWindow()->GetInteractor()->SetInteractorStyle(
        this->InteractorStyle);
      }
    }
}

void vtkRenderView::SetInteractorStyle(vtkInteractorStyle* style)
{
  if (!style)
    {
    vtkErrorMacro("Interactor style must not be null.");
    return;
    }
  if (style != this->InteractorStyle)
    {
    if (this->InteractorStyle)
      {
      this->InteractorStyle->RemoveObserver(this->GetObserver());
      this->InteractorStyle->Delete();
      }
    this->InteractorStyle = style;
    this->InteractorStyle->Register(this);
    this->InteractorStyle->AddObserver(
      vtkCommand::SelectionChangedEvent, this->GetObserver());
    vtkInteractorStyleRubberBand2D* style2D =
      vtkInteractorStyleRubberBand2D::SafeDownCast(this->InteractorStyle);
    vtkInteractorStyleRubberBand3D* style3D =
      vtkInteractorStyleRubberBand3D::SafeDownCast(this->InteractorStyle);
    if (style2D)
      {
      style2D->SetRenderOnMouseMove(this->GetDisplayHoverText());
      this->InteractionMode = INTERACTION_MODE_2D;
      }
    else if (style3D)
      {
      this->InteractionMode = INTERACTION_MODE_3D;
      }
    else
      {
      this->InteractionMode = INTERACTION_MODE_UNKNOWN;
      }
    }
}

void vtkRenderView::SetIconTexture(vtkTexture* texture)
{
  this->IconActor->SetTexture(texture);
}

vtkTexture* vtkRenderView::GetIconTexture()
{
  return this->IconActor->GetTexture();
}

void vtkRenderView::SetIconSize(int x, int y)
{
  this->IconGlyph->SetIconSize(x, y);
  vtkstd::string func = vtkVariant(x).ToString() + "*iHat + " +
                        vtkVariant(y).ToString() + "*jHat";
  this->IconSize->SetFunction(func.c_str());
}

int* vtkRenderView::GetIconSize()
{
  return this->IconGlyph->GetIconSize();
}

void vtkRenderView::AddLabels(vtkAlgorithmOutput* conn, vtkTextProperty* prop)
{
  this->LabelAppend->AddInputConnection(0, conn);
  int id = this->LabelAppend->GetNumberOfInputConnections(0) - 1;
  this->LabelSize->SetFontProperty(prop, id);
  this->LabelMapper->SetLabelTextProperty(prop, id);
  this->LabelMapper2D->SetLabelTextProperty(prop, id);
#ifdef VTK_USE_QT
  this->QtLabelSize->SetFontProperty(prop, id);
  this->QtLabelMapper->SetLabelTextProperty(prop, id);
#endif
}

void vtkRenderView::RemoveLabels(vtkAlgorithmOutput* conn)
{
  int numConn = this->LabelAppend->GetNumberOfInputConnections(0);
  int index = conn->GetIndex();
  if (this->LabelAppend->GetInputConnection(0, index) == conn)
    {
    this->LabelAppend->RemoveInputConnection(0, conn);
    int updatedNum = this->LabelAppend->GetNumberOfInputConnections(0);
    if (updatedNum != numConn - 1)
      {
      vtkErrorMacro("Labels must have been added more than once!");
      return;
      }
    for (int i = index; i < updatedNum; ++i)
      {
      vtkTextProperty* prop = this->LabelSize->GetFontProperty(i+1);
      this->LabelSize->SetFontProperty(prop, i);
      this->LabelMapper->SetLabelTextProperty(prop, i);
      this->LabelMapper2D->SetLabelTextProperty(prop, i);
#ifdef VTK_USE_QT
      this->QtLabelSize->SetFontProperty(prop, i);
      this->QtLabelMapper->SetLabelTextProperty(prop, i);
#endif
      }
    }
}

void vtkRenderView::AddIcons(vtkAlgorithmOutput* conn)
{
  this->IconAppend->AddInputConnection(0, conn);
}

void vtkRenderView::RemoveIcons(vtkAlgorithmOutput* conn)
{
  this->IconAppend->RemoveInputConnection(0, conn);
}

void vtkRenderView::ProcessEvents(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  if (caller == this->Renderer && (eventId == vtkCommand::StartEvent ||
      eventId == vtkCommand::ComputeVisiblePropBoundsEvent))
    {
    this->PrepareForRendering();
    }
  if (vtkDataRepresentation::SafeDownCast(caller) &&
      eventId == vtkCommand::SelectionChangedEvent)
    {
    this->Render();
    }
  else if (caller == this->InteractorStyle &&
           eventId == vtkCommand::SelectionChangedEvent)
    {
    vtkSmartPointer<vtkSelection> selection =
      vtkSmartPointer<vtkSelection>::New();
    this->GenerateSelection(callData, selection);
    // Call select on the representation(s)
    for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
      {
      this->GetRepresentation(i)->Select(this, selection);
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

void vtkRenderView::RepresentationSelectionChanged(vtkDataRepresentation* vtkNotUsed(rep), vtkSelection* vtkNotUsed(selection))
{
  this->Render();
}

void vtkRenderView::Render()
{
  if (this->Renderer->GetRenderWindow())
    {
    this->Renderer->ResetCameraClippingRange();
    this->Renderer->GetRenderWindow()->Render();
    }
}

void vtkRenderView::SetDisplayHoverText(bool b)
{
  this->Balloon->SetVisibility(b);
  vtkInteractorStyleRubberBand2D* style2D =
    vtkInteractorStyleRubberBand2D::SafeDownCast(this->InteractorStyle);
  if (style2D)
    {
    style2D->SetRenderOnMouseMove(b);
    }
  this->DisplayHoverText = b;
}

void vtkRenderView::PrepareForRendering()
{
  this->Update();
  if (this->IconActor->GetTexture() &&
      this->IconActor->GetTexture()->GetInput())
    {
    this->IconActor->GetTexture()->MapColorScalarsThroughLookupTableOff();
    this->IconActor->GetTexture()->GetInput()->Update();
    int* dim = this->IconActor->GetTexture()->GetInput()->GetDimensions();
    this->IconGlyph->SetIconSheetSize(dim);
    }

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
  if (this->InteractorStyle && this->InteractorStyle->GetInteractor())
    {
    this->InteractorStyle->GetInteractor()->GetEventPosition(pos);
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
        if (this->InteractorStyle && this->InteractorStyle->GetInteractor())
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
  this->SetLabelPlacementAndRenderMode(mode, this->GetLabelRenderMode());
}

void vtkRenderView::SetLabelRenderMode(int mode)
{
  this->SetLabelPlacementAndRenderMode(this->GetLabelPlacementMode(), mode);
}

void vtkRenderView::SetLabelPlacementAndRenderMode( int placement_mode, int render_mode )
{
  //First, make sure the render mode is set on all the representations.
  // TODO: Setup global labeller render mode
  if( render_mode != this->LabelRenderMode )
    {
    this->LabelRenderMode = render_mode;
    
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

  //Now, setup the pipeline for the various permutations of placement mode 
  // and render modes...
  switch( this->LabelRenderMode )
    {
    case QT:
      if (placement_mode == LABEL_PLACER)
        {
#ifdef VTK_USE_QT
        this->LabelActor->SetMapper(this->QtLabelMapper);
        this->QtLabelPlacer->SetOutputCoordinateSystem( vtkLabelPlacer::DISPLAY );
        this->QtLabelMapper->SetInputConnection(this->QtLabelPlacer->GetOutputPort(0));
        this->IconTransform->SetInputConnection(this->QtLabelPlacer->GetOutputPort(1));
#else
      vtkErrorMacro("Qt label rendering not supported.");
#endif
        }
      else if (placement_mode == DYNAMIC_2D)
        {
#ifdef VTK_USE_QT
//FIXME - Qt label rendering doesn't have a dynamic2D label mapper; so,
//  use freetype dynamic labeling with an error...
        vtkErrorMacro("Qt-based label rendering is not available with dynamic 2D label placement.  Using freetype-based label rendering.\n");
        this->QtLabelPlacer->SetOutputCoordinateSystem( vtkLabelPlacer::DISPLAY );
        this->LabelActor->SetMapper(this->QtLabelMapper);
        this->QtLabelMapper->SetInputConnection(this->QtLabelPlacer->GetOutputPort(0));
        this->IconTransform->SetInputConnection(this->QtLabelPlacer->GetOutputPort(1));

#else
     vtkErrorMacro("Qt label rendering not supported.");
#endif
        }
      else
        {
#ifdef VTK_USE_QT
        this->LabelActor->SetMapper(this->QtLabelMapper);
        this->QtLabelPlacer->SetOutputCoordinateSystem( vtkLabelPlacer::DISPLAY );
        this->QtLabelMapper->SetInputConnection(this->LabelAppend->GetOutputPort());
        this->IconTransform->SetInputConnection(this->IconSize->GetOutputPort());

#else
      vtkErrorMacro("Qt label rendering not supported.");
#endif
        }
      break;
    default:
      /*
        <graphviz>
        // If labeller is LABEL_PLACER:
        digraph {
        LabelAppend -> LabelSize -> LabelHierarchy
        LabelHierarchy -> LabelPlacer
        LabelPlacer -> LabelMapper -> LabelActor
        IconAppend -> IconSize -> LabelHierarchy
        LabelPlacer -> IconTransform -> IconGlyph -> IconMapper -> IconActor
        }
        // If labeller is DYNAMIC_2D:
        digraph {
        LabelAppend -> LabelMapper2D -> LabelActor
        IconAppend -> IconSize -> IconTransform -> IconGlyph -> IconMapper -> IconActor
        }
        // If labeller is ALL:
        digraph {
        LabelAppend -> LabelMapper -> LabelActor
        IconAppend -> IconSize -> IconTransform -> IconGlyph -> IconMapper -> IconActor
        }
        </graphviz>
      */
      if (placement_mode == LABEL_PLACER)
        {
        this->LabelActor->SetMapper(this->LabelMapper);
        this->LabelMapper->SetInputConnection(this->LabelPlacer->GetOutputPort(0));
        this->IconTransform->SetInputConnection(this->LabelPlacer->GetOutputPort(1));
        }
      else if (placement_mode == DYNAMIC_2D)
        {
        this->LabelActor->SetMapper(this->LabelMapper2D);
        this->LabelMapper->SetInputConnection(this->LabelAppend->GetOutputPort());
        this->IconTransform->SetInputConnection(this->IconSize->GetOutputPort());
        }
      else
        {
        this->LabelActor->SetMapper(this->LabelMapper);
        this->LabelMapper->SetInputConnection(this->LabelAppend->GetOutputPort());
        this->IconTransform->SetInputConnection(this->IconSize->GetOutputPort());
        }
    }
}
  
int vtkRenderView::GetLabelPlacementMode()
{
  if (this->LabelActor->GetMapper() == this->LabelMapper2D.GetPointer())
    {
    return DYNAMIC_2D;
    }
  return LABEL_PLACER;
}

void vtkRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
  os << indent << "InteractorStyle: ";
  if (this->InteractorStyle)
    {
    os << "\n";
    this->InteractorStyle->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}
