/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleAreaSelectHover.cxx

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

#include "vtkInteractorStyleAreaSelectHover.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkAreaLayout.h"
#include "vtkBalloonRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkExtractEdges.h"
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
#include "vtkSectorSource.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkWorldPointPicker.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkInteractorStyleAreaSelectHover);

vtkCxxSetObjectMacro(vtkInteractorStyleAreaSelectHover, Layout, vtkAreaLayout);

//----------------------------------------------------------------------------
vtkInteractorStyleAreaSelectHover::vtkInteractorStyleAreaSelectHover()
{
  this->Picker = vtkWorldPointPicker::New();
  this->Balloon = vtkBalloonRepresentation::New();
  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->Layout = 0;
  this->LabelField = 0;
  this->UseRectangularCoordinates = false;

  this->HighlightData = vtkPolyData::New();
  vtkPolyDataMapper *highMap = vtkPolyDataMapper::New();
  highMap->SetInputData(this->HighlightData);
  this->HighlightActor = vtkActor::New();
  this->HighlightActor->SetMapper(highMap);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetLineWidth(4.0);
  highMap->Delete();
}

//----------------------------------------------------------------------------
vtkInteractorStyleAreaSelectHover::~vtkInteractorStyleAreaSelectHover()
{
  this->HighlightData->Delete();
  this->HighlightActor->Delete();
  this->Picker->Delete();
  this->Balloon->Delete();
  if (this->Layout)
    {
    this->Layout->Delete();
    this->Layout = NULL;
    }
  this->SetLabelField(0);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleAreaSelectHover::SetInteractor(vtkRenderWindowInteractor *rwi)
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
      ren->RemoveActor(HighlightActor);
      }
    }
  vtkInteractorStyleRubberBand2D::SetInteractor(rwi);
  if (rwi && rwi->GetRenderWindow())
    {
    this->FindPokedRenderer(0, 0);
    ren = this->CurrentRenderer;
    if (ren)
      {
      ren->AddActor(HighlightActor);
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleAreaSelectHover::PrintSelf(ostream& os, vtkIndent indent)
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

//----------------------------------------------------------------------------
vtkIdType vtkInteractorStyleAreaSelectHover::GetIdAtPos(int x, int y)
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

  if (this->Layout)
    {
    float posFloat[3];
    for (int i = 0; i < 3; i++)
      {
      posFloat[i] = pos[i];
      }
    id = this->Layout->FindVertex(posFloat);
    }

  return id;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleAreaSelectHover::GetBoundingAreaForItem(vtkIdType id, float *sinfo)
{
  if (this->Layout)
    {
    this->Layout->GetBoundingArea(id, sinfo);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleAreaSelectHover::OnMouseMove()
{
  if (this->Interaction == vtkInteractorStyleRubberBand2D::SELECTING)
    {
    this->Balloon->SetVisibility(false);
    this->Superclass::OnMouseMove();
    return;
    }
  this->Balloon->SetVisibility(true);

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
  vtkIdType id = this->GetIdAtPos(x,y);

  if( id != -1 )
    {
    this->GetBoundingAreaForItem(id,sinfo);
    }

  double loc[2] = {static_cast<double>(x), static_cast<double>(y)};
  this->Balloon->EndWidgetInteraction(loc);

  if (this->Layout && this->Layout->GetOutput())
    {
    vtkAbstractArray* absArray = this->Layout->GetOutput()->GetVertexData()->GetAbstractArray(this->LabelField);
    //find the information for the correct sector,
    //  unless there isn't a sector or it is the root node
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
          extract->SetInputConnection(sector->GetOutputPort());

          VTK_CREATE(vtkAppendPolyData, append);
          append->AddInputConnection(extract->GetOutputPort());
          append->Update();

          this->HighlightData->ShallowCopy(append->GetOutput());
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
          }
        }
      this->HighlightActor->VisibilityOn();
      }
    else
      {
      this->Balloon->SetBalloonText("");
      HighlightActor->VisibilityOff();
      }

    this->Balloon->StartWidgetInteraction(loc);

    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    this->GetInteractor()->Render();
    }

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleAreaSelectHover::SetHighLightColor(double r, double g, double b)
{
  this->HighlightActor->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleAreaSelectHover::SetHighLightWidth(double lw)
{
  this->HighlightActor->GetProperty()->SetLineWidth(lw);
}

//----------------------------------------------------------------------------
double vtkInteractorStyleAreaSelectHover::GetHighLightWidth()
{
  return this->HighlightActor->GetProperty()->GetLineWidth();
}

