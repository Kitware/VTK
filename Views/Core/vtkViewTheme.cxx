/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewTheme.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkViewTheme.h"

#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"

vtkStandardNewMacro(vtkViewTheme);
vtkCxxSetObjectMacro(vtkViewTheme, PointLookupTable, vtkScalarsToColors);
vtkCxxSetObjectMacro(vtkViewTheme, CellLookupTable, vtkScalarsToColors);
vtkCxxSetObjectMacro(vtkViewTheme, PointTextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkViewTheme, CellTextProperty, vtkTextProperty);

vtkViewTheme::vtkViewTheme()
{
  this->PointSize = 5;
  this->LineWidth = 1;

  this->PointColor[0] = this->PointColor[1] = this->PointColor[2] = 1;
  this->PointOpacity = 1;
  vtkLookupTable* plut = vtkLookupTable::New();
  plut->SetHueRange(0.667, 0);
  plut->SetSaturationRange(1, 1);
  plut->SetValueRange(1, 1);
  plut->SetAlphaRange(1, 1);
  plut->Build();
  this->PointLookupTable = plut;

  this->CellColor[0] = this->CellColor[1] = this->CellColor[2] = 1;
  this->CellOpacity = 0.5;
  vtkLookupTable* clut = vtkLookupTable::New();
  clut->SetHueRange(0.667, 0);
  clut->SetSaturationRange(0.5, 1);
  clut->SetValueRange(0.5, 1);
  clut->SetAlphaRange(0.5, 1);
  clut->Build();
  this->CellLookupTable = clut;

  this->OutlineColor[0] = this->OutlineColor[1] = this->OutlineColor[2] = 0;

  this->SelectedPointColor[0] = this->SelectedPointColor[2] = 1;
  this->SelectedPointColor[1] = 0;
  this->SelectedPointOpacity = 1;
  this->SelectedCellColor[0] = this->SelectedCellColor[2] = 1;
  this->SelectedCellColor[1] = 0;
  this->SelectedCellOpacity = 1;

  this->BackgroundColor[0] = this->BackgroundColor[1] =
    this->BackgroundColor[2] = 0.0;
  this->BackgroundColor2[0] = this->BackgroundColor2[1] =
    this->BackgroundColor2[2] = 0.3;

  this->ScalePointLookupTable = true;
  this->ScaleCellLookupTable = true;

  this->PointTextProperty = vtkTextProperty::New();
  this->PointTextProperty->SetColor(1, 1, 1);
  this->PointTextProperty->BoldOn();
  this->PointTextProperty->SetJustificationToCentered();
  this->PointTextProperty->SetVerticalJustificationToCentered();
  this->PointTextProperty->SetFontSize(12);
  this->CellTextProperty = vtkTextProperty::New();
  this->CellTextProperty->SetColor(0.7, 0.7, 0.7);
  this->CellTextProperty->BoldOn();
  this->CellTextProperty->SetJustificationToCentered();
  this->CellTextProperty->SetVerticalJustificationToCentered();
  this->CellTextProperty->SetFontSize(10);
}

vtkViewTheme::~vtkViewTheme()
{
  if (this->CellLookupTable)
  {
    this->CellLookupTable->Delete();
  }
  if (this->PointLookupTable)
  {
    this->PointLookupTable->Delete();
  }
  if (this->CellTextProperty)
  {
    this->CellTextProperty->Delete();
  }
  if (this->PointTextProperty)
  {
    this->PointTextProperty->Delete();
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetPointHueRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetHueRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetPointHueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetHueRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetPointHueRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetHueRange();
  }
  return 0;
}

void vtkViewTheme::GetPointHueRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetHueRange(mn, mx);
  }
}

void vtkViewTheme::GetPointHueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetHueRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetPointSaturationRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetSaturationRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetPointSaturationRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetSaturationRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetPointSaturationRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetSaturationRange();
  }
  return 0;
}

void vtkViewTheme::GetPointSaturationRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetSaturationRange(mn, mx);
  }
}

void vtkViewTheme::GetPointSaturationRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetSaturationRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetPointValueRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetValueRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetPointValueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetValueRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetPointValueRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetValueRange();
  }
  return 0;
}

void vtkViewTheme::GetPointValueRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetValueRange(mn, mx);
  }
}

void vtkViewTheme::GetPointValueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetValueRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetPointAlphaRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetAlphaRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetPointAlphaRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->SetAlphaRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetPointAlphaRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    return lut->GetAlphaRange();
  }
  return 0;
}

void vtkViewTheme::GetPointAlphaRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetAlphaRange(mn, mx);
  }
}

void vtkViewTheme::GetPointAlphaRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->PointLookupTable))
  {
    lut->GetAlphaRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetCellHueRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetHueRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetCellHueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetHueRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetCellHueRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetHueRange();
  }
  return 0;
}

void vtkViewTheme::GetCellHueRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetHueRange(mn, mx);
  }
}

void vtkViewTheme::GetCellHueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetHueRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetCellSaturationRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetSaturationRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetCellSaturationRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetSaturationRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetCellSaturationRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetSaturationRange();
  }
  return 0;
}

void vtkViewTheme::GetCellSaturationRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetSaturationRange(mn, mx);
  }
}

void vtkViewTheme::GetCellSaturationRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetSaturationRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetCellValueRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetValueRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetCellValueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetValueRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetCellValueRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetValueRange();
  }
  return 0;
}

void vtkViewTheme::GetCellValueRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetValueRange(mn, mx);
  }
}

void vtkViewTheme::GetCellValueRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetValueRange(rng);
  }
}

//---------------------------------------------------------------------------
void vtkViewTheme::SetCellAlphaRange(double mn, double mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetAlphaRange(mn, mx);
    lut->Build();
  }
}

void vtkViewTheme::SetCellAlphaRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->SetAlphaRange(rng);
    lut->Build();
  }
}

double* vtkViewTheme::GetCellAlphaRange()
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    return lut->GetAlphaRange();
  }
  return 0;
}

void vtkViewTheme::GetCellAlphaRange(double& mn, double& mx)
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetAlphaRange(mn, mx);
  }
}

void vtkViewTheme::GetCellAlphaRange(double rng[2])
{
  if (vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->CellLookupTable))
  {
    lut->GetAlphaRange(rng);
  }
}

void vtkViewTheme::SetVertexLabelColor(double r, double g, double b)
{
  if (this->PointTextProperty)
  {
    this->PointTextProperty->SetColor(r, g, b);
  }
}

double *vtkViewTheme::GetVertexLabelColor()
{
  return this->PointTextProperty ? this->PointTextProperty->GetColor() : 0;
}

void vtkViewTheme::SetEdgeLabelColor(double r, double g, double b)
{
  if (this->CellTextProperty)
  {
    this->CellTextProperty->SetColor(r, g, b);
  }
}

double *vtkViewTheme::GetEdgeLabelColor()
{
  return this->CellTextProperty ? this->CellTextProperty->GetColor() : 0;
}

vtkViewTheme* vtkViewTheme::CreateOceanTheme()
{
  vtkViewTheme* theme = vtkViewTheme::New();

  theme->SetPointSize(7);
  theme->SetLineWidth(3);

  theme->SetBackgroundColor(.8, .8, .8);
  theme->SetBackgroundColor2(1, 1, 1);
  theme->GetPointTextProperty()->SetColor(0, 0, 0);
  theme->GetCellTextProperty()->SetColor(.2, .2, .2);

  theme->SetPointColor(0.5, 0.5, 0.5);
  theme->SetPointHueRange(0.667, 0);
  theme->SetPointSaturationRange(1, 1);
  theme->SetPointValueRange(0.75, 0.75);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.5);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.75, 1);
  theme->SetCellValueRange(0.75, .75);
  theme->SetCellSaturationRange(1, 1);

  theme->SetOutlineColor(0,0,0);

  theme->SetSelectedPointColor(.9, .4, .9);
  theme->SetSelectedCellColor(.8, .3, .8);

  return theme;
}

vtkViewTheme* vtkViewTheme::CreateNeonTheme()
{
  vtkViewTheme* theme = vtkViewTheme::New();

  theme->SetPointSize(7);
  theme->SetLineWidth(3);

  theme->SetBackgroundColor(.2,.2,.4);
  theme->SetBackgroundColor2(.1,.1,.2);
  theme->GetPointTextProperty()->SetColor(1, 1, 1);
  theme->GetCellTextProperty()->SetColor(.7, .7, .7);

  theme->SetPointColor(0.5, 0.5, 0.6);
  theme->SetPointHueRange(0.6, 0);
  theme->SetPointSaturationRange(1, 1);
  theme->SetPointValueRange(1, 1);

  theme->SetCellColor(0.5, 0.5, 0.7);
  theme->SetCellOpacity(0.5);
  theme->SetCellHueRange(0.57, 0);
  theme->SetCellAlphaRange(0.75, 1);
  theme->SetCellValueRange(0.75, 1);
  theme->SetCellSaturationRange(1, 1);

  theme->SetOutlineColor(0, 0, 0);

  theme->SetSelectedPointColor(.9, .4, .9);
  theme->SetSelectedCellColor(.8, .3, .8);

  return theme;
}

vtkViewTheme* vtkViewTheme::CreateMellowTheme()
{
  vtkViewTheme* theme = vtkViewTheme::New();

  theme->SetPointSize(7);
  theme->SetLineWidth(2);

  theme->SetBackgroundColor(0.3, 0.3, 0.25); // Darker Tan
  theme->SetBackgroundColor2(0.6, 0.6, 0.5); // Tan
  theme->GetPointTextProperty()->SetColor(1, 1, 1);
  theme->GetCellTextProperty()->SetColor(.7, .7, 1);

  theme->SetPointColor(0.0, 0.0, 1.0);
  theme->SetPointHueRange(0.667, 0);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.4);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.4, 1);
  theme->SetCellValueRange(0.5, 1);
  theme->SetCellSaturationRange(0.5, 1);

  theme->SetOutlineColor(0, 0, 0);

  theme->SetSelectedPointColor(1, 1, 1);
  theme->SetSelectedCellColor(0, 0, 0);

  return theme;
}

bool vtkViewTheme::LookupMatchesPointTheme(vtkScalarsToColors* s2c)
{
  if (!s2c)
  {
    return false;
  }
  vtkLookupTable* lut = vtkLookupTable::SafeDownCast(s2c);
  if (!lut)
  {
    return false;
  }
  if (lut->GetHueRange()[0] == this->GetPointHueRange()[0] &&
      lut->GetHueRange()[1] == this->GetPointHueRange()[1] &&
      lut->GetSaturationRange()[0] == this->GetPointSaturationRange()[0] &&
      lut->GetSaturationRange()[1] == this->GetPointSaturationRange()[1] &&
      lut->GetValueRange()[0] == this->GetPointValueRange()[0] &&
      lut->GetValueRange()[1] == this->GetPointValueRange()[1] &&
      lut->GetAlphaRange()[0] == this->GetPointAlphaRange()[0] &&
      lut->GetAlphaRange()[1] == this->GetPointAlphaRange()[1])
  {
    return true;
  }
  return false;
}

bool vtkViewTheme::LookupMatchesCellTheme(vtkScalarsToColors* s2c)
{
  if (!s2c)
  {
    return false;
  }
  vtkLookupTable* lut = vtkLookupTable::SafeDownCast(s2c);
  if (!lut)
  {
    return false;
  }
  if (lut->GetHueRange()[0] == this->GetCellHueRange()[0] &&
      lut->GetHueRange()[1] == this->GetCellHueRange()[1] &&
      lut->GetSaturationRange()[0] == this->GetCellSaturationRange()[0] &&
      lut->GetSaturationRange()[1] == this->GetCellSaturationRange()[1] &&
      lut->GetValueRange()[0] == this->GetCellValueRange()[0] &&
      lut->GetValueRange()[1] == this->GetCellValueRange()[1] &&
      lut->GetAlphaRange()[0] == this->GetCellAlphaRange()[0] &&
      lut->GetAlphaRange()[1] == this->GetCellAlphaRange()[1])
  {
    return true;
  }
  return false;
}

void vtkViewTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PointSize: "
     << this->PointSize << endl;
  os << indent << "LineWidth: "
     << this->LineWidth << endl;
  os << indent << "PointColor: "
     << this->PointColor[0] << ","
     << this->PointColor[1] << ","
     << this->PointColor[2] << endl;
  os << indent << "PointOpacity: " << this->PointOpacity << endl;
  os << indent << "CellColor: "
     << this->CellColor[0] << ","
     << this->CellColor[1] << ","
     << this->CellColor[2] << endl;
  os << indent << "CellOpacity: " << this->CellOpacity << endl;
  os << indent << "OutlineColor: "
     << this->OutlineColor[0] << ","
     << this->OutlineColor[1] << ","
     << this->OutlineColor[2] << endl;
  os << indent << "SelectedPointColor: "
     << this->SelectedPointColor[0] << ","
     << this->SelectedPointColor[1] << ","
     << this->SelectedPointColor[2] << endl;
  os << indent << "SelectedPointOpacity: " << this->SelectedPointOpacity << endl;
  os << indent << "SelectedCellColor: "
     << this->SelectedCellColor[0] << ","
     << this->SelectedCellColor[1] << ","
     << this->SelectedCellColor[2] << endl;
  os << indent << "SelectedCellOpacity: " << this->SelectedCellOpacity << endl;
  os << indent << "BackgroundColor: "
     << this->BackgroundColor[0] << ","
     << this->BackgroundColor[1] << ","
     << this->BackgroundColor[2] << endl;
  os << indent << "BackgroundColor2: "
     << this->BackgroundColor2[0] << ","
     << this->BackgroundColor2[1] << ","
     << this->BackgroundColor2[2] << endl;
  os << indent << "PointLookupTable: " << (this->PointLookupTable ? "" : "(none)") << endl;
  if (this->PointLookupTable)
  {
    this->PointLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "CellLookupTable: " << (this->CellLookupTable ? "" : "(none)") << endl;
  if (this->CellLookupTable)
  {
    this->CellLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "PointTextProperty: " << (this->PointTextProperty ? "" : "(none)") << endl;
  if (this->PointTextProperty)
  {
    this->PointTextProperty->PrintSelf(os, indent.GetNextIndent());
    os << indent << "VertexLabelColor: "
       << this->PointTextProperty->GetColor()[0] << ","
       << this->PointTextProperty->GetColor()[1] << ","
       << this->PointTextProperty->GetColor()[2] << endl;
  }
  os << indent << "CellTextProperty: " << (this->CellTextProperty ? "" : "(none)") << endl;
  if (this->CellTextProperty)
  {
    this->CellTextProperty->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelColor: "
       << this->CellTextProperty->GetColor()[0] << ","
       << this->CellTextProperty->GetColor()[1] << ","
       << this->CellTextProperty->GetColor()[2] << endl;
  }
  os << indent << "ScalePointLookupTable: " << this->ScalePointLookupTable << endl;
  os << indent << "ScaleCellLookupTable: " << this->ScaleCellLookupTable << endl;
}

