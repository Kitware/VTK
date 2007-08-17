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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkRenderView.h"

#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkViewTheme.h"
#include "vtkVisibleCellSelector.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkCxxRevisionMacro(vtkRenderView, "1.1");
vtkStandardNewMacro(vtkRenderView);
vtkCxxSetObjectMacro(vtkRenderView, InteractorStyle, vtkInteractorStyle);
//----------------------------------------------------------------------------
vtkRenderView::vtkRenderView()
{
  this->Renderer = vtkRenderer::New();
  this->Renderer->AddObserver(vtkCommand::StartEvent, this->GetObserver());
  this->InteractorStyle = vtkInteractorStyleRubberBand3D::New();
  this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
  
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
  else if (caller == this->InteractorStyle && eventId == vtkCommand::SelectionChangedEvent)
    {
    // Do a visible cell selection.
    unsigned int* rect = reinterpret_cast<unsigned int*>(callData);
    bool singleSelectMode = false;
    unsigned int pos1X = rect[0];
    unsigned int pos1Y = rect[1];
    unsigned int pos2X = rect[2];
    unsigned int pos2Y = rect[3];
    int stretch = 2;
    if (pos1X == pos2X && pos1Y == pos2Y)
      {
      singleSelectMode = true;
      pos1X = pos1X - stretch > 0 ? pos1X - stretch : 0;
      pos1Y = pos1Y - stretch > 0 ? pos1Y - stretch : 0;
      pos2X = pos2X + stretch;
      pos2Y = pos2Y + stretch;
      }
    unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
    unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
    unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
    unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;
    vtkVisibleCellSelector* vcs = vtkVisibleCellSelector::New();
    vcs->SetRenderer(this->Renderer);
    vcs->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
    vcs->SetProcessorId(0);
    vcs->SetRenderPasses(0, 1, 0, 0, 1);
    vcs->Select();  
    
    vtkSelection* selection = vtkSelection::New();
    vcs->GetSelectedIds(selection);
    
    // Add prop pointers to the selection
    for (unsigned int s = 0; s < selection->GetNumberOfChildren(); s++)
      {
      vtkSelection* child = selection->GetChild(s);
      int propId = child->GetProperties()->Get(vtkSelection::PROP_ID());
      vtkProp* prop = vcs->GetActorFromId(propId);
      //cerr << "child " << s << " selected from prop " << propId << " " << reinterpret_cast<vtkTypeUInt64>(prop) << endl; 
      child->GetProperties()->Set(vtkSelection::PROP(), prop);
      }
    vcs->Delete();
    
    // Call select on the representation(s)
    for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
      {
      this->GetRepresentation(i)->Select(this, selection);
      }
    
    selection->Delete();
    }
  
  this->Superclass::ProcessEvents(caller, eventId, callData);
}

//----------------------------------------------------------------------------
void vtkRenderView::Update()
{
  if (this->Renderer->GetRenderWindow())
    {
    this->Renderer->ResetCameraClippingRange();
    this->Renderer->GetRenderWindow()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkRenderView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
}

//----------------------------------------------------------------------------
void vtkRenderView::RepresentationSelectionChanged(
    vtkDataRepresentation* vtkNotUsed(rep),
    vtkSelection* vtkNotUsed(selection))
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
}
