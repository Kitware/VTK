// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCategoryLegend.h"
#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkScalarsToColors.h"
#include "vtkTextProperty.h"
#include "vtkVariantArray.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCategoryLegend);
vtkCxxSetObjectMacro(vtkCategoryLegend, Values, vtkVariantArray);
vtkCxxSetObjectMacro(vtkCategoryLegend, ScalarsToColors, vtkScalarsToColors);

//------------------------------------------------------------------------------
vtkCategoryLegend::vtkCategoryLegend()
{
  this->SetInline(false);
  this->SetHorizontalAlignment(vtkChartLegend::RIGHT);
  this->SetVerticalAlignment(vtkChartLegend::BOTTOM);

  this->ScalarsToColors = nullptr;
  this->Values = nullptr;

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

//------------------------------------------------------------------------------
vtkCategoryLegend::~vtkCategoryLegend()
{
  this->SetValues(nullptr);
  this->SetScalarsToColors(nullptr);
}

//------------------------------------------------------------------------------
bool vtkCategoryLegend::Paint(vtkContext2D* painter)
{
  if (!this->Visible || this->ScalarsToColors == nullptr || this->Values == nullptr)
  {
    return true;
  }

  // Draw a box around the legend.
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  this->GetBoundingRect(painter);
  painter->DrawRect(
    this->Rect.GetX(), this->Rect.GetY(), this->Rect.GetWidth(), this->Rect.GetHeight());

  // Draw the title (if any)
  vtkVector2f stringBounds[2];
  float titleHeight = 0.0;
  if (!this->Title.empty())
  {
    painter->ApplyTextProp(this->TitleProperties);
    painter->ComputeStringBounds(this->Title, stringBounds->GetData());
    titleHeight = stringBounds[1].GetY() + this->Padding;

    float x = this->Rect.GetX() + this->Rect.GetWidth() / 2.0;
    float y = this->Rect.GetY() + this->Rect.GetHeight() - this->Padding;
    painter->DrawString(x, y, this->Title);
  }

  painter->ApplyTextProp(this->LabelProperties);

  // compute the height of a sample string.
  // The height of this string will also be used as the size of
  // the color marks.
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float stringHeight = stringBounds[1].GetY();

  // the starting X positions of our marks & labels
  float markX = this->Rect.GetX() + this->TitleWidthOffset + this->Padding;
  float labelX = markX + stringHeight + this->Padding;

  // the Y value of the row that we're currently drawing
  float y =
    this->Rect.GetY() + this->Rect.GetHeight() - this->Padding - floor(stringHeight) - titleHeight;

  // draw all of the marks & labels
  for (vtkIdType l = 0; l < this->Values->GetNumberOfTuples(); ++l)
  {
    std::string currentString = this->Values->GetValue(l).ToString();
    if (currentString.empty())
    {
      continue;
    }

    if (this->ScalarsToColors->GetAnnotatedValueIndex(this->Values->GetValue(l)) == -1)
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
    this->ScalarsToColors->GetAnnotationColor(this->ScalarsToColors->GetAnnotatedValue(-1), color);
    painter->GetBrush()->SetColorF(color[0], color[1], color[2]);
    painter->DrawRect(markX, y, stringHeight, stringHeight);

    // draw the outlier label
    painter->DrawString(labelX, y, this->OutlierLabel);
  }

  return true;
}

//------------------------------------------------------------------------------
vtkRectf vtkCategoryLegend::GetBoundingRect(vtkContext2D* painter)
{
  if (this->CacheBounds && this->RectTime > this->GetMTime() && this->RectTime > this->PlotTime &&
    this->RectTime > this->ScalarsToColors->GetMTime() && this->RectTime > this->Values->GetMTime())
  {
    return this->Rect;
  }

  painter->ApplyTextProp(this->LabelProperties);

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
  if (!this->Title.empty())
  {
    painter->ApplyTextProp(this->TitleProperties);

    painter->ComputeStringBounds(this->Title, stringBounds->GetData());
    titleWidth = stringBounds[1].GetX();
    titleHeight = stringBounds[1].GetY() + this->Padding;

    painter->ApplyTextProp(this->LabelProperties);
  }

  // Calculate the widest legend label
  float maxWidth = 0.0;
  int numSkippedValues = 0;
  this->TitleWidthOffset = 0.0;
  this->HasOutliers = false;

  for (vtkIdType l = 0; l < this->Values->GetNumberOfTuples(); ++l)
  {
    if (this->Values->GetValue(l).ToString().empty())
    {
      ++numSkippedValues;
      continue;
    }
    if (this->ScalarsToColors->GetAnnotatedValueIndex(this->Values->GetValue(l)) == -1)
    {
      ++numSkippedValues;
      this->HasOutliers = true;
      continue;
    }
    painter->ComputeStringBounds(this->Values->GetValue(l).ToString(), stringBounds->GetData());
    if (stringBounds[1].GetX() > maxWidth)
    {
      maxWidth = stringBounds[1].GetX();
    }
  }

  // Calculate size of outlier label (if necessary)
  if (this->HasOutliers)
  {
    painter->ComputeStringBounds(this->OutlierLabel, stringBounds->GetData());
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

  float h = ceil((numLabels * (height + this->Padding)) + this->Padding + titleHeight);

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

//------------------------------------------------------------------------------
void vtkCategoryLegend::SetTitle(const vtkStdString& title)
{
  this->Title = title;
}

//------------------------------------------------------------------------------
vtkStdString vtkCategoryLegend::GetTitle()
{
  return this->Title;
}

//------------------------------------------------------------------------------
void vtkCategoryLegend::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HasOutliers: " << this->HasOutliers << endl;
  os << indent << "TitleWidthOffset: " << this->TitleWidthOffset << endl;
  os << indent << "ScalarsToColors: \n";
  if (this->ScalarsToColors)
  {
    this->ScalarsToColors->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent.GetNextIndent() << "(null)" << endl;
  }
  os << indent << "OutlierLabel: " << this->OutlierLabel << endl;
  os << indent << "Title: " << this->Title << endl;
  os << indent << "TitleProperties: \n";
  this->TitleProperties->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
