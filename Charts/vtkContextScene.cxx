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
#include "vtkContextScenePrivate.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextKeyEvent.h"

// Get my new commands
#include "vtkCommand.h"

#include "vtkAnnotationLink.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkContextBufferId.h"
#include "vtkOpenGLContextBufferId.h"
#include "vtkOpenGLRenderWindow.h"

// My STL containers
#include <assert.h>

//-----------------------------------------------------------------------------
// Minimal storage class for STL containers etc.
class vtkContextScene::Private
{
public:
  Private()
    {
    this->Event.SetButton(vtkContextMouseEvent::NO_BUTTON);
    this->IsDirty = true;
    }
  ~Private()
    {
    }

  // The item with a current mouse down
  vtkWeakPointer<vtkAbstractContextItem> itemMousePressCurrent;
  // Item the mouse was last over
  vtkWeakPointer<vtkAbstractContextItem> itemPicked;
  // Mouse event structure
  vtkContextMouseEvent Event;
  bool IsDirty;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContextScene);
vtkCxxSetObjectMacro(vtkContextScene, AnnotationLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkContextScene::vtkContextScene()
{
  this->Storage = new Private;
  this->AnnotationLink = NULL;
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId=0;
  this->BufferIdDirty=true;
  this->BufferIdSupportTested=false;
  this->BufferIdSupported=false;
  this->UseBufferId = true;
  this->ScaleTiles = true;
  this->Transform = NULL;
  this->Children = new vtkContextScenePrivate(NULL);
  this->Children->SetScene(this);
}

//-----------------------------------------------------------------------------
vtkContextScene::~vtkContextScene()
{
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
  delete this->Children;
}

//-----------------------------------------------------------------------------
void vtkContextScene::SetRenderer(vtkRenderer *r)
{
  this->Renderer=r;
  this->BufferIdSupportTested=false;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::Paint(vtkContext2D *painter)
{
  vtkDebugMacro("Paint event called.");
  size_t size = this->Children->size();
  if (size && this->Transform)
    {
    painter->PushMatrix();
    painter->SetTransform(this->Transform);
    }
  this->Children->PaintItems(painter);
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
  size_t size = this->Children->size();

  if(size>16777214) // 24-bit limit, 0 reserved for background encoding.
    {
    vtkWarningMacro(<<"picking will not work properly as there are two many items. Items over 16777214 will be ignored.");
    size=16777214;
    }
  for (size_t i = 0; i < size; ++i)
    {
    this->LastPainter->ApplyId(i+1);
    (*this->Children)[i]->Paint(this->LastPainter);
    }
  this->Storage->IsDirty = false;
}

//-----------------------------------------------------------------------------
unsigned int vtkContextScene::AddItem(vtkAbstractContextItem *item)
{
  return this->Children->AddItem(item);
}

//-----------------------------------------------------------------------------
bool vtkContextScene::RemoveItem(vtkAbstractContextItem* item)
{
  return this->Children->RemoveItem(item);
}

//-----------------------------------------------------------------------------
bool vtkContextScene::RemoveItem(unsigned int index)
{
  return this->Children->RemoveItem(index);
}

//-----------------------------------------------------------------------------
vtkAbstractContextItem * vtkContextScene::GetItem(unsigned int index)
{
  if (index < this->Children->size())
    {
    return this->Children->at(index);
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
unsigned int vtkContextScene::GetNumberOfItems()
{
  return static_cast<unsigned int>(this->Children->size());
}

//-----------------------------------------------------------------------------
void vtkContextScene::ClearItems()
{
  this->Children->Clear();
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
void vtkContextScene::SetDirty(bool isDirty)
{
  if (this->Storage->IsDirty == isDirty)
    {
    return;
    }
  this->Storage->IsDirty = isDirty;
  if(this->Storage->IsDirty)
    {
    this->BufferIdDirty=true;
    }
  this->Modified();
}


//-----------------------------------------------------------------------------
bool vtkContextScene::GetDirty() const
{
  return this->Storage->IsDirty;
}

// ----------------------------------------------------------------------------
void vtkContextScene::ReleaseGraphicsResources()
{
  if(this->BufferId!=0)
    {
    this->BufferId->ReleaseGraphicsResources();
    }
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
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
bool vtkContextScene::ProcessSelectionEvent(unsigned int rect[5])
{
  cout << "ProcessSelectionEvent called! " << endl;
  cout << "Rect:";
  for (int i = 0; i < 5; ++i)
    {
    cout << "\t" << rect[i];
    }
  cout << endl;
  return false;
}

// ----------------------------------------------------------------------------
void vtkContextScene::TestBufferIdSupport()
{
  if(!this->BufferIdSupportTested)
    {
    vtkOpenGLContextBufferId *b=vtkOpenGLContextBufferId::New();
    b->SetContext(static_cast<vtkOpenGLRenderWindow *>(
                    this->Renderer->GetRenderWindow()));
    this->BufferIdSupported=b->IsSupported();
    b->ReleaseGraphicsResources();
    b->Delete();
    this->BufferIdSupportTested=true;
    }
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
vtkAbstractContextItem* vtkContextScene::GetPickedItem()
{
  vtkContextMouseEvent &event = this->Storage->Event;
  for(vtkContextScenePrivate::const_reverse_iterator it =
      this->Children->rbegin(); it != this->Children->rend(); ++it)
    {
    vtkAbstractContextItem* item = (*it)->GetPickedItem(event);
    if (item)
      {
      return item;
      }
    }
  return NULL;
}

// ----------------------------------------------------------------------------
vtkIdType vtkContextScene::GetPickedItem(int x, int y)
{
  vtkIdType result = -1;
  this->TestBufferIdSupport();
  if (this->UseBufferId && this->BufferIdSupported)
    {
    this->UpdateBufferId();
    result=this->BufferId->GetPickedItem(x,y);
    }
  else
    {
    size_t i = this->Children->size()-1;
    vtkContextMouseEvent &event = this->Storage->Event;
    for(vtkContextScenePrivate::const_reverse_iterator it =
        this->Children->rbegin(); it != this->Children->rend(); ++it, --i)
      {
      if ((*it)->Hit(event))
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

  if(result<-1 || result>=static_cast<vtkIdType>(this->GetNumberOfItems()))
    {
    result=-1;
    }

  assert("post: valid_result" && result>=-1 &&
         result<static_cast<vtkIdType>(this->GetNumberOfItems()));
  return result;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::MouseMoveEvent(const vtkContextMouseEvent &e)
{
  bool res = false;
  vtkContextMouseEvent &event = this->Storage->Event;
  this->EventCopy(e);

  vtkAbstractContextItem* newItemPicked = this->GetPickedItem();
#if 0
  if (newItemPicked)
    {
    cerr << "picked a " << newItemPicked->GetClassName() << endl;
    }
  else
    {
    cerr << "picked nothing" << endl;
    }
#endif
  if (this->Storage->itemPicked.GetPointer() != newItemPicked)
    {
    if (this->Storage->itemPicked.GetPointer())
      {
      // Make sure last picked object is still part of this scene.
      if (this->Storage->itemPicked->GetScene() == this)
        {
        vtkAbstractContextItem* cur = this->Storage->itemPicked;
        res = this->ProcessItem(cur, event, &vtkAbstractContextItem::MouseLeaveEvent) || res;
        }
      }
    if (newItemPicked)
      {
      vtkAbstractContextItem* cur = newItemPicked;
      res = this->ProcessItem(cur, event, &vtkAbstractContextItem::MouseEnterEvent) || res;
      }
    }

  this->Storage->itemPicked = newItemPicked;

  // Fire mouse move event regardless of where it occurred.

  // Check if there is a selected item that needs to receive a move event
  if (this->Storage->itemMousePressCurrent.GetPointer() &&
      this->Storage->itemMousePressCurrent->GetScene() == this)
    {
    vtkAbstractContextItem* cur = this->Storage->itemMousePressCurrent;
    res = this->ProcessItem(cur, event, &vtkAbstractContextItem::MouseMoveEvent) || res;
    }
  else if (this->Storage->itemPicked.GetPointer())
    {
    vtkAbstractContextItem* cur = this->Storage->itemPicked;
    res = this->ProcessItem(cur, event, &vtkAbstractContextItem::MouseMoveEvent) || res;
    }

  // Update the last positions now
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  return res;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::ButtonPressEvent(const vtkContextMouseEvent &e)
{
  bool res = false;
  vtkContextMouseEvent &event = this->Storage->Event;
  this->EventCopy(e);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  event.SetButton(e.GetButton());

  vtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
    {
    vtkAbstractContextItem* cur = newItemPicked;
    res = this->ProcessItem(cur, event,
                            &vtkAbstractContextItem::MouseButtonPressEvent);
    }
  this->Storage->itemMousePressCurrent = newItemPicked;
  return res;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::ButtonReleaseEvent(const vtkContextMouseEvent &e)
{
  bool res = false;
  if (this->Storage->itemMousePressCurrent.GetPointer())
    {
    vtkContextMouseEvent &event = this->Storage->Event;
    this->EventCopy(e);
    event.SetButton(e.GetButton());
    vtkAbstractContextItem* cur = this->Storage->itemMousePressCurrent;
    res = this->ProcessItem(cur, event,
                            &vtkAbstractContextItem::MouseButtonReleaseEvent);
    this->Storage->itemMousePressCurrent = NULL;
    }
  this->Storage->Event.SetButton(vtkContextMouseEvent::NO_BUTTON);
  return res;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::DoubleClickEvent(const vtkContextMouseEvent &e)
{
  bool res = false;
  vtkContextMouseEvent &event = this->Storage->Event;
  this->EventCopy(e);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  event.SetButton(e.GetButton());

  vtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
    {
    vtkAbstractContextItem* cur = newItemPicked;
    res = this->ProcessItem(cur, event,
                            &vtkAbstractContextItem::MouseDoubleClickEvent);
    }
  return res;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::MouseWheelEvent(int delta,const vtkContextMouseEvent &e)
{
  bool res = false;
  vtkContextMouseEvent &event = this->Storage->Event;
  this->EventCopy(e);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  event.SetButton(vtkContextMouseEvent::NO_BUTTON);

  vtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
    {
    vtkContextMouseEvent itemEvent = event;
    vtkAbstractContextItem* cur = newItemPicked;
    itemEvent.SetPos(cur->MapFromScene(event.GetPos()));
    itemEvent.SetLastPos(cur->MapFromScene(event.GetLastPos()));
    while (cur && !cur->MouseWheelEvent(itemEvent, delta))
      {
      cur = cur->GetParent();
      if (cur)
        {
        itemEvent.SetPos(cur->MapToParent(itemEvent.GetPos()));
        itemEvent.SetLastPos(cur->MapToParent(itemEvent.GetLastPos()));
        }
      }
    res = (cur != 0);
    }
  return res;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::KeyPressEvent(const vtkContextKeyEvent &keyEvent)
{
  vtkContextMouseEvent &event = this->Storage->Event;
  event.SetScreenPos(keyEvent.GetPosition());
  vtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
    {
    return newItemPicked->KeyPressEvent(keyEvent);
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextScene::KeyReleaseEvent(const vtkContextKeyEvent &keyEvent)
{
  vtkContextMouseEvent &event = this->Storage->Event;
  event.SetScreenPos(keyEvent.GetPosition());
  vtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
    {
    return newItemPicked->KeyReleaseEvent(keyEvent);
    }
  return false;
}

//-----------------------------------------------------------------------------
inline bool vtkContextScene::ProcessItem(vtkAbstractContextItem* cur,
                                         const vtkContextMouseEvent& event,
                                         MouseEvents eventPtr)
{
  bool res = false;
  vtkContextMouseEvent itemEvent = event;
  itemEvent.SetPos(cur->MapFromScene(event.GetPos()));
  itemEvent.SetLastPos(cur->MapFromScene(event.GetLastPos()));
  while (cur && !(cur->*eventPtr)(itemEvent))
    {
    cur = cur->GetParent();
    if (cur)
      {
      itemEvent.SetPos(cur->MapToParent(itemEvent.GetPos()));
      itemEvent.SetLastPos(cur->MapToParent(itemEvent.GetLastPos()));
      }
    }
  res = (cur != 0);
  return res;
}

//-----------------------------------------------------------------------------
inline void vtkContextScene::EventCopy(const vtkContextMouseEvent &e)
{
  vtkContextMouseEvent &event = this->Storage->Event;
  event.SetPos(e.GetPos());
  event.SetScreenPos(vtkVector2i(e.GetPos().Cast<int>().GetData()));
  event.SetScenePos(e.GetPos());
  event.SetInteractor(e.GetInteractor());
}

//-----------------------------------------------------------------------------
void vtkContextScene::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out the chart's geometry if it has been set
  os << indent << "Widthxheight: " << this->Geometry[0] << "\t" << this->Geometry[1]
     << endl;
}
