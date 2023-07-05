// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSliderRepresentation.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkSliderRepresentation::vtkSliderRepresentation()
{
  this->MinimumValue = 0.0;
  this->Value = 0.0;
  this->MaximumValue = 1.0;
  this->CurrentT = 0;
  this->PickedT = 0.0;

  this->SliderLength = 0.05;
  this->SliderWidth = 0.05;
  this->EndCapLength = 0.025;
  this->EndCapWidth = 0.05;
  this->TubeWidth = 0.025;

  // Labels and text
  this->ShowSliderLabel = 1;

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat, 8, "%s", "%0.3g");

  this->LabelHeight = 0.05;
  this->TitleHeight = 0.15;
}

//------------------------------------------------------------------------------
vtkSliderRepresentation::~vtkSliderRepresentation()
{
  delete[] this->LabelFormat;
  this->LabelFormat = nullptr;
}

//------------------------------------------------------------------------------
void vtkSliderRepresentation::SetMinimumValue(double minValue)
{
  if (minValue == this->MinimumValue)
  {
    return;
  }

  if (minValue >= this->MaximumValue)
  {
    this->MaximumValue = minValue + 1;
  }

  this->MinimumValue = minValue;

  if (this->Value < this->MinimumValue)
  {
    this->Value = this->MinimumValue;
  }
  else if (this->Value > this->MaximumValue)
  {
    this->Value = this->MaximumValue;
  }
  this->CurrentT = (this->Value - this->MinimumValue) / (this->MaximumValue - this->MinimumValue);

  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkSliderRepresentation::SetMaximumValue(double maxValue)
{
  if (maxValue == this->MaximumValue)
  {
    return;
  }

  if (maxValue <= this->MinimumValue)
  {
    this->MinimumValue = maxValue - 1;
  }

  this->MaximumValue = maxValue;

  if (this->Value < this->MinimumValue)
  {
    this->Value = this->MinimumValue;
  }
  else if (this->Value > this->MaximumValue)
  {
    this->Value = this->MaximumValue;
  }
  this->CurrentT = (this->Value - this->MinimumValue) / (this->MaximumValue - this->MinimumValue);

  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkSliderRepresentation::SetValue(double value)
{
  if (value == this->Value)
  {
    return;
  }

  if (value < this->MinimumValue)
  {
    value = this->MinimumValue;
  }

  if (value > this->MaximumValue)
  {
    value = this->MaximumValue;
  }

  this->Value = value;
  this->CurrentT = (value - this->MinimumValue) / (this->MaximumValue - this->MinimumValue);

  this->Modified();
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkSliderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Minimum Value: " << this->MinimumValue << "\n";
  os << indent << "Maximum Value: " << this->MaximumValue << "\n";
  os << indent << "Value: " << this->Value << "\n";

  os << indent << "Slider Length: " << this->SliderLength << "\n";
  os << indent << "Slider Width: " << this->SliderWidth << "\n";
  os << indent << "End Cap Length: " << this->EndCapLength << "\n";
  os << indent << "End Cap Width: " << this->EndCapWidth << "\n";
  os << indent << "Tube Width: " << this->TubeWidth << "\n";

  os << indent << "Show Slider Label: " << (this->ShowSliderLabel ? "On\n" : "Off\n");
  os << indent << "Label Format: " << this->LabelFormat << "\n";
  os << indent << "Label Height: " << this->LabelHeight << "\n";
  os << indent << "Title Height: " << this->TitleHeight << "\n";
}
VTK_ABI_NAMESPACE_END
