/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMark.h"

//#include "vtkAbstractArray.h"
#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkPanelMark.h"
#include "vtkPen.h"
//#include "vtkTable.h"

#include "vtkBarMark.h"
#include "vtkLineMark.h"
#include "vtkWedgeMark.h"
#include "vtkValueHolder.txx"
#include "vtkInformation.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMark, "1.2");
vtkStandardNewMacro(vtkMark);

//-----------------------------------------------------------------------------
vtkMark::vtkMark()
{
  this->Fields=vtkInformation::New();
  this->Parent = NULL;
  this->Index = 0;
  this->ParentMarkIndex = 0;
  this->ParentDataIndex = 0;
}

//-----------------------------------------------------------------------------
vtkMark::~vtkMark()
{
  this->Fields->Delete();
}

//-----------------------------------------------------------------------------
vtkMark* vtkMark::CreateMark(int type)
{
  vtkMark* m = NULL;
  switch (type)
    {
    case BAR:
      m = vtkBarMark::New();
      break;
    case LINE:
      m = vtkLineMark::New();
      break;
    case WEDGE:
      m=vtkWedgeMark::New();
      break;
    }
  return m;
}

//-----------------------------------------------------------------------------
void vtkMark::Extend(vtkMark* m)
{
  this->Data = m->GetData();
  this->Left.SetValue(m->GetLeft());
  this->Right.SetValue(m->GetRight());
  this->Top.SetValue(m->GetTop());
  this->Bottom.SetValue(m->GetBottom());
  this->Title.SetValue(m->GetTitle());
  this->FillColor.SetValue(m->GetFillColor());
  this->LineColor.SetValue(m->GetLineColor());
  this->LineWidth.SetValue(m->GetLineWidth());
  this->Width.SetValue(m->GetWidth());
  this->Height.SetValue(m->GetHeight());
  this->InnerRadius.SetValue(m->GetInnerRadius());
  this->OuterRadius.SetValue(m->GetOuterRadius());
  this->StartAngle.SetValue(m->GetStartAngle());
  this->StopAngle.SetValue(m->GetStopAngle());
  this->Angle.SetValue(m->GetAngle());
}

//-----------------------------------------------------------------------------
void vtkMark::Update()
  {
  this->Left.Update(this);
  this->Right.Update(this);
  this->Top.Update(this);
  this->Bottom.Update(this);
  this->Title.Update(this);
  this->FillColor.Update(this);
  this->LineColor.Update(this);
  this->LineWidth.Update(this);
  this->Width.Update(this);
  this->Height.Update(this);
  this->InnerRadius.Update(this);
  this->OuterRadius.Update(this);
  this->StartAngle.Update(this);
  this->StopAngle.Update(this);
  this->Angle.Update(this);
  }

void vtkMark::SetData(vtkDataValue data)
  {
    this->Data = data;
    this->DataChanged();
  }

vtkDataValue vtkMark::GetData()
  { return this->Data; }

void vtkMark::SetLeft(vtkValue<double> v)
  { this->Left.SetValue(v); }


vtkValue<double>& vtkMark::GetLeft()
  { return this->Left.GetValue(); }

void vtkMark::SetRight(vtkValue<double> v)
  { this->Right.SetValue(v); }

vtkValue<double>& vtkMark::GetRight()
  { return this->Right.GetValue(); }

void vtkMark::SetTop(vtkValue<double> v)
  { this->Top.SetValue(v); }

vtkValue<double>& vtkMark::GetTop()
  { return this->Top.GetValue(); }

void vtkMark::SetBottom(vtkValue<double> v)
  { this->Bottom.SetValue(v); }

vtkValue<double>& vtkMark::GetBottom()
  { return this->Bottom.GetValue(); }

void vtkMark::SetTitle(vtkValue<vtkstd::string> v)
  { this->Title.SetValue(v); }

vtkValue<vtkstd::string>& vtkMark::GetTitle()
  { return this->Title.GetValue(); }

void vtkMark::SetLineColor(vtkValue<vtkColor> v)
  { this->LineColor.SetValue(v); }

vtkValue<vtkColor>& vtkMark::GetLineColor()
  { return this->LineColor.GetValue(); }

void vtkMark::SetFillColor(vtkValue<vtkColor> v)
  { this->FillColor.SetValue(v); }

vtkValue<vtkColor>& vtkMark::GetFillColor()
  { return this->FillColor.GetValue(); }

void vtkMark::SetLineWidth(vtkValue<double> v)
  { this->LineWidth.SetValue(v); }

vtkValue<double>& vtkMark::GetLineWidth()
  { return this->LineWidth.GetValue(); }

void vtkMark::SetWidth(vtkValue<double> v)
  { this->Width.SetValue(v); }

vtkValue<double>& vtkMark::GetWidth()
  { return this->Width.GetValue(); }

void vtkMark::SetHeight(vtkValue<double> v)
  { this->Height.SetValue(v); }
vtkValue<double>& vtkMark::GetHeight()
  { return this->Height.GetValue(); }


// For wedges.
void vtkMark::SetOuterRadius(vtkValue<double> v)
{
  this->OuterRadius.SetValue(v);
}

vtkValue<double>& vtkMark::GetOuterRadius()
{
  return this->OuterRadius.GetValue();
}
  
void vtkMark::SetInnerRadius(vtkValue<double> v)
{
  this->InnerRadius.SetValue(v);
}

vtkValue<double>& vtkMark::GetInnerRadius()
{
  return this->InnerRadius.GetValue();
}
  
void vtkMark::SetStartAngle(vtkValue<double> v)
{
  this->StartAngle.SetValue(v);
}

vtkValue<double>& vtkMark::GetStartAngle()
{
  return this->StartAngle.GetValue();
}
  
void vtkMark::SetStopAngle(vtkValue<double> v)
{
  this->StopAngle.SetValue(v);
}

vtkValue<double>& vtkMark::GetStopAngle()
{
  return this->StopAngle.GetValue();
}
  
void vtkMark::SetAngle(vtkValue<double> v)
{
  this->Angle.SetValue(v);
}

vtkValue<double>& vtkMark::GetAngle()
{
  return this->Angle.GetValue();
}

void vtkMark::SetParent(vtkPanelMark* p)
  { this->Parent = p; }

vtkPanelMark* vtkMark::GetParent()
  { return this->Parent; }

void vtkMark::SetIndex(vtkIdType i)
  { this->Index = i; }

vtkIdType vtkMark::GetIndex()
  { return this->Index; }

void vtkMark::DataChanged()
  {
  this->Left.SetDirty(true);
  this->Right.SetDirty(true);
  this->Top.SetDirty(true);
  this->Bottom.SetDirty(true);
  this->Title.SetDirty(true);
  this->FillColor.SetDirty(true);
  this->LineColor.SetDirty(true);
  this->LineWidth.SetDirty(true);
  this->Width.SetDirty(true);
  this->Height.SetDirty(true);
  this->InnerRadius.SetDirty(true);
  this->OuterRadius.SetDirty(true);
  this->StartAngle.SetDirty(true);
  this->StopAngle.SetDirty(true);
  this->Angle.SetDirty(true);
  }

int vtkMark::GetType()
{
  return BAR;
}

double vtkMark::GetCousinLeft()
  {
  if (this->Parent && this->ParentDataIndex > 0)
    {
    vtkMark* cousin = this->Parent->GetMarkInstance(this->ParentMarkIndex, this->ParentDataIndex-1);
    return cousin->Left.GetArray(cousin)[this->Index];
    }
  return 0.0;
  }

double vtkMark::GetCousinRight()
  {
  if (this->Parent && this->ParentDataIndex > 0)
    {
    vtkMark* cousin = this->Parent->GetMarkInstance(this->ParentMarkIndex, this->ParentDataIndex-1);
    return cousin->Right.GetArray(cousin)[this->Index];
    }
  return 0.0;
  }

double vtkMark::GetCousinTop()
  {
  if (this->Parent && this->ParentDataIndex > 0)
    {
    vtkMark* cousin = this->Parent->GetMarkInstance(this->ParentMarkIndex, this->ParentDataIndex-1);
    return cousin->Top.GetArray(cousin)[this->Index];
    }
  return 0.0;
  }

double vtkMark::GetCousinBottom()
  {
  if (this->Parent && this->ParentDataIndex > 0)
    {
    vtkMark* cousin = this->Parent->GetMarkInstance(this->ParentMarkIndex, this->ParentDataIndex-1);
    return cousin->Bottom.GetArray(cousin)[this->Index];
    }
  return 0.0;
  }

double vtkMark::GetCousinWidth()
  {
  if (this->Parent && this->ParentDataIndex > 0)
    {
    vtkMark* cousin = this->Parent->GetMarkInstance(this->ParentMarkIndex, this->ParentDataIndex-1);
    return cousin->Width.GetArray(cousin)[this->Index];
    }
  return 0.0;
  }

double vtkMark::GetCousinHeight()
  {
  if (this->Parent && this->ParentDataIndex > 0)
    {
    vtkMark* cousin = this->Parent->GetMarkInstance(this->ParentMarkIndex, this->ParentDataIndex-1);
    return cousin->Height.GetArray(cousin)[this->Index];
    }
  return 0.0;
  }


//-----------------------------------------------------------------------------
void vtkMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
