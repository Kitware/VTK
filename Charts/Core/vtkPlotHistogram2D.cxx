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
using Dispatcher2 = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>;

/**
 * Worker to compute magnitude of vector array.
 */
struct MagnitudeWorker
{
  template <typename ArrayT1, typename ArrayT2>
  void operator()(ArrayT1* vecs, ArrayT2* mags)
  {
    const auto vecRange = vtk::DataArrayTupleRange(vecs);
    auto magRange = vtk::DataArrayValueRange<1>(mags);

    using VecTuple = typename decltype(vecRange)::const_reference;
    using MagType = typename decltype(magRange)::ValueType;

    auto computeMag = [](const VecTuple& tuple) -> MagType {
      MagType mag = 0;
      for (const auto& comp : tuple)
      {
        mag += (comp * comp);
      }
      return std::sqrt(mag);
    };

    vtkSMPTools::Transform(vecRange.cbegin(), vecRange.cend(), magRange.begin(), computeMag);
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
  const vtkVector2f& tolerance, vtkVector2f* location, vtkIdType* vtkNotUsed(segmentId))
{
  if (!this->Input)
  {
    return -1;
  }

  (void)tolerance;
  double bounds[4];
  this->GetBounds(bounds);
  double spacing[3];

  this->Input->GetSpacing(spacing);

  if (point.GetX() < bounds[0] || point.GetX() > bounds[1] + spacing[0] ||
    point.GetY() < bounds[2] || point.GetY() > bounds[3] + spacing[1])
  {
    return -1;
  }

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

  double bounds[4];
  this->GetBounds(bounds);
  int width = this->Input->GetExtent()[1] - this->Input->GetExtent()[0] + 1;
  int height = this->Input->GetExtent()[3] - this->Input->GetExtent()[2] + 1;
  int pointX = seriesIndex % width + this->Input->GetExtent()[0];
  int pointY = seriesIndex / width + this->Input->GetExtent()[2];

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
          if (this->XAxis->GetTickLabels() && pointX >= 0 &&
            pointX < this->XAxis->GetTickLabels()->GetNumberOfTuples())
          {
            tooltipLabel += this->XAxis->GetTickLabels()->GetValue(pointX);
          }
          break;
        case 'j':
          if (this->YAxis->GetTickLabels() && pointY >= 0 &&
            pointY < this->YAxis->GetTickLabels()->GetNumberOfTuples())
          {
            tooltipLabel += this->YAxis->GetTickLabels()->GetValue(pointY);
          }
          break;
        case 'v':
          if (pointX >= 0 && pointX < width && pointY >= 0 && pointY < height)
          {
            tooltipLabel += this->GetNumber(this->GetInputScalarValue(pointX, pointY, 0), nullptr);
          }
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
    int nbComponents = 0;
    void* const input = this->GetInputScalarPointer(nbComponents);

    if (!input)
    {
      return false;
    }

    const int inputType = this->Input->GetScalarType();
    int dimension = this->Input->GetDimensions()[0] * this->Input->GetDimensions()[1];
    unsigned char* output = reinterpret_cast<unsigned char*>(this->Output->GetScalarPointer());

    this->TransferFunction->MapScalarsThroughTable2(
      input, output, inputType, dimension, nbComponents, 4);
  }

  return true;
}

//------------------------------------------------------------------------------
void* vtkPlotHistogram2D::GetInputScalarPointer(int& nbComponents)
{
  int vectorMode = this->TransferFunction->GetVectorMode();
  nbComponents = this->Input->GetNumberOfScalarComponents();

  if (vtkPlotHistogram2D::CanComputeMagnitude(nbComponents) &&
    vectorMode == vtkScalarsToColors::VectorModes::MAGNITUDE)
  {
    nbComponents = 1;
    MagnitudeWorker worker;
    vtkDataArray* vecs = this->Input->GetPointData()->GetScalars();
    this->MagnitudeArray.TakeReference(vecs->NewInstance());
    this->MagnitudeArray->SetNumberOfComponents(nbComponents);
    this->MagnitudeArray->SetNumberOfTuples(vecs->GetNumberOfTuples());

    if (!Dispatcher2::Execute(vecs, this->MagnitudeArray.Get(), worker))
    {
      // Otherwise fallback to using the vtkDataArray API.
      worker(vecs, this->MagnitudeArray.Get());
    }

    return this->MagnitudeArray->GetVoidPointer(0);
  }

  OffsetWorker worker;
  vtkDataArray* scalars = this->Input->GetPointData()->GetScalars();
  void* offset = scalars->GetVoidPointer(0);
  int component = this->TransferFunction->GetVectorComponent();

  if (!Dispatcher::Execute(scalars, worker, offset, component))
  {
    // Otherwise fallback to using the vtkDataArray API.
    worker(scalars, offset, component);
  }

  return offset;
}

//------------------------------------------------------------------------------
double vtkPlotHistogram2D::GetInputScalarValue(int x, int y, int z)
{
  int vectorMode = this->TransferFunction->GetVectorMode();
  int nbComponents = this->Input->GetNumberOfScalarComponents();

  if (vtkPlotHistogram2D::CanComputeMagnitude(nbComponents) &&
    vectorMode == vtkScalarsToColors::VectorModes::MAGNITUDE)
  {
    int coordinates[3] = { x, y, z };
    vtkIdType index = this->Input->GetTupleIndex(this->MagnitudeArray, coordinates);

    // An error message was already generated by GetTupleIndex.
    return index < 0 ? std::nan("") : this->MagnitudeArray->GetTuple1(index);
  }

  int vectorComponent = this->TransferFunction->GetVectorComponent();
  return this->Input->GetScalarComponentAsDouble(x, y, z, vectorComponent);
}

//------------------------------------------------------------------------------
bool vtkPlotHistogram2D::CanComputeMagnitude(const int nbComponents)
{
  return nbComponents == 2 || nbComponents == 3;
}

//------------------------------------------------------------------------------
void vtkPlotHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
