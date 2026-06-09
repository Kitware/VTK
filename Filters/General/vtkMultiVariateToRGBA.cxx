// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiVariateToRGBA.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkTable.h"

#include <algorithm>
#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

#include <cmath>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMultiVariateToRGBA);

namespace
{
// Implement numpy.mod using std::fmod for a / b.
// std::fmod returns a remainder with the sign of the dividend (a) while
// numpy.mod returns a reminder with he sign of the divisor (b)
// This code was ported from python/numpy so we use numpy_mod.
double numpy_mod(double a, double b)
{
  double r = std::fmod(a, b);
  // If remainder is non-zero and has a different sign than b, the divisor, adjust.
  if (r != 0.0 && ((r < 0.0) != (b < 0.0)))
  {
    r += b;
  }
  return r;
}

/**
 * Normalize each modality group together.
 *
 * For example, if Indices `[0, 1, 2]` are the same modality, we will
 * normalize them all together, meaning we compute the min/max over
 * all three scalars.
 *
 * In this example, all channels are in the same modality, so they all
 * get normalized together.
 */
struct NormalizeWorker
{
  NormalizeWorker(const std::vector<int>& indices,
    std::function<double(double)> globalMin = nullptr,
    std::function<double(double)> globalMax = nullptr, double newMin = 0, double newMax = 1)
    : Indices(indices)
    , GlobalMin(globalMin)
    , GlobalMax(globalMax)
    , NewMin(newMin)
    , NewMax(newMax)
  {
  }

  template <typename Input, typename Output>
  void operator()(Input* scalarsArray, Output* normalizedArray)
  {
    auto scalars = vtk::DataArrayTupleRange(scalarsArray);
    auto normalized = vtk::DataArrayTupleRange(normalizedArray);
    using ScalarsRangeType = decltype(scalars);
    using ValueType = typename ScalarsRangeType::ComponentType;
    using TupleReferenceType = typename ScalarsRangeType::TupleReferenceType;
    using TupleIteratorType = typename ScalarsRangeType::TupleIteratorType;

    ValueType minValue = std::numeric_limits<ValueType>::max(),
              maxValue = std::numeric_limits<ValueType>::lowest();
    for (int k = 0; k < this->Indices.size(); ++k)
    {
      auto comparator = [this, k](const TupleReferenceType& a, const TupleReferenceType& b)
      { return a[this->Indices[k]] < b[this->Indices[k]]; };
      TupleIteratorType minIt = std::min_element(scalars.begin(), scalars.end(), comparator);
      ValueType minCurrent = (*minIt)[this->Indices[k]];
      TupleIteratorType maxIt = std::max_element(scalars.begin(), scalars.end(), comparator);
      ValueType maxCurrent = (*maxIt)[this->Indices[k]];
      if (minCurrent < minValue)
      {
        minValue = minCurrent;
      }
      if (maxCurrent > maxValue)
      {
        maxValue = maxCurrent;
      }
    }
    if (this->GlobalMin)
    {
      minValue = this->GlobalMin(minValue);
    }
    if (this->GlobalMax)
    {
      maxValue = this->GlobalMax(maxValue);
    }
    vtkIdType nTuples = scalars.size();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      for (int k = 0; k < this->Indices.size(); ++k)
      {
        normalized[i][this->Indices[k]] = (this->NewMax - this->NewMin) *
            (scalars[i][this->Indices[k]] - minValue) / (maxValue - minValue) +
          this->NewMin;
      }
    }
  }
  std::vector<int> Indices;
  double NewMin = 0;
  double NewMax = 1;
  std::function<double(double)> GlobalMin = nullptr;
  std::function<double(double)> GlobalMax = nullptr;
};

/**
 * Computes Generalized Barycentric Coordinate (GBC) interpolation
 * Uses
 * RadViz Deluxe: An Attribute-Aware Display for Multivariate Data
 */
struct ComputeGBCWorker
{
  ComputeGBCWorker(vtkMultiVariateToRGBA* filter, double verticesRotation)
    : Filter(filter)
    , VerticesRotationRad(vtkMath::RadiansFromDegrees(verticesRotation))
  {
  }
  template <typename Input, typename Output>
  void operator()(Input* normalizedArray, Output* gbcArray)
  {
    auto normalized = vtk::DataArrayTupleRange(normalizedArray);
    auto gbc = vtk::DataArrayTupleRange(gbcArray);

    // computes the vertices (VF), Table 1, GBC, first column
    vtkIdType nComponents = normalized.GetTupleSize();
    this->Vertices = vtkSmartPointer<vtkDoubleArray>::New();
    this->Vertices->SetName("vertices");
    this->Vertices->SetNumberOfComponents(2);
    this->Vertices->SetNumberOfTuples(nComponents);
    auto vertices = vtk::DataArrayTupleRange(this->Vertices);
    std::vector<double> angle(nComponents);
    angle[0] = vtkMath::Pi() / 2;
    constexpr double adjustment = 0.997;
    vertices[0][0] = std::cos(angle[0]) * adjustment;
    vertices[0][1] = std::sin(angle[0]) * adjustment;
    for (int i = 1; i < nComponents; ++i)
    {
      angle[i] = angle[i - 1] - 2 * vtkMath::Pi() / nComponents;
      vertices[i][0] = std::cos(angle[i]) * adjustment;
      vertices[i][1] = std::sin(angle[i]) * adjustment;
      // Wrap to the 0 to 2*pi range
      angle[i] = numpy_mod(angle[i], 2 * vtkMath::Pi());
    }
    std::sort(angle.begin(), angle.end());

    vtkIdType nTuples = normalized.size();
    std::array<std::array<double, 2>, 2> m{
      { { cos(this->VerticesRotationRad), -sin(this->VerticesRotationRad) },
        { sin(this->VerticesRotationRad), cos(this->VerticesRotationRad) } }
    };
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      // compute points (MF), Table 1, GBC, second column
      // TupleRange doesn't work with std::accumulate / std::reduce
      double tempsum = 0;
      for (int k = 0; k < nComponents; ++k)
      {
        tempsum += normalized[i][k];
      }
      if (vtkMathUtilities::FuzzyCompare(tempsum, 0.0))
      {
        continue;
      }
      gbc[i][0] = 0;
      gbc[i][1] = 0;
      for (int k = 0; k < nComponents; ++k)
      {
        for (int j = 0; j < 2; ++j)
        {
          gbc[i][j] += normalized[i][k] * vertices[k][j] / tempsum;
        }
      }

      // tempangle is the angle: gbc[i], (0,0), (1, 0).
      // find temp_b, temp_a, which are the angles for the vertices
      // between which tempangle fits?
      double tempangle = std::atan2(gbc[i][1], gbc[i][0]);
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
      // scale gbc from a point on a side of the regular polygon to a point on the circle
      // circumscribed to the polygon
      double lth = std::sqrt(std::pow(gbc[i][0], 2) + std::pow(gbc[i][1], 2)) *
        std::cos(-(temp_a + temp_b) / 2 + tempangle) / std::cos((temp_a - temp_b) / 2);
      gbc[i][0] = lth * std::cos(tempangle);
      gbc[i][1] = lth * std::sin(tempangle);

      if (this->Filter->GetFlipY())
      {
        gbc[i][1] = -gbc[i][1];
      }

      // rotate the gbc points with VerticesRotation
      double result_gbc[2];
      result_gbc[0] = m[0][0] * gbc[i][0] + m[0][1] * gbc[i][1];
      result_gbc[1] = m[1][0] * gbc[i][0] + m[1][1] * gbc[i][1];
      gbc[i][0] = result_gbc[0];
      gbc[i][1] = result_gbc[1];
    }
    for (int i = 0; i < nComponents; ++i)
    {
      if (this->Filter->GetFlipY())
      {
        vertices[i][1] = -vertices[i][1];
      }
      // rotate the vertices
      double result_vertices[2];
      // vtkMath::MultiplyMatrixWithVector<2, 2>(m, vertices[i], result_vertices);
      result_vertices[0] = m[0][0] * vertices[i][0] + m[0][1] * vertices[i][1];
      result_vertices[1] = m[1][0] * vertices[i][0] + m[1][1] * vertices[i][1];
      vertices[i][0] = result_vertices[0];
      vertices[i][1] = result_vertices[1];
    }
  }
  vtkMultiVariateToRGBA* Filter = nullptr;
  double VerticesRotationRad = 0;
  vtkSmartPointer<vtkDoubleArray> Vertices;
};

struct GBCToHSLWorker
{
  GBCToHSLWorker(vtkMultiVariateToRGBA* filter, double lightness)
    : Filter(filter)
    , Lightness(lightness)
  {
  }

  template <typename Input, typename Output>
  void operator()(Input* gbcScalars, Output* hslScalars)
  {
    auto gbc = vtk::DataArrayTupleRange(gbcScalars);
    auto hsl = vtk::DataArrayTupleRange(hslScalars);

    vtkIdType nComponents = gbc.GetTupleSize();

    vtkIdType nTuples = gbc.size();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double gbcValue[2] = { gbc[i][0], gbc[i][1] };
      if (this->Filter->GetFlipY())
      {
        gbcValue[1] = -gbcValue[1];
      }
      double radius = std::sqrt(std::pow(gbcValue[0], 2) + std::pow(gbcValue[1], 2));
      if (radius > 1)
      {
        gbcValue[0] /= radius;
        gbcValue[1] /= radius;
      }
      double hue = std::atan2(gbcValue[1], gbcValue[0]) + vtkMath::Pi() / 2;
      hue = numpy_mod(hue, 2 * vtkMath::Pi()) / (2 * vtkMath::Pi());
      double saturation = std::sqrt(std::pow(gbcValue[0], 2) + std::pow(gbcValue[1], 2));
      hsl[i][0] = hue;
      hsl[i][1] = saturation;
      hsl[i][2] = this->Lightness;
    }
  }
  vtkMultiVariateToRGBA* Filter = nullptr;
  double Lightness = 0;
};

struct HSLToRGBWorker
{
  template <typename Input, typename Output>
  void operator()(Input* hslScalars, Output* rgbaScalars)
  {
    auto hsl = vtk::DataArrayTupleRange(hslScalars);
    auto rgba = vtk::DataArrayTupleRange(rgbaScalars);

    vtkIdType nComponents = hsl.GetTupleSize();

    vtkIdType nTuples = hsl.size();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double m2;
      double l = hsl[i][2];
      if (l < 0.5)
      {
        double lower_l = hsl[i][2];
        double lower_s = hsl[i][1];
        m2 = lower_l * (1 + lower_s);
      }
      else
      {
        double upper_l = hsl[i][2];
        double upper_s = hsl[i][1];
        m2 = upper_l + upper_s - upper_l * upper_s;
      }
      double m1 = 2 * l - m2;
      double h = hsl[i][0];
      rgba[i][0] = this->v(m1, m2, h + 1.0 / 3);
      rgba[i][1] = this->v(m1, m2, h);
      rgba[i][2] = this->v(m1, m2, h - 1.0 / 3);
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
    auto normalized = vtk::DataArrayTupleRange(normalizedScalars);
    auto rgba = vtk::DataArrayTupleRange(rgbaScalars);

    vtkIdType nComponents = normalized.GetTupleSize();

    vtkIdType nTuples = normalized.size();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      double mean = 0;
      for (int k = 0; k < nComponents; ++k)
      {
        mean += normalized[i][k];
      }
      mean /= nComponents;

      // Make nonzero voxels have an alpha of the mean of the channels.
      rgba[i][3] = mean;
    }
  }
};

template <typename DataType>
vtkSmartPointer<vtkDataArray> createSOAArray(vtkDataSetAttributes* attributes)
{
  vtkAOSDataArrayTemplate<DataType>* a0 =
    vtkAOSDataArrayTemplate<DataType>::FastDownCast(attributes->GetArray(0));
  vtkIdType size = a0->GetNumberOfTuples();
  auto soaArray = vtkSmartPointer<vtkSOADataArrayTemplate<DataType>>::New();
  soaArray->SetNumberOfComponents(attributes->GetNumberOfArrays());
  soaArray->SetNumberOfTuples(size);
  for (int i = 0; i < attributes->GetNumberOfArrays(); ++i)
  {
    vtkAOSDataArrayTemplate<DataType>* a =
      vtkAOSDataArrayTemplate<DataType>::FastDownCast(attributes->GetArray(i));
    if (!a)
    {
      // arrays in attributes are of different type
      return nullptr;
    }
    soaArray->SetArray(i, a->GetPointer(0), size, false /*updateMaxId*/, true /*save*/);
  }
  return soaArray;
}

struct CreateSOAWorker
{
  CreateSOAWorker(vtkDataSetAttributes* attributes)
    : Attributes(attributes)
  {
  }
  template <typename Input>
  void operator()(Input* scalarsArray)
  {
    auto scalars = vtk::DataArrayTupleRange(scalarsArray);
    using ScalarsRangeType = decltype(scalars);
    using ValueType = typename ScalarsRangeType::ComponentType;
    this->SOAArray = createSOAArray<ValueType>(this->Attributes);
  }
  vtkDataSetAttributes* Attributes;
  vtkSmartPointer<vtkDataArray> SOAArray;
};
}

//------------------------------------------------------------------------------
vtkMultiVariateToRGBA::vtkMultiVariateToRGBA()
{
  this->VerticesRotation = 0;
  // by default process active point/cell scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkMultiVariateToRGBA::~vtkMultiVariateToRGBA() = default;

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVerticesRotation(double rotation)
{
  vtkDebugMacro(<< " setting VerticesRotation to " << rotation);
  rotation = std::fmod(rotation, 360);
  if (this->VerticesRotation != rotation)
  {
    this->VerticesRotation = rotation;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetModalityGroups(std::vector<std::vector<int>>& modalityGroups)
{
  vtkDebugMacro(<< " setting ModalityGroups to " << fmt::format("{}", this->ModalityGroups));
  if (this->ModalityGroups != modalityGroups)
  {
    this->ModalityGroups = modalityGroups;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<std::vector<int>> vtkMultiVariateToRGBA::GetModalityGroups() VTK_FUTURE_CONST
{
  vtkDebugMacro(<< " returning ModalityGroups of " << fmt::format("{}", this->ModalityGroups));
  return this->ModalityGroups;
}

//------------------------------------------------------------------------------
int vtkMultiVariateToRGBA::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkTable* table = vtkTable::SafeDownCast(inData);
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(inData);
  if (table)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), table->NewInstance());
  }
  else if (dataset)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), dataset->NewInstance());
  }
  else
  {
    vtkErrorMacro("Input type not supported: " << inData->GetClassName());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkMultiVariateToRGBA::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->StoreGBC && !this->StoreRGBA)
  {
    vtkErrorMacro("You cannot have both StoreGBC and StoreRGBA set to false");
    return 0;
  }
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* outData = outInfo->Get(vtkDataObject::DATA_OBJECT());
  outData->ShallowCopy(inData);
  auto [attributes, scalars] = getAttributesAndScalars(outData, inputVector);
  if (!scalars)
  {
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
  for (int i = 0; i < scalars->GetNumberOfComponents(); ++i)
  {
    normalizedScalars->SetComponentName(i, scalars->GetComponentName(i));
  }
  typedef vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>
    Dispatcher;
  for (int i = 0; i < this->ModalityGroups.size(); ++i)
  {
    NormalizeWorker worker(this->ModalityGroups[i], this->GlobalMin, this->GlobalMax);
    if (!Dispatcher::Execute(scalars, normalizedScalars, worker))
    {
      worker(scalars.GetPointer(), normalizedScalars.GetPointer());
    }
  }
  vtkNew<vtkDoubleArray> gbcScalars;
  gbcScalars->SetName("gbc");
  gbcScalars->SetNumberOfComponents(2);
  gbcScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  {
    ComputeGBCWorker worker(this, this->VerticesRotation);
    if (!Dispatcher::Execute(normalizedScalars, gbcScalars, worker))
    {
      worker(normalizedScalars.GetPointer(), gbcScalars.GetPointer());
    }
    if (this->StoreVertices)
    {
      outData->GetFieldData()->AddArray(worker.Vertices);
    }
  }
  if (this->StoreGBC)
  {
    attributes->AddArray(gbcScalars);
  }
  if (this->StoreRGBA)
  {
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
    attributes->AddArray(rgbaScalars);
  }
  return 1;
}

//------------------------------------------------------------------------------
std::pair<vtkDataSetAttributes*, vtkSmartPointer<vtkDataArray>>
vtkMultiVariateToRGBA::getAttributesAndScalars(
  vtkDataObject* input, vtkInformationVector** inputVector)
{
  vtkTable* table = vtkTable::SafeDownCast(input);
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(input);
  vtkDataSetAttributes* attributes = nullptr;
  if (table)
  {
    // we don't use the datasetScalarsOverride for a table
    attributes = table->GetRowData();
    if (attributes->GetNumberOfArrays() == 0)
    {
      vtkErrorWithObjectMacro(input, "No arrays found: ");
      return { attributes, nullptr };
    }
    auto* array0 = attributes->GetArray(0);
    int dataType = array0->GetDataType();
    // for a table, convert the columns into a SOA array with multiple components
    if (attributes->GetNumberOfArrays() > 1)
    {
      typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> Dispatcher;
      CreateSOAWorker worker(attributes);
      if (!Dispatcher::Execute(array0, worker))
      {
        vtkErrorWithObjectMacro(input, "Array type not supported: " << dataType);
        return { attributes, nullptr };
      }
      else
      {
        return { attributes, worker.SOAArray };
      }
    }
    else
    {
      // otherwise, we just use the existing arrays with multiple components
      if (array0->GetNumberOfComponents() > 1)
      {
        return { attributes, array0 };
      }
      else
      {
        vtkErrorWithObjectMacro(input, "Array has only one components: " << array0->GetName());
        return { attributes, nullptr };
      }
    }
  }
  else if (dataset)
  {
    int association = vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS;
    vtkDataArray* array = this->GetInputArrayToProcess(0, inputVector, association);
    if (array)
    {
      switch (association)
      {
        case vtkDataObject::FIELD_ASSOCIATION_CELLS:
          attributes = dataset->GetCellData();
          return { attributes, array };
        case vtkDataObject::FIELD_ASSOCIATION_POINTS:
          attributes = dataset->GetPointData();
          return { attributes, array };
        case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
          attributes = dataset->GetPointData();
          if (!attributes)
          {
            attributes = dataset->GetCellData();
          }
          return { attributes, array };
        default:
          vtkErrorWithObjectMacro(input, "Invalid association: " << association);
          return { nullptr, nullptr };
      }
    }
    else
    {
      vtkErrorWithObjectMacro(input, "No scalar data to process");
      return { nullptr, nullptr };
    }
  }
  vtkErrorWithObjectMacro(input, "Invalid input type: " << input->GetClassName());
  return { nullptr, nullptr };
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
