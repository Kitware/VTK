/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScenePicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScenePicker.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkProp.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVisibleCellSelector.h"
#include "vtkCommand.h"

class vtkScenePickerSelectionRenderCommand : public vtkCommand
{
public:
  vtkScenePicker * m_Picker;
  static vtkScenePickerSelectionRenderCommand *New()
    {
    return new vtkScenePickerSelectionRenderCommand;
    }

  virtual void Execute(vtkObject *vtkNotUsed(o), unsigned long event, void*)
    {
    if (event == vtkCommand::StartInteractionEvent)
      {
      this->InteractiveRender = true;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->InteractiveRender = false;
      }
    else if (event == vtkCommand::EndEvent)
      {
      if (!this->InteractiveRender)
        {
        m_Picker->PickRender();
        }
      this->m_Picker->SetRenderer(this->m_Picker->Renderer);
      }
    }

protected:
  vtkScenePickerSelectionRenderCommand() 
                                      : InteractiveRender(false) {}
  virtual ~vtkScenePickerSelectionRenderCommand() {}
  bool InteractiveRender;
};

vtkCxxRevisionMacro(vtkScenePicker, "1.1");
vtkStandardNewMacro(vtkScenePicker);

//----------------------------------------------------------------------------
vtkScenePicker::vtkScenePicker()
{
  this->EnableVertexPicking  = 1;
  this->Renderer             = NULL;
  this->Interactor           = NULL;
  this->VisibleCellSelector  = vtkVisibleCellSelector::New();
  this->NeedToUpdate         = false;
  this->VertId               = -1;
  this->CellId               = -1;
  this->Prop                 = NULL;
  this->SelectionRenderCommand 
    = vtkScenePickerSelectionRenderCommand::New();
  this->SelectionRenderCommand->m_Picker = this;
}

//----------------------------------------------------------------------------
vtkScenePicker::~vtkScenePicker()
{
  this->SetRenderer(NULL);
  this->VisibleCellSelector->Delete();
  this->SelectionRenderCommand->Delete();
}

//----------------------------------------------------------------------------
void vtkScenePicker::SetRenderer( vtkRenderer * r )
{
  vtkRenderWindowInteractor *rwi = NULL;
  if (r && r->GetRenderWindow())
    {
    rwi = r->GetRenderWindow()->GetInteractor();
    }
  this->SetInteractor(rwi);
  
  if (this->Renderer == r)
    {
    return;
    }
  if (r && !r->GetRenderWindow())
    {
    vtkErrorMacro( << "Renderer: " << this->Renderer 
                   << " does not have its render window set." );
    return;
    }
  
  if (this->Renderer)
    {
    this->Renderer->GetRenderWindow()->RemoveObserver( 
                        this->SelectionRenderCommand );
    }

  vtkSetObjectBodyMacro( Renderer, vtkRenderer, r );
  
  if (this->Renderer)
    {
    this->Renderer->GetRenderWindow()->AddObserver( vtkCommand::EndEvent,
          this->SelectionRenderCommand, 0.01 );
    }

  this->VisibleCellSelector->SetRenderer(this->Renderer);
  this->FirstTime = true;
}

//----------------------------------------------------------------------------
void vtkScenePicker::SetInteractor( 
                         vtkRenderWindowInteractor *rwi )
{
  if (this->Interactor == rwi)
    {
    return;
    }
  if (this->Interactor)
    {
    this->Interactor->RemoveObserver( this->SelectionRenderCommand );
    }

  vtkSetObjectBodyMacro( Interactor, vtkRenderWindowInteractor, rwi );
  
  if (this->Interactor)
    {
    this->Interactor->AddObserver( vtkCommand::StartInteractionEvent,
          this->SelectionRenderCommand, 0.01 );
    this->Interactor->AddObserver( vtkCommand::EndInteractionEvent,
          this->SelectionRenderCommand, 0.01 );
    }
}

//----------------------------------------------------------------------------
// Do a selection render.. for caching object selection stuff.
// This is used for Object selection . We have to perform
// "select" and "mouse over" and "mouse out" as the mouse moves around the
// scene (or the mouse is clicked in the case of "select"). I do not want
// to do a conventional pick for this function because it's too darn slow.
// The VisibleCellSelector will be used here to pick-render the entire
// screen, store on a buffer the colored cells and read back as
// the mouse moves around the moused pick. This extra render from the
// VisibleCellSelector will be done only if the camera isn't in motion,
// otherwise motion would be too frickin slow.
//
void vtkScenePicker::PickRender()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
    {
    return;
    }

  double vp[4];
  this->Renderer->GetViewport( vp );
  int size[2] = { this->Renderer->GetRenderWindow()->GetSize()[0],
                  this->Renderer->GetRenderWindow()->GetSize()[1] };

  int rx1 = (int)(vp[0]*(size[0] - 1));
  int ry1 = (int)(vp[1]*(size[1] - 1));
  int rx2 = (int)(vp[2]*(size[0] - 1));
  int ry2 = (int)(vp[3]*(size[1] - 1));

  this->PickRender( rx1,ry1,rx2,ry2 );
}

// ----------------------------------------------------------------------------
// Do a selection render.. for caching object selection stuff.
void vtkScenePicker::PickRender( 
               int x0, int y0, int x1, int y1 )
{
  this->Renderer->GetRenderWindow()->RemoveObserver( 
      this->SelectionRenderCommand );
  
  this->VisibleCellSelector->SetRenderPasses(
          0,1,0,1,1,this->EnableVertexPicking);
  this->VisibleCellSelector->SetArea((int)x0,(int)y0,(int)x1,(int)y1);
  this->VisibleCellSelector->Select();
  this->NeedToUpdate = true;
  this->FirstTime    = false;

  this->Renderer->GetRenderWindow()->AddObserver( 
        vtkCommand::EndEvent, this->SelectionRenderCommand, 0.01 );
}

//----------------------------------------------------------------------------
vtkIdType vtkScenePicker::GetCellId( int displayPos[2] )
{
  this->Update( displayPos );
  return this->CellId;
}

//----------------------------------------------------------------------------
vtkProp * vtkScenePicker::GetViewProp( int displayPos[2] )
{
  this->Update( displayPos );
  return this->Prop;
}

//----------------------------------------------------------------------------
vtkIdType vtkScenePicker::GetVertexId( int displayPos[2] )
{
  if (!this->EnableVertexPicking)
    {
    return -1;
    }
  this->Update( displayPos );
  return this->CellId;
}

//----------------------------------------------------------------------------
void vtkScenePicker::Update( int displayPos[2] )
{
  if (this->FirstTime)
    {
    this->PickRender();
    }

  if (this->NeedToUpdate ||
      this->LastQueriedDisplayPos[0] != displayPos[0] ||
      this->LastQueriedDisplayPos[1] != displayPos[1])
    {
    vtkIdType dummy;
    this->VisibleCellSelector->GetPixelSelection( displayPos,
      dummy, this->CellId, this->VertId, this->Prop );

    this->LastQueriedDisplayPos[0] = displayPos[0];
    this->LastQueriedDisplayPos[1] = displayPos[1];
    this->NeedToUpdate             = false;
    }
}

//----------------------------------------------------------------------------
void vtkScenePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
