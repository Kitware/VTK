/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPanelMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPanelMark.h"

#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkTransform2D.h"
#include <cassert>
#include "vtkContextBufferId.h"
#include "vtkContextScene.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPanelMark, "1.8");
vtkStandardNewMacro(vtkPanelMark);

//-----------------------------------------------------------------------------
vtkPanelMark::vtkPanelMark()
{
  this->BufferId=0;
  this->MouseOver=false;
  this->ActiveItem=-1;
}

//-----------------------------------------------------------------------------
vtkPanelMark::~vtkPanelMark()
{
  if(this->BufferId!=0)
    {
    this->BufferId->Delete();
    }
}

//-----------------------------------------------------------------------------
vtkMark* vtkPanelMark::Add(int type)
{
  vtkSmartPointer<vtkMark> m;
  m.TakeReference(vtkMark::CreateMark(type));
  if (this->Marks.size() > 0)
    {
    m->Extend(this->Marks.back());
    }
  this->Marks.push_back(m);
  m->SetParent(this);
  m->SetScene(this->Scene);
  return m;
}

//-----------------------------------------------------------------------------
void vtkPanelMark::Update()
{
  this->MarkInstances.clear();
  this->Left.Update(this);
  this->Right.Update(this);
  this->Top.Update(this);
  this->Bottom.Update(this);  
  vtkIdType numMarks = static_cast<vtkIdType>(this->Marks.size());
  
  // Create only a single instance if no real data is set on panel. 
  vtkIdType numChildren = 1; 
  vtkDataElement data = this->Data.GetData(this);
  if(data.IsValid())
    {    
    numChildren = data.GetNumberOfChildren();
    }
  for (vtkIdType j = 0; j < numMarks; ++j)
    {
    for (vtkIdType i = 0; i < numChildren; ++i)
      {
      this->Index = i;
      this->Marks[j]->DataChanged();
      this->Marks[j]->Update();
      vtkSmartPointer<vtkMark> m;
      m.TakeReference(vtkMark::CreateMark(this->Marks[j]->GetType()));
      m->Extend(this->Marks[j]);
      m->SetParent(this->Marks[j]->GetParent());
      m->SetParentMarkIndex(j);
      m->SetParentDataIndex(i);
      this->MarkInstances.push_back(m);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPanelMark::PaintIds()
{
  vtkDebugMacro("PaintId called.");
  size_t size = this->Marks.size();
  
  if(size>16777214) // 24-bit limit, 0 reserved for background encoding.
    {
    vtkWarningMacro(<<"picking will not work properly as there are two many items. Items over 16777214 will be ignored.");
    size=16777214;
    }
  for (size_t i = 0; i < size; ++i)
    {
    this->Scene->GetLastPainter()->SetTransform(this->GetTransform());
    this->Scene->GetLastPainter()->ApplyId(i+1);
    this->Marks[i]->Paint(this->Scene->GetLastPainter());
    }
}

// ----------------------------------------------------------------------------
void vtkPanelMark::UpdateBufferId()
{
  vtkAbstractContextBufferId *bi=this->Scene->GetBufferId();
  
  int width=bi->GetWidth();
  int height=bi->GetHeight();
  
  if(this->BufferId==0 || width!=this->BufferId->GetWidth() ||
     height!=this->BufferId->GetHeight())
    {
    if(this->BufferId==0)
      {
      this->BufferId=vtkContextBufferId::New();
      }
    this->BufferId->SetWidth(width);
    this->BufferId->SetHeight(height);
    this->BufferId->Allocate();
    
    this->Scene->GetLastPainter()->BufferIdModeBegin(this->BufferId);
    this->PaintIds();
    this->Scene->GetLastPainter()->BufferIdModeEnd();
    }
}

// ----------------------------------------------------------------------------
vtkIdType vtkPanelMark::GetPickedItem(int x, int y)
{
  this->UpdateBufferId();
  
  vtkIdType result=this->BufferId->GetPickedItem(x,y);
  
  assert("post: valid_result" && result>=-1 &&
         result<static_cast<vtkIdType>(this->Marks.size()));
  return result;
}

// ----------------------------------------------------------------------------
bool vtkPanelMark::MouseEnterEvent(const vtkContextMouseEvent &
                                   vtkNotUsed(mouse))
{
  this->MouseOver=true;
  return false;
}

// ----------------------------------------------------------------------------
bool vtkPanelMark::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  bool result=false;
  
  // we can have this->MouseOver false if the enter event have been caught
  // previously by another context item.
  
  if(this->MouseOver)
    {
    vtkIdType numMarks=static_cast<vtkIdType>(this->Marks.size());
    if(numMarks!=0)
      {
      vtkIdType pickedItem=this->GetPickedItem(mouse.ScreenPos[0],
                                               mouse.ScreenPos[1]);
      
      if(this->ActiveItem!=pickedItem)
        {
        if(this->ActiveItem!=-1)
          {
          this->Marks[this->ActiveItem]->MouseLeaveEvent(mouse);
          }
        this->ActiveItem=pickedItem;
        if(this->ActiveItem!=-1)
          {
          this->Marks[this->ActiveItem]->MouseEnterEvent(mouse);
          }
        }
      
      // propagate mouse move events
      size_t size = this->Marks.size();
      for (size_t i = 0; i < size; ++i)
        {
        this->Marks[i]->MouseMoveEvent(mouse);
        }
      }
    }
  
  return result;
}

// ----------------------------------------------------------------------------
bool vtkPanelMark::MouseLeaveEvent(const vtkContextMouseEvent &
                                   vtkNotUsed(mouse))
{
  this->MouseOver=false;
  return false;
}
  
// ----------------------------------------------------------------------------
bool vtkPanelMark::Hit(const vtkContextMouseEvent &mouse)
{ 
  // propagate to all the contained marks
  vtkIdType numMarks=static_cast<vtkIdType>(this->Marks.size());
  vtkIdType j=0;
  bool result=false;
  while(!result && j<numMarks)
    {
    result=this->Marks[j]->Hit(mouse);
    ++j;
    }
  return result;
}

//-----------------------------------------------------------------------------
bool vtkPanelMark::Paint(vtkContext2D* painter)
{
  //TODO: Be smarter about the update
  this->Update();

  if (!painter->GetTransform())
    {
    vtkSmartPointer<vtkTransform2D> trans = vtkSmartPointer<vtkTransform2D>::New();
    trans->Identity();
    painter->SetTransform(trans);
    }

  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);  
  size_t numMarks = this->Marks.size();
  
  vtkDataElement data = this->Data.GetData(this);
  vtkIdType numChildren = 1;
  if(data.IsValid())
    {
    numChildren = data.GetNumberOfChildren();
    }
  for (size_t j = 0; j < numMarks; ++j)
    {
    for (vtkIdType i = 0; i < numChildren; ++i)
      {
      this->Index = i;
      painter->GetTransform()->Translate(left[i], bottom[i]);            
      painter->SetTransform(painter->GetTransform());      
      this->MarkInstances[j*numChildren + i]->Paint(painter);
      painter->GetTransform()->Translate(-left[i], -bottom[i]);
      painter->SetTransform(painter->GetTransform());
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPanelMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
