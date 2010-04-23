/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWedgeMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWedgeMark.h"
#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkMarkUtil.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkContextScene.h"
#include "vtkContextBufferId.h"
//#include "vtkOpenGLContextBufferId.h"
#include "vtkCommand.h" // EnterEvent,LeaveEvent
//#include "vtkRenderer.h"
//#include "vtkOpenGLRenderWindow.h"

vtkInformationKeyMacro(vtkWedgeMark,ANGLE,Double);
vtkInformationKeyMacro(vtkWedgeMark,INNER_RADIUS,Double);
vtkInformationKeyMacro(vtkWedgeMark,FILL_STYLE,String);

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkWedgeMark);

// ----------------------------------------------------------------------------
vtkWedgeMark::vtkWedgeMark()
{
  this->BufferId=0;
  this->MouseOver=false;
  this->ActiveItem=-1;
  this->PaintIdMode=false;
  
  // this->Information created in vtkMark
  
  // add the default keys.
  this->AddWedgeDefault();
}

// ----------------------------------------------------------------------------
vtkWedgeMark::~vtkWedgeMark()
{
  if(this->BufferId!=0)
    {
    this->BufferId->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkWedgeMark::PaintIds()
{
  assert("pre: not_yet" && !this->PaintIdMode);
  vtkDebugMacro("PaintId called.");
  
  this->Scene->GetLastPainter()->SetTransform(this->GetTransform());
  this->PaintIdMode=true;
  this->Paint(this->Scene->GetLastPainter());
  this->PaintIdMode=false;
  
  assert("post: done" && !this->PaintIdMode);
}

// ----------------------------------------------------------------------------
void vtkWedgeMark::UpdateBufferId()
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
vtkIdType vtkWedgeMark::GetPickedItem(int x, int y)
{
  this->UpdateBufferId();
  
  vtkIdType result=this->BufferId->GetPickedItem(x,y);
  
  assert("post: valid_result" && result>=-1 &&
         result<this->Data.GetData(this).GetNumberOfChildren());
  return result;
}

// ----------------------------------------------------------------------------
bool vtkWedgeMark::MouseEnterEvent(const vtkContextMouseEvent &
                                   vtkNotUsed(mouse))
{
  this->MouseOver=true;
  return false;
}

// ----------------------------------------------------------------------------
bool vtkWedgeMark::MouseMoveEvent(const vtkContextMouseEvent &mouse)
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
          this->MouseLeaveEventOnSector(this->ActiveItem);
          }
        this->ActiveItem=pickedItem;
        if(this->ActiveItem!=-1)
          {
          this->MouseEnterEventOnSector(this->ActiveItem);
          }
        }
      }
    }
  
  return result;
}

// ----------------------------------------------------------------------------
bool vtkWedgeMark::MouseLeaveEvent(const vtkContextMouseEvent &
                                   vtkNotUsed(mouse))
{
  this->MouseOver=false;
  return false;
}

// ----------------------------------------------------------------------------
void vtkWedgeMark::MouseEnterEventOnSector(int sector)
{
  this->InvokeEvent(vtkCommand::EnterEvent,&sector);
}

// ----------------------------------------------------------------------------
void vtkWedgeMark::MouseLeaveEventOnSector(int sector)
{
  this->InvokeEvent(vtkCommand::LeaveEvent,&sector);
}

//-----------------------------------------------------------------------------
bool vtkWedgeMark::Hit(const vtkContextMouseEvent &mouse)
{
  bool result;
  
  // each sector can have different left/bottom outer/inner radius, so we
  // can't just try to hit a big bounding box first.
  
  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);
  double* lineWidth = this->LineWidth.GetArray(this);
  double *outerRadius=this->OuterRadius.GetArray(this);
  double *innerRadius=this->InnerRadius.GetArray(this);
  double *angle=this->Angle.GetArray(this);
  
  vtkIdType numChildren=this->Data.GetData(this).GetNumberOfChildren();
  double a0=0.0;
  double a1=0.0;
  
  vtkIdType i=0;
  result=false;
  while(!result && i<numChildren)
    {
    a0=a1;
    a1=angle[i]+a0;
    
    // four tests to test if the hit is inside:
    // 1. Is it outside the circle of center (left,bottom) and radius
    // innerRadius-lineWidth?
    // 2. Is it inside the circle of center (left,bottom) and radius
    // outerRadius+lineWidth?
    // 3. Is it on the positive side of the start line shifted by lineWidth on
    // the negative side?
    // 4. Is it on the negative side of the start line shifted by lineWidth on
    // the negative side?
    // hit=test1 && test2 && test3 && test4.
    
    // Test1
    double dx=mouse.Pos[0]-left[i];
    double dy=mouse.Pos[1]-bottom[i];
    double dot=dx*dx+dy*dy;
    double r=innerRadius[i]-lineWidth[i];
    
    // distance between center and point is greater than radius
    result=dot>=r*r;
    
    if(result)
      {
      // Test 2
      r=outerRadius[i]+lineWidth[i];
      result=dot<=r*r;
      if(result)
        {
        // Test 3
         double a0r=vtkMath::RadiansFromDegrees(a0);
         
         result=cos(a0r)*dy-sin(a0r)*dx+lineWidth[i]>=0;
         if(result)
           {
           // Test 4
           double a1r=vtkMath::RadiansFromDegrees(a1);
           
           result=cos(a1r)*dy-sin(a1r)*dx-lineWidth[i]<=0;
           }
        }
      }
      
    ++i;
    }
  
//  cout << "wedge hit=" << result << endl;
  
  return result;
}

// ----------------------------------------------------------------------------
void vtkWedgeMark::AddWedgeDefault()
{
  // 1. fill style is a categorial color
  this->Fields->Set(vtkWedgeMark::FILL_STYLE(),"categorial");
  // 2. no stroke
  
  this->SetLineColor(vtkMarkUtil::DefaultSeriesColorFromIndex);
  this->SetFillColor(vtkMarkUtil::DefaultSeriesColorFromIndex);
}

// ----------------------------------------------------------------------------
int vtkWedgeMark::GetType()
{
  return WEDGE;
}

// ----------------------------------------------------------------------------
double vtkWedgeMark::GetMidAngle()
{
  return 0.0;
}
  
// ----------------------------------------------------------------------------
double vtkWedgeMark::GetMidRadius()
{
//  double innerRadius=0.0;
//  if(!this->Fields->Has(vtkWedgeMark::INNER_RADIUS()))
//    {
//    }
  return 0.0;
}
  
//-----------------------------------------------------------------------------
bool vtkWedgeMark::Paint(vtkContext2D *painter)
{
  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);
  vtkColor* fillColor = this->FillColor.GetArray(this);
  vtkColor* lineColor = this->LineColor.GetArray(this);
  double* lineWidth = this->LineWidth.GetArray(this);
  
  double *outerRadius=this->OuterRadius.GetArray(this);
  double *innerRadius=this->InnerRadius.GetArray(this);
//  double *startAngle=this->StartAngle.GetArray(this);
//  double *stopAngle=this->StopAngle.GetArray(this);
  double *angle=this->Angle.GetArray(this);
  
  vtkIdType numChildren = this->Data.GetData(this).GetNumberOfChildren();
  
  if(this->PaintIdMode)
    {
    if(numChildren>16777214) // 24-bit limit, 0 reserved for background encoding.
      {
      vtkWarningMacro(<<"picking will not work properly as there are too many children. Children over 16777214 will be ignored.");
      numChildren=16777214;
      }
    
    }
  
  double a0=0.0;
  double a1=0.0;
  for (vtkIdType i = 0; i < numChildren; ++i)
    {
    a0=a1;
    a1=angle[i]+a0;
    
    painter->GetBrush()->SetColorF(fillColor[i].Red,
                                   fillColor[i].Green,
                                   fillColor[i].Blue,
                                   fillColor[i].Alpha);
    if(this->PaintIdMode)
      {
      painter->ApplyId(i+1);
      }
    
    painter->DrawWedge(left[i],bottom[i],outerRadius[i],innerRadius[i],
                       a0,a1);
    
    if (lineWidth[i] > 0.0)
      {
      painter->GetPen()->SetWidth(lineWidth[i]);
      painter->GetPen()->SetColorF(lineColor[i].Red,
                                lineColor[i].Green,
                                lineColor[i].Blue,
                                lineColor[i].Alpha);
      
      double a0r=vtkMath::RadiansFromDegrees(a0);
      double a1r=vtkMath::RadiansFromDegrees(a1);
      
      // bottom line of the wedge
      painter->DrawLine(left[i]+innerRadius[i]*cos(a0r),
                        bottom[i]+innerRadius[i]*sin(a0r),
                        left[i]+outerRadius[i]*cos(a0r),
                        bottom[i]+outerRadius[i]*sin(a0r));
      // upper line of the wedge
      painter->DrawLine(left[i]+innerRadius[i]*cos(a1r),
                        bottom[i]+innerRadius[i]*sin(a1r),
                        left[i]+outerRadius[i]*cos(a1r),
                        bottom[i]+outerRadius[i]*sin(a1r));
      
      // inside arc
      painter->DrawArc(left[i],bottom[i],innerRadius[i],a0,a1);
      // outside arc
      painter->DrawArc(left[i],bottom[i],outerRadius[i],a0,a1);
      }
    }
  return true;
}

// ----------------------------------------------------------------------------
void vtkWedgeMark::ReleaseGraphicsResources()
{
  if(this->BufferId!=0)
    {
    this->BufferId->ReleaseGraphicsResources();
    }
}

//-----------------------------------------------------------------------------
void vtkWedgeMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
