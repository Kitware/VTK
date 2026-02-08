// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageMultiVariateToRGBA.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

#include <cmath>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageMultiVariateToRGBA);

namespace
{
double numpy_mod(double a, double b)
{
  // fmod returns a remainder with the sign of the dividend (a)
  double r = std::fmod(a, b);
  // If remainder is non-zero and has a different sign than the divisor, adjust.
  if (r != 0.0 && ((r < 0.0) != (b < 0.0)))
  {
    r += b;
  }
  return r;
}

/**
 * Normalize each modality group together.
 * For example, if Indices `[0, 1, 2]` are the same modality, we will
 * normalize them all together.
 * In this example, all channels are in the same modality, so they all
 * get normalized together.
 */
struct NormalizeWorker
{
  NormalizeWorker(const std::vector<int>& indices, double newMin = 0, double newMax = 1)
    : Indices(indices)
    , NewMin(newMin)
    , NewMax(newMax)
  {
  }

  template <typename Input, typename Output>
  void operator()(Input* scalarsArray, Output* normalizedArray)
  {
    double range[2];
    double minValue = std::numeric_limits<double>::max(),
           maxValue = std::numeric_limits<double>::lowest();
    for (int k = 0; k < this->Indices.size(); ++k)
    {
      scalarsArray->GetRange(range, this->Indices[k]);
      if (range[0] < minValue)
      {
        minValue = range[0];
      }
      if (range[1] > maxValue)
      {
        maxValue = range[1];
      }
    }
    vtkDataArrayAccessor<Input> scalars(scalarsArray);
    vtkDataArrayAccessor<Output> normalized(normalizedArray);
    vtkIdType nTuples = scalarsArray->GetNumberOfTuples();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      for (int k = 0; k < this->Indices.size(); ++k)
      {
        normalized.Set(i, this->Indices[k],
          (this->NewMax - this->NewMin) * (scalars.Get(i, this->Indices[k]) - minValue) /
              (maxValue - minValue) +
            this->NewMin);
      }
    }
  }
  std::vector<int> Indices;
  double NewMin = 0;
  double NewMax = 1;
};

struct ComputeGBCWorker
{
  template <typename Input, typename Output>
  void operator()(Input* normalizedArray, Output* gbcArray)
  {
    vtkDataArrayAccessor<Input> normalized(normalizedArray);
    vtkDataArrayAccessor<Output> gbc(gbcArray);

    vtkIdType nComponents = normalizedArray->GetNumberOfComponents();
    std::vector<double> angle(nComponents);
    std::vector<std::array<double, 2>> components(nComponents);
    angle[0] = vtkMath::Pi() / 2;
    components[0][0] = std::cos(angle[0]);
    components[0][1] = std::sin(angle[0]);
    constexpr double adjustment = 0.997;
    for (int i = 1; i < nComponents; ++i)
    {
      angle[i] = angle[i - 1] - 2 * vtkMath::Pi() / nComponents;
      components[i][0] = std::cos(angle[i]) * adjustment;
      components[i][1] = std::sin(angle[i]) * adjustment;
      // Wrap to the 0 to 2*pi range
      angle[i] = numpy_mod(angle[i], 2 * vtkMath::Pi());
    }
    std::sort(angle.begin(), angle.end());

    vtkIdType nTuples = normalizedArray->GetNumberOfTuples();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double tempsum = 0;
      for (int k = 0; k < nComponents; ++k)
      {
        tempsum += normalized.Get(i, k);
      }
      gbc.Set(i, 0, 0);
      gbc.Set(i, 1, 0);
      if (!vtkMathUtilities::FuzzyCompare(tempsum, 0.0))
      {
        for (int j = 0; j < 2; ++j)
        {
          for (int k = 0; k < nComponents; ++k)
          {
            gbc.Set(i, j, gbc.Get(i, j) + normalized.Get(i, k) * components[k][j] / tempsum);
          }
        }
        double tempangle = std::atan2(gbc.Get(i, 1), gbc.Get(i, 0));
        tempangle = numpy_mod(tempangle, 2 * vtkMath::Pi());

        bool flag = false;
        double temp_a = 0;
        double temp_b = 0;
        for (int j = 0; j < nComponents - 1; ++j)
        {
          if (angle[j] <= tempangle && tempangle < angle[j + 1])
          {
            temp_a = angle[j + 1];
            temp_b = angle[j];
            flag = true;
            break;
          }
        }
        if (!flag)
        {
          temp_a = angle[0] + vtkMath::Pi() * 2;
          temp_b = angle[nComponents - 1];
        }

        double lth = std::sqrt(std::pow(gbc.Get(i, 0), 2) + std::pow(gbc.Get(i, 1), 2)) /
          std::cos((temp_a - temp_b) / 2) * std::cos(-(temp_a + temp_b) / 2 + tempangle);
        gbc.Set(i, 0, lth * std::cos(tempangle));
        gbc.Set(i, 1, lth * std::sin(tempangle));
      }
    }
  }
};

struct GBCToHSLWorker
{
  GBCToHSLWorker(vtkObject* filter, double lightness)
    : Filter(filter)
    , Lightness(lightness)
  {
  }

  template <typename Input, typename Output>
  void operator()(Input* gbcScalars, Output* hslScalars)
  {
    vtkDataArrayAccessor<Input> gbc(gbcScalars);
    vtkDataArrayAccessor<Output> hsl(hslScalars);

    vtkIdType nComponents = gbcScalars->GetNumberOfComponents();

    vtkIdType nTuples = gbcScalars->GetNumberOfTuples();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double gbcValue[2] = { gbc.Get(i, 0), gbc.Get(i, 1) };
      double radius = std::sqrt(std::pow(gbcValue[0], 2) + std::pow(gbcValue[1], 2));
      if (radius > 1)
      {
        gbcValue[0] /= radius;
        gbcValue[1] /= radius;
      }
      double hue = std::atan2(gbcValue[1], gbcValue[0]) + vtkMath::Pi() / 2;
      hue = numpy_mod(hue, 2 * vtkMath::Pi()) / (2 * vtkMath::Pi());
      double saturation = std::sqrt(std::pow(gbcValue[0], 2) + std::pow(gbcValue[1], 2));
      hsl.Set(i, 0, hue);
      hsl.Set(i, 1, saturation);
      hsl.Set(i, 2, this->Lightness);
    }
  }
  vtkObject* Filter;
  double Lightness;
};

struct HSLToRGBWorker
{
  template <typename Input, typename Output>
  void operator()(Input* hslScalars, Output* rgbaScalars)
  {
    vtkDataArrayAccessor<Input> hsl(hslScalars);
    vtkDataArrayAccessor<Output> rgba(rgbaScalars);

    vtkIdType nComponents = hslScalars->GetNumberOfComponents();

    vtkIdType nTuples = hslScalars->GetNumberOfTuples();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double m2;
      double l = hsl.Get(i, 2);
      if (l < 0.5)
      {
        double lower_l = hsl.Get(i, 2);
        double lower_s = hsl.Get(i, 1);
        m2 = lower_l * (1 + lower_s);
      }
      else
      {
        double upper_l = hsl.Get(i, 2);
        double upper_s = hsl.Get(i, 1);
        m2 = upper_l + upper_s - upper_l * upper_s;
      }
      double m1 = 2 * l - m2;
      double h = hsl.Get(i, 0);
      rgba.Set(i, 0, this->v(m1, m2, h + 1.0 / 3));
      rgba.Set(i, 1, this->v(m1, m2, h));
      rgba.Set(i, 2, this->v(m1, m2, h - 1.0 / 3));
    }
  }

  double v(double m1, double m2, double h)
  {
    h = numpy_mod(h, 1.0);
    double m_diff = m2 - m1;
    double result;
    if (h < 1.0 / 6)
    {
      result = m1 + m_diff * h * 6;
    }
    else if (h < 0.5)
    {
      result = m2;
    }
    else if (h < 2.0 / 3)
    {
      result = m1 + m_diff * (2.0 / 3 - h) * 6;
    }
    else
    {
      result = m1;
    }
    return result;
  }
};

struct ComputeAlphaWorker
{
  template <typename Input, typename Output>
  void operator()(Input* normalizedScalars, Output* rgbaScalars)
  {
    vtkDataArrayAccessor<Input> normalized(normalizedScalars);
    vtkDataArrayAccessor<Output> rgba(rgbaScalars);

    vtkIdType nComponents = normalizedScalars->GetNumberOfComponents();

    vtkIdType nTuples = normalizedScalars->GetNumberOfTuples();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double mean = 0;
      for (int k = 0; k < nComponents; ++k)
      {
        mean += normalized.Get(i, k);
      }
      mean /= nComponents;

      // Make nonzero voxels have an alpha of the mean of the channels.
      rgba.Set(i, 3, mean);
    }
  }
};

}

//==============================================================================
vtkImageMultiVariateToRGBA::vtkImageMultiVariateToRGBA()
{
  this->ColorWheelRotation = 0;
}

//==============================================================================
vtkImageMultiVariateToRGBA::~vtkImageMultiVariateToRGBA() {}

//==============================================================================
void vtkImageMultiVariateToRGBA::SetModalityGroups(std::vector<std::vector<int>>& modalityGroups)
{
  vtkDebugMacro(<< " setting ModalityGroups to " << fmt::format("{}", this->ModalityGroups));
  if (this->ModalityGroups != modalityGroups)
  {
    this->ModalityGroups = modalityGroups;
    this->Modified();
  }
}

//==============================================================================
std::vector<std::vector<int>> vtkImageMultiVariateToRGBA::GetModalityGroups() VTK_FUTURE_CONST
{
  vtkDebugMacro(<< " returning ModalityGroups of " << fmt::format("{}", this->ModalityGroups));
  return this->ModalityGroups;
}

//==============================================================================
int vtkImageMultiVariateToRGBA::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* inData = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* outData = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outData->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  int inExt[6];
  inData->GetExtent(inExt);
  if (inExt[1] < inExt[0] || inExt[3] < inExt[2] || inExt[5] < inExt[4])
  {
    vtkErrorMacro("A scalar input with variables as a components "
                  "is needed for this filter");
    return 0;
  }

  auto* pointData = inData->GetPointData();
  vtkDataArray* scalars = pointData->GetScalars();
  if (!pointData || !scalars)
  {
    vtkErrorMacro("A scalar input with variables as a components is needed for this filter");
    return 0;
  }
  int numComponents = scalars->GetNumberOfComponents();
  if (this->ModalityGroups.empty())
  {
    // add one group with all variables
    this->ModalityGroups.resize(1);
    this->ModalityGroups[0].resize(numComponents);
    std::iota(this->ModalityGroups[0].begin(), this->ModalityGroups[0].end(), 0);
  }
  vtkNew<vtkDoubleArray> normalizedScalars;
  normalizedScalars->SetName("normalized");
  normalizedScalars->SetNumberOfComponents(scalars->GetNumberOfComponents());
  normalizedScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  typedef vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>
    Dispatcher;
  for (int i = 0; i < this->ModalityGroups.size(); ++i)
  {
    NormalizeWorker worker(this->ModalityGroups[i]);
    if (!Dispatcher::Execute(scalars, normalizedScalars, worker))
    {
      worker(scalars, normalizedScalars.GetPointer());
    }
  }
  vtkNew<vtkDoubleArray> gbcScalars;
  gbcScalars->SetName("gbc");
  gbcScalars->SetNumberOfComponents(2);
  gbcScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  {
    ComputeGBCWorker worker;
    if (!Dispatcher::Execute(normalizedScalars, gbcScalars, worker))
    {
      worker(normalizedScalars.GetPointer(), gbcScalars.GetPointer());
    }
  }
  vtkNew<vtkDoubleArray> hslScalars;
  hslScalars->SetName("hsl");
  hslScalars->SetNumberOfComponents(3);
  hslScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  {
    GBCToHSLWorker worker(this, 0.55);
    if (!Dispatcher::Execute(gbcScalars, hslScalars, worker))
    {
      worker(gbcScalars.GetPointer(), hslScalars.GetPointer());
    }
  }
  vtkNew<vtkDoubleArray> rgbaScalars;
  rgbaScalars->SetName("rgba");
  rgbaScalars->SetNumberOfComponents(4);
  rgbaScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  {
    HSLToRGBWorker worker;
    if (!Dispatcher::Execute(hslScalars, rgbaScalars, worker))
    {
      worker(hslScalars.GetPointer(), rgbaScalars.GetPointer());
    }
  }
  {
    ComputeAlphaWorker worker;
    if (!Dispatcher::Execute(normalizedScalars, rgbaScalars, worker))
    {
      worker(normalizedScalars.GetPointer(), rgbaScalars.GetPointer());
    }
  }
  outData->GetPointData()->AddArray(normalizedScalars);
  outData->GetPointData()->AddArray(gbcScalars);
  outData->GetPointData()->AddArray(hslScalars);
  outData->GetPointData()->AddArray(rgbaScalars);
  return 1;
}

//==============================================================================
void vtkImageMultiVariateToRGBA::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
