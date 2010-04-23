/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBarMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBarMark.h"

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkMarkUtil.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkContextScene.h"
#include "vtkContextBufferId.h"
#include <cassert>
#include "vtkCommand.h" // EnterEvent,LeaveEvent
#include "vtkPanelMark.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBarMark);

//-----------------------------------------------------------------------------
vtkBarMark::vtkBarMark()
{
//  this->DebugOn();
  this->BufferId=0;
  this->MouseOver=false;
  this->ActiveItem=-1;
//  this->PaintIdMode=false;
  
  this->SetFillColor(vtkMarkUtil::DefaultSeriesColorFromParent);
  this->SetLineWidth(1);
  this->SetLineColor(vtkColor(0.0, 0.0, 0.0, 1.0));
}

//-----------------------------------------------------------------------------
vtkBarMark::~vtkBarMark()
{
  if(this->BufferId!=0)
    {
    this->BufferId->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkBarMark::PaintIds()
{
  assert("pre: not_yet" && !this->PaintIdMode);
  vtkDebugMacro("PaintId called.");
  
  // this call happens in the mark template, not in the mark instances.
  if(this->Parent!=0)
    {
//    this->PaintIdMode=true;
    this->Parent->PaintIdsOfMark(this);
//    this->PaintIdMode=false;
//    this->MarkInstances[j*numChildren + i]->Paint(painter);
    }
  
//  this->Scene->GetLastPainter()->SetTransform(this->GetTransform());
//  this->PaintIdMode=true;
//  this->Paint(this->Scene->GetLastPainter());
//  this->PaintIdMode=false;
  
  assert("post: done" && !this->PaintIdMode);
}

// ----------------------------------------------------------------------------
void vtkBarMark::UpdateBufferId()
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
//      vtkOpenGLContextBufferId *b=vtkOpenGLContextBufferId::New();
//      this->BufferId=b;
//      b->SetContext(static_cast<vtkOpenGLRenderWindow *>(
//                      this->Scene->GetRenderer()->GetRenderWindow()));
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
vtkIdType vtkBarMark::GetPickedItem(int x, int y)
{
  this->UpdateBufferId();
  
  vtkIdType result=this->BufferId->GetPickedItem(x,y);
  
  assert("post: valid_result" && result>=-1 &&
         result<this->Data.GetData(this).GetNumberOfChildren());
  return result;
}

// ----------------------------------------------------------------------------
bool vtkBarMark::MouseEnterEvent(const vtkContextMouseEvent &
                                   vtkNotUsed(mouse))
{
  this->MouseOver=true;
  return false;
}

// ----------------------------------------------------------------------------
bool vtkBarMark::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  bool result=false;
  
  // we can have this->MouseOver false if the enter event have been caught
  // previously by another context item.
  
  if(this->MouseOver)
    {
    vtkIdType numChildren=this->Data.GetData(this).GetNumberOfChildren();
    if(numChildren!=0)
      {
      vtkIdType pickedItem=this->GetPickedItem(mouse.ScreenPos[0],
                                               mouse.ScreenPos[1]);
      
      if(pickedItem!=-1)
        {
//        cout << "picked sector is"<< pickedItem << endl;
        }
      if(this->ActiveItem!=pickedItem)
        {
        if(this->ActiveItem!=-1)
          {
          this->MouseLeaveEventOnItem(this->ActiveItem);
          }
        this->ActiveItem=pickedItem;
        if(this->ActiveItem!=-1)
          {
          this->MouseEnterEventOnItem(this->ActiveItem);
          }
        }
      }
    }
  
  return result;
}

// ----------------------------------------------------------------------------
bool vtkBarMark::MouseLeaveEvent(const vtkContextMouseEvent &
                                 vtkNotUsed(mouse))
{
  this->MouseOver=false;
  return false;
}

// ----------------------------------------------------------------------------
void vtkBarMark::MouseEnterEventOnItem(int item)
{
  this->InvokeEvent(vtkCommand::EnterEvent,&item);
}

// ----------------------------------------------------------------------------
void vtkBarMark::MouseLeaveEventOnItem(int item)
{
  this->InvokeEvent(vtkCommand::LeaveEvent,&item);
}

//-----------------------------------------------------------------------------
bool vtkBarMark::Hit(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
unsigned char ConvertColor(double d)
{
  return static_cast<unsigned char>(255*d);
}

//-----------------------------------------------------------------------------
bool vtkBarMark::Paint(vtkContext2D *painter)
{
  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);
  double* width = this->Width.GetArray(this);
  double* height = this->Height.GetArray(this);
  vtkColor* fillColor = this->FillColor.GetArray(this);
  vtkColor* lineColor = this->LineColor.GetArray(this);
  double* lineWidth = this->LineWidth.GetArray(this);
  vtkIdType numChildren = this->Data.GetData(this).GetNumberOfChildren();
  
  if(this->PaintIdMode)
    {
    if(numChildren>16777214) // 24-bit limit, 0 reserved for background encoding.
      {
      vtkWarningMacro(<<"picking will not work properly as there are too many children. Children over 16777214 will be ignored.");
      numChildren=16777214;
      }
    }
  
  
  for (vtkIdType i = 0; i < numChildren; ++i)
    {
    painter->GetBrush()->SetColor(ConvertColor(fillColor[i].Red),
                                  ConvertColor(fillColor[i].Green),
                                  ConvertColor(fillColor[i].Blue),
                                  ConvertColor(fillColor[i].Alpha));
    painter->GetPen()->SetColor(ConvertColor(lineColor[i].Red),
                                ConvertColor(lineColor[i].Green),
                                ConvertColor(lineColor[i].Blue),
                                ConvertColor(lineColor[i].Alpha));
    
    if(this->PaintIdMode)
      {
      painter->ApplyId(i+1);
      }
    
    if (lineWidth[i] > 0.0)
      {
      painter->GetPen()->SetWidth(lineWidth[i]);
      }
    else
      {
      painter->GetPen()->SetOpacity(0);
      }
    painter->DrawRect(left[i], bottom[i], width[i], height[i]);
    }
  return true;
}

// ----------------------------------------------------------------------------
void vtkBarMark::ReleaseGraphicsResources()
{
  if(this->BufferId!=0)
    {
    this->BufferId->ReleaseGraphicsResources();
    }
}

//-----------------------------------------------------------------------------
void vtkBarMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
