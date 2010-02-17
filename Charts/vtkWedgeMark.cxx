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

vtkInformationKeyMacro(vtkWedgeMark,ANGLE,Double);
vtkInformationKeyMacro(vtkWedgeMark,INNER_RADIUS,Double);
vtkInformationKeyMacro(vtkWedgeMark,FILL_STYLE,String);

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkWedgeMark, "1.3");
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
bool vtkWedgeMark::Hit(const vtkContextMouseEvent &)
{
  return false;
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
    painter->GetPen()->SetColorF(lineColor[i].Red,
                                lineColor[i].Green,
                                lineColor[i].Blue,
                                lineColor[i].Alpha);
    if (lineWidth[i] > 0.0)
      {
      painter->GetPen()->SetWidth(lineWidth[i]);
      }
    else
      {
      painter->GetPen()->SetOpacity(0);
      }
    painter->DrawWedge(left[i],bottom[i],outerRadius[i],innerRadius[i],
                       a0,a1);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkWedgeMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
