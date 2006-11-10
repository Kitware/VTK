/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTreeMapHover.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractorStyleTreeMapHover.h"

#include "vtkActor.h"
#include "vtkBalloonRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTreeMapLayout.h"
#include "vtkTreeMapToPolyData.h"
#include "vtkWorldPointPicker.h"

vtkCxxRevisionMacro(vtkInteractorStyleTreeMapHover, "1.5");
vtkStandardNewMacro(vtkInteractorStyleTreeMapHover);

//----------------------------------------------------------------------------

vtkInteractorStyleTreeMapHover::vtkInteractorStyleTreeMapHover()
{
  this->Picker = vtkWorldPointPicker::New();
  this->Balloon = vtkBalloonRepresentation::New();
  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  //this->Balloon->SetNeedToRender(true);
  this->Layout = NULL;
  this->LabelField = 0;
  this->CurrentSelectedId = -1;
  this->TreeMapToPolyData = NULL;
  this->Layout = NULL;

  //Setup up pipelines for highlighting and selecting nodes
  this->SelectionPoints = vtkPoints::New();
  this->SelectionPoints->SetNumberOfPoints(5);
  this->HighlightPoints = vtkPoints::New();
  this->HighlightPoints->SetNumberOfPoints(5);
  vtkCellArray *selA = vtkCellArray::New();
  selA->InsertNextCell(5);
  vtkCellArray *highA = vtkCellArray::New();
  highA->InsertNextCell(5);
  int i;
  for (i = 0; i < 5; ++i)
    {
    selA->InsertCellPoint(i);
    highA->InsertCellPoint(i);
    }
  vtkPolyData  *selData = vtkPolyData::New();
  selData->SetPoints(this->SelectionPoints);
  selData->SetLines(selA);
  vtkPolyDataMapper *selMap = vtkPolyDataMapper::New();
  selMap->SetInput(selData);
  this->SelectionActor = vtkActor::New();
  this->SelectionActor->SetMapper(selMap);
  this->SelectionActor->VisibilityOff();
  this->SelectionActor->PickableOff();
  this->SelectionActor->GetProperty()->SetLineWidth(2.0);
  vtkPolyData  *highData = vtkPolyData::New();
  highData->SetPoints(this->HighlightPoints);
  highData->SetLines(highA);
  vtkPolyDataMapper *highMap = vtkPolyDataMapper::New();
  highMap->SetInput(highData);
  this->HighlightActor = vtkActor::New();
  this->HighlightActor->SetMapper(highMap);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetColor(1, 1, 1);
  this->HighlightActor->GetProperty()->SetLineWidth(1.0);
  selA->Delete();
  selData->Delete();
  selMap->Delete();
  highA->Delete();
  highData->Delete();
  highMap->Delete();
}

//----------------------------------------------------------------------------

vtkInteractorStyleTreeMapHover::~vtkInteractorStyleTreeMapHover()
{
  this->SelectionPoints->Delete();
  this->HighlightPoints->Delete();
  this->SelectionActor->Delete();
  this->HighlightActor->Delete();
  this->Picker->Delete();
  this->Balloon->Delete();
  if (this->Layout != NULL)
    {
    this->Layout->Delete();
    this->Layout = NULL;
    }
  if (this->TreeMapToPolyData != NULL)
    {
    this->TreeMapToPolyData->Delete();
    this->TreeMapToPolyData = NULL;
    }
  this->SetLabelField(0);
}

vtkCxxSetObjectMacro(vtkInteractorStyleTreeMapHover, Layout, vtkTreeMapLayout);

vtkCxxSetObjectMacro(vtkInteractorStyleTreeMapHover, TreeMapToPolyData, vtkTreeMapToPolyData);

void vtkInteractorStyleTreeMapHover::SetInteractor(vtkRenderWindowInteractor
                                                   *rwi)
{
  // See if we already had one
  vtkRenderWindowInteractor *mrwi = this->GetInteractor();
  vtkRenderer *ren;
  if (mrwi && mrwi->GetRenderWindow()) 
    {
    ren = mrwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    if (ren) 
      {
      ren->RemoveActor(SelectionActor);
      ren->RemoveActor(HighlightActor);
      }
    }
  vtkInteractorStyleImage::SetInteractor(rwi);
  if (rwi && rwi->GetRenderWindow())
    {
    ren = rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    if (ren) 
      {
      ren->AddActor(SelectionActor);
      ren->AddActor(HighlightActor);
      }
    }
}

//----------------------------------------------------------------------------

void vtkInteractorStyleTreeMapHover::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Layout: " << (this->Layout ? "" : "(none)") << endl;
  if (this->Layout)
    {
    this->Layout->PrintSelf(os, indent.GetNextIndent());
    }

  os << indent << "TreeMapToPolyData: " << (this->TreeMapToPolyData ? "" : "(none)") << endl;
  if (this->TreeMapToPolyData)
    {
    this->TreeMapToPolyData->PrintSelf(os, indent.GetNextIndent());
    }

  os << indent << "LabelField: " << (this->LabelField ? this->LabelField : "(none)") << endl;
}

vtkIdType vtkInteractorStyleTreeMapHover::GetTreeMapIdAtPos(int x, int y)
{
  vtkIdType id=-1;
  
  vtkRenderer* r = this->GetInteractor()->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
  if (r == NULL)
    {
    return id;
    }

  // Use the hardware picker to find a point in world coordinates.
  this->Picker->Pick(x, y, 0, r);
  double pos[3];
  this->Picker->GetPickPosition(pos);
  
  if (this->Layout != NULL)
    {
    float posFloat[3];
    for (int i = 0; i < 3; i++)
      {
      posFloat[i] = pos[i];
      }
    id = Layout->FindNode(posFloat);
    }
    
  return id;
}

void vtkInteractorStyleTreeMapHover::GetBoundingBoxForTreeMapItem(vtkIdType id, float *binfo)
{
  if (this->Layout)
    {
    this->Layout->GetBoundingBox(id, binfo);
    }
}

void vtkInteractorStyleTreeMapHover::OnMouseMove()
{
  vtkRenderer* r = this->GetInteractor()->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
  if (r == NULL)
    {
    return;
    }

  if (!r->HasViewProp(this->Balloon))
    {
    r->AddActor(this->Balloon);
    this->Balloon->SetRenderer(r);
    }

  // Use the hardware picker to find a point in world coordinates.
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  float binfo[4];
  vtkIdType id = this->GetTreeMapIdAtPos(x,y);
  this->GetBoundingBoxForTreeMapItem(id,binfo);
  
  double loc[2] = {x, y};
  this->Balloon->EndWidgetInteraction(loc);
  
  if ((this->Layout!=NULL) && (this->Layout->GetOutput()!=NULL))
    {

    vtkAbstractArray* absArray = this->Layout->GetOutput()->GetPointData()->GetAbstractArray(this->LabelField);
    if (absArray != NULL)
      {
      vtkStringArray* strArray = vtkStringArray::SafeDownCast(absArray);
      if ((id > -1) && (strArray!=NULL))
        {
        this->Balloon->SetBalloonText(strArray->GetValue(id));
        vtkTree* tree = this->Layout->GetOutput();
        double z;
        if (this->TreeMapToPolyData != NULL)
          {
          z = this->TreeMapToPolyData->GetLevelDeltaZ() 
            * (tree->GetLevel(id) + 1);
          }
        else
          {
          z = 0.02;
          }
        this->HighlightPoints->SetPoint(0, binfo[0], binfo[2], z);
        this->HighlightPoints->SetPoint(1, binfo[1], binfo[2], z);
        this->HighlightPoints->SetPoint(2, binfo[1], binfo[3], z);
        this->HighlightPoints->SetPoint(3, binfo[0], binfo[3], z);
        this->HighlightPoints->SetPoint(4, binfo[0], binfo[2], z);
        this->HighlightPoints->Modified();
        this->HighlightActor->VisibilityOn();
        }
      else
        {
        this->Balloon->SetBalloonText("");
              HighlightActor->VisibilityOff();
        }
      }

    this->Balloon->StartWidgetInteraction(loc);

    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    this->Superclass::OnMouseMove();
    this->GetInteractor()->Render();
    }
}

void vtkInteractorStyleTreeMapHover::SetHighLightColor(double r, 
                                                       double g, double b)
{
  this->HighlightActor->GetProperty()->SetColor(r, g, b);
}

void vtkInteractorStyleTreeMapHover::SetSelectionLightColor(double r, 
                                                            double g, double b)
{
  this->SelectionActor->GetProperty()->SetColor(r, g, b);
}

void vtkInteractorStyleTreeMapHover::SetHighLightWidth(double lw)
{
  this->HighlightActor->GetProperty()->SetLineWidth(lw);
}

double vtkInteractorStyleTreeMapHover::GetHighLightWidth()
{
  return this->HighlightActor->GetProperty()->GetLineWidth();
}

void vtkInteractorStyleTreeMapHover::SetSelectionWidth(double lw)
{
  this->SelectionActor->GetProperty()->SetLineWidth(lw);
}

double vtkInteractorStyleTreeMapHover::GetSelectionWidth()
{
  return this->SelectionActor->GetProperty()->GetLineWidth();
}

//---------------------------------------------------------------------------
void vtkInteractorStyleTreeMapHover::OnLeftButtonUp()
{

  // Get the id of the object underneath the mouse
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->CurrentSelectedId = GetTreeMapIdAtPos(x,y);
  this->HighLightCurrentSelectedItem();

  // Get the pedigree id of this object and
  // send out an event with that id as data
  vtkAbstractArray* absArray = 
    this->Layout->GetOutput()->GetPointData()->GetAbstractArray(
      "PedigreeNodeId");
  if (absArray)
    {
    vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(absArray);
    if (idArray)
      {
      vtkIdType pedigreeId = idArray->GetValue(this->CurrentSelectedId);
      this->InvokeEvent(vtkCommand::UserEvent, &pedigreeId);
      }
    }

  Superclass::OnLeftButtonUp();
}

void vtkInteractorStyleTreeMapHover::HighLightItem(vtkIdType id)
{
  this->CurrentSelectedId = id;
  this->HighLightCurrentSelectedItem();
}

void vtkInteractorStyleTreeMapHover::HighLightCurrentSelectedItem()
{
  float binfo[4];
  this->GetBoundingBoxForTreeMapItem(this->CurrentSelectedId,binfo);
  if (this->CurrentSelectedId > -1)
    {
    vtkTree* tree = this->Layout->GetOutput();
    double z;
    if (this->TreeMapToPolyData != NULL)
      {
      z = this->TreeMapToPolyData->GetLevelDeltaZ() 
        * (tree->GetLevel(this->CurrentSelectedId) + 1);
      }
    else
      {
      z = 0.01;
      }
    this->SelectionPoints->SetPoint(0, binfo[0], binfo[2], z);
    this->SelectionPoints->SetPoint(1, binfo[1], binfo[2], z);
    this->SelectionPoints->SetPoint(2, binfo[1], binfo[3], z);
    this->SelectionPoints->SetPoint(3, binfo[0], binfo[3], z);
    this->SelectionPoints->SetPoint(4, binfo[0], binfo[2], z);
    this->SelectionPoints->Modified();
    this->SelectionActor->VisibilityOn();
    }
  else
    {
    SelectionActor->VisibilityOff();
    }
  if (this->GetInteractor())
    {
    this->GetInteractor()->Render();
    }
}
