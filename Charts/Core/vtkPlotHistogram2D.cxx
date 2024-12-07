// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotHistogram2D.h"

#include "vtkArrayDispatch.h"
#include "vtkAxis.h"
#include "vtkContext2D.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkScalarsToColors.h"
#include "vtkStringArray.h"

#include "vtkObjectFactory.h"

#include <algorithm>

namespace
{
using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

/**
 * Worker to compute magnitude of vector array.
 */
struct MagnitudeWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* vecs, vtkDoubleArray* mags)
  {
    VTK_ASSUME(vecs->GetNumberOfComponents() == 2 || vecs->GetNumberOfComponents() == 3);

    const auto vecRange = vtk::DataArrayTupleRange(vecs);
    auto magRange = vtk::DataArrayValueRange<1>(mags);

    using VecTuple = typename decltype(vecRange)::ConstTupleReferenceType;

    vtkSMPTools::Transform(vecRange.cbegin(), vecRange.cend(), magRange.begin(),
      [](const VecTuple& tuple)
      {
        double mag = 0.0;
        for (const auto comp : tuple)
        {
          const double castedValue = static_cast<double>(comp); // Needed to avoid value overflow
          mag += castedValue * castedValue;
        }
        return std::sqrt(mag);
      });
  }
};

/**
 * Worker to return void pointer with the right offset.
 */
struct OffsetWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* input, void*& output, int offset)
  {
    const auto inputRange = vtk::DataArrayTupleRange(input);
    using ValueType = typename decltype(inputRange)::ComponentType;

    ValueType* offsetOutput = static_cast<ValueType*>(output);
    offsetOutput += offset;

    output = static_cast<void*>(offsetOutput);
  }
};

}

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlotHistogram2D);

//------------------------------------------------------------------------------
vtkPlotHistogram2D::vtkPlotHistogram2D()
{
  this->TooltipDefaultLabelFormat = "%x,  %y:  %v";
}

//------------------------------------------------------------------------------
vtkPlotHistogram2D::~vtkPlotHistogram2D() = default;

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::Update()
{
  if (!this->Visible)
  {
    return;
  }
  // Check if we have an input image
  if (!this->Input)
  {
    vtkDebugMacro(<< "Update event called with no input image.");
    return;
  }

  bool dataUpdated = false;
  if (this->Input->GetMTime() > this->BuildTime)
  {
    dataUpdated = true;
  }

  if (dataUpdated || this->CacheRequiresUpdate())
  {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateCache();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkPlotHistogram2D::Paint(vtkContext2D* painter)
{
  if (this->Output)
  {
    if (this->Input)
    {
      double bounds[4];
      this->GetBounds(bounds);
      this->Position = vtkRectf(bounds[0], bounds[2], bounds[1] - bounds[0], bounds[3] - bounds[2]);
    }
    painter->DrawImage(this->Position, this->Output);
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::SetInputData(vtkImageData* data, vtkIdType)
{
  // FIXME: Store the z too, for slices.
  this->Input = data;
}

//------------------------------------------------------------------------------
vtkImageData* vtkPlotHistogram2D::GetInputImageData()
{
  return this->Input;
}

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::SetTransferFunction(vtkScalarsToColors* function)
{
  this->TransferFunction = function;
}

//------------------------------------------------------------------------------
vtkScalarsToColors* vtkPlotHistogram2D::GetTransferFunction()
{
  return this->TransferFunction;
}

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::GetBounds(double bounds[4])
{
  if (this->Input)
  {
    std::copy(this->Input->GetBounds(), this->Input->GetBounds() + 4, bounds);

    // Adding a spacing increment is necessary in order to draw in the context 2D correctly
    double* spacing = this->Input->GetSpacing();
    bounds[1] += spacing[0];
    bounds[3] += spacing[1];
  }
  else
  {
    std::fill(bounds, bounds + 4, 0.);
  }
}

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::SetPosition(const vtkRectf& pos)
{
  this->Position = pos;
}

//------------------------------------------------------------------------------
vtkRectf vtkPlotHistogram2D::GetPosition()
{
  return this->Position;
}

//------------------------------------------------------------------------------
vtkIdType vtkPlotHistogram2D::GetNearestPoint(const vtkVector2f& point,
  const vtkVector2f& vtkNotUsed(tolerance), vtkVector2f* location, vtkIdType* vtkNotUsed(segmentId))
{
  if (!this->Input)
  {
    return -1;
  }

  double bounds[4];
  this->GetBounds(bounds);

  if (point.GetX() < bounds[0] || point.GetX() > bounds[1] || point.GetY() < bounds[2] ||
    point.GetY() > bounds[3])
  {
    return -1;
  }

  double spacing[3];
  this->Input->GetSpacing(spacing);

  // Can't use vtkImageData::FindPoint() / GetPoint(), as ImageData points are
  // rendered as the bottom left corner of a histogram cell, not the center
  int locX = vtkMath::Floor((point.GetX() - bounds[0]) / spacing[0]);
  int locY = vtkMath::Floor((point.GetY() - bounds[2]) / spacing[1]);
  int width = this->Input->GetExtent()[1] - this->Input->GetExtent()[0] + 1;

  // Discretize to ImageData point values
  location->SetX(locX * spacing[0] + bounds[0]);
  location->SetY(locY * spacing[1] + bounds[2]);

  return (locX + (locY * width));
}

//------------------------------------------------------------------------------
vtkStdString vtkPlotHistogram2D::GetTooltipLabel(
  const vtkVector2d& plotPos, vtkIdType seriesIndex, vtkIdType)
{
  // This does not call the Superclass vtkPlot::GetTooltipLabel(), since the
  // format tags internally refer to different values
  std::string tooltipLabel;
  std::string& format =
    this->TooltipLabelFormat.empty() ? this->TooltipDefaultLabelFormat : this->TooltipLabelFormat;

  if (!this->Input)
  {
    return tooltipLabel;
  }

  int extent[6];
  this->Input->GetExtent(extent);

  int width = extent[1] - extent[0] + 1;
  int pointX = seriesIndex % width + extent[0];
  int pointY = seriesIndex / width + extent[2];

  // Parse TooltipLabelFormat and build tooltipLabel
  bool escapeNext = false;
  for (size_t i = 0; i < format.length(); ++i)
  {
    if (escapeNext)
    {
      switch (format[i])
      {
        case 'x':
          tooltipLabel += this->GetNumber(plotPos.GetX(), this->XAxis);
          break;
        case 'y':
          tooltipLabel += this->GetNumber(plotPos.GetY(), this->YAxis);
          break;
        case 'i':
          if (this->XAxis->GetTickLabels() && this->XAxis->GetTickLabels()->GetNumberOfTuples() > 0)
          {
            vtkIdType labelIndex =
              vtkPlotHistogram2D::GetLabelIndexFromValue(plotPos.GetX(), this->XAxis);
            if (labelIndex >= 0)
            {
              tooltipLabel += this->XAxis->GetTickLabels()->GetValue(labelIndex);
            }
          }
          break;
        case 'j':
          if (this->YAxis->GetTickLabels() && this->YAxis->GetTickLabels()->GetNumberOfTuples())
          {
            vtkIdType labelIndex =
              vtkPlotHistogram2D::GetLabelIndexFromValue(plotPos.GetY(), this->YAxis);
            if (labelIndex >= 0)
            {
              tooltipLabel += this->YAxis->GetTickLabels()->GetValue(labelIndex);
            }
          }
          break;
        case 'v':
          tooltipLabel += this->GetNumber(this->GetInputArrayValue(pointX, pointY, 0), nullptr);
          break;
        default: // If no match, insert the entire format tag
          tooltipLabel += "%";
          tooltipLabel += format[i];
          break;
      }
      escapeNext = false;
    }
    else
    {
      if (format[i] == '%')
      {
        escapeNext = true;
      }
      else
      {
        tooltipLabel += format[i];
      }
    }
  }
  return tooltipLabel;
}

//------------------------------------------------------------------------------
bool vtkPlotHistogram2D::UpdateCache()
{
  if (!this->Input)
  {
    return false;
  }
  if (!this->Output)
  {
    this->Output = vtkSmartPointer<vtkImageData>::New();
  }
  this->Output->SetExtent(this->Input->GetExtent());
  this->Output->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  if (this->TransferFunction)
  {
    vtkDataArray* inputDataArray = nullptr;
    void* const inputVoidArray = this->GetInputArrayPointer(inputDataArray);

    if (!inputVoidArray || !inputDataArray)
    {
      return false;
    }

    const int inputType = inputDataArray->GetDataType();
    const int nbComponents = inputDataArray->GetNumberOfComponents();
    const int dimension = this->Input->GetDimensions()[0] * this->Input->GetDimensions()[1];
    unsigned char* output = reinterpret_cast<unsigned char*>(this->Output->GetScalarPointer());

    this->TransferFunction->MapScalarsThroughTable2(
      inputVoidArray, output, inputType, dimension, nbComponents, 4);
  }

  return true;
}

//------------------------------------------------------------------------------
void* vtkPlotHistogram2D::GetInputArrayPointer(vtkDataArray*& inputArray)
{
  vtkDataArray* selectedArray = this->GetSelectedArray();
  if (!selectedArray)
  {
    inputArray = nullptr;
    return nullptr;
  }

  int vectorMode = this->TransferFunction->GetVectorMode();
  if (vtkPlotHistogram2D::CanComputeMagnitude(selectedArray) &&
    vectorMode == vtkScalarsToColors::VectorModes::MAGNITUDE)
  {
    MagnitudeWorker worker;
    this->MagnitudeArray->SetNumberOfTuples(selectedArray->GetNumberOfTuples());

    if (!Dispatcher::Execute(selectedArray, worker, this->MagnitudeArray))
    {
      // Otherwise fallback to using the vtkDataArray API.
      worker(selectedArray, this->MagnitudeArray);
    }

    inputArray = this->MagnitudeArray;
    return this->MagnitudeArray->GetVoidPointer(0);
  }

  OffsetWorker worker;
  void* array = selectedArray->GetVoidPointer(0);
  int vectorComponent = this->TransferFunction->GetVectorComponent();

  if (!Dispatcher::Execute(selectedArray, worker, array, vectorComponent))
  {
    // Otherwise fallback to using the vtkDataArray API.
    worker(selectedArray, array, vectorComponent);
  }

  inputArray = selectedArray;
  return array;
}

//------------------------------------------------------------------------------
double vtkPlotHistogram2D::GetInputArrayValue(int x, int y, int z)
{
  vtkDataArray* selectedArray = this->GetSelectedArray();
  if (!selectedArray)
  {
    vtkErrorMacro("Trying to get value while no array was selected.");
    return std::nan("");
  }

  int coordinates[3] = { x, y, z };
  vtkIdType index = this->Input->GetTupleIndex(selectedArray, coordinates);

  if (index < 0)
  {
    // An error message was already generated by GetTupleIndex.
    return std::nan("");
  }

  if (!this->TransferFunction)
  {
    vtkErrorMacro("Trying to get value while no transfer function was set.");
    return std::nan("");
  }

  int vectorMode = this->TransferFunction->GetVectorMode();
  if (vtkPlotHistogram2D::CanComputeMagnitude(selectedArray) &&
    vectorMode == vtkScalarsToColors::VectorModes::MAGNITUDE)
  {
    return this->MagnitudeArray->GetTuple1(index);
  }

  int vectorComponent = this->TransferFunction->GetVectorComponent();
  return selectedArray->GetComponent(index, vectorComponent);
}

//------------------------------------------------------------------------------
bool vtkPlotHistogram2D::CanComputeMagnitude(vtkDataArray* array)
{
  const int nbComponents = array ? array->GetNumberOfComponents() : 0;
  return nbComponents == 2 || nbComponents == 3;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkPlotHistogram2D::GetSelectedArray()
{
  return this->ArrayName.empty() ? this->Input->GetPointData()->GetScalars()
                                 : this->Input->GetPointData()->GetArray(this->ArrayName.c_str());
}

//------------------------------------------------------------------------------
vtkIdType vtkPlotHistogram2D::GetLabelIndexFromValue(double value, vtkAxis* axis)
{
  auto tickRange = vtk::DataArrayValueRange<1>(axis->GetTickPositions());
  const auto iterator = std::find_if(
    tickRange.cbegin(), tickRange.cend(), [&value](const double& elem) { return value < elem; });

  vtkIdType labelIndex = iterator - tickRange.cbegin();
  return labelIndex - 1;
}

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
