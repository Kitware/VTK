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
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
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
#include <tuple>

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
  // customRange is nullptr when no custom range is set for this group, in
  // which case the global min/max is computed from the data (and reduced
  // across processes if controller is not null). Otherwise, customRange
  // points to a [min, max] pair holding the global range to use, and it is
  // used as is: no AllReduce is performed even if controller is not null,
  // since the custom range is meant to already be the global range.
  NormalizeWorker(const std::vector<int>& indices, vtkMultiProcessController* controller,
    const double* customRange, double newMin = 0, double newMax = 1)
    : GroupIndices(indices)
    , NewMin(newMin)
    , NewMax(newMax)
    , Controller(controller)
    , CustomRange(customRange)
  {
  }

  template <typename Input>
  void operator()(Input* scalarsArray, vtkDoubleArray* normalizedArray)
  {
    auto scalars = vtk::DataArrayTupleRange(scalarsArray);
    auto normalized = vtk::DataArrayTupleRange(normalizedArray);

    double minValue, maxValue;
    if (this->CustomRange)
    {
      // a custom range was set, use it instead of the range computed from the data
      minValue = this->CustomRange[0];
      maxValue = this->CustomRange[1];
    }
    else
    {
      // min/max of all components in the group, per thread then combined
      vtkSMPThreadLocal<std::array<double, 2>> threadRanges(
        std::array<double, 2>{ VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX });
      vtkSMPTools::For(0, scalars.size(),
        [&](vtkIdType begin, vtkIdType end)
        {
          double chunkMin = VTK_DOUBLE_MAX;
          double chunkMax = -VTK_DOUBLE_MAX;
          for (vtkIdType i = begin; i < end; ++i)
          {
            for (int index : this->GroupIndices)
            {
              double value = static_cast<double>(scalars[i][index]);
              chunkMin = std::min(chunkMin, value);
              chunkMax = std::max(chunkMax, value);
            }
          }
          std::array<double, 2>& threadRange = threadRanges.Local();
          threadRange[0] = std::min(threadRange[0], chunkMin);
          threadRange[1] = std::max(threadRange[1], chunkMax);
        });
      minValue = VTK_DOUBLE_MAX;
      maxValue = -VTK_DOUBLE_MAX;
      for (const std::array<double, 2>& threadRange : threadRanges)
      {
        minValue = std::min(minValue, threadRange[0]);
        maxValue = std::max(maxValue, threadRange[1]);
      }
      if (this->Controller)
      {
        // one MIN reduction for both: the max is the opposite of the min of the opposites
        double local[2] = { minValue, -maxValue };
        double global[2];
        this->Controller->AllReduce(local, global, 2, vtkCommunicator::MIN_OP);
        minValue = global[0];
        maxValue = -global[1];
      }
      if (minValue > maxValue)
      {
        // no values in any process: use the range of an empty vtkDataArray
        minValue = VTK_DOUBLE_MAX;
        maxValue = VTK_DOUBLE_MIN;
      }
    }
    this->MinValue = minValue;
    this->MaxValue = maxValue;
    vtkIdType nTuples = scalars.size();
    // a group with a constant value would divide by zero, it is mapped to NewMin
    const double range = maxValue - minValue;
    const double scale = range == 0 ? 0 : (this->NewMax - this->NewMin) / range;
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          for (size_t k = 0; k < this->GroupIndices.size(); ++k)
          {
            double value = scale * (scalars[i][this->GroupIndices[k]] - minValue) + this->NewMin;
            if (this->CustomRange)
            {
              // a custom range can be narrower than the data
              value = std::min(std::max(value, this->NewMin), this->NewMax);
            }
            normalized[i][this->GroupIndices[k]] = value;
          }
        }
      });
  }
  std::vector<int> GroupIndices;
  double NewMin = 0;
  double NewMax = 1;
  vtkMultiProcessController* Controller = nullptr;
  const double* CustomRange = nullptr;
  /**
   * The global [min, max] range actually used to normalize this group,
   * i.e. CustomRange when it was set, otherwise the range computed from
   * the data (and reduced across processes if Controller is not null).
   */
  double MinValue = 0;
  double MaxValue = 0;
};

/**
 * Computes generalized barycentric coordinate (GBC) positions for each data point.
 * See: "RadViz Deluxe: An Attribute-Aware Display for Multivariate Data"
 */
struct ComputeGBCWorker
{
  ComputeGBCWorker(double verticesRotation, bool verticesClockwiseOrdering)
    : VerticesRotationRad(vtkMath::RadiansFromDegrees(verticesRotation))
    , VerticesClockwiseOrdering(verticesClockwiseOrdering)
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
    // the first vertex is at the top
    angle[0] = vtkMath::Pi() / 2;
    const double angleStep =
      (this->VerticesClockwiseOrdering ? -2 : 2) * vtkMath::Pi() / nComponents;
    constexpr double adjustment = 0.997;
    vertices[0][0] = std::cos(angle[0]) * adjustment;
    vertices[0][1] = std::sin(angle[0]) * adjustment;
    for (int i = 1; i < nComponents; ++i)
    {
      angle[i] = angle[i - 1] + angleStep;
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
      // rotate the vertices
      double result_vertices[2];
      // vtkMath::MultiplyMatrixWithVector<2, 2>(m, vertices[i], result_vertices);
      result_vertices[0] = m[0][0] * vertices[i][0] + m[0][1] * vertices[i][1];
      result_vertices[1] = m[1][0] * vertices[i][0] + m[1][1] * vertices[i][1];
      vertices[i][0] = result_vertices[0];
      vertices[i][1] = result_vertices[1];
    }
  }
  double VerticesRotationRad = 0;
  bool VerticesClockwiseOrdering = true;
  vtkSmartPointer<vtkDoubleArray> Vertices;
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

/**
 * Computes the RGB color of each point from its GBC position: the hue is
 * its angle, increasing clockwise from the top, and the saturation its
 * distance from the center, at most 1.
 */
struct GBCToRGBWorker
{
  GBCToRGBWorker(double lightness)
    : Lightness(lightness)
  {
  }

  void operator()(vtkDoubleArray* gbcScalars, vtkDoubleArray* rgbScalars)
  {
    auto gbc = vtk::DataArrayTupleRange<2>(gbcScalars);
    auto rgb = vtk::DataArrayTupleRange<3>(rgbScalars);

    vtkIdType nTuples = gbc.size();
    vtkSMPTools::For(0, nTuples,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          // negate Y so that the hue increases clockwise, starting at the top
          double x = gbc[i][0];
          double y = -gbc[i][1];
          double hue = std::atan2(y, x) + vtkMath::Pi() / 2;
          hue = numpy_mod(hue, 2 * vtkMath::Pi()) / (2 * vtkMath::Pi());
          double saturation = std::min(std::sqrt(x * x + y * y), 1.0);
          double color[3];
          hslToRgb(hue, saturation, this->Lightness, color);
          rgb[i][0] = color[0];
          rgb[i][1] = color[1];
          rgb[i][2] = color[2];
        }
      });
  }
  double Lightness = 0;
};

/**
 * Make pixels with very small average (over components) normalized
 * value transparent.
 */
struct ComputeAlphaWorker
{
  ComputeAlphaWorker() = default;
  void operator()(vtkDoubleArray* normalizedScalars, vtkDoubleArray* alphaScalars)
  {
    auto normalized = vtk::DataArrayTupleRange(normalizedScalars);
    auto alpha = vtk::DataArrayValueRange<1>(alphaScalars);

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
          alpha[i] = mean;
        }
      });
  }
};

/**
 * Copies the RGB and alpha computed before the lens into 'output' (RGB is
 * set to 0 if 'rgbArray' is null), capping the alpha of the points outside
 * the lens (inside if InvertLens is true).
 */
struct ApplyLensWorker
{
  template <typename OutputArray>
  void operator()(OutputArray* outputArray, vtkDoubleArray* rgbArray, vtkDoubleArray* alphaArray,
    vtkDoubleArray* gbcArray)
  {
    using ValueType = vtk::GetAPIType<OutputArray>;
    auto output = vtk::DataArrayTupleRange<4>(outputArray);
    const double* rgb = rgbArray ? rgbArray->GetPointer(0) : nullptr;
    auto alphaRange = vtk::DataArrayValueRange<1>(alphaArray);
    auto gbc = vtk::DataArrayTupleRange<2>(gbcArray);
    // squared distances, as the radius is not negative
    const double radius2 = this->LensRadius * this->LensRadius;
    vtkSMPTools::For(0, gbc.size(),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          for (int c = 0; c < 3; ++c)
          {
            output[i][c] = rgb ? static_cast<ValueType>(rgb[3 * i + c]) : 0;
          }
          double alpha = alphaRange[i];
          const double dx = gbc[i][0] - this->LensCenter[0];
          const double dy = gbc[i][1] - this->LensCenter[1];
          if ((dx * dx + dy * dy > radius2) != this->InvertLens)
          {
            alpha = std::min(this->OutsideLensAlpha, alpha);
          }
          output[i][3] = static_cast<ValueType>(alpha);
        }
      });
  }
  double LensCenter[2];
  double LensRadius;
  double OutsideLensAlpha;
  bool InvertLens;
};

/**
 * Creates a SOA array from a list of one component column arrays.
 */
template <typename DataType>
vtkSmartPointer<vtkDataArray> createSOAArray(const std::vector<vtkAbstractArray*>& columns)
{
  vtkAOSDataArrayTemplate<DataType>* a0 =
    vtkAOSDataArrayTemplate<DataType>::FastDownCast(columns[0]);
  if (!a0)
  {
    // the first column is not an AOS array of the expected type
    return nullptr;
  }
  vtkIdType size = a0->GetNumberOfTuples();
  auto soaArray = vtkSmartPointer<vtkSOADataArrayTemplate<DataType>>::New();
  soaArray->SetNumberOfComponents(static_cast<int>(columns.size()));
  soaArray->SetNumberOfTuples(size);
  for (size_t i = 0; i < columns.size(); ++i)
  {
    vtkAOSDataArrayTemplate<DataType>* a =
      vtkAOSDataArrayTemplate<DataType>::FastDownCast(columns[i]);
    if (!a || a->GetNumberOfComponents() != 1)
    {
      // arrays in columns are of different type or have several components
      return nullptr;
    }
    soaArray->SetArray(
      static_cast<int>(i), a->GetPointer(0), size, false /*updateMaxId*/, true /*save*/);
  }
  return soaArray;
}

/**
 * Copies a list of one component numeric column arrays, of any types, into a
 * double array with a component per column. Returns nullptr if a column is
 * not a one component numeric array.
 */
vtkSmartPointer<vtkDataArray> copyColumns(const std::vector<vtkAbstractArray*>& columns)
{
  auto result = vtkSmartPointer<vtkDoubleArray>::New();
  result->SetNumberOfComponents(static_cast<int>(columns.size()));
  result->SetNumberOfTuples(columns[0]->GetNumberOfTuples());
  for (size_t i = 0; i < columns.size(); ++i)
  {
    auto* column = vtkDataArray::FastDownCast(columns[i]);
    if (!column || column->GetNumberOfComponents() != 1 ||
      !result->CopyComponent(static_cast<int>(i), column, 0))
    {
      return nullptr;
    }
  }
  return result;
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
 * Returns unique variable names. An unnamed variable is named after its
 * index. A name already used, by a named variable or an earlier variable,
 * gets the first free " (2)", " (3)", ... suffix, so named variables keep
 * their name, and the first of several variables with the same name keeps
 * it.
 */
std::vector<std::string> uniqueVariableNames(const std::vector<const char*>& rawNames)
{
  auto isNamed = [](const char* name) { return name && *name; };
  std::set<std::string> taken;
  for (const char* name : rawNames)
  {
    if (isNamed(name))
    {
      taken.insert(name);
    }
  }
  std::set<std::string> given;
  std::vector<std::string> names;
  names.reserve(rawNames.size());
  for (size_t i = 0; i < rawNames.size(); ++i)
  {
    const char* rawName = rawNames[i];
    if (isNamed(rawName) && given.insert(rawName).second)
    {
      names.emplace_back(rawName);
      continue;
    }
    std::string base = isNamed(rawName) ? std::string(rawName) : vtk::to_string(i);
    std::string name = base;
    for (int n = 2; taken.count(name) > 0; ++n)
    {
      name = base + " (" + vtk::to_string(n) + ")";
    }
    taken.insert(name);
    names.push_back(std::move(name));
  }
  return names;
}

std::vector<std::string> columnNames(vtkTable* table)
{
  std::vector<const char*> rawNames;
  rawNames.reserve(table->GetNumberOfColumns());
  for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    rawNames.push_back(table->GetColumnName(i));
  }
  return uniqueVariableNames(rawNames);
}

std::vector<std::string> componentNames(vtkDataArray* array)
{
  std::vector<const char*> rawNames;
  rawNames.reserve(array->GetNumberOfComponents());
  for (int i = 0; i < array->GetNumberOfComponents(); ++i)
  {
    rawNames.push_back(array->GetComponentName(i));
  }
  return uniqueVariableNames(rawNames);
}

/**
 * Read-only view over a subset of components of a vtkDataArray, used to
 * expose only the components marked visible via
 * vtkMultiVariateToRGBA::SetVariableVisibility without copying the underlying
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
  }
  return result;
}

/**
 * Returns true for a table without rows or a dataset without points and
 * cells, such as the pieces of a serial reader on all processes but one.
 */
bool IsEmptyPiece(vtkDataObject* data)
{
  if (vtkTable* table = vtkTable::SafeDownCast(data))
  {
    return table->GetNumberOfRows() == 0;
  }
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(data);
  return dataset && dataset->GetNumberOfPoints() == 0 && dataset->GetNumberOfCells() == 0;
}

/**
 * Returns the row data of a table, or the cell or point data of a dataset
 * for 'association'. Any association other than cells, including
 * POINTS_THEN_CELLS, gives the point data.
 */
vtkDataSetAttributes* GetAttributesForAssociation(vtkDataObject* data, int association)
{
  if (vtkTable* table = vtkTable::SafeDownCast(data))
  {
    return table->GetRowData();
  }
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(data);
  return association == vtkDataObject::FIELD_ASSOCIATION_CELLS
    ? static_cast<vtkDataSetAttributes*>(dataset->GetCellData())
    : static_cast<vtkDataSetAttributes*>(dataset->GetPointData());
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
void vtkMultiVariateToRGBA::Modified()
{
  if (!this->InLensModified)
  {
    this->NonLensModifiedTime.Modified();
  }
  // cleared before ModifiedEvent, in case an observer changes something else
  this->InLensModified = false;
  this->Superclass::Modified();
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::LensModified()
{
  this->InLensModified = true;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetOutsideLensAlpha(double alpha)
{
  if (this->OutsideLensAlpha != alpha)
  {
    this->OutsideLensAlpha = alpha;
    this->LensModified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetInvertLens(bool invert)
{
  if (this->InvertLens != invert)
  {
    this->InvertLens = invert;
    this->LensModified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVerticesRotation(double rotation)
{
  vtkDebugMacro(<< " setting VerticesRotation to " << rotation);
  rotation = numpy_mod(rotation, 360);
  // a tiny negative rotation rounds to 360
  if (rotation == 360)
  {
    rotation = 0;
  }
  if (this->VerticesRotation != rotation)
  {
    this->VerticesRotation = rotation;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetCustomRange(const std::vector<double>& customRange)
{
  vtkDebugMacro(<< " setting CustomRange to " << fmt::format("{}", customRange));
  if (this->CustomRange != customRange)
  {
    this->CustomRange = customRange;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<double> vtkMultiVariateToRGBA::GetCustomRange() VTK_FUTURE_CONST
{
  return this->CustomRange;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::ClearCustomRange()
{
  if (!this->CustomRange.empty())
  {
    this->CustomRange.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::AddCustomRange(double min, double max)
{
  this->CustomRange.push_back(min);
  this->CustomRange.push_back(max);
  this->Modified();
}

//------------------------------------------------------------------------------
std::vector<double> vtkMultiVariateToRGBA::GetRange() VTK_FUTURE_CONST
{
  return this->Range;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetPartitionIndices(const std::vector<int>& partitionIndices)
{
  vtkDebugMacro(<< " setting PartitionIndices to " << fmt::format("{}", partitionIndices));
  if (this->PartitionIndices != partitionIndices)
  {
    this->PartitionIndices = partitionIndices;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<int> vtkMultiVariateToRGBA::GetPartitionIndices() VTK_FUTURE_CONST
{
  return this->PartitionIndices;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetPartitionOffsets(const std::vector<int>& partitionOffsets)
{
  vtkDebugMacro(<< " setting PartitionOffsets to " << fmt::format("{}", partitionOffsets));
  if (this->PartitionOffsets != partitionOffsets)
  {
    this->PartitionOffsets = partitionOffsets;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<int> vtkMultiVariateToRGBA::GetPartitionOffsets() VTK_FUTURE_CONST
{
  return this->PartitionOffsets;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::ClearPartitionIndices()
{
  if (!this->PartitionIndices.empty())
  {
    this->PartitionIndices.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::AddPartitionIndex(int index)
{
  this->PartitionIndices.push_back(index);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::ClearPartitionOffsets()
{
  if (!this->PartitionOffsets.empty())
  {
    this->PartitionOffsets.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::AddPartitionOffset(int offset)
{
  this->PartitionOffsets.push_back(offset);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkStringArray> vtkMultiVariateToRGBA::GetAllVariableNames()
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
    for (const std::string& name : ::columnNames(table))
    {
      names->InsertNextValue(name);
    }
  }
  else if (dataset)
  {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS;
    vtkDataArray* array = this->GetInputArrayToProcess(0, input, association);
    if (array)
    {
      for (const std::string& name : ::componentNames(array))
      {
        names->InsertNextValue(name);
      }
    }
  }
  return names;
}

//------------------------------------------------------------------------------
bool vtkMultiVariateToRGBA::IsVariableVisible(
  int index, const std::string& name, bool numeric) const
{
  // at most one of the exceptions is not empty, and an exception flips the default
  bool exception = this->NameVisibilityExceptions.count(name) > 0 ||
    this->IndexVisibilityExceptions.count(index) > 0;
  return numeric && exception != this->VariablesVisibleByDefault;
}

//------------------------------------------------------------------------------
bool vtkMultiVariateToRGBA::IsNumericVariable(int index)
{
  vtkTable* table = vtkTable::SafeDownCast(this->GetInputDataObject(0, 0));
  return !table || vtkDataArray::SafeDownCast(table->GetColumn(index)) != nullptr;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVariableVisibility(const std::string& name, bool visible)
{
  this->StartVisibleIndexList = false;
  if (!this->IndexVisibilityExceptions.empty())
  {
    vtkErrorMacro("The visibility is set by index, start over before setting it by name");
    return;
  }
  bool changed = visible == this->VariablesVisibleByDefault
    ? this->NameVisibilityExceptions.erase(name) > 0
    : this->NameVisibilityExceptions.insert(name).second;
  if (changed)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVariableVisibility(int index, bool visible)
{
  this->StartVisibleIndexList = false;
  if (index < 0)
  {
    vtkErrorMacro("Invalid variable index: " << index);
    return;
  }
  if (!this->NameVisibilityExceptions.empty())
  {
    vtkErrorMacro("The visibility is set by name, start over before setting it by index");
    return;
  }
  bool changed = visible == this->VariablesVisibleByDefault
    ? this->IndexVisibilityExceptions.erase(index) > 0
    : this->IndexVisibilityExceptions.insert(index).second;
  if (changed)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVariableVisibilityAll(bool visible)
{
  this->StartVisibleIndexList = false;
  if (this->VariablesVisibleByDefault != visible || !this->NameVisibilityExceptions.empty() ||
    !this->IndexVisibilityExceptions.empty())
  {
    this->VariablesVisibleByDefault = visible;
    this->NameVisibilityExceptions.clear();
    this->IndexVisibilityExceptions.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkMultiVariateToRGBA::GetVariableVisibility(const std::string& name)
{
  vtkSmartPointer<vtkStringArray> names = this->GetAllVariableNames();
  vtkIdType index = names->LookupValue(name);
  // a variable not in the current input is not visible
  return index >= 0 &&
    this->IsVariableVisible(
      static_cast<int>(index), name, this->IsNumericVariable(static_cast<int>(index)));
}

//------------------------------------------------------------------------------
bool vtkMultiVariateToRGBA::GetVariableVisibility(int index)
{
  vtkSmartPointer<vtkStringArray> names = this->GetAllVariableNames();
  return index >= 0 && index < names->GetNumberOfValues() &&
    this->IsVariableVisible(index, names->GetValue(index), this->IsNumericVariable(index));
}

//------------------------------------------------------------------------------
vtkStringArray* vtkMultiVariateToRGBA::GetVisibleVariables()
{
  vtkSmartPointer<vtkStringArray> names = this->GetAllVariableNames();
  // refilled, not replaced, so pointers returned before stay valid
  if (!this->VisibleVariablesCache)
  {
    this->VisibleVariablesCache = vtkSmartPointer<vtkStringArray>::New();
  }
  this->VisibleVariablesCache->Initialize();
  for (vtkIdType i = 0; i < names->GetNumberOfValues(); ++i)
  {
    if (this->IsVariableVisible(
          static_cast<int>(i), names->GetValue(i), this->IsNumericVariable(static_cast<int>(i))))
    {
      this->VisibleVariablesCache->InsertNextValue(names->GetValue(i));
    }
  }
  return this->VisibleVariablesCache;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVisibleVariables(vtkStringArray* names)
{
  this->StartVisibleIndexList = false;
  std::set<std::string> visible;
  if (names)
  {
    for (vtkIdType i = 0; i < names->GetNumberOfValues(); ++i)
    {
      visible.insert(names->GetValue(i));
    }
  }
  if (this->VariablesVisibleByDefault || !this->IndexVisibilityExceptions.empty() ||
    this->NameVisibilityExceptions != visible)
  {
    this->VariablesVisibleByDefault = false;
    this->NameVisibilityExceptions = std::move(visible);
    this->IndexVisibilityExceptions.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetVisibleVariableIndices(const std::vector<int>& indices)
{
  this->StartVisibleIndexList = false;
  if (std::any_of(indices.begin(), indices.end(), [](int index) { return index < 0; }))
  {
    vtkErrorMacro("Invalid variable indices: " << fmt::format("{}", fmt::join(indices, ", ")));
    return;
  }
  std::set<int> visible(indices.begin(), indices.end());
  if (this->VariablesVisibleByDefault || !this->NameVisibilityExceptions.empty() ||
    this->IndexVisibilityExceptions != visible)
  {
    this->VariablesVisibleByDefault = false;
    this->NameVisibilityExceptions.clear();
    this->IndexVisibilityExceptions = std::move(visible);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<int> vtkMultiVariateToRGBA::GetVisibleVariableIndices()
{
  vtkSmartPointer<vtkStringArray> names = this->GetAllVariableNames();
  std::vector<int> indices;
  for (vtkIdType i = 0; i < names->GetNumberOfValues(); ++i)
  {
    if (this->IsVariableVisible(
          static_cast<int>(i), names->GetValue(i), this->IsNumericVariable(static_cast<int>(i))))
    {
      indices.push_back(static_cast<int>(i));
    }
  }
  return indices;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::ResetVisibleVariableIndices()
{
  this->SetVariableVisibilityAll(true);
  this->StartVisibleIndexList = true;
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::AddVisibleVariableIndex(int index)
{
  if (index < 0)
  {
    vtkErrorMacro("Invalid variable index: " << index);
    return;
  }
  if (this->StartVisibleIndexList)
  {
    // every variable is visible: the first index added hides the others
    this->VariablesVisibleByDefault = false;
  }
  this->SetVariableVisibility(index, true);
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkMultiVariateToRGBA::GetNameVisibilityExceptions() VTK_FUTURE_CONST
{
  return { this->NameVisibilityExceptions.begin(), this->NameVisibilityExceptions.end() };
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetNameVisibilityExceptions(const std::vector<std::string>& names)
{
  std::set<std::string> exceptions(names.begin(), names.end());
  if (this->NameVisibilityExceptions != exceptions)
  {
    this->NameVisibilityExceptions = std::move(exceptions);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::vector<int> vtkMultiVariateToRGBA::GetIndexVisibilityExceptions() VTK_FUTURE_CONST
{
  return { this->IndexVisibilityExceptions.begin(), this->IndexVisibilityExceptions.end() };
}

//------------------------------------------------------------------------------
void vtkMultiVariateToRGBA::SetIndexVisibilityExceptions(const std::vector<int>& indices)
{
  std::set<int> exceptions(indices.begin(), indices.end());
  if (this->IndexVisibilityExceptions != exceptions)
  {
    this->IndexVisibilityExceptions = std::move(exceptions);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkMultiVariateToRGBA::GetVisibleComponentsArray(vtkDataArray* array)
{
  std::vector<int> componentIndices;
  std::vector<std::string> names = ::componentNames(array);
  for (int c = 0; c < array->GetNumberOfComponents(); ++c)
  {
    if (this->IsVariableVisible(c, names[c]))
    {
      componentIndices.push_back(c);
    }
  }
  if (componentIndices.empty())
  {
    return nullptr;
  }

  // Fast path: every component is visible (the indices increase), so use the
  // array directly and vtkArrayDispatch can still fast-path on its type.
  if (static_cast<int>(componentIndices.size()) == array->GetNumberOfComponents())
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
  return implicitArray;
}

//------------------------------------------------------------------------------
int vtkMultiVariateToRGBA::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // no input connection: the pipeline reports it
  vtkDataObject* inData = inInfo ? inInfo->Get(vtkDataObject::DATA_OBJECT()) : nullptr;
  if (!inData)
  {
    return 0;
  }
  vtkDataObject* outData = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!vtkTable::SafeDownCast(inData) && !vtkDataSet::SafeDownCast(inData))
  {
    vtkErrorMacro("Input type not supported: " << inData->GetClassName());
    // empty the previous output on error
    if (outData)
    {
      outData->Initialize();
    }
    return 0;
  }
  // keep the output across executions, e.g. lens-only changes
  if (!outData || !outData->IsA(inData->GetClassName()))
  {
    auto newData = vtkSmartPointer<vtkDataObject>::Take(inData->NewInstance());
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newData);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkMultiVariateToRGBA::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->StoreRGBA && this->StoreA)
  {
    vtkErrorMacro("You cannot have both StoreRGBA and StoreA set to true");
    return 0;
  }
  if (!this->StoreGBC && !this->StoreRGBA && !this->StoreA)
  {
    vtkErrorMacro("You cannot have StoreGBC, StoreRGBA and StoreA all set to false");
    return 0;
  }
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* outData = outInfo->Get(vtkDataObject::DATA_OBJECT());
  outData->ShallowCopy(inData);
  bool parallel = this->Controller && this->Controller->GetNumberOfProcesses() > 1;
  // Only the lens changed since the last execution: reuse its results,
  // without looking up the scalars again.
  int useCache = (this->StoreRGBA || this->StoreA) && this->CachedAlpha &&
    this->CachedInput == inData && this->NonLensModifiedTime < this->CacheTime &&
    inData->GetMTime() < this->CacheTime.GetMTime();
  if (parallel)
  {
    // every process must take the same path: the full one has collectives
    int localUseCache = useCache;
    this->Controller->AllReduce(&localUseCache, &useCache, 1, vtkCommunicator::MIN_OP);
  }
  if (useCache)
  {
    vtkDataSetAttributes* attributes =
      GetAttributesForAssociation(outData, this->CachedAssociation);
    if (this->StoreVertices)
    {
      outData->GetFieldData()->AddArray(this->CachedVertices);
    }
    if (this->StoreGBC)
    {
      attributes->AddArray(this->CachedGBC);
    }
    attributes->AddArray(this->ApplyLens());
    return 1;
  }
  bool emptyPiece = parallel && IsEmptyPiece(outData);
  vtkTable* table = vtkTable::SafeDownCast(outData);
  // an empty piece looks up its own scalars if it has them, so all pieces can
  // be empty; otherwise (e.g. a serial reader) the others decide the number
  // of components and the association
  bool ownScalars = !emptyPiece ||
    (table ? table->GetNumberOfColumns() > 0
           : this->GetInputArrayToProcess(0, inputVector) != nullptr);
  vtkDataSetAttributes* attributes = nullptr;
  vtkSmartPointer<vtkDataArray> scalars;
  if (ownScalars)
  {
    std::tie(attributes, scalars) = GetAttributesAndScalars(outData, inputVector);
  }
  if (parallel)
  {
    // all processes return together, or all take part in the collectives below
    int numComponents = scalars ? scalars->GetNumberOfComponents() : 0;
    int canContinue = scalars || emptyPiece;
    // empty pieces send the identity of each reduction
    int minComponents = scalars ? numComponents : VTK_INT_MAX;
    // one MIN reduction for both: for 0/1 flags, MIN is a logical AND
    int localMin[2] = { canContinue, minComponents };
    int globalMin[2];
    this->Controller->AllReduce(localMin, globalMin, 2, vtkCommunicator::MIN_OP);
    canContinue = globalMin[0];
    minComponents = globalMin[1];
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(outData);
    int cellScalars = scalars && dataset && attributes == dataset->GetCellData();
    // one MAX reduction for both: for 0/1 flags, MAX is a logical OR
    int localMax[2] = { numComponents, cellScalars };
    int globalMax[2];
    this->Controller->AllReduce(localMax, globalMax, 2, vtkCommunicator::MAX_OP);
    int maxComponents = globalMax[0];
    cellScalars = globalMax[1];
    if (!canContinue)
    {
      return 0;
    }
    if (maxComponents == 0)
    {
      vtkErrorMacro("No scalar data to process on any process");
      return 0;
    }
    if (minComponents != maxComponents)
    {
      vtkErrorMacro("Processes have scalars with different numbers of components: "
        << minComponents << " and " << maxComponents);
      return 0;
    }
    if (!scalars)
    {
      scalars = vtkSmartPointer<vtkDoubleArray>::New();
      scalars->SetNumberOfComponents(maxComponents);
      // put the output arrays where the other processes do
      attributes = GetAttributesForAssociation(outData,
        cellScalars ? vtkDataObject::FIELD_ASSOCIATION_CELLS
                    : vtkDataObject::FIELD_ASSOCIATION_POINTS);
    }
  }
  else if (!scalars)
  {
    return 0;
  }
  int numComponents = scalars->GetNumberOfComponents();
  // Use local copies: the default partition depends on the current number of
  // components and RequestData should not modify filter state.
  std::vector<int> partition = this->PartitionIndices;
  std::vector<int> partitionOffsets = this->PartitionOffsets;
  if (partition.empty() && partitionOffsets.empty())
  {
    // add one group with all variables
    partition.resize(numComponents);
    std::iota(partition.begin(), partition.end(), 0);
    partitionOffsets.assign(1, numComponents);
  }
  else
  {
    // make sure the partition is well formed
    std::vector<int> indices = partition;
    std::sort(indices.begin(), indices.end());
    std::vector<int> expected(numComponents);
    std::iota(expected.begin(), expected.end(), 0);
    // strictly increasing and positive: a group cannot be empty and cannot
    // index outside the partition
    bool validOffsets = !partitionOffsets.empty() && partitionOffsets[0] > 0 &&
      partitionOffsets.back() == numComponents &&
      std::adjacent_find(partitionOffsets.begin(), partitionOffsets.end(),
        [](int previous, int current) { return previous >= current; }) == partitionOffsets.end();
    if (indices != expected || !validOffsets)
    {
      vtkErrorMacro(<< fmt::format("Invalid partition {} with offsets {}: every component index "
                                   "in [0, {}) must appear exactly once and PartitionOffsets must "
                                   "be strictly increasing, positive and end at {}",
                      partition, partitionOffsets, numComponents, numComponents));
      return 0;
    }
  }
  if (!this->CustomRange.empty() && this->CustomRange.size() != 2 * partitionOffsets.size())
  {
    vtkErrorMacro(
      << fmt::format(
           "Invalid CustomRange with {} elements: it must be empty or hold exactly one "
           "[min, max] pair (2 elements) per partition group ({} groups, {} elements expected)",
           this->CustomRange.size(), partitionOffsets.size(), 2 * partitionOffsets.size()));
    return 0;
  }
  for (size_t i = 0; i < this->CustomRange.size(); i += 2)
  {
    if (this->CustomRange[i] > this->CustomRange[i + 1])
    {
      vtkErrorMacro(<< fmt::format("Invalid CustomRange for group {}: min {} is greater "
                                   "than max {}",
                      i / 2, this->CustomRange[i], this->CustomRange[i + 1]));
      return 0;
    }
  }

  vtkNew<vtkDoubleArray> normalizedScalars;
  normalizedScalars->SetNumberOfComponents(scalars->GetNumberOfComponents());
  normalizedScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  // the input scalars can be of any numeric type; the intermediate arrays created by this
  // filter are always vtkDoubleArray, so only the input array needs to be dispatched.
  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> InputDispatcher;
  std::vector<double> range(2 * partitionOffsets.size());
  for (size_t i = 0, offset = 0; i < partitionOffsets.size(); offset = partitionOffsets[i], ++i)
  {
    std::vector<int> groupIndices(
      partition.begin() + offset, partition.begin() + partitionOffsets[i]);
    const double* customRange = this->CustomRange.empty() ? nullptr : &this->CustomRange[2 * i];
    NormalizeWorker worker(groupIndices, this->Controller, customRange);
    if (!InputDispatcher::Execute(scalars, worker, normalizedScalars.GetPointer()))
    {
      worker(scalars.GetPointer(), normalizedScalars.GetPointer());
    }
    range[2 * i] = worker.MinValue;
    range[2 * i + 1] = worker.MaxValue;
  }
  this->Range = range;
  vtkNew<vtkDoubleArray> gbcScalars;
  gbcScalars->SetName("gbc");
  gbcScalars->SetNumberOfComponents(2);
  gbcScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
  {
    ComputeGBCWorker worker(this->VerticesRotation, this->VerticesClockwiseOrdering);
    worker(normalizedScalars.GetPointer(), gbcScalars.GetPointer());
    if (this->StoreVertices)
    {
      outData->GetFieldData()->AddArray(worker.Vertices);
    }
    this->CachedVertices = worker.Vertices;
  }
  if (this->StoreGBC)
  {
    attributes->AddArray(gbcScalars);
  }
  if (this->StoreRGBA || this->StoreA)
  {
    vtkSmartPointer<vtkDoubleArray> rgbScalars;
    if (this->StoreRGBA)
    {
      rgbScalars = vtkSmartPointer<vtkDoubleArray>::New();
      rgbScalars->SetNumberOfComponents(3);
      rgbScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
      GBCToRGBWorker worker(vtkMultiVariateToRGBA::ColorLightness);
      worker(gbcScalars.GetPointer(), rgbScalars);
    }
    // StoreA: only alpha is computed, ApplyLens sets RGB to 0.
    vtkNew<vtkDoubleArray> alphaScalars;
    alphaScalars->SetNumberOfTuples(scalars->GetNumberOfTuples());
    {
      ComputeAlphaWorker worker;
      worker(normalizedScalars.GetPointer(), alphaScalars.GetPointer());
    }
    this->CachedGBC = gbcScalars.GetPointer();
    this->CachedRGB = rgbScalars;
    this->CachedAlpha = alphaScalars.GetPointer();
    this->CachedInput = inData;
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(outData);
    this->CachedAssociation = dataset && attributes == dataset->GetCellData()
      ? vtkDataObject::FIELD_ASSOCIATION_CELLS
      : vtkDataObject::FIELD_ASSOCIATION_POINTS;
    this->CacheTime.Modified();
    attributes->AddArray(this->ApplyLens());
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkMultiVariateToRGBA::ApplyLens()
{
  auto rgba = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(this->OutputScalarType));
  rgba->SetName("rgba");
  rgba->SetNumberOfComponents(4);
  rgba->SetNumberOfTuples(this->CachedAlpha->GetNumberOfTuples());
  // clamped here, not by the setters, so the order of the setters does not matter
  std::array<double, 2> center = vtkMultiVariateToRGBA::GetLensCenter(
    this->LensRadius, 1, this->LensCenter, std::array<double, 2>{ 0, 0 }.data());
  ApplyLensWorker worker{ { center[0], center[1] }, this->LensRadius, this->OutsideLensAlpha,
    this->InvertLens };
  vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>::Execute(rgba.GetPointer(), worker,
    this->CachedRGB.GetPointer(), this->CachedAlpha.GetPointer(), this->CachedGBC.GetPointer());
  return rgba;
}

//------------------------------------------------------------------------------
std::pair<vtkDataSetAttributes*, vtkSmartPointer<vtkDataArray>>
vtkMultiVariateToRGBA::GetAttributesAndScalars(
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
      vtkErrorMacro("No arrays found");
      return { attributes, nullptr };
    }

    // Only keep the visible columns, in their natural order.
    std::vector<vtkAbstractArray*> columns;
    std::vector<std::string> names = ::columnNames(table);
    for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
      vtkAbstractArray* column = table->GetColumn(i);
      if (this->IsVariableVisible(
            static_cast<int>(i), names[i], vtkDataArray::SafeDownCast(column) != nullptr))
      {
        if (column->GetNumberOfComponents() != 1)
        {
          vtkErrorMacro("Column has several components: " << names[i]);
          return { attributes, nullptr };
        }
        columns.push_back(column);
      }
    }
    if (columns.empty())
    {
      vtkErrorMacro("No visible columns found");
      return { attributes, nullptr };
    }
    if (columns.size() < 3)
    {
      vtkErrorMacro("Number of visible columns is less than 3: " << columns.size());
      return { attributes, nullptr };
    }
    // convert the columns into a SOA array with a component per column
    typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> Dispatcher;
    CreateSOAWorker worker(columns);
    if (Dispatcher::Execute(vtkDataArray::FastDownCast(columns[0]), worker) && worker.SOAArray)
    {
      return { attributes, worker.SOAArray };
    }
    // columns of different types, or not AOS: copy them
    vtkSmartPointer<vtkDataArray> copy = copyColumns(columns);
    if (!copy)
    {
      vtkErrorMacro("Cannot copy the columns");
    }
    return { attributes, copy };
  }
  else if (dataset)
  {
    int association = vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS;
    vtkDataArray* array = this->GetInputArrayToProcess(0, inputVector, association);
    if (array)
    {
      // the array was found, so POINTS_THEN_CELLS is resolved to one of these
      if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        association != vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
        vtkErrorMacro("Invalid association: " << association);
        return { nullptr, nullptr };
      }
      attributes = GetAttributesForAssociation(dataset, association);
      // Only keep the visible components, without copying the data.
      vtkSmartPointer<vtkDataArray> visibleArray = this->GetVisibleComponentsArray(array);
      if (!visibleArray)
      {
        vtkErrorMacro("No visible components found");
        return { attributes, nullptr };
      }
      if (visibleArray->GetNumberOfComponents() < 3)
      {
        vtkErrorMacro(
          "Number of visible components is less than 3: " << visibleArray->GetNumberOfComponents());
        return { attributes, nullptr };
      }
      return { attributes, visibleArray };
    }
    else
    {
      vtkErrorMacro("No scalar data to process");
      return { nullptr, nullptr };
    }
  }
  vtkErrorMacro("Invalid input type: " << input->GetClassName());
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
  if (lensRadius < 0)
  {
    vtkErrorMacro("Invalid lens radius: " << lensRadius << ", it must be >= 0");
    return;
  }
  if (lensRadius != this->LensRadius)
  {
    this->LensRadius = lensRadius;
    this->LensModified();
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
  if (this->LensCenter[0] != lensCenter[0] || this->LensCenter[1] != lensCenter[1])
  {
    std::copy(lensCenter, lensCenter + 2, this->LensCenter);
    this->LensModified();
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
  os << indent << "Controller: " << this->Controller << "\n";
  os << indent << "VerticesRotation: " << this->VerticesRotation << "\n";
  os << indent << "VerticesClockwiseOrdering: " << this->VerticesClockwiseOrdering << "\n";
  os << indent << "StoreGBC: " << this->StoreGBC << "\n";
  os << indent << "StoreVertices: " << this->StoreVertices << "\n";
  os << indent << "StoreRGBA: " << this->StoreRGBA << "\n";
  os << indent << "StoreA: " << this->StoreA << "\n";
  os << indent << "PartitionIndices: " << fmt::format("{}", this->PartitionIndices) << "\n";
  os << indent << "PartitionOffsets: " << fmt::format("{}", this->PartitionOffsets) << "\n";
  os << indent << "CustomRange: " << fmt::format("{}", this->CustomRange) << "\n";
  os << indent << "Range: " << fmt::format("{}", this->Range) << "\n";
  os << indent << "LensCenter: (" << this->LensCenter[0] << ", " << this->LensCenter[1] << ")\n";
  os << indent << "LensRadius: " << this->LensRadius << "\n";
  os << indent << "OutsideLensAlpha: " << this->OutsideLensAlpha << "\n";
  os << indent << "InvertLens: " << this->InvertLens << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "VariablesVisibleByDefault: " << this->VariablesVisibleByDefault << "\n";
  os << indent << "NameVisibilityExceptions: " << fmt::format("{}", this->NameVisibilityExceptions)
     << "\n";
  os << indent
     << "IndexVisibilityExceptions: " << fmt::format("{}", this->IndexVisibilityExceptions) << "\n";
}
VTK_ABI_NAMESPACE_END
