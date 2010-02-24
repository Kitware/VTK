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

vtkInformationKeyMacro(vtkWedgeMark,ANGLE,Double);
vtkInformationKeyMacro(vtkWedgeMark,INNER_RADIUS,Double);
vtkInformationKeyMacro(vtkWedgeMark,FILL_STYLE,String);

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkWedgeMark, "1.5");
vtkStandardNewMacro(vtkWedgeMark);

// ----------------------------------------------------------------------------
vtkWedgeMark::vtkWedgeMark()
{
  // this->Information created in vtkMark
  
  // add the default keys.
  this->AddWedgeDefault();
}

// ----------------------------------------------------------------------------
vtkWedgeMark::~vtkWedgeMark()
{
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

//-----------------------------------------------------------------------------
void vtkWedgeMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
