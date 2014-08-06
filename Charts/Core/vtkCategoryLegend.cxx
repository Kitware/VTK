/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCategoryLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkCategoryLegend.h"
#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkScalarsToColors.h"
#include "vtkTextProperty.h"
#include "vtkVariantArray.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCategoryLegend);

//-----------------------------------------------------------------------------
vtkCategoryLegend::vtkCategoryLegend()
{
  this->SetInline(false);
  this->SetHorizontalAlignment(vtkChartLegend::RIGHT);
  this->SetVerticalAlignment(vtkChartLegend::BOTTOM);

  this->ScalarsToColors = NULL;
  this->Values = NULL;

  this->TitleProperties->SetColor(this->LabelProperties->GetColor());
  this->TitleProperties->SetFontSize(this->LabelProperties->GetFontSize());
  this->TitleProperties->SetFontFamily(this->LabelProperties->GetFontFamily());
  this->TitleProperties->SetJustificationToCentered();
  this->TitleProperties->SetVerticalJustificationToTop();
  this->TitleProperties->SetBold(1);

  this->TitleWidthOffset = 0.0;
  this->HasOutliers = false;
  this->OutlierLabel = "outliers";
}

//-----------------------------------------------------------------------------
vtkCategoryLegend::~vtkCategoryLegend()
{
}

//-----------------------------------------------------------------------------
bool vtkCategoryLegend::Paint(vtkContext2D* painter)
{
  if (!this->Visible || this->ScalarsToColors == NULL || this->Values == NULL)
    {
    return true;
    }

  // Draw a box around the legend.
  painter->ApplyPen(this->Pen.GetPointer());
  painter->ApplyBrush(this->Brush.GetPointer());
  this->GetBoundingRect(painter);
  painter->DrawRect(this->Rect.GetX(), this->Rect.GetY(),
                    this->Rect.GetWidth(), this->Rect.GetHeight());

  // Draw the title (if any)
  vtkVector2f stringBounds[2];
  float titleHeight = 0.0;
  if (this->Title != "")
    {
    painter->ApplyTextProp(this->TitleProperties.GetPointer());
    painter->ComputeStringBounds(this->Title, stringBounds->GetData());
    titleHeight = stringBounds[1].GetY() + this->Padding;

    float x = this->Rect.GetX() + this->Rect.GetWidth() / 2.0;
    float y = this->Rect.GetY() + this->Rect.GetHeight() - this->Padding;
    painter->DrawString(x, y, this->Title);
    }

  painter->ApplyTextProp(this->LabelProperties.GetPointer());

  // compute the height of a sample string.
  // The height of this string will also be used as the size of
  // the color marks.
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float stringHeight = stringBounds[1].GetY();

  // the starting X positions of our marks & labels
  float markX = this->Rect.GetX() + this->TitleWidthOffset + this->Padding;
  float labelX = markX + stringHeight + this->Padding;

  // the Y value of the row that we're currently drawing
  float y = this->Rect.GetY() + this->Rect.GetHeight() -
                  this->Padding - floor(stringHeight) - titleHeight;

  // draw all of the marks & labels
  for (vtkIdType l = 0; l < this->Values->GetNumberOfTuples(); ++l)
    {
    vtkStdString currentString = this->Values->GetValue(l).ToString();
    if (currentString == "")
      {
      continue;
      }

    if (this->ScalarsToColors->GetAnnotatedValueIndex(
      this->Values->GetValue(l)) == -1)
      {
      continue;
      }

    // paint the color mark for this category
    double color[4];
    this->ScalarsToColors->GetAnnotationColor(this->Values->GetValue(l), color);
    painter->GetBrush()->SetColorF(color[0], color[1], color[2]);
    painter->DrawRect(markX, y, stringHeight, stringHeight);

    // draw this category's label
    painter->DrawString(labelX, y, this->Values->GetValue(l).ToString());

    // Move y position down another row
    y -= stringHeight + this->Padding;
    }

  if (this->HasOutliers)
    {
    // paint the outlier color mark
    double color[4];
    this->ScalarsToColors->GetAnnotationColor(
      this->ScalarsToColors->GetAnnotatedValue(-1), color);
    painter->GetBrush()->SetColorF(color[0], color[1], color[2]);
    painter->DrawRect(markX, y, stringHeight, stringHeight);

    // draw the outlier label
    painter->DrawString(labelX, y, this->OutlierLabel);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkCategoryLegend::SetScalarsToColors(vtkScalarsToColors* stc)
{
  this->ScalarsToColors = stc;
}

//-----------------------------------------------------------------------------
vtkScalarsToColors * vtkCategoryLegend::GetScalarsToColors()
{
  return this->ScalarsToColors;
}

//-----------------------------------------------------------------------------
vtkRectf vtkCategoryLegend::GetBoundingRect(vtkContext2D *painter)
{
  if (this->CacheBounds && this->RectTime > this->GetMTime() &&
      this->RectTime > this->PlotTime &&
      this->RectTime > this->ScalarsToColors->GetMTime() &&
      this->RectTime > this->Values->GetMTime())
    {
    return this->Rect;
    }

  painter->ApplyTextProp(this->LabelProperties.GetPointer());

  vtkVector2f stringBounds[2];
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float height = stringBounds[1].GetY();

  // programmatically set Padding here.  This results in better
  // appearance when we zoom in or out on the legend.
  this->Padding = static_cast<int>(height / 4.0);
  if (this->Padding < 1)
    {
    this->Padding = 1;
    }

  // Calculate size of title (if any)
  float titleHeight = 0.0f;
  float titleWidth = 0.0f;
  if (this->Title != "")
    {
    painter->ApplyTextProp(this->TitleProperties.GetPointer());

    painter->ComputeStringBounds(this->Title, stringBounds->GetData());
    titleWidth = stringBounds[1].GetX();
    titleHeight = stringBounds[1].GetY() + this->Padding;

    painter->ApplyTextProp(this->LabelProperties.GetPointer());
    }

  // Calculate the widest legend label
  float maxWidth = 0.0;
  int numSkippedValues = 0;
  this->TitleWidthOffset = 0.0;
  this->HasOutliers = false;

  for (vtkIdType l = 0; l < this->Values->GetNumberOfTuples(); ++l)
    {
    if (this->Values->GetValue(l).ToString() == "")
      {
      ++numSkippedValues;
      continue;
      }
    if (this->ScalarsToColors->GetAnnotatedValueIndex(
      this->Values->GetValue(l)) == -1)
      {
      ++numSkippedValues;
      this->HasOutliers = true;
      continue;
      }
    painter->ComputeStringBounds(this->Values->GetValue(l).ToString(),
                                 stringBounds->GetData());
    if (stringBounds[1].GetX() > maxWidth)
      {
      maxWidth = stringBounds[1].GetX();
      }
    }

  // Calculate size of outlier label (if necessary)
  if (this->HasOutliers)
    {
    painter->ComputeStringBounds(this->OutlierLabel,
                                 stringBounds->GetData());
    if (stringBounds[1].GetX() > maxWidth)
      {
      maxWidth = stringBounds[1].GetX();
      }
    }

  if (titleWidth > maxWidth)
    {
    this->TitleWidthOffset = (titleWidth - maxWidth) / 2.0;
    maxWidth = titleWidth;
    }

  int numLabels = this->Values->GetNumberOfTuples() - numSkippedValues;
  if (this->HasOutliers)
    {
    ++numLabels;
    }

  // 3 paddings: one on the left, one on the right, and one between the
  // color mark and its label.
  float w = ceil(maxWidth + 3 * this->Padding + height);

  float h = ceil((numLabels * (height + this->Padding)) + this->Padding
            + titleHeight);

  float x = floor(this->Point[0]);
  float y = floor(this->Point[1]);

  // Compute bottom left point based on current alignment.
  if (this->HorizontalAlignment == vtkChartLegend::CENTER)
    {
    x -= w / 2.0;
    }
  else if (this->HorizontalAlignment == vtkChartLegend::RIGHT)
    {
    x -= w;
    }
  if (this->VerticalAlignment == vtkChartLegend::CENTER)
    {
    y -= h / 2.0;
    }
  else if (this->VerticalAlignment == vtkChartLegend::TOP)
    {
    y -= h;
    }

  this->Rect = vtkRectf(x, y, w, h);
  this->RectTime.Modified();
  return this->Rect;
}

//-----------------------------------------------------------------------------
void vtkCategoryLegend::SetTitle(const vtkStdString &title)
{
  this->Title = title;
}

//-----------------------------------------------------------------------------
vtkStdString vtkCategoryLegend::GetTitle()
{
  return this->Title;
}
