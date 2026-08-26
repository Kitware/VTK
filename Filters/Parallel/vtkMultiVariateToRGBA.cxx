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
#include "vtkImplicitArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMultiVariateToRGBA);
vtkCxxSetObjectMacro(vtkMultiVariateToRGBA, Controller, vtkMultiProcessController);

namespace
{
// Implement numpy.mod using std::fmod for a / b.
// std::fmod returns a remainder with the sign of the dividend (a) while
// numpy.mod returns a remainder with the sign of the divisor (b).
// This code mirrors behavior from Python/NumPy mod, hence the name.
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
 * Normalizes all components in a modality group together.
 *
 * For example, if indices `[0, 1, 2]` belong to the same modality, the
 * min/max is computed across all three components and each is scaled to
 * [NewMin, NewMax] using that shared range.
 */
struct NormalizeWorker
{
  NormalizeWorker(const std::vector<int>& indices, vtkMultiProcessController* controller,
    double newMin = 0, double newMax = 1)
    : GroupIndices(indices)
    , NewMin(newMin)
    , NewMax(newMax)
    , Controller(controller)
  {
  }

  template <typename Input>
  void operator()(Input* scalarsArray, vtkDoubleArray* normalizedArray)
  {
    auto scalars = vtk::DataArrayTupleRange(scalarsArray);
    auto normalized = vtk::DataArrayTupleRange(normalizedArray);
    using ScalarsRangeType = decltype(scalars);
    using ValueType = typename ScalarsRangeType::ComponentType;

    // single pass over the tuples for the min/max of all components in the group
    ValueType minValue = std::numeric_limits<ValueType>::max(),
              maxValue = std::numeric_limits<ValueType>::lowest();
    vtkIdType nTuples = scalars.size();
    for (vtkIdType i = 0; i < nTuples; ++i)
    {
      for (size_t k = 0; k < this->GroupIndices.size(); ++k)
      {
        ValueType value = scalars[i][this->GroupIndices[k]];
        if (value < minValue)
        {
          minValue = value;
        }
        if (value > maxValue)
        {
          maxValue = value;
        }
      }
    }
    if (this->Controller)
    {
      double globalMin;
      double localMin = minValue;
      this->Controller->AllReduce(&localMin, &globalMin, 1, vtkCommunicator::MIN_OP);
      minValue = globalMin;
    }
    if (this->Controller)
    {
      double globalMax;
      double localMax = maxValue;
      this->Controller->AllReduce(&localMax, &globalMax, 1, vtkCommunicator::MAX_OP);
      maxValue = globalMax;
    }
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          for (size_t k = 0; k < this->GroupIndices.size(); ++k)
          {
            normalized[i][this->GroupIndices[k]] = (this->NewMax - this->NewMin) *
                (scalars[i][this->GroupIndices[k]] - minValue) / (maxValue - minValue) +
              this->NewMin;
          }
        }
      });
  }
  std::vector<int> GroupIndices;
  double NewMin = 0;
  double NewMax = 1;
  vtkMultiProcessController* Controller = nullptr;
};

/**
 * Computes generalized barycentric coordinate (GBC) positions for each data point.
 * See: "RadViz Deluxe: An Attribute-Aware Display for Multivariate Data"
 */
struct ComputeGBCWorker
{
  ComputeGBCWorker(vtkMultiVariateToRGBA* filter, double verticesRotation)
    : Filter(filter)
    , VerticesRotationRad(vtkMath::RadiansFromDegrees(verticesRotation))
  {
  }
  void operator()(vtkDoubleArray* normalizedArray, vtkDoubleArray* gbcArray)
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
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          // the array is not zero-initialized on allocation
          gbc[i][0] = 0;
          gbc[i][1] = 0;
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
      });
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

  void operator()(vtkDoubleArray* gbcScalars, vtkDoubleArray* hslScalars)
  {
    auto gbc = vtk::DataArrayTupleRange(gbcScalars);
    auto hsl = vtk::DataArrayTupleRange(hslScalars);

    vtkIdType nTuples = gbc.size();
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
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
      });
  }
  vtkMultiVariateToRGBA* Filter = nullptr;
  double Lightness = 0;
};

double hslValue(double m1, double m2, double h)
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

void hslToRgb(double h, double s, double l, double* rgb)
{
  double m2;
  if (l < 0.5)
  {
    m2 = l * (1 + s);
  }
  else
  {
    m2 = l + s - l * s;
  }
  double m1 = 2 * l - m2;
  rgb[0] = hslValue(m1, m2, h + 1.0 / 3);
  rgb[1] = hslValue(m1, m2, h);
  rgb[2] = hslValue(m1, m2, h - 1.0 / 3);
}

struct HSLToRGBWorker
{
  void operator()(vtkDoubleArray* hslScalars, vtkDoubleArray* rgbaScalars)
  {
    auto hsl = vtk::DataArrayTupleRange(hslScalars);
    auto rgba = vtk::DataArrayTupleRange(rgbaScalars);

    vtkIdType nTuples = hsl.size();
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          double rgb[3];
          hslToRgb(hsl[i][0], hsl[i][1], hsl[i][2], rgb);
          rgba[i][0] = rgb[0];
          rgba[i][1] = rgb[1];
          rgba[i][2] = rgb[2];
        }
      });
  }
};

struct ComputeAlphaWorker
{
  ComputeAlphaWorker() = default;
  void operator()(vtkDoubleArray* normalizedScalars, vtkDoubleArray* rgbaScalars)
  {
    auto normalized = vtk::DataArrayTupleRange(normalizedScalars);
    auto rgba = vtk::DataArrayTupleRange(rgbaScalars);

    vtkIdType nComponents = normalized.GetTupleSize();

    vtkIdType nTuples = normalized.size();
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
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
      });
  }
};

struct ComputeLensAlphaWorker
{
  ComputeLensAlphaWorker(double* lensCenter, double lensRadius, double outsideLensAlpha)
    : LensRadius(lensRadius)
    , OutsideLensAlpha(outsideLensAlpha)
  {
    std::copy(lensCenter, lensCenter + 2, this->LensCenter);
  }
  void operator()(vtkDoubleArray* gbcScalars, vtkDoubleArray* rgbaScalars)
  {
    auto gbc = vtk::DataArrayTupleRange(gbcScalars);
    auto rgba = vtk::DataArrayTupleRange(rgbaScalars);

    vtkIdType nTuples = gbc.size();
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          double distance = std::sqrt(std::pow(gbc[i][0] - this->LensCenter[0], 2) +
            std::pow(gbc[i][1] - this->LensCenter[1], 2));
          if (distance > this->LensRadius)
          {
            rgba[i][3] = std::min(this->OutsideLensAlpha, static_cast<double>(rgba[i][3]));
          }
        }
      });
  }
  double LensCenter[2];
  double LensRadius;
  double OutsideLensAlpha;
};

/**
 * Creats a SOA array from a list of column arrays.
 */
template <typename DataType>
vtkSmartPointer<vtkDataArray> createSOAArray(const std::vector<vtkAbstractArray*>& columns)
{
  vtkAOSDataArrayTemplate<DataType>* a0 =
    vtkAOSDataArrayTemplate<DataType>::FastDownCast(columns[0]);
  vtkIdType size = a0->GetNumberOfTuples();
  auto soaArray = vtkSmartPointer<vtkSOADataArrayTemplate<DataType>>::New();
  soaArray->SetNumberOfComponents(static_cast<int>(columns.size()));
  soaArray->SetNumberOfTuples(size);
  for (size_t i = 0; i < columns.size(); ++i)
  {
    vtkAOSDataArrayTemplate<DataType>* a =
      vtkAOSDataArrayTemplate<DataType>::FastDownCast(columns[i]);
    if (!a)
    {
      // arrays in columns are of different type
      return nullptr;
    }
    soaArray->SetArray(
      static_cast<int>(i), a->GetPointer(0), size, false /*updateMaxId*/, true /*save*/);
  }
  return soaArray;
}

struct CreateSOAWorker
{
  CreateSOAWorker(const std::vector<vtkAbstractArray*>& columns)
    : Columns(columns)
  {
  }
  template <typename Input>
  void operator()(Input* scalarsArray)
  {
    auto scalars = vtk::DataArrayTupleRange(scalarsArray);
    using ScalarsRangeType = decltype(scalars);
    using ValueType = typename ScalarsRangeType::ComponentType;
    this->SOAArray = createSOAArray<ValueType>(this->Columns);
  }
  std::vector<vtkAbstractArray*> Columns;
  vtkSmartPointer<vtkDataArray> SOAArray;
};

/**
 * Read-only view over a subset of components of a vtkDataArray, used to
 * expose only the components marked visible via
 * vtkMultiVariateToRGBA::SetColumnVisibility without copying the underlying
 * data. ComponentMap[i] is the index, in the original array, of the i-th
 * component of this view.
 */
struct VisibleComponentsBackend
{
  vtkSmartPointer<vtkDataArray> Array;
  std::vector<int> ComponentMap;

  double mapComponent(vtkIdType tupleIdx, int comp) const
  {
    return this->Array->GetComponent(tupleIdx, this->ComponentMap[comp]);
  }

  double operator()(vtkIdType valueIdx) const
  {
    vtkIdType numComponents = static_cast<vtkIdType>(this->ComponentMap.size());
    return this->mapComponent(valueIdx / numComponents, static_cast<int>(valueIdx % numComponents));
  }
};

/**
 * Builds a new SOA array exposing only the components listed in
 * 'componentIndices', by borrowing (not copying) the per-component buffers
 * of 'soaArray'.
 */
template <typename DataType>
vtkSmartPointer<vtkDataArray> createFilteredSOAArray(
  vtkSOADataArrayTemplate<DataType>* soaArray, const std::vector<int>& componentIndices)
{
  auto result = vtkSmartPointer<vtkSOADataArrayTemplate<DataType>>::New();
  result->SetNumberOfComponents(static_cast<int>(componentIndices.size()));
  vtkIdType numTuples = soaArray->GetNumberOfTuples();
  result->SetNumberOfTuples(numTuples);
  for (size_t i = 0; i < componentIndices.size(); ++i)
  {
    result->SetArray(static_cast<int>(i), soaArray->GetComponentArrayPointer(componentIndices[i]),
      numTuples, false /*updateMaxId*/, true /*save*/);
    result->SetComponentName(static_cast<int>(i), soaArray->GetComponentName(componentIndices[i]));
  }
  return result;
}
}

//------------------------------------------------------------------------------
vtkMultiVariateToRGBA::vtkMultiVariateToRGBA()
{
  // by default process active point/cell scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkMultiVariateToRGBA::~vtkMultiVariateToRGBA()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVerticesRotation(double rotation)
{
  vtkDebugMacro(<< " setting VerticesRotation to " << rotation);
  rotation = numpy_mod(rotation, 360);
  if (this->VerticesRotation != rotation)
  {
    this->VerticesRotation = rotation;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetComponentsPartition(
  const std::vector<std::vector<int>>& modalityGroups)
{
  vtkDebugMacro(<< " setting ModalityGroups to " << fmt::format("{}", this->ComponentsPartition));
  if (this->ComponentsPartition != modalityGroups)
  {
    this->ComponentsPartition = modalityGroups;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<std::vector<int>> vtkMultiVariateToRGBA::GetComponentsPartition() VTK_FUTURE_CONST
{
  vtkDebugMacro(<< " returning ModalityGroups of " << fmt::format("{}", this->ComponentsPartition));
  return this->ComponentsPartition;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkStringArray> vtkMultiVariateToRGBA::GetAllColumnNames()
{
  vtkSmartPointer<vtkStringArray> names = vtkSmartPointer<vtkStringArray>::New();
  vtkDataObject* input = this->GetInputDataObject(0, 0);
  if (!input)
  {
    return names;
  }
  vtkTable* table = vtkTable::SafeDownCast(input);
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(input);
  if (table)
  {
    for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
      names->InsertNextValue(table->GetColumnName(i));
    }
  }
  else if (dataset)
  {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS;
    vtkDataArray* array = this->GetInputArrayToProcess(0, input, association);
    if (array)
    {
      for (int i = 0; i < array->GetNumberOfComponents(); ++i)
      {
        const char* name = array->GetComponentName(i);
        names->InsertNextValue(name ? name : std::to_string(i));
      }
    }
  }
  return names;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetColumnVisibility(const std::string& name, bool visible)
{
  vtkIdType foundIndex = -1;
  for (vtkIdType i = 0; i < this->HiddenColumns->GetNumberOfTuples(); ++i)
  {
    if (this->HiddenColumns->GetValue(i) == name)
    {
      foundIndex = i;
      break;
    }
  }
  if (!visible)
  {
    if (foundIndex >= 0)
    {
      // Already hidden, nothing more needs to be done
      return;
    }
    // Add the column to the end of the list
    this->HiddenColumns->InsertNextValue(name);
    this->Modified();
  }
  else
  {
    if (foundIndex < 0)
    {
      // Already visible, nothing more needs to be done
      return;
    }
    // Move all the later elements down by one, and reduce the size
    for (vtkIdType i = foundIndex; i < this->HiddenColumns->GetNumberOfTuples() - 1; ++i)
    {
      this->HiddenColumns->SetValue(i, this->HiddenColumns->GetValue(i + 1));
    }
    this->HiddenColumns->SetNumberOfTuples(this->HiddenColumns->GetNumberOfTuples() - 1);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetColumnVisibilityAll(bool visible)
{
  if (visible)
  {
    this->HiddenColumns->Initialize();
  }
  else
  {
    this->HiddenColumns->DeepCopy(this->GetAllColumnNames());
  }
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkMultiVariateToRGBA::GetColumnVisibility(const std::string& name)
{
  for (vtkIdType i = 0; i < this->HiddenColumns->GetNumberOfTuples(); ++i)
  {
    if (this->HiddenColumns->GetValue(i) == name)
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkMultiVariateToRGBA::GetVisibleColumns()
{
  this->VisibleColumnsCache = this->GetProcessedColumnNames();
  return this->VisibleColumnsCache;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVisibleColumns(vtkStringArray* visColumns)
{
  std::set<std::string> visibleNames;
  if (visColumns)
  {
    for (vtkIdType i = 0; i < visColumns->GetNumberOfTuples(); ++i)
    {
      visibleNames.insert(visColumns->GetValue(i));
    }
  }
  vtkSmartPointer<vtkStringArray> all = this->GetAllColumnNames();
  this->HiddenColumns->Initialize();
  for (vtkIdType i = 0; i < all->GetNumberOfTuples(); ++i)
  {
    const vtkStdString& name = all->GetValue(i);
    if (visibleNames.find(name) == visibleNames.end())
    {
      this->HiddenColumns->InsertNextValue(name);
    }
  }
  this->Modified();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkStringArray> vtkMultiVariateToRGBA::GetProcessedColumnNames()
{
  // keep only the names of the current input that are not hidden,
  // preserving their natural order.
  vtkSmartPointer<vtkStringArray> all = this->GetAllColumnNames();
  vtkSmartPointer<vtkStringArray> result = vtkSmartPointer<vtkStringArray>::New();
  for (vtkIdType i = 0; i < all->GetNumberOfTuples(); ++i)
  {
    const vtkStdString& name = all->GetValue(i);
    if (this->GetColumnVisibility(name))
    {
      result->InsertNextValue(name);
    }
  }
  return result;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkMultiVariateToRGBA::GetVisibleComponentsArray(vtkDataArray* array)
{
  std::vector<int> componentIndices;
  for (int c = 0; c < array->GetNumberOfComponents(); ++c)
  {
    // unnamed components get their index as name, same as GetAllColumnNames
    const char* compName = array->GetComponentName(c);
    if (this->GetColumnVisibility(compName ? compName : std::to_string(c)))
    {
      componentIndices.push_back(c);
    }
  }
  if (componentIndices.empty())
  {
    return nullptr;
  }

  // Fast path: nothing filtered out, use the original array directly so
  // that vtkArrayDispatch can still fast-path on its concrete type.
  bool identity = static_cast<int>(componentIndices.size()) == array->GetNumberOfComponents();
  for (size_t i = 0; identity && i < componentIndices.size(); ++i)
  {
    identity = componentIndices[i] == static_cast<int>(i);
  }
  if (identity)
  {
    return array;
  }

  // For SOA-backed arrays, borrow only the visible components' buffers
  // directly into a new (smaller) SOA array: no data copy, and no need for
  // the implicit-array indirection below.
  if (auto* soaDouble = vtkSOADataArrayTemplate<double>::FastDownCast(array))
  {
    return createFilteredSOAArray(soaDouble, componentIndices);
  }
  if (auto* soaFloat = vtkSOADataArrayTemplate<float>::FastDownCast(array))
  {
    return createFilteredSOAArray(soaFloat, componentIndices);
  }

  // Fallback for AOS (and any other) arrays, whose components cannot be
  // cheaply borrowed since they are interleaved in memory: wrap with a
  // read-only implicit array view over the visible components.
  auto backend = std::make_shared<VisibleComponentsBackend>();
  backend->Array = array;
  backend->ComponentMap = componentIndices;

  vtkNew<vtkImplicitArray<VisibleComponentsBackend>> implicitArray;
  implicitArray->SetBackend(backend);
  implicitArray->SetNumberOfComponents(static_cast<int>(componentIndices.size()));
  implicitArray->SetNumberOfTuples(array->GetNumberOfTuples());
  for (size_t i = 0; i < componentIndices.size(); ++i)
  {
    implicitArray->SetComponentName(
      static_cast<int>(i), array->GetComponentName(componentIndices[i]));
  }
  return implicitArray;
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
    auto newTable = vtkSmartPointer<vtkTable>::Take(table->NewInstance());
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newTable);
  }
  else if (dataset)
  {
    auto newDataSet = vtkSmartPointer<vtkDataSet>::Take(dataset->NewInstance());
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newDataSet);
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
  // Use a local copy: the default partition depends on the current number of
  // components and RequestData should not modify filter state.
  std::vector<std::vector<int>> partition = this->ComponentsPartition;
  if (partition.empty())
  {
    // add one group with all variables
    partition.resize(1);
    partition[0].resize(numComponents);
    std::iota(partition[0].begin(), partition[0].end(), 0);
  }
  else
  {
    // every component index has to appear in the partition exactly once
    std::vector<int> indices;
    for (const std::vector<int>& group : partition)
    {
      indices.insert(indices.end(), group.begin(), group.end());
    }
    std::sort(indices.begin(), indices.end());
    std::vector<int> expected(numComponents);
    std::iota(expected.begin(), expected.end(), 0);
    if (indices != expected)
    {
      vtkErrorMacro(<< fmt::format("Invalid partition {}: every component index in [0, {}) "
                                   "must appear exactly once",
                      partition, numComponents));
      return 0;
    }
  }

  vtkNew<vtkDoubleArray> normalizedScalars;
  normalizedScalars->SetName("normalized");
  normalizedScalars->SetNumberOfComponents(scalars->GetNumberOfComponents());
  normalizedScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  for (int i = 0; i < scalars->GetNumberOfComponents(); ++i)
  {
    normalizedScalars->SetComponentName(i, scalars->GetComponentName(i));
  }
  // the input scalars can be of any numeric type; the intermediate arrays created by this
  // filter are always vtkDoubleArray, so only the input array needs to be dispatched.
  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> InputDispatcher;
  for (size_t i = 0; i < partition.size(); ++i)
  {
    NormalizeWorker worker(partition[i], this->Controller);
    if (!InputDispatcher::Execute(scalars, worker, normalizedScalars.GetPointer()))
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
    worker(normalizedScalars.GetPointer(), gbcScalars.GetPointer());
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
      GBCToHSLWorker worker(this, vtkMultiVariateToRGBA::ColorLightness);
      worker(gbcScalars.GetPointer(), hslScalars.GetPointer());
    }
    vtkNew<vtkDoubleArray> rgbaScalars;
    rgbaScalars->SetName("rgba");
    rgbaScalars->SetNumberOfComponents(4);
    rgbaScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
    {
      HSLToRGBWorker worker;
      worker(hslScalars.GetPointer(), rgbaScalars.GetPointer());
    }
    {
      ComputeAlphaWorker worker;
      worker(normalizedScalars.GetPointer(), rgbaScalars.GetPointer());
    }
    {
      ComputeLensAlphaWorker worker(this->LensCenter, this->LensRadius, this->OutsideLensAlphaCap);
      worker(gbcScalars.GetPointer(), rgbaScalars.GetPointer());
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

    // Only keep the columns that are not hidden, in their natural order.
    std::vector<vtkAbstractArray*> columns;
    for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
      const char* name = table->GetColumnName(i);
      if (name && this->GetColumnVisibility(name))
      {
        columns.push_back(table->GetColumn(i));
      }
    }
    if (columns.empty())
    {
      vtkErrorWithObjectMacro(input, "No visible columns found: ");
      return { attributes, nullptr };
    }

    auto* array0 = vtkDataArray::FastDownCast(columns[0]);
    if (!array0)
    {
      vtkErrorWithObjectMacro(input, "Column is not a numeric array: " << columns[0]->GetName());
      return { attributes, nullptr };
    }
    // for a table, convert the columns into a SOA array with multiple components
    if (columns.size() > 1)
    {
      if (columns.size() < 3)
      {
        vtkErrorWithObjectMacro(
          input, "Number of visible columns is less than 3: " << columns.size());
        return { attributes, nullptr };
      }
      typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> Dispatcher;
      CreateSOAWorker worker(columns);
      if (!Dispatcher::Execute(array0, worker))
      {
        vtkErrorWithObjectMacro(input, "Array type not supported: " << array0->GetDataType());
        return { attributes, nullptr };
      }
      else
      {
        return { attributes, worker.SOAArray };
      }
    }
    else
    {
      // otherwise, we just use the existing array with multiple components
      if (array0->GetNumberOfComponents() > 2)
      {
        return { attributes, array0 };
      }
      else
      {
        vtkErrorWithObjectMacro(input, "Array has less than 3 components: " << array0->GetName());
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
          break;
        case vtkDataObject::FIELD_ASSOCIATION_POINTS:
          attributes = dataset->GetPointData();
          break;
        case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
          attributes = dataset->GetPointData();
          if (!attributes)
          {
            attributes = dataset->GetCellData();
          }
          break;
        default:
          vtkErrorWithObjectMacro(input, "Invalid association: " << association);
          return { nullptr, nullptr };
      }
      // Only keep the components marked visible (or every component, if
      // none has been explicitly configured), using a read-only implicit
      // array view so the underlying data is not copied.
      vtkSmartPointer<vtkDataArray> visibleArray = this->GetVisibleComponentsArray(array);
      if (!visibleArray)
      {
        vtkErrorWithObjectMacro(input, "No visible components found: ");
        return { attributes, nullptr };
      }
      if (visibleArray->GetNumberOfComponents() < 3)
      {
        vtkErrorWithObjectMacro(input,
          "Number of visible components is less than 3: " << visibleArray->GetNumberOfComponents());
        return { attributes, nullptr };
      }
      return { attributes, visibleArray };
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

double vtkMultiVariateToRGBA::GetLensRadius(
  double lensRadius, double radius, const double* lensCenter, const double* center)
{
  // Ensure the lens circle intersects the reference circle.
  // r is the distance between the two circle centers.
  double r =
    std::sqrt(std::pow(lensCenter[0] - center[0], 2) + std::pow(lensCenter[1] - center[1], 2));
  double newLensRadius;
  // If the two circles do not intersect, grow the lens radius until they just touch.
  if (r - lensRadius > radius)
  {
    newLensRadius = r - radius;
  }
  else
  {
    newLensRadius = lensRadius;
  }
  return newLensRadius;
}

void vtkMultiVariateToRGBA::SetLensRadius(double lensRadius)
{
  double newLensRadius = vtkMultiVariateToRGBA::GetLensRadius(
    lensRadius, 1, this->LensCenter, std::array<double, 2>{ 0, 0 }.data());
  if (newLensRadius != this->LensRadius)
  {
    this->LensRadius = newLensRadius;
    this->Modified();
  }
}

std::array<double, 2> vtkMultiVariateToRGBA::GetLensCenter(
  double lensRadius, double radius, const double* lensCenter, const double* center)
{
  // Ensure the lens circle intersects the reference circle.
  // r is the distance between the two circle centers.
  double r =
    std::sqrt(std::pow(lensCenter[0] - center[0], 2) + std::pow(lensCenter[1] - center[1], 2));
  std::array<double, 2> newLensCenter;
  // If the two circles do not intersect, move the lens center toward the reference
  // circle until they just touch.
  if (r - lensRadius > radius)
  {
    std::array<double, 2> intersection;
    intersection = { ((lensCenter[0] - center[0]) / r) * radius,
      ((lensCenter[1] - center[1]) / r) * radius };
    newLensCenter = { (intersection[0] / radius) * (radius + lensRadius) + center[0],
      (intersection[1] / radius) * (radius + lensRadius) + center[1] };
  }
  else
  {
    std::copy(lensCenter, lensCenter + 2, newLensCenter.begin());
  }
  return newLensCenter;
}

void vtkMultiVariateToRGBA::SetLensCenter(double centerX, double centerY)
{
  SetLensCenter(std::array<double, 2>{ centerX, centerY }.data());
}

void vtkMultiVariateToRGBA::SetLensCenter(const double* lensCenter)
{
  std::array<double, 2> newLensCenter = vtkMultiVariateToRGBA::GetLensCenter(
    this->LensRadius, 1, lensCenter, std::array<double, 2>{ 0, 0 }.data());
  if (this->LensCenter[0] != newLensCenter[0] || this->LensCenter[1] != newLensCenter[1])
  {
    std::copy(newLensCenter.begin(), newLensCenter.end(), this->LensCenter);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::HSLToRGB(double h, double s, double l, double rgb[3])
{
  hslToRgb(h, s, l, rgb);
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
