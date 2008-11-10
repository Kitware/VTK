/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTreeRingHover.cxx

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

#include "vtkInteractorStyleTreeRingHover.h"

#include "vtkActor.h"
#include "vtkBalloonRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
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
#include "vtkTreeRingLayout.h"
#include "vtkWorldPointPicker.h"
#include "vtkVariant.h"
#include "vtkSectorSource.h"
#include "vtkExtractEdges.h"
#include "vtkAppendPolyData.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkInteractorStyleTreeRingHover, "1.5");
vtkStandardNewMacro(vtkInteractorStyleTreeRingHover);

vtkCxxSetObjectMacro(vtkInteractorStyleTreeRingHover, Layout, vtkTreeRingLayout);

//----------------------------------------------------------------------------
vtkInteractorStyleTreeRingHover::vtkInteractorStyleTreeRingHover()
{
  this->Picker = vtkWorldPointPicker::New();
  this->Balloon = vtkBalloonRepresentation::New();
  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->Layout = NULL;
  this->LabelField = 0;
  this->CurrentSelectedId = -1;

  this->SelectionData = vtkPolyData::New();
  vtkPolyDataMapper *selMap = vtkPolyDataMapper::New();
  selMap->SetInput(this->SelectionData);
  this->SelectionActor = vtkActor::New();
  this->SelectionActor->SetMapper(selMap);
  this->SelectionActor->VisibilityOff();
  this->SelectionActor->PickableOff();
  this->SelectionActor->GetProperty()->SetLineWidth(4.0);

  this->HighlightData = vtkPolyData::New();
  vtkPolyDataMapper *highMap = vtkPolyDataMapper::New();
  highMap->SetInput(this->HighlightData);
  this->HighlightActor = vtkActor::New();
  this->HighlightActor->SetMapper(highMap);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetColor(0, 0, 0);
  this->HighlightActor->GetProperty()->SetLineWidth(2.0);
  selMap->Delete();
  highMap->Delete();
}

//----------------------------------------------------------------------------
vtkInteractorStyleTreeRingHover::~vtkInteractorStyleTreeRingHover()
{
  this->SelectionData->Delete();
  this->HighlightData->Delete();
  this->SelectionActor->Delete();
  this->HighlightActor->Delete();
  this->Picker->Delete();
  this->Balloon->Delete();
  if (this->Layout != NULL)
    {
    this->Layout->Delete();
    this->Layout = NULL;
    }
  this->SetLabelField(0);
}

void vtkInteractorStyleTreeRingHover::SetInteractor(vtkRenderWindowInteractor *rwi)
{
  // See if we already had one
  vtkRenderWindowInteractor *mrwi = this->GetInteractor();
  vtkRenderer *ren;
  if (mrwi && mrwi->GetRenderWindow()) 
    {
    this->FindPokedRenderer(0, 0);
    ren = this->CurrentRenderer;
    if (ren) 
      {
      ren->RemoveActor(SelectionActor);
      ren->RemoveActor(HighlightActor);
      }
    }
  vtkInteractorStyleImage::SetInteractor(rwi);
  if (rwi && rwi->GetRenderWindow())
    {
    this->FindPokedRenderer(0, 0);
    ren = this->CurrentRenderer;
    if (ren) 
      {
      ren->AddActor(SelectionActor);
      ren->AddActor(HighlightActor);
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTreeRingHover::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Layout: " << (this->Layout ? "" : "(none)") << endl;
  if (this->Layout)
    {
    this->Layout->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "LabelField: " << (this->LabelField ? this->LabelField : "(none)") << endl;
}

vtkIdType vtkInteractorStyleTreeRingHover::GetTreeRingIdAtPos(int x, int y)
{
  vtkIdType id=-1;
  
  vtkRenderer* r = this->CurrentRenderer;
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
    id = Layout->FindVertex(posFloat);
    }
    
  return id;
}

void vtkInteractorStyleTreeRingHover::GetBoundingSectorForTreeRingItem(vtkIdType id, float *sinfo)
{
  if (this->Layout)
    {
    this->Layout->GetBoundingSector(id, sinfo);
    }
}

void vtkInteractorStyleTreeRingHover::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);
  vtkRenderer* r = this->CurrentRenderer;
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
  float sinfo[4];
  vtkIdType id = this->GetTreeRingIdAtPos(x,y);
  
  if( id != -1 )
  {
    this->GetBoundingSectorForTreeRingItem(id,sinfo);
  }

  double loc[2] = {x, y};
  this->Balloon->EndWidgetInteraction(loc);
  
  if ((this->Layout!=NULL) && (this->Layout->GetOutput()!=NULL))
    {

    vtkAbstractArray* absArray = this->Layout->GetOutput()->GetVertexData()->GetAbstractArray(this->LabelField);
      //find the information for the correct sector,
      //  unless there isn't a sector or it is the root node
//    if (absArray != NULL && id > 0 )  
    if (absArray != NULL && id > -1 )
      {
      vtkStdString str;
      if (vtkStringArray::SafeDownCast(absArray))
        {
        str = vtkStringArray::SafeDownCast(absArray)->GetValue(id);
        }
      if (vtkDataArray::SafeDownCast(absArray))
        {
        str = vtkVariant(vtkDataArray::SafeDownCast(absArray)->GetTuple(id)[0]).ToString();
        }
      this->Balloon->SetBalloonText(str);
      double z = 0.02;
//FIXME-jfsheph Need to generate the edge data directly...
//       float binfo[8];
//       binfo[0] = sinfo[2]*cos(vtkMath::DegreesToRadians()*sinfo[0]);
//       binfo[1] = sinfo[2]*sin(vtkMath::DegreesToRadians()*sinfo[0]);
//       binfo[2] = sinfo[3]*cos(vtkMath::DegreesToRadians()*sinfo[0]);
//       binfo[3] = sinfo[3]*sin(vtkMath::DegreesToRadians()*sinfo[0]);
//       binfo[4] = sinfo[3]*cos(vtkMath::DegreesToRadians()*sinfo[1]);
//       binfo[5] = sinfo[3]*sin(vtkMath::DegreesToRadians()*sinfo[1]);
//       binfo[6] = sinfo[2]*cos(vtkMath::DegreesToRadians()*sinfo[1]);
//       binfo[7] = sinfo[2]*sin(vtkMath::DegreesToRadians()*sinfo[1]);

      VTK_CREATE(vtkSectorSource, sector);
      sector->SetInnerRadius(sinfo[2]);
      sector->SetOuterRadius(sinfo[3]);
      sector->SetZCoord(z);
      sector->SetStartAngle(sinfo[0]);
      sector->SetEndAngle(sinfo[1]);
//FIXME-jfsheph Are we satisfied with this level of resolution?
      int resolution = (int)((sinfo[1]-sinfo[0])/1);
      if( resolution < 1 )
          resolution = 1;
      sector->SetCircumferentialResolution(resolution);
      sector->Update();
    
      VTK_CREATE(vtkExtractEdges, extract);
      extract->SetInput(sector->GetOutput());
      
      VTK_CREATE(vtkAppendPolyData, append);
      append->AddInput(extract->GetOutput());
      append->Update();
      
      this->HighlightData->ShallowCopy(append->GetOutput());
      this->HighlightActor->VisibilityOn();
      }
    else
      {
      this->Balloon->SetBalloonText("");
      HighlightActor->VisibilityOff();
      }

    this->Balloon->StartWidgetInteraction(loc);

    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    this->Superclass::OnMouseMove();
    this->GetInteractor()->Render();
    }
}

void vtkInteractorStyleTreeRingHover::SetHighLightColor(double r, 
                                                       double g, double b)
{
  this->HighlightActor->GetProperty()->SetColor(r, g, b);
}

void vtkInteractorStyleTreeRingHover::SetSelectionLightColor(double r, 
                                                            double g, double b)
{
  this->SelectionActor->GetProperty()->SetColor(r, g, b);
}

void vtkInteractorStyleTreeRingHover::SetHighLightWidth(double lw)
{
  this->HighlightActor->GetProperty()->SetLineWidth(lw);
}

double vtkInteractorStyleTreeRingHover::GetHighLightWidth()
{
  return this->HighlightActor->GetProperty()->GetLineWidth();
}

void vtkInteractorStyleTreeRingHover::SetSelectionWidth(double lw)
{
  this->SelectionActor->GetProperty()->SetLineWidth(lw);
}

double vtkInteractorStyleTreeRingHover::GetSelectionWidth()
{
  return this->SelectionActor->GetProperty()->GetLineWidth();
}

//---------------------------------------------------------------------------
void vtkInteractorStyleTreeRingHover::OnLeftButtonUp()
{
  // Get the id of the object underneath the mouse
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  this->CurrentSelectedId = GetTreeRingIdAtPos(x,y);

  // Get the pedigree id of this object and
  // send out an event with that id as data
  vtkIdType id = this->CurrentSelectedId;
  vtkAbstractArray* absArray = 
    this->Layout->GetOutput()->GetVertexData()->GetAbstractArray(
      "PedigreeVertexId");
  if (absArray)
    {
    vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(absArray);
    if (idArray)
      {
      id = idArray->GetValue(this->CurrentSelectedId);
      }
    }
  this->InvokeEvent(vtkCommand::UserEvent, &id);

  this->HighLightCurrentSelectedItem();
  Superclass::OnLeftButtonUp();
}

void vtkInteractorStyleTreeRingHover::HighLightItem(vtkIdType id)
{
  this->CurrentSelectedId = id;
  this->HighLightCurrentSelectedItem();
}

void vtkInteractorStyleTreeRingHover::HighLightCurrentSelectedItem()
{
    //FIXME-jfsheph Need to generate the edges only rather than extracting them from the sector data...
  float sinfo[4];

    //don't worry about selections in non-drawn regions or 
    // in the root nodes sector region...
//  if (this->CurrentSelectedId > 0)
  if (this->CurrentSelectedId > -1)
    {
    this->GetBoundingSectorForTreeRingItem(this->CurrentSelectedId,sinfo);

    double z = 0.01;
//     float binfo[8];
//     binfo[0] = sinfo[0]*cos(vtkMath::DegreesToRadians()*sinfo[2]);
//     binfo[1] = sinfo[0]*sin(vtkMath::DegreesToRadians()*sinfo[2]);
//     binfo[2] = sinfo[1]*cos(vtkMath::DegreesToRadians()*sinfo[2]);
//     binfo[3] = sinfo[1]*sin(vtkMath::DegreesToRadians()*sinfo[2]);
//     binfo[4] = sinfo[1]*cos(vtkMath::DegreesToRadians()*sinfo[3]);
//     binfo[5] = sinfo[1]*sin(vtkMath::DegreesToRadians()*sinfo[3]);
//     binfo[6] = sinfo[0]*cos(vtkMath::DegreesToRadians()*sinfo[3]);
//     binfo[7] = sinfo[0]*sin(vtkMath::DegreesToRadians()*sinfo[3]);
 
    VTK_CREATE(vtkSectorSource, sector);
    sector->SetInnerRadius(sinfo[2]);
    sector->SetOuterRadius(sinfo[3]);
    sector->SetZCoord(z);
    sector->SetStartAngle(sinfo[0]);
    sector->SetEndAngle(sinfo[1]);
//FIXME-jfsheph - Are we satisfied with this level of resolution?
    int resolution = (int)((sinfo[1]-sinfo[0])/1);
      if( resolution < 1 )
          resolution = 1;
      sector->SetCircumferentialResolution(resolution);
    sector->Update();
    
    VTK_CREATE(vtkExtractEdges, extract);
    extract->SetInput(sector->GetOutput());
    
    VTK_CREATE(vtkAppendPolyData, append);
    append->AddInput(extract->GetOutput());
    append->Update();

    this->SelectionData->ShallowCopy(append->GetOutput());

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
