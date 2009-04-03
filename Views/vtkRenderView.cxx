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

#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkViewTheme.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkCxxRevisionMacro(vtkRenderView, "1.12");
vtkStandardNewMacro(vtkRenderView);
//----------------------------------------------------------------------------
vtkRenderView::vtkRenderView()
{
  this->Renderer = vtkRenderer::New();
  this->Renderer->AddObserver(vtkCommand::StartEvent, this->GetObserver());
  this->Renderer->AddObserver(vtkCommand::ComputeVisiblePropBoundsEvent,
    this->GetObserver());
  this->InteractorStyle = vtkInteractorStyleRubberBand3D::New();
  this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
  this->SelectionMode = SURFACE;
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();  
}

//----------------------------------------------------------------------------
vtkRenderView::~vtkRenderView()
{
  this->Renderer->Delete();
  this->InteractorStyle->Delete();
}

//----------------------------------------------------------------------------
void vtkRenderView::SetInteractorStyle(vtkInteractorStyle* style)
{
  // This is just like vtkCxxSetObjectMacro but changes the observer.
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting InteractorStyle to " << style );
  if (this->InteractorStyle != style)
    {
    vtkInteractorStyle* tempSGMacroVar = this->InteractorStyle;
    this->InteractorStyle = style;
    if (this->InteractorStyle != NULL)
      {
      this->InteractorStyle->Register(this);
      this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
      }
    if (tempSGMacroVar != NULL)
      {
      tempSGMacroVar->RemoveObserver(this->GetObserver());
      tempSGMacroVar->UnRegister(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkRenderView::SetupRenderWindow(vtkRenderWindow* win)
{
  win->AddRenderer(this->Renderer);
  if (!win->GetInteractor())
    {
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    win->SetInteractor(iren);
    iren->Initialize();
    iren->Delete();
    }
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkRenderView::GetRenderWindow() 
{
  return this->Renderer->GetRenderWindow();
}

//----------------------------------------------------------------------------
void vtkRenderView::ProcessEvents(vtkObject* caller, unsigned long eventId, 
  void* callData)
{
  if (caller == this->Renderer && eventId == vtkCommand::StartEvent)
    {
    this->PrepareForRendering();
    }
  else if (caller == this->Renderer &&
           eventId == vtkCommand::ComputeVisiblePropBoundsEvent)
    {
    this->Update();
    }
  else if (caller == this->InteractorStyle && eventId == vtkCommand::SelectionChangedEvent)
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

    vtkSelection* selection = 0; 
    if (this->SelectionMode == SURFACE)
      {
      // Do a visible cell selection.
      vtkHardwareSelector* vcs = vtkHardwareSelector::New();
      vcs->SetRenderer(this->Renderer);
      vcs->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
      selection = vcs->Select();  
      vcs->Delete();
      }
    else
      {
      // Do a frustum selection.
      int displayRectangle[4] = {screenMinX, screenMinY, screenMaxX, screenMaxY};
      vtkDoubleArray *frustcorners = vtkDoubleArray::New();
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

      selection = vtkSelection::New();
      vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
      node->SetContentType(vtkSelectionNode::FRUSTUM);
      node->SetFieldType(vtkSelectionNode::CELL);
      node->SetSelectionList(frustcorners);
      selection->AddNode(node);
      frustcorners->Delete();
      }
    
    // Call select on the representation(s)
    for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
      {
      this->GetRepresentation(i)->Select(this, selection);
      }
    
    selection->Delete();
    }
  else if(eventId == vtkCommand::SelectionChangedEvent)
    {
    if (this->Renderer->GetRenderWindow())
      {
      this->Renderer->ResetCameraClippingRange();
      this->Renderer->GetRenderWindow()->Render();
      }
    Superclass::ProcessEvents(caller, eventId, callData);
    }
  else
    {
    this->Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkRenderView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
}

//----------------------------------------------------------------------------
void vtkRenderView::PrepareForRendering()
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Renderer: " << endl;
  this->Renderer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "InteractorStyle: " << endl;
  this->InteractorStyle->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
}
