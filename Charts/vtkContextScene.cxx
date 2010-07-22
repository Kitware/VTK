/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextScene.h"

#include "vtkContextItem.h"
#include "vtkContext2D.h"
#include "vtkTransform2D.h"
#include "vtkMatrix3x3.h"

// Get my new commands
#include "vtkCommand.h"

#include "vtkAnnotationLink.h"
#include "vtkInteractorStyle.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkObjectFactory.h"
#include "vtkContextBufferId.h"
#include "vtkOpenGLContextBufferId.h"
#include "vtkOpenGLRenderWindow.h"

// My STL containers
#include <vtkstd/vector>
#include <assert.h>

//-----------------------------------------------------------------------------
// Minimal command class to handle callbacks.
class vtkContextScene::Command : public vtkCommand
{
public:
  Command(vtkContextScene *scene) { this->Target = scene; }

  virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData)
    {
    if (this->Target)
      {
      vtkInteractorStyle *style = vtkInteractorStyle::SafeDownCast(caller);
      vtkRenderWindowInteractor *interactor = NULL;
      if (style)
        {
        interactor = vtkRenderWindowInteractor::SafeDownCast(style->GetInteractor());
        }

      int x = -1;
      int y = -1;
      if (interactor)
        {
        x = interactor->GetEventPosition()[0];
        y = interactor->GetEventPosition()[1];
        }
      else
        {
        return;
        }

//      cout << "eventId: " << eventId << " -> "
//           << this->GetStringFromEventId(eventId) << endl;

      switch (eventId)
        {
        case vtkCommand::MouseMoveEvent :
          this->Target->MouseMoveEvent(x, y);
          break;
        case vtkCommand::LeftButtonPressEvent :
          this->Target->ButtonPressEvent(0, x, y);
          break;
        case vtkCommand::MiddleButtonPressEvent :
          this->Target->ButtonPressEvent(1, x, y);
          break;
        case vtkCommand::RightButtonPressEvent :
          this->Target->ButtonPressEvent(2, x, y);
          break;
        case vtkCommand::LeftButtonReleaseEvent :
          this->Target->ButtonReleaseEvent(0, x, y);
          break;
        case vtkCommand::MiddleButtonReleaseEvent :
          this->Target->ButtonReleaseEvent(1, x, y);
          break;
        case vtkCommand::RightButtonReleaseEvent :
          this->Target->ButtonReleaseEvent(2, x, y);
          break;
        case vtkCommand::MouseWheelForwardEvent :
          // There is a forward and a backward event - not clear on deltas...
          this->Target->MouseWheelEvent(1, x, y);
          break;
        case vtkCommand::MouseWheelBackwardEvent :
          // There is a forward and a backward event - not clear on deltas...
          this->Target->MouseWheelEvent(-1, x, y);
          break;
        case vtkCommand::SelectionChangedEvent :
          this->Target->ProcessSelectionEvent(caller, callData);
          break;
        default:
          this->Target->ProcessEvents(caller, eventId, callData);
        }
      this->Target->CheckForRepaint();
      }
    }

  void SetTarget(vtkContextScene* t) { this->Target = t; }

  vtkContextScene *Target;
};

//-----------------------------------------------------------------------------
// Minimal storage class for STL containers etc.
class vtkContextScene::Private
{
public:
  Private()
    {
    this->itemMousePressCurrent = -1;
    this->Event.Button = -1;
    this->IsDirty = true;
    }
  ~Private()
    {
    size_t size = this->items.size();
    for (size_t i = 0; i < size; ++i)
      {
      this->items[i]->Delete();
      this->items[i] = NULL;
      }
    }

  vtkstd::vector<vtkContextItem *> items;
  vtkstd::vector<bool> itemState;
  int itemMousePressCurrent; // Index of the item with a current mouse down
  vtkContextMouseEvent Event; // Mouse event structure
  bool IsDirty;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContextScene);
vtkCxxSetObjectMacro(vtkContextScene, AnnotationLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkContextScene::vtkContextScene()
{
  this->Observer = new vtkContextScene::Command(this);
  this->Storage = new Private;
  this->AnnotationLink = NULL;
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId=0;
  this->BufferIdDirty=true;
  this->UseBufferId = false;
  this->Transform = NULL;
}

//-----------------------------------------------------------------------------
vtkContextScene::~vtkContextScene()
{
  this->Observer->Delete();
  this->Observer = NULL;
  delete this->Storage;
  this->Storage = NULL;
  this->SetAnnotationLink(NULL);
  if(this->BufferId!=0)
    {
    this->BufferId->Delete();
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkContextScene::SetRenderer(vtkRenderer *r)
{
  this->Renderer=r;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::Paint(vtkContext2D *painter)
{
  vtkDebugMacro("Paint event called.");
  size_t size = this->Storage->items.size();
  if (size && this->Transform)
    {
    painter->PushMatrix();
    painter->SetTransform(this->Transform);
    }
  for (size_t i = 0; i < size; ++i)
    {
    painter->SetTransform(this->Storage->items[i]->GetTransform());
    this->Storage->items[i]->Paint(painter);
    }
  if (size && this->Transform)
    {
    painter->PopMatrix();
    }
  if(this->Storage->IsDirty)
    {
    this->BufferIdDirty=true;
    }
  this->Storage->IsDirty = false;
  this->LastPainter=painter;
  return true;
}

//-----------------------------------------------------------------------------
void vtkContextScene::PaintIds()
{
  vtkDebugMacro("PaintId called.");
  size_t size = this->Storage->items.size();

  if(size>16777214) // 24-bit limit, 0 reserved for background encoding.
    {
    vtkWarningMacro(<<"picking will not work properly as there are two many items. Items over 16777214 will be ignored.");
    size=16777214;
    }
  for (size_t i = 0; i < size; ++i)
    {
    this->LastPainter->SetTransform(this->Storage->items[i]->GetTransform());
    this->LastPainter->ApplyId(i+1);
    this->Storage->items[i]->Paint(this->LastPainter);
    }
  this->Storage->IsDirty = false;
}

//-----------------------------------------------------------------------------
void vtkContextScene::AddItem(vtkContextItem *item)
{
  item->Register(this);
  item->SetScene(this);
  this->Storage->items.push_back(item);
  this->Storage->itemState.push_back(false);
}

//-----------------------------------------------------------------------------
int vtkContextScene::GetNumberOfItems()
{
  return static_cast<int>(this->Storage->items.size());
}

//-----------------------------------------------------------------------------
vtkContextItem * vtkContextScene::GetItem(int index)
{
  if (index < this->GetNumberOfItems())
    {
    return this->Storage->items[index];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
int vtkContextScene::GetViewWidth()
{
  if (this->Renderer)
    {
    return this->Renderer->GetRenderWindow()->GetSize()[0];
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkContextScene::GetViewHeight()
{
  if (this->Renderer)
    {
    return this->Renderer->GetRenderWindow()->GetSize()[1];
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkContextScene::GetSceneWidth()
{
  return this->Geometry[0];
}

//-----------------------------------------------------------------------------
int vtkContextScene::GetSceneHeight()
{
  return this->Geometry[1];
}

//-----------------------------------------------------------------------------
void vtkContextScene::SetInteractorStyle(vtkInteractorStyle *interactor)
{
  //cout << "Interactor style " << interactor << " " << interactor->GetClassName() << endl;
  interactor->AddObserver(vtkCommand::SelectionChangedEvent, this->Observer);
  interactor->AddObserver(vtkCommand::AnyEvent, this->Observer);
}

//-----------------------------------------------------------------------------
void vtkContextScene::ProcessEvents(vtkObject* caller, unsigned long eventId,
                             void*)
{
  vtkDebugMacro("ProcessEvents called! " << caller->GetClassName() << "\t"
      << vtkCommand::GetStringFromEventId(eventId)
      << "\n\t" << vtkInteractorStyleRubberBand2D::SafeDownCast(caller)->GetInteraction());
}

//-----------------------------------------------------------------------------
void vtkContextScene::SetDirty(bool isDirty)
{
  this->Storage->IsDirty = isDirty;
  if(this->Storage->IsDirty)
    {
    this->BufferIdDirty=true;
    }
}

// ----------------------------------------------------------------------------
void vtkContextScene::ReleaseGraphicsResources()
{
  if(this->BufferId!=0)
    {
    this->BufferId->ReleaseGraphicsResources();
    }
  vtkstd::vector<vtkContextItem *>::iterator it;
  for (it = this->Storage->items.begin(); it != this->Storage->items.end(); ++it)
    {
    (*it)->ReleaseGraphicsResources();
    }
}

//-----------------------------------------------------------------------------
vtkWeakPointer<vtkContext2D> vtkContextScene::GetLastPainter()
{
  return this->LastPainter;
}

//-----------------------------------------------------------------------------
vtkAbstractContextBufferId *vtkContextScene::GetBufferId()
{
  return this->BufferId;
}

//-----------------------------------------------------------------------------
void vtkContextScene::SetTransform(vtkTransform2D* transform)
{
  if (this->Transform == transform)
    {
    return;
    }
  this->Transform->Delete();
  this->Transform = transform;
  this->Transform->Register(this);
}

//-----------------------------------------------------------------------------
vtkTransform2D* vtkContextScene::GetTransform()
{
  if (this->Transform)
    {
    return this->Transform;
    }
  else
    {
    this->Transform = vtkTransform2D::New();
    return this->Transform;
    }
}

//-----------------------------------------------------------------------------
void vtkContextScene::CheckForRepaint()
{
  // Called after interaction events - cause the scene to be repainted if any
  // events marked the scene as dirty.
  if (this->Renderer && this->Storage->IsDirty)
    {
    this->Renderer->GetRenderWindow()->Render();
    }
}

//-----------------------------------------------------------------------------
void vtkContextScene::ProcessSelectionEvent(vtkObject* caller, void* callData)
{
  cout << "ProcessSelectionEvent called! " << caller << "\t" << callData << endl;
  unsigned int *rect = reinterpret_cast<unsigned int *>(callData);
  cout << "Rect:";
  for (int i = 0; i < 5; ++i)
    {
    cout << "\t" << rect[i];
    }
  cout << endl;
}

// ----------------------------------------------------------------------------
void vtkContextScene::UpdateBufferId()
{
  int lowerLeft[2];
  int width;
  int height;
  this->Renderer->GetTiledSizeAndOrigin(&width,&height,lowerLeft,
                                        lowerLeft+1);

  if(this->BufferId==0 || this->BufferIdDirty ||
     width!=this->BufferId->GetWidth() ||
     height!=this->BufferId->GetHeight())
    {
    if(this->BufferId==0)
      {
      vtkOpenGLContextBufferId *b=vtkOpenGLContextBufferId::New();
      this->BufferId=b;
      b->SetContext(static_cast<vtkOpenGLRenderWindow *>(
                      this->Renderer->GetRenderWindow()));
      }
    this->BufferId->SetWidth(width);
    this->BufferId->SetHeight(height);
    this->BufferId->Allocate();

    this->LastPainter->BufferIdModeBegin(this->BufferId);
    this->PaintIds();
    this->LastPainter->BufferIdModeEnd();

    this->BufferIdDirty=false;
    }
}

// ----------------------------------------------------------------------------
vtkIdType vtkContextScene::GetPickedItem(int x, int y)
{
  vtkIdType result = -1;
  if (this->UseBufferId)
    {
    this->UpdateBufferId();
    result=this->BufferId->GetPickedItem(x,y);
    }
  else
    {
    int size = static_cast<int>(this->Storage->items.size());
    vtkContextMouseEvent &event = this->Storage->Event;
    for (int i = size-1; i >= 0; --i)
      {
      if (this->Storage->items[i]->Hit(event))
        {
        result = static_cast<vtkIdType>(i);
        break;
        }
      }
    }

  // Work-around for Qt bug under Linux (and maybe other platforms), 4.5.2
  // or 4.6.2 :
  // when the cursor leaves the window, Qt returns an extra mouse move event
  // with coordinates out of the window area. The problem is that the pixel
  // underneath is not owned by the OpenGL context, hence the bufferid contains
  // garbage (see OpenGL pixel ownership test).
  // As a workaround, any value out of the scope of
  // [-1,this->GetNumberOfItems()-1] is set to -1 (<=> no hit)

  if(result<-1 || result>=this->GetNumberOfItems())
    {
    result=-1;
    }

  assert("post: valid_result" && result>=-1 &&
         result<this->GetNumberOfItems());
  return result;
}

//-----------------------------------------------------------------------------
void vtkContextScene::MouseMoveEvent(int x, int y)
{
  int size = static_cast<int>(this->Storage->items.size());
  vtkContextMouseEvent &event = this->Storage->Event;
  event.ScreenPos[0] = x;
  event.ScreenPos[1] = y;
  event.ScenePos[0] = x;
  event.ScenePos[1] = y;

  if(size != 0)
    {
    // Fire mouse enter and leave event prior to firing a mouse event.
    vtkIdType pickedItem=this->GetPickedItem(x,y);
//    if(pickedItem>=0)
//      {
//      cout << "item i="<< pickedItem << " is under the mouse cursor."<< endl;
//      }

    for (int i = size-1; i >= 0; --i)
      {
      if (this->Storage->itemMousePressCurrent == i)
        {
        // Don't send the mouse move event twice...
        continue;
        }
      this->PerformTransform(this->Storage->items[i]->GetTransform(), event);

      if (i==pickedItem)
        {
        if (!this->Storage->itemState[i] && this->Storage->itemMousePressCurrent < 0)
          {
          this->Storage->itemState[i] = true;
          this->Storage->items[i]->MouseEnterEvent(event);
//          cout << "enter" << endl;
          }
        }
      else
        {
        if (this->Storage->itemState[i])
          {
          this->Storage->itemState[i] = false;
          this->Storage->items[i]->MouseLeaveEvent(event);
//          cout << "leave" << endl;
          }
        }
      }

    // Fire mouse move event regardless of where it occurred.

    // Check if there is a selected item that needs to receive a move event
    if (this->Storage->itemMousePressCurrent >= 0)
      {
      this->PerformTransform(
        this->Storage->items[this->Storage->itemMousePressCurrent]->GetTransform(),
        event);
      this->Storage->items[this->Storage->itemMousePressCurrent]->MouseMoveEvent(event);
      }
    else
      {
      // Propagate mouse move events
      for (int i = size-1; i >= 0; --i)
        {
        if (this->Storage->items[i]->MouseMoveEvent(event))
          {
          break;
          }
        }
      }
    }

  // Update the last positions now
  event.LastScreenPos[0] = event.ScreenPos[0];
  event.LastScreenPos[1] = event.ScreenPos[1];
  event.LastScenePos[0] = event.ScenePos[0];
  event.LastScenePos[1] = event.ScenePos[1];
}

//-----------------------------------------------------------------------------
void vtkContextScene::ButtonPressEvent(int button, int x, int y)
{
  int size = static_cast<int>(this->Storage->items.size());
  vtkContextMouseEvent &event = this->Storage->Event;
  event.ScreenPos[0] = event.LastScreenPos[0] = x;
  event.ScreenPos[1] = event.LastScreenPos[1] = y;
  event.ScenePos[0] = event.LastScenePos[0] = x;
  event.ScenePos[1] = event.LastScenePos[1] = y;
  event.Button = button;
  for (int i = size-1; i >= 0; --i)
    {
    this->PerformTransform(this->Storage->items[i]->GetTransform(), event);
    if (this->Storage->items[i]->Hit(event))
      {
      if (this->Storage->items[i]->MouseButtonPressEvent(event))
        {
        // The event was accepted - stop propagating
        this->Storage->itemMousePressCurrent = i;
        return;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkContextScene::ButtonReleaseEvent(int button, int x, int y)
{
  if (this->Storage->itemMousePressCurrent >= 0)
    {
    vtkContextMouseEvent &event = this->Storage->Event;
    event.ScreenPos[0] = x;
    event.ScreenPos[1] = y;
    event.ScenePos[0] = x;
    event.ScenePos[1] = y;
    event.Button = button;
    this->PerformTransform(
        this->Storage->items[this->Storage->itemMousePressCurrent]->GetTransform(),
        event);
    this->Storage->items[this->Storage->itemMousePressCurrent]->MouseButtonReleaseEvent(event);
    this->Storage->itemMousePressCurrent = -1;
    }
  this->Storage->Event.Button = -1;
}

void vtkContextScene::MouseWheelEvent(int delta, int x, int y)
{
  int size = static_cast<int>(this->Storage->items.size());
  vtkContextMouseEvent &event = this->Storage->Event;
  event.ScreenPos[0] = event.LastScreenPos[0] = x;
  event.ScreenPos[1] = event.LastScreenPos[1] = y;
  event.ScenePos[0] = event.LastScenePos[0] = x;
  event.ScenePos[1] = event.LastScenePos[1] = y;
  //event.Button = 1;
  for (int i = size-1; i >= 0; --i)
    {
    this->PerformTransform(this->Storage->items[i]->GetTransform(), event);
    if (this->Storage->items[i]->Hit(event))
      {
      if (this->Storage->items[i]->MouseWheelEvent(event, delta))
        {
        // The event was accepted - stop propagating
        break;
        }
      }
    }

  if (this->Renderer)
    {
    this->Renderer->GetRenderWindow()->Render();
    }
}

//-----------------------------------------------------------------------------
inline void vtkContextScene::PerformTransform(vtkTransform2D *transform,
                                              vtkContextMouseEvent &mouse)
{
  if (transform)
    {
    transform->InverseTransformPoints(&mouse.ScenePos[0], &mouse.Pos[0], 1);
    }
  else
    {
    mouse.Pos[0] = mouse.ScenePos[0];
    mouse.Pos[1] = mouse.ScenePos[1];
    }
}

//-----------------------------------------------------------------------------
void vtkContextScene::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out the chart's geometry if it has been set
  os << indent << "Widthxheight: " << this->Geometry[0] << "\t" << this->Geometry[1]
     << endl;
}
