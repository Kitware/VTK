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
#include "vtkHardwareSelector.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"

class vtkScenePickerSelectionRenderCommand : public vtkCommand
{
public:
  vtkScenePicker * m_Picker;
  static vtkScenePickerSelectionRenderCommand *New()
  {
    return new vtkScenePickerSelectionRenderCommand;
  }

  void Execute(vtkObject *vtkNotUsed(o), unsigned long event, void*) VTK_OVERRIDE
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
  ~vtkScenePickerSelectionRenderCommand() VTK_OVERRIDE {}
  bool InteractiveRender;
};

vtkStandardNewMacro(vtkScenePicker);

//----------------------------------------------------------------------------
vtkScenePicker::vtkScenePicker()
{
  this->EnableVertexPicking  = 1;
  this->Renderer             = NULL;
  this->Interactor           = NULL;
  this->Selector  = vtkHardwareSelector::New();
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
  this->Selector->Delete();
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

  this->Selector->SetRenderer(this->Renderer);
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
// The Selector will be used here to pick-render the entire
// screen, store on a buffer the colored cells and read back as
// the mouse moves around the moused pick. This extra render from the
// Selector will be done only if the camera isn't in motion,
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

  int rx1 = static_cast<int>(vp[0]*(size[0] - 1));
  int ry1 = static_cast<int>(vp[1]*(size[1] - 1));
  int rx2 = static_cast<int>(vp[2]*(size[0] - 1));
  int ry2 = static_cast<int>(vp[3]*(size[1] - 1));

  this->PickRender( rx1,ry1,rx2,ry2 );
}

// ----------------------------------------------------------------------------
// Do a selection render.. for caching object selection stuff.
void vtkScenePicker::PickRender(
               int x0, int y0, int x1, int y1 )
{
  this->Renderer->GetRenderWindow()->RemoveObserver(
      this->SelectionRenderCommand );

  if (this->EnableVertexPicking)
  {
    this->Selector->SetFieldAssociation(
      vtkDataObject::FIELD_ASSOCIATION_POINTS);
  }
  else
  {
    this->Selector->SetFieldAssociation(
      vtkDataObject::FIELD_ASSOCIATION_CELLS);
  }
  cout << "Area: " << x0 << ", " << y0 << ", " << x1 << ", " << y1 << endl;
  this->Selector->SetArea(x0,y0,x1,y1);
  if (!this->Selector->CaptureBuffers())
  {
    vtkErrorMacro("Failed to capture buffers.");
  }
  this->NeedToUpdate = true;
  this->PickRenderTime.Modified();
  this->Renderer->GetRenderWindow()->AddObserver(
        vtkCommand::EndEvent, this->SelectionRenderCommand, 0.01 );
}

//----------------------------------------------------------------------------
vtkIdType vtkScenePicker::GetCellId( int displayPos[2] )
{
  if (this->EnableVertexPicking)
  {
    return -1;
  }
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
  if (this->PickRenderTime <= this->GetMTime())
  {
    this->PickRender();
  }

  if (this->NeedToUpdate ||
      this->LastQueriedDisplayPos[0] != displayPos[0] ||
      this->LastQueriedDisplayPos[1] != displayPos[1])
  {
    this->Prop = 0;
    unsigned int dpos[2] = {0, 0};
    if (displayPos[0] >= 0 && displayPos[1] >= 0)
    {
      dpos[0] = static_cast<unsigned int>(displayPos[0]);
      dpos[1] = static_cast<unsigned int>(displayPos[1]);
      vtkHardwareSelector::PixelInformation info = this->Selector->GetPixelInformation(dpos);
      this->CellId = info.AttributeID;
      this->Prop = info.Prop;
    }
    this->LastQueriedDisplayPos[0] = displayPos[0];
    this->LastQueriedDisplayPos[1] = displayPos[1];
    this->NeedToUpdate             = false;
  }
}

//----------------------------------------------------------------------------
void vtkScenePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "EnableVertexPicking: " << this->EnableVertexPicking << endl;
}
