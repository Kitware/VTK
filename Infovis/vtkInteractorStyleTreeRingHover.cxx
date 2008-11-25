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

vtkCxxRevisionMacro(vtkInteractorStyleTreeRingHover, "1.7");
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
  this->UseRectangularCoordinates = false;
  
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
  os << indent << "UseRectangularCoordinates: " << this->UseRectangularCoordinates << endl;
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
    
    if( this->UseRectangularCoordinates )
    {
      id = Layout->FindVertexRectangular(posFloat);
    }
    else
    {
      id = Layout->FindVertex(posFloat);
    }
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
      if( this->UseRectangularCoordinates )
      {
        VTK_CREATE(vtkPoints, highlightPoints);
        highlightPoints->SetNumberOfPoints(5);

        VTK_CREATE(vtkCellArray, highA);
        highA->InsertNextCell(5);
        for( int i = 0; i < 5; ++i)
        {
          highA->InsertCellPoint(i);
        }
        highlightPoints->SetPoint(0, sinfo[0], sinfo[2], z);
        highlightPoints->SetPoint(1, sinfo[1], sinfo[2], z);
        highlightPoints->SetPoint(2, sinfo[1], sinfo[3], z);
        highlightPoints->SetPoint(3, sinfo[0], sinfo[3], z);
        highlightPoints->SetPoint(4, sinfo[0], sinfo[2], z);
        this->HighlightData->SetPoints(highlightPoints);
        this->HighlightData->SetLines(highA);
        this->HighlightActor->VisibilityOn();
      }
      else
      {
        if( sinfo[1] - sinfo[0] != 360. )
        {
          VTK_CREATE(vtkSectorSource, sector);
          sector->SetInnerRadius(sinfo[2]);
          sector->SetOuterRadius(sinfo[3]);
          sector->SetZCoord(z);
          sector->SetStartAngle(sinfo[0]);
          sector->SetEndAngle(sinfo[1]);
          
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
          VTK_CREATE(vtkPoints, highlightPoints);
          highlightPoints->SetNumberOfPoints(240);

          double conversion = vtkMath::Pi()/180.;
          double current_angle = 0.;
          
          VTK_CREATE(vtkCellArray, highA);
          for( int i = 0; i < 120; ++i)
          {
            highA->InsertNextCell(2);
            double current_x = sinfo[2]*cos(conversion*current_angle);
            double current_y = sinfo[2]*sin(conversion*current_angle);
            highlightPoints->SetPoint( i, current_x, current_y, z );

            current_angle += 3.;

            highA->InsertCellPoint(i);
            highA->InsertCellPoint((i+1)%120);
          }
          
          current_angle = 0.;
          for( int i = 0; i < 120; ++i)
          {
            highA->InsertNextCell(2);
            double current_x = sinfo[3]*cos(conversion*current_angle);
            double current_y = sinfo[3]*sin(conversion*current_angle);
            highlightPoints->SetPoint( 120+i, current_x, current_y, z );

            current_angle += 3.;

            highA->InsertCellPoint(120+i);
            highA->InsertCellPoint(120+((i+1)%120));
          }
          this->HighlightData->SetPoints(highlightPoints);
          this->HighlightData->SetLines(highA);
          this->HighlightActor->VisibilityOn();
        }  
      }
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

void vtkInteractorStyleTreeRingHover::SetHighLightColor(double r, double g, double b)
{
  this->HighlightActor->GetProperty()->SetColor(r, g, b);
}

void vtkInteractorStyleTreeRingHover::SetSelectionLightColor(double r, double g, double b)
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
  float sinfo[4];

    //don't worry about selections in non-drawn regions or 
    // in the root nodes sector region...
//  if (this->CurrentSelectedId > 0)
  if (this->CurrentSelectedId > -1)
  {
    this->GetBoundingSectorForTreeRingItem(this->CurrentSelectedId,sinfo);

    double z = 0.01;
    
    if( this->UseRectangularCoordinates )
    {
      VTK_CREATE(vtkPoints, selectionPoints);
      selectionPoints->SetNumberOfPoints(5);
      
      VTK_CREATE(vtkCellArray, highA);
      highA->InsertNextCell(5);
      for( int i = 0; i < 5; ++i)
      {
        highA->InsertCellPoint(i);
      }
      selectionPoints->SetPoint(0, sinfo[0], sinfo[2], z);
      selectionPoints->SetPoint(1, sinfo[1], sinfo[2], z);
      selectionPoints->SetPoint(2, sinfo[1], sinfo[3], z);
      selectionPoints->SetPoint(3, sinfo[0], sinfo[3], z);
      selectionPoints->SetPoint(4, sinfo[0], sinfo[2], z);
      this->SelectionData->SetPoints(selectionPoints);
      this->SelectionData->SetLines(highA);
      this->SelectionActor->VisibilityOn();
    }
    else
    {
      if( sinfo[1] - sinfo[0] != 360. )
      {
        VTK_CREATE(vtkSectorSource, sector);
        sector->SetInnerRadius(sinfo[2]);
        sector->SetOuterRadius(sinfo[3]);
        sector->SetZCoord(z);
        sector->SetStartAngle(sinfo[0]);
        sector->SetEndAngle(sinfo[1]);
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
        VTK_CREATE(vtkPoints, selectionPoints);
        selectionPoints->SetNumberOfPoints(240);
        
        double conversion = vtkMath::Pi()/180.;
        double current_angle = 0.;
        
        VTK_CREATE(vtkCellArray, selectionA);
        for( int i = 0; i < 120; ++i)
        {
          selectionA->InsertNextCell(2);
          double current_x = sinfo[2]*cos(conversion*current_angle);
          double current_y = sinfo[2]*sin(conversion*current_angle);
          selectionPoints->SetPoint( i, current_x, current_y, z );
          
          current_angle += 3.;
          
          selectionA->InsertCellPoint(i);
          selectionA->InsertCellPoint((i+1)%120);
        }
        
        current_angle = 0.;
        for( int i = 0; i < 120; ++i)
        {
          selectionA->InsertNextCell(2);
          double current_x = sinfo[3]*cos(conversion*current_angle);
          double current_y = sinfo[3]*sin(conversion*current_angle);
          selectionPoints->SetPoint( 120+i, current_x, current_y, z );
          
          current_angle += 3.;
          
          selectionA->InsertCellPoint(120+i);
          selectionA->InsertCellPoint(120+((i+1)%120));
        }
        this->SelectionData->SetPoints(selectionPoints);
        this->SelectionData->SetLines(selectionA);
        this->SelectionActor->VisibilityOn();
      }  
    }
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
