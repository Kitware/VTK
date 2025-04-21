// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"

#include "vtkAbstractArray.h"
#include "vtkAbstractPointLocator.h"
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataAssembly.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkExtractEdges.h"
#include "vtkFieldData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCellCenters.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrixUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkQuaternion.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticPointLocator.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkTypeName.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVariantArray.h"

#include <array>
#include <atomic>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//============================================================================
struct IdentityMapper
{
  IdentityMapper(vtkDataSet* ds)
    : Size(ds->GetNumberOfPoints())
  {
  }

  IdentityMapper(vtkIdType size)
    : Size(size)
  {
  }

  static vtkIdType Map(vtkIdType id) { return id; }

  bool Success = true;
  vtkIdType Size;
};

//============================================================================
struct DummyMapper
{
  DummyMapper(vtkDataObject*) {}

  bool Success = true;
};

//============================================================================
// Traits define over vtkDataObject subclasses to do trait-based dispatching
template <class DataObjectT>
struct DataObjectTraits;

//============================================================================
template <>
struct DataObjectTraits<vtkPointSet>
{
  static constexpr bool IsStructured = false;
  static constexpr bool HasPoints = true;
  static constexpr bool HasCells = true;
  using CellCenters = vtkCellCenters;
};

//============================================================================
template <>
struct DataObjectTraits<vtkImageData>
{
  static constexpr bool IsStructured = true;
  static constexpr bool HasPoints = true;
  static constexpr bool HasCells = true;
  using BaseMapper = IdentityMapper;
};

//============================================================================
template <>
struct DataObjectTraits<vtkRectilinearGrid>
{
  static constexpr bool IsStructured = true;
  static constexpr bool IsRectilinear = true;
  static constexpr bool HasPoints = true;
  static constexpr bool HasCells = true;
  using BaseMapper = IdentityMapper;
};

//============================================================================
template <>
struct DataObjectTraits<vtkStructuredGrid>
{
  static constexpr bool IsStructured = true;
  static constexpr bool HasPoints = true;
  static constexpr bool HasCells = true;
  using BaseMapper = IdentityMapper;
};

//============================================================================
template <>
struct DataObjectTraits<vtkExplicitStructuredGrid>
{
  static constexpr bool IsStructured = true;
  static constexpr bool HasPoints = true;
  static constexpr bool HasCells = true;
  using BaseMapper = IdentityMapper;
};

//============================================================================
template <>
struct DataObjectTraits<vtkHyperTreeGrid>
{
  static constexpr bool IsStructured = false;
  static constexpr bool HasPoints = false;
  static constexpr bool HasCells = true;
  using BaseMapper = DummyMapper;
  using CellCenters = vtkHyperTreeGridCellCenters;
};

//============================================================================
template <>
struct DataObjectTraits<vtkTable>
{
  static constexpr bool HasPoints = false;
  static constexpr bool HasCells = false;
};

//============================================================================
template <int N, class ValueT>
bool RealVectorsNearlyEqualImpl(ValueT norm2, ValueT dot, double toleranceFactor)
{
  // See VectorsComparator<true> for justification over the formula
  return norm2 - 2 * dot <= toleranceFactor * norm2 * std::numeric_limits<ValueT>::epsilon();
}

//============================================================================
template <bool FloatingPointT>
struct VectorsComparator;

//============================================================================
template <>
struct VectorsComparator<true /* FloatingPointT */>
{
  template <int N, class VectorT1, class VectorT2>
  static bool NearlyEqual(VectorT1&& u, VectorT2&& v, double toleranceFactor)
  {
    using ValueType = typename vtkMatrixUtilities::ScalarTypeExtractor<VectorT1>::value_type;

    // Doing some math here given 2 vectors u and v:
    // u == v <=> ||u - v||^2 = 0
    //        <=> ||u||^2 - 2 <u, v> + ||v||^2 = 0
    //        <=> (||u||^2 + ||v||^2) - 2 <u, v> = 0
    // (||u||^2 + ||v||^2) >= 2 <u, v> because ||u - v||^2 >= 0 by construction.
    // So we can use epsilon on our squared norm sum to account for rounding error.
    //
    // Hence, for each vector pair, we need to test:
    // (||u||^2 + ||v||^2) - 2 <u, v> < toleranceFactor * (||u||^2 + ||v||^2) * epsilon
    //
    // Note the absence of absolute value. We have positiveness guarantees because of the
    // positiveness of the norm.
    //
    // This method has the advantage of not relying on max or abs, which avoids using conditionals
    // in the formulation.

    ValueType squaredNormSum = vtkMath::SquaredNorm<N>(std::forward<VectorT1>(u)) +
      vtkMath::SquaredNorm<N>(std::forward<VectorT2>(v));
    ValueType dot =
      vtkMath::Dot<ValueType, N>(std::forward<VectorT1>(u), std::forward<VectorT2>(v));

    return RealVectorsNearlyEqualImpl<N>(squaredNormSum, dot, toleranceFactor);
  }
};

//============================================================================
template <class ValueType, int N>
struct VectorDiff
{
  template <class VectorT1, class VectorT2>
  VectorDiff(VectorT1&& u, VectorT2&& v)
  {
    for (int i = 0; i < N; ++i)
    {
      this->Diff[i] = u[i] - v[i];
    }
  }

  std::array<ValueType, N> Diff;
};

//============================================================================
template <class ValueType>
struct VectorDiff<ValueType, 0>
{
  template <class VectorT1, class VectorT2>
  VectorDiff(VectorT1&& u, VectorT2&& v)
  {
    this->Diff.resize(u.size());
    for (int i = 0; i < static_cast<int>(this->Diff.size()); ++i)
    {
      this->Diff[i] = u[i] - v[i];
    }
  }

  std::vector<ValueType> Diff;
};

//============================================================================
template <>
struct VectorsComparator<false /* FloatingPointT */>
{
  template <int N, class VectorT1, class VectorT2>
  static bool NearlyEqual(VectorT1&& u, VectorT2&& v, double)
  {
    using ValueType = typename vtkMatrixUtilities::ScalarTypeExtractor<VectorT1>::value_type;

    // We do not want to overflow the integer, so we directly test ||u - v||^2 == 0
    return vtkMath::SquaredNorm<N>(std::move(
             VectorDiff<ValueType, N>(std::forward<VectorT1>(u), std::forward<VectorT2>(v))
               .Diff)) == 0;
  }
};

//----------------------------------------------------------------------------
template <int N, class VectorT1, class VectorT2>
bool VectorsAreNearlyEqual(VectorT1&& u, VectorT2&& v, double toleranceFactor)
{
  using ValueType = typename vtkMatrixUtilities::ScalarTypeExtractor<VectorT1>::value_type;

  return VectorsComparator<std::is_floating_point<ValueType>::value>::template NearlyEqual<N>(
    std::forward<VectorT1>(u), std::forward<VectorT2>(v), toleranceFactor);
}

//============================================================================
// This class is used to provide access to wanted information when testing tuples between 2 arrays.
// One can custom design when to stop iterating over the array with Aborter and set a state
// to variables accessible to the user with Decider
template <class DeciderT, class AborterT>
struct VectorMatchingProcessor
{
  VectorMatchingProcessor(double toleranceFactor, DeciderT& decider, AborterT& aborter)
    : ToleranceFactor(toleranceFactor)
    , Decider(decider)
    , Aborter(aborter)
  {
  }

  template <int N, class ArrayT1, class ArrayT2>
  void Execute(vtkIdType id1, const ArrayT1& array1, vtkIdType id2, const ArrayT2& array2)
  {
    static_assert(N >= 0, "N cannot be negative when taking this path.");

    using ConstTupleRef1 = typename ArrayT1::ConstTupleReferenceType;
    using ConstTupleRef2 = typename ArrayT2::ConstTupleReferenceType;

    ConstTupleRef1 u = array1[id1];
    ConstTupleRef2 v = array2[id1];

    this->Decider(VectorsAreNearlyEqual<N>(u, v, this->ToleranceFactor), id1, id2);
  }

  template <int, class ArrayT>
  void Execute(vtkIdType id1, ArrayT* array1, vtkIdType id2, ArrayT* array2)
  {
    int ncomps = array1->GetNumberOfComponents();
    for (int comp = 0; comp < ncomps; ++comp)
    {
      this->Decider(
        array1->GetValue(id1 * ncomps + comp) == array2->GetValue(id2 * ncomps + comp), id1, id2);
    }
  }

  bool GetAbort() const { return this->Aborter(); }

  double ToleranceFactor;

  /**
   * Interface between variables accessible to the user and the process of testing 2 tuples
   */
  DeciderT& Decider;

  /**
   * This tells when to stop iterating over tuples. It should typically be linked to the state
   * set by Decider.
   */
  AborterT& Aborter;
};

//----------------------------------------------------------------------------
template <class DeciderT, class AborterT>
VectorMatchingProcessor<DeciderT&, AborterT&> MakeVectorMatchingProcessor(
  double toleranceFactor, DeciderT& decider, AborterT& aborter)
{
  return VectorMatchingProcessor<DeciderT&, AborterT&>(toleranceFactor, decider, aborter);
}

//============================================================================
template <int N, class ArrayT1, class ArrayT2, class ArrayMapperT1, class ArrayMapperT2,
  class ProcessorT>
struct GenericArrayWorker
{
  template <class ArrayTT1, class ArrayTT2, class ProcessorTT>
  GenericArrayWorker(ArrayTT1&& array1, ArrayTT2&& array2, const ArrayMapperT1& mapper1,
    const ArrayMapperT2& mapper2, ProcessorTT&& processor, vtkUnsignedCharArray* ghosts,
    unsigned char ghostsToSkip)
    : Array1(std::forward<ArrayTT1>(array1))
    , Array2(std::forward<ArrayTT2>(array2))
    , Mapper1(mapper1)
    , Mapper2(mapper2)
    , Processor(std::forward<ProcessorTT>(processor))
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (this->Processor.GetAbort())
    {
      return;
    }

    ArrayT1& array1Ref = this->Array1;
    ArrayT2& array2Ref = this->Array2;

    auto execute = [this, &array1Ref, &array2Ref](vtkIdType id)
    {
      this->Processor.template Execute<N>(
        this->Mapper1.Map(id), array1Ref, this->Mapper2.Map(id), array2Ref);
    };

    if (!this->Ghosts || (this->Ghosts && !this->GhostsToSkip))
    {
      for (vtkIdType id = begin; id < end && !this->Processor.GetAbort(); ++id)
      {
        execute(id);
      }
    }
    else
    {
      auto ghosts = vtk::DataArrayValueRange<1>(this->Ghosts);
      for (vtkIdType id = begin; id < end && !this->Processor.GetAbort(); ++id)
      {
        if (!(ghosts[this->Mapper1.Map(id)] & this->GhostsToSkip))
        {
          execute(id);
        }
      }
    }
  }

  ArrayT1&& Array1;
  ArrayT2&& Array2;
  const ArrayMapperT1& Mapper1;
  const ArrayMapperT2& Mapper2;
  ProcessorT&& Processor;
  vtkUnsignedCharArray* Ghosts;
  unsigned char GhostsToSkip;
};

//----------------------------------------------------------------------------
template <int N = -1, class ArrayT1, class ArrayT2, class ArrayMapperT1, class ArrayMapperT2,
  class ProcessorT>
GenericArrayWorker<N, ArrayT1, ArrayT2, ArrayMapperT1, ArrayMapperT2, ProcessorT>
MakeGenericArrayWorker(ArrayT1&& array1, ArrayT2&& array2, const ArrayMapperT1& mapper1,
  const ArrayMapperT2& mapper2, ProcessorT&& processor, vtkUnsignedCharArray* ghosts,
  unsigned char ghostsToSkip)
{
  return GenericArrayWorker<N, ArrayT1, ArrayT2, ArrayMapperT1, ArrayMapperT2, ProcessorT>(
    std::forward<ArrayT1>(array1), std::forward<ArrayT2>(array2), mapper1, mapper2,
    std::forward<ProcessorT>(processor), ghosts, ghostsToSkip);
}

//============================================================================
template <class Array1T, class Array2T, class ArrayMapperT1, class ArrayMapperT2, class ProcessorT>
struct DataArrayMatchingWorker
{
  template <class ProcessorTT>
  DataArrayMatchingWorker(Array1T* array1, Array2T* array2, const ArrayMapperT1& mapper1,
    const ArrayMapperT2& mapper2, vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip,
    ProcessorTT&& processor)
    : Array1(array1)
    , Array2(array2)
    , Mapper1(mapper1)
    , Mapper2(mapper2)
    , Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
    , Processor(std::forward<ProcessorTT>(processor))
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (this->Processor.GetAbort())
    {
      return;
    }

    switch (this->Array1->GetNumberOfComponents())
    {
      case 1:
        this->Impl<1>(begin, end); // Scalar
        break;
      case 2:
        this->Impl<2>(begin, end); // 2D vector
        break;
      case 3:
        this->Impl<3>(begin, end); // 3D vector
        break;
      case 4:
        this->Impl<4>(begin, end); // Quaternion
        break;
      case 9:
        this->Impl<9>(begin, end); // 3x3 matrix
        break;
      default:
        this->Impl<vtkMath::DYNAMIC_VECTOR_SIZE()>(begin, end);
        break;
    }
  }

  template <int N>
  void Impl(vtkIdType begin, vtkIdType end)
  {
    auto array1 = vtk::DataArrayTupleRange<N>(this->Array1);
    auto array2 = vtk::DataArrayTupleRange<N>(this->Array2);

    auto arrayWorker = MakeGenericArrayWorker<N>(array1, array2, this->Mapper1, this->Mapper2,
      this->Processor, this->Ghosts, this->GhostsToSkip);

    arrayWorker(begin, end);
  }

  Array1T* Array1;
  Array2T* Array2;
  const ArrayMapperT1& Mapper1;
  const ArrayMapperT2& Mapper2;
  vtkUnsignedCharArray* Ghosts;
  unsigned char GhostsToSkip;
  ProcessorT&& Processor;
};

//----------------------------------------------------------------------------
template <class LauncherT, class ArrayT, class ArrayMapperT1, class ArrayMapperT2, class ProcessorT>
void LaunchArrayWorker(vtkAbstractArray* array1, vtkAbstractArray* array2,
  const ArrayMapperT1& mapper1, const ArrayMapperT2& mapper2, ProcessorT&& processor,
  vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip)
{
  LauncherT::Launch(0, mapper1.Size,
    MakeGenericArrayWorker(vtkArrayDownCast<ArrayT>(array1), vtkArrayDownCast<ArrayT>(array2),
      mapper1, mapper2, std::forward<ProcessorT>(processor), ghosts, ghostsToSkip));
}

//============================================================================
template <class LauncherT>
struct DataArrayMatchingDispatcher
{
  template <class Array1T, class Array2T, class ArrayMapperT1, class ArrayMapperT2,
    class ProcessorT>
  void operator()(Array1T* array1, Array2T* array2, const ArrayMapperT1& mapper1,
    const ArrayMapperT2& mapper2, ProcessorT&& processor, vtkUnsignedCharArray* ghosts = nullptr,
    unsigned char ghostsToSkip = 0)
  {
    LauncherT::Launch(0, mapper1.Size,
      DataArrayMatchingWorker<Array1T, Array2T, ArrayMapperT1, ArrayMapperT2, ProcessorT>(array1,
        array2, mapper1, mapper2, ghosts, ghostsToSkip, std::forward<ProcessorT>(processor)));
  }
};

//----------------------------------------------------------------------------
template <class LauncherT, class ArrayMapperT1, class ArrayMapperT2, class ProcessorT>
void DispatchArrays(vtkAbstractArray* array1, vtkAbstractArray* array2,
  const ArrayMapperT1& mapper1, const ArrayMapperT2& mapper2, ProcessorT&& processor,
  vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip)
{
  switch (array1->GetDataType())
  {
    case VTK_STRING:
      LaunchArrayWorker<LauncherT, vtkStringArray>(array1, array2, mapper1, mapper2,
        std::forward<ProcessorT>(processor), ghosts, ghostsToSkip);
      break;
    case VTK_BIT:
      LaunchArrayWorker<LauncherT, vtkBitArray>(array1, array2, mapper1, mapper2,
        std::forward<ProcessorT>(processor), ghosts, ghostsToSkip);
      break;
    case VTK_VARIANT:
      LaunchArrayWorker<LauncherT, vtkVariantArray>(array1, array2, mapper1, mapper2,
        std::forward<ProcessorT>(processor), ghosts, ghostsToSkip);
      break;
    default: // vtkDataArray
    {
      auto da1 = vtkArrayDownCast<vtkDataArray>(array1);
      auto da2 = vtkArrayDownCast<vtkDataArray>(array2);
      using DataArrayDispatcher =
        vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>;
      DataArrayMatchingDispatcher<LauncherT> dataArrayWorker;

      if (!DataArrayDispatcher::Execute(da1, da2, dataArrayWorker, mapper1, mapper2,
            std::forward<ProcessorT>(processor), ghosts, ghostsToSkip))
      {
        dataArrayWorker(
          da1, da2, mapper1, mapper2, std::forward<ProcessorT>(processor), ghosts, ghostsToSkip);
      }
      break;
    }
  }
}

//============================================================================
// Maps any id to a constant value.
// This can be used to compare a tuple of given id (using this mapper)
// to a set of tuples (using IdListMapper)
class ConstantMapper
{
public:
  ConstantMapper(vtkIdType id, vtkIdType size)
    : Id(id)
    , Size(size)
  {
  }

  vtkIdType Map(vtkIdType) const { return this->Id; }

  bool Success = true;

  vtkIdType Id;
  vtkIdType Size;
};

//============================================================================
// Maps ids using a vtkIdList
class IdListMapper
{
public:
  IdListMapper(vtkIdList* ids)
    : Ids(ids)
    , Size(ids->GetNumberOfIds())
  {
  }

  vtkIdType Map(vtkIdType id) const { return this->Ids->GetId(id); }

  bool Success = true;

  vtkIdList* Ids;
  vtkIdType Size;
};

//============================================================================
// Runs a worker using vtkSMPTools
struct SMPLauncher
{
  template <class WorkerT>
  static void Launch(vtkIdType begin, vtkIdType end, WorkerT&& worker)
  {
    vtkSMPTools::For(begin, end, worker);
  }
};

//============================================================================
// Runs a worker "as is"
struct SerialLauncher
{
  template <class WorkerT>
  static void Launch(vtkIdType begin, vtkIdType end, WorkerT&& worker)
  {
    worker(begin, end);
  }
};

//----------------------------------------------------------------------------
const char* GetArrayName(vtkAbstractArray* array)
{
  return array->GetName() ? array->GetName() : "";
}

//----------------------------------------------------------------------------
bool ArrayErrorHandler(vtkAbstractArray* array1, vtkAbstractArray* array2)
{
  if (!array1 || !array2)
  {
    vtkLog(ERROR, "Unexpected nullptr array pointer.");
    return false;
  }

  if (array1->GetNumberOfComponents() != array2->GetNumberOfComponents())
  {
    vtkLog(ERROR,
      "Arrays \"" << GetArrayName(array1) << "\" do not have the same number of components:"
                  << array1->GetNumberOfComponents() << " != " << array2->GetNumberOfComponents());
    return false;
  }

  if (array1->GetNumberOfTuples() != array2->GetNumberOfTuples())
  {
    vtkLog(ERROR,
      "Arrays \"" << GetArrayName(array1) << "\" do not have the same number of tuples:"
                  << array1->GetNumberOfTuples() << " != " << array2->GetNumberOfTuples());
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool ArrayErrorHandler(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkIdType n)
{
  if (!array1)
  {
    vtkLog(ERROR, "Unexpected nullptr array pointer");
    return false;
  }

  if (array1->GetNumberOfTuples() != n)
  {
    vtkLog(ERROR,
      "Expected " << n << " tuples in array \"" << GetArrayName(array1) << "\". Found "
                  << array1->GetNumberOfTuples() << " instead.");
    return false;
  }

  return ArrayErrorHandler(array1, array2);
}

//============================================================================
// PointMatchingWorker is to be called on ustructured meshes / point clouds.
// Given 2 inputs a and b, it uses a point locator built over b, queries points from a
// within a small error scaled to theirs numerical precision.
// A point map is creating for each pair, allowing in the future to have constant-time
// point mapping between a and b.
template <class ArrayT>
struct PointMatchingWorker
{
  using ValueType =
    typename decltype(vtk::DataArrayTupleRange(std::declval<ArrayT*>()))::ComponentType;

  PointMatchingWorker(vtkDataSet* query, vtkDataSet* target, vtkAbstractPointLocator* locator,
    std::vector<vtkIdType>& pointIdMap, double toleranceFactor)
    : Query(query)
    , Target(target)
    , Locator(locator)
    , PointIdMap(pointIdMap)
    , ToleranceFactor(toleranceFactor)
  {
    vtkIdType numberOfPoints = query->GetNumberOfPoints();
    if (target->GetNumberOfPoints() != numberOfPoints)
    {
      vtkLog(ERROR,
        "Tested DataSets do not have the same number of points: "
          << numberOfPoints << " != " << target->GetNumberOfPoints() << ".");
      this->Fail();
      return;
    }

    this->PointIdMap.resize(Query->GetNumberOfPoints());

    vtkPointData* queryPD = this->Query->GetPointData();
    vtkPointData* targetPD = this->Target->GetPointData();
    int nArrays = queryPD->GetNumberOfArrays();
    this->QueryArrays.reserve(nArrays);
    this->TargetArrays.reserve(nArrays);

    this->QueryGhosts = queryPD->GetGhostArray();
    this->TargetGhosts = targetPD->GetGhostArray();

    unsigned char queryGhostsToSkip = queryPD->GetGhostsToSkip();

    if (!this->QueryGhosts != !this->TargetGhosts)
    {
      vtkLog(ERROR, "One input has ghosts, the other doesn't.");
      this->Fail();
      return;
    }

    this->GhostsToSkip = queryGhostsToSkip;

    for (int i = 0; i < nArrays; ++i)
    {
      // Sorting QueryArrays and TargetArrays in the same order
      vtkAbstractArray* queryArray = queryPD->GetAbstractArray(i);
      if (queryArray == this->QueryGhosts)
      {
        continue;
      }

      if (!queryArray)
      {
        vtkLog(ERROR, "Array at index " << i << " is nullptr.");
        this->Fail();
        return;
      }
      if (vtkAbstractArray* targetArray = targetPD->GetAbstractArray(queryArray->GetName()))
      {
        if (!ArrayErrorHandler(queryArray, targetArray, numberOfPoints))
        {
          vtkLog(ERROR, "Array matching failure");
          this->Fail();
          return;
        }
        this->QueryArrays.push_back(queryArray);
        this->TargetArrays.push_back(targetArray);
      }
      else
      {
        vtkLog(
          ERROR, "Could not find array \"" << GetArrayName(queryArray) << "\" in other dataset.");
        this->Fail();
        return;
      }
    }
  }

  void Fail() { this->Success.store(false, std::memory_order_relaxed); }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    double p[3];
    vtkNew<vtkIdList> pointIds;

    for (vtkIdType pointId = begin; pointId < end && this->Success.load(std::memory_order_relaxed);
         ++pointId)
    {
      vtkIdType targetPointId = -1;
      this->Query->GetPoint(pointId, p);
      ValueType radius = this->ToleranceFactor * vtkMath::SquaredNorm<3>(p) *
        std::numeric_limits<ValueType>::epsilon();
      this->Locator->FindPointsWithinRadius(radius, p, pointIds);
      switch (vtkIdType nPoints = pointIds->GetNumberOfIds())
      {
        case 0:
          vtkLog(ERROR,
            "Could not find a matching point at point id " << pointId << " in other dataset.");
          this->Fail();
          return;
        case 1:
          targetPointId = pointIds->GetId(0);
          break;
        default: // We have multiple points at the same location in this->Target
        {
          if (this->QueryArrays.empty())
          {
            targetPointId = pointIds->GetId(0);
            break;
          }

          // We need special care if the input is a ghost to skip.
          // We just need to find one ghost to skip in the target ghost array candidates
          if (this->QueryGhosts && this->QueryGhosts->GetValue(pointId) & this->GhostsToSkip)
          {
            for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
            {
              if (this->TargetGhosts->GetValue(pointIds->GetId(i)) & this->GhostsToSkip)
              {
                targetPointId = pointIds->GetId(i);
              }
            }
            if (targetPointId == -1)
            {
              this->Success.store(false, std::memory_order_release);
            }
            break;
          }

          // The following algorithm will match a matching point within the candidates.
          // 2 points are matching if they have the same coordinates (already the case)
          // AND if their vtkPointData match as well.
          //
          // For each array within the point data, we look at the matching candidates w.r.t.
          // this array, and we perform the intersection of all the winning candidates.
          //
          // It is possible that at the end multiple points match, but it is not our problem.
          // Such scenario will be discriminated when looking at the topology, which is done
          // when checking the cells.
          std::unordered_set<vtkIdType> pointIdCandidates, pointIdCandidatesSource;

          auto initializer = [&pointIdCandidatesSource](bool equals, vtkIdType, vtkIdType id)
          {
            if (equals)
            {
              pointIdCandidatesSource.insert(id);
            }
          };

          auto intersector = [&pointIdCandidatesSource, &pointIdCandidates](
                               bool equals, vtkIdType, vtkIdType id)
          {
            if (equals)
            {
              auto it = pointIdCandidatesSource.find(id);
              if (it != pointIdCandidatesSource.end())
              {
                pointIdCandidates.insert(id);
              }
            }
          };

          auto noAbort = [] { return false; };

          auto processorInit =
            MakeVectorMatchingProcessor(this->ToleranceFactor, initializer, noAbort);
          auto processorInter =
            MakeVectorMatchingProcessor(this->ToleranceFactor, intersector, noAbort);

          ConstantMapper queryMapper(pointId, nPoints);
          IdListMapper targetMapper(pointIds);

          DispatchArrays<SerialLauncher>(this->QueryArrays.front(), this->TargetArrays.front(),
            queryMapper, targetMapper, std::move(processorInit), this->QueryGhosts,
            this->GhostsToSkip);

          for (std::size_t i = 1; i < this->QueryArrays.size(); ++i)
          {
            DispatchArrays<SerialLauncher>(this->QueryArrays[i], this->TargetArrays[i], queryMapper,
              targetMapper, processorInter, this->QueryGhosts, this->GhostsToSkip);

            pointIdCandidatesSource.clear();
            std::swap(pointIdCandidates, pointIdCandidatesSource);
          }

          if (pointIdCandidatesSource.empty())
          {
            vtkLog(ERROR,
              "Found point candidates when watching point position,"
                << " but their tuples don't match.");
            this->Fail();
            return;
          }

          targetPointId = *(pointIdCandidatesSource.begin());
        }
      }

      this->PointIdMap[pointId] = targetPointId;
    }
  }

  vtkDataSet* Query;
  vtkDataSet* Target;
  vtkAbstractPointLocator* Locator;
  std::vector<vtkAbstractArray*> QueryArrays;
  std::vector<vtkAbstractArray*> TargetArrays;
  vtkUnsignedCharArray* QueryGhosts = nullptr;
  vtkUnsignedCharArray* TargetGhosts = nullptr;
  unsigned char GhostsToSkip = 0;
  std::vector<vtkIdType>& PointIdMap;
  double ToleranceFactor;
  std::atomic_bool Success = { true };
};

//============================================================================
struct PointMatchingDispatcher
{
  PointMatchingDispatcher(std::vector<vtkIdType>& pointIdMap, bool mapPoints)
    : PointIdMap(pointIdMap)
    , MapPoints(mapPoints)
  {
  }

  template <class ArrayT>
  void operator()(ArrayT*, vtkDataSet* query, vtkDataSet* target, vtkAbstractPointLocator* locator,
    double toleranceFactor)
  {
    PointMatchingWorker<ArrayT> worker(query, target, locator, this->PointIdMap, toleranceFactor);
    vtkSMPTools::For(0, query->GetNumberOfPoints(), worker);
    this->Success = worker.Success.load(std::memory_order_acquire);
  }

  bool Success;
  std::vector<vtkIdType>& PointIdMap;
  bool MapPoints;
};

//============================================================================
// These mappers are basically functions that map a nth id to its point id.
// It is particularly of interest for unstructured data, where you might have
// to map points that are sorted differently.
//
// The mapper is built at construction of the class. If it is not possible to map 2 points,
// then the member Success is set to false, and we can terminate.
//
// The mapper also embeds a Size member that tells how many points are to map.
// It is useful in particular for IdListMapper, which maps a subset of the dataset.
//
// For structured data, the mapper will set Success to false if the parameters spawning the geometry
// of the dataset do not match one another.
template <class DataObjectT, class = void>
struct DataSetPointMapper;

//============================================================================
template <>
struct DataSetPointMapper<vtkPointSet>
{
  DataSetPointMapper(vtkPointSet* ps1, vtkPointSet* ps2, double toleranceFactor)
    : Size(ps1->GetNumberOfPoints())
  {
    auto testValidInput = [](vtkPointSet* input)
    {
      if (vtkPoints* points = input->GetPoints())
      {
        if (!points->GetData())
        {
          vtkLog(ERROR, "The vtkDataArray* in a vtkPoints* of the input is nullptr.");
          return false;
        }
      }
      else
      {
        vtkLog(ERROR, "There is a nullptr vtkPoints* in one dataset.");
        return false;
      }
      return true;
    };

    if (!ps1->GetNumberOfPoints() && !ps2->GetNumberOfPoints())
    {
      // Nothing to compare
      this->Success = true;
      return;
    }
    if (!testValidInput(ps1) || !testValidInput(ps2))
    {
      this->Success = false;
      return;
    }

    auto comparePoints = [this, &toleranceFactor](
                           vtkPointSet* query, vtkPointSet* target, bool mapPoints)
    {
      vtkNew<vtkStaticPointLocator> locator;
      locator->SetDataSet(target);
      locator->BuildLocator();
      vtkDataArray* points = query->GetPoints()->GetData();

      using PointDispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
      PointMatchingDispatcher pointWorker(this->PointIdMap, mapPoints);
      if (!PointDispatcher::Execute(points, pointWorker, query, target, locator, toleranceFactor))
      {
        pointWorker(points, query, target, locator, toleranceFactor);
      }
      return pointWorker.Success;
    };

    // We need to test both sides
    if (!(this->Success = comparePoints(ps1, ps2, true) && comparePoints(ps2, ps1, false)))
    {
      vtkLog(ERROR, "Point positions don't match between the 2 input " << ps1->GetClassName());
    }
  }

  vtkIdType Map(vtkIdType pointId) const { return this->PointIdMap[pointId]; }

  bool Success;
  vtkIdType Size;

  std::vector<vtkIdType> PointIdMap;
};

//----------------------------------------------------------------------------
template <class ArrayMapperT>
bool TestAbstractArray(vtkAbstractArray* array1, vtkAbstractArray* array2, ArrayMapperT&& mapper,
  double toleranceFactor, vtkUnsignedCharArray* ghosts = nullptr, unsigned char ghostsToSkip = 0)
{
  std::atomic_bool success{ true };

  auto decider = [&success](bool equals, vtkIdType id1, vtkIdType id2)
  {
    if (!equals)
    {
      vtkLog(ERROR, "Tuples mapped at id " << id1 << " and " << id2 << " do not match.");
      success.store(false, std::memory_order_release);
    }
  };

  auto aborter = [&success] { return !success.load(std::memory_order_relaxed); };

  DispatchArrays<SMPLauncher>(array1, array2, IdentityMapper(mapper.Size),
    std::forward<ArrayMapperT>(mapper),
    MakeVectorMatchingProcessor(toleranceFactor, decider, aborter), ghosts, ghostsToSkip);

  return success.load(std::memory_order_acquire);
}

//============================================================================
template <class DataSetT>
struct StructuredDataSetMapper : public DataObjectTraits<DataSetT>::BaseMapper
{
  using BaseMapper = typename DataObjectTraits<DataSetT>::BaseMapper;

  StructuredDataSetMapper(DataSetT* ds1, DataSetT* ds2, double toleranceFactor)
    : BaseMapper(ds1)
  {
    int e1[6], e2[6];
    ds1->GetExtent(e1);
    ds2->GetExtent(e2);
    if (!(this->Success &= VectorsAreNearlyEqual<6>(e1, e2, toleranceFactor)))
    {
      vtkLog(ERROR, "Extent doesn't match between the 2 input " << vtk::TypeName<DataSetT>());
    }
  }
};

//============================================================================
template <>
struct DataSetPointMapper<vtkImageData> : public StructuredDataSetMapper<vtkImageData>
{
  DataSetPointMapper(vtkImageData* im1, vtkImageData* im2, double toleranceFactor)
    : StructuredDataSetMapper<vtkImageData>(im1, im2, toleranceFactor)
  {
    if (!this->Success)
    {
      return;
    }

    double origin1[3], origin2[3], spacing1[3], spacing2[3];
    vtkQuaternion<double> q1, q2;

    auto extractStructure =
      [](vtkImageData* im, double origin[3], double spacing[3], vtkQuaternion<double>& q)
    {
      im->GetOrigin(origin);
      im->GetSpacing(spacing);
      vtkMatrix3x3* m = im->GetDirectionMatrix();
      vtkMath::Matrix3x3ToQuaternion(m->GetData(), q.GetData());
    };

    extractStructure(im1, origin1, spacing1, q1);
    extractStructure(im2, origin2, spacing2, q2);

    if (!(this->Success &=
          RealVectorsNearlyEqualImpl<4>(
            2.0, vtkMath::Dot<double, 4>(q1.GetData(), q2.GetData()), toleranceFactor) &&
            VectorsAreNearlyEqual<3>(origin1, origin2, toleranceFactor) &&
            VectorsAreNearlyEqual<3>(spacing1, spacing2, toleranceFactor)))
    {
      vtkLog(ERROR, "Structure doesn't match between the 2 input vtkImageData");
    }
  }
};

//============================================================================
// For vtkRectilinearGrid and vtkHyperTreeGrid
template <class RectilinearGridT>
struct DataSetPointMapper<RectilinearGridT,
  typename std::enable_if<DataObjectTraits<RectilinearGridT>::IsRectilinear == true>::type>
  : public StructuredDataSetMapper<RectilinearGridT>
{
  DataSetPointMapper(RectilinearGridT* rg1, RectilinearGridT* rg2, double toleranceFactor)
    : StructuredDataSetMapper<RectilinearGridT>(rg1, rg2, toleranceFactor)
  {
    if (!this->Success)
    {
      return;
    }

    auto extractStructure = [](RectilinearGridT* rg, vtkDataArray* coords[3])
    {
      coords[0] = rg->GetXCoordinates();
      coords[1] = rg->GetYCoordinates();
      coords[2] = rg->GetZCoordinates();
      int dim[3];
      rg->GetDimensions(dim);

      for (int i = 0; i < 3; ++i)
      {
        if (dim[i] != coords[i]->GetNumberOfValues())
        {
          vtkLog(ERROR,
            "Not right number of coordinates in dimension " << i << " for "
                                                            << vtk::TypeName<RectilinearGridT>());
          return false;
        }
      }
      return true;
    };

    vtkDataArray* coords1[3];
    vtkDataArray* coords2[3];

    if (!(this->Success &= extractStructure(rg1, coords1) && extractStructure(rg2, coords2)))
    {
      return;
    }

    using DataArrayDispatcher = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Reals>;

    vtkIdType errorId;
    auto decider = [&errorId, this](bool equals, vtkIdType id, vtkIdType)
    {
      if (!equals)
      {
        vtkLog(ERROR, "Coords don't match at id " << id << ".");
        errorId = id;
        this->Success = false;
      }
    };
    auto aborter = [this] { return !this->Success; };

    DataArrayMatchingDispatcher<SerialLauncher> dataArrayWorker;
    auto processor = MakeVectorMatchingProcessor(toleranceFactor, decider, aborter);

    for (int dim = 0; dim < 3 && this->Success; ++dim)
    {
      if (coords1[0]->GetNumberOfComponents() != 1)
      {
        vtkLog(ERROR, "Coordinates in a rectilinear grid must have one component each.");
        this->Success = false;
        return;
      }

      IdentityMapper identity(coords1[dim]->GetNumberOfTuples());
      if (!DataArrayDispatcher::Execute(
            coords1[dim], coords2[dim], dataArrayWorker, identity, identity, processor))
      {
        dataArrayWorker(coords1[dim], coords2[dim], identity, identity, processor);
      }

      if (!this->Success)
      {
        vtkLog(ERROR,
          "Failure on Coords in dimension " << dim << ". " << coords1[dim]->GetTuple1(errorId)
                                            << " != " << coords2[dim]->GetTuple1(errorId) << ".");
      }
    }

    if (!this->Success)
    {
      vtkLog(
        ERROR, "Structure doesn't match between the 2 input " << vtk::TypeName<RectilinearGridT>());
    }
  }
};

//============================================================================
template <class StructuredPointSetT>
struct DataSetPointMapper<StructuredPointSetT,
  typename std::enable_if<std::is_base_of<vtkPointSet, StructuredPointSetT>::value>::type>
  : public StructuredDataSetMapper<StructuredPointSetT>
{
  DataSetPointMapper(StructuredPointSetT* ps1, StructuredPointSetT* ps2, double toleranceFactor)
    : StructuredDataSetMapper<StructuredPointSetT>(ps1, ps2, toleranceFactor)
  {
    if (!this->Success)
    {
      return;
    }

    vtkPoints* points1 = ps1->GetPoints();
    vtkPoints* points2 = ps2->GetPoints();

    if (!points1 || !points2)
    {
      this->Success &= false;
      return;
    }

    vtkDataArray* da1 = points1->GetData();
    vtkDataArray* da2 = points2->GetData();

    if (!(this->Success &= TestAbstractArray(da1, da2, *this, toleranceFactor)))
    {
      vtkLog(ERROR,
        "Point positions don't match between the 2 input " << vtk::TypeName<StructuredPointSetT>());
    }
  }
};

//----------------------------------------------------------------------------
template <class FieldDataT = void, class ArrayMapperT>
bool TestFieldData(vtkFieldData* fd1, vtkFieldData* fd2, ArrayMapperT&& mapper,
  double toleranceFactor, bool ignoreNumberOfTuples = false)
{
  if (!fd1 || !fd2)
  {
    vtkLog(ERROR, "One of the 2 input vtkFieldData is nullptr.");
    return false;
  }

  vtkIdType numberOfTuples = fd1->GetNumberOfTuples();

  std::string fdName =
    std::is_same<FieldDataT, void>::value ? fd1->GetClassName() : vtk::TypeName<FieldDataT>();

  if (!ignoreNumberOfTuples && numberOfTuples != fd2->GetNumberOfTuples())
  {
    vtkLog(ERROR,
      "Mismatched number of tuples in " << fdName << ", " << numberOfTuples
                                        << " != " << fd2->GetNumberOfTuples() << ".");
    return false;
  }

  vtkUnsignedCharArray* ghosts1 = fd1->GetGhostArray();
  vtkUnsignedCharArray* ghosts2 = fd2->GetGhostArray();

  unsigned char ghostsToSkip1 = fd1->GetGhostsToSkip();
  unsigned char ghostsToSkip2 = fd2->GetGhostsToSkip();

  if (!ghosts1 != !ghosts2 || ghostsToSkip1 != ghostsToSkip2)
  {
    vtkLog(ERROR, "Ghost element status of the 2 input " << fdName << " do not match.");
    return false;
  }

  for (int id = 0; id < fd1->GetNumberOfArrays(); ++id)
  {
    vtkAbstractArray* array1 = fd1->GetAbstractArray(id);

    vtkAbstractArray* array2 = array1 ? fd2->GetAbstractArray(array1->GetName()) : nullptr;

    if (!ArrayErrorHandler(array1, array2))
    {
      vtkLog(ERROR, "Cannot process arrays.");
      return false;
    }

    // We can skip ghost arrays, we process them separately.
    if (array1 == ghosts1)
    {
      continue;
    }

    if (ignoreNumberOfTuples)
    {
      mapper.Size = array1->GetNumberOfTuples();
    }

    if (!TestAbstractArray(array1, array2, std::forward<ArrayMapperT>(mapper), toleranceFactor,
          ghosts1, ghostsToSkip1))
    {
      vtkLog(ERROR, "Array mismatch for " << array1->GetName() << " in input " << fdName << ".");
      return false;
    }
  }

  if (ghosts1 && !TestAbstractArray(ghosts1, ghosts2, std::forward<ArrayMapperT>(mapper), 0))
  {
    vtkLog(ERROR, "Ghost arrays in " << fdName << " do not match.");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
template <class FieldDataT = void, class DataSetT, class PointMapperT>
typename std::enable_if<DataObjectTraits<DataSetT>::HasPoints == true, bool>::type TestPointData(
  DataSetT* ds1, DataSetT* ds2, PointMapperT&& pointMapper, double toleranceFactor)
{
  return TestFieldData<FieldDataT>(ds1->GetPointData(), ds2->GetPointData(),
    std::forward<PointMapperT>(pointMapper), toleranceFactor);
}

//----------------------------------------------------------------------------
template <class FieldDataT = void, class DataObjectT>
bool TestPoints(DataObjectT* ds1, DataObjectT* ds2, double toleranceFactor)
{
  DataSetPointMapper<DataObjectT> pointMapper(ds1, ds2, toleranceFactor);

  if (!pointMapper.Success)
  {
    vtkLog(ERROR, "Point geometry doesn't match");
    return false;
  }

  return TestPointData<FieldDataT>(ds1, ds2, pointMapper, toleranceFactor);
}

//----------------------------------------------------------------------------
bool TestEdgeCenters(vtkPointSet* ps1, vtkPointSet* ps2, double toleranceFactor)
{
  auto computeEdgeCenters = [](vtkPointSet* ps)
  {
    vtkNew<vtkExtractEdges> edges;
    edges->SetInputData(ps);

    vtkNew<vtkCellCenters> centers;
    centers->SetInputConnection(edges->GetOutputPort());
    centers->Update();

    return vtkSmartPointer<vtkPointSet>(vtkPointSet::SafeDownCast(centers->GetOutputDataObject(0)));
  };

  auto centers1 = computeEdgeCenters(ps1);
  auto centers2 = computeEdgeCenters(ps2);

  // We want to output errors using vtkCellData in the log rather than vtkPointData
  return TestPoints<vtkCellData>(centers1.GetPointer(), centers2.GetPointer(), toleranceFactor);
}

//----------------------------------------------------------------------------
bool TestEdgeCenters(vtkHyperTreeGrid*, vtkHyperTreeGrid*, double)
{
  // No need to check edges with vtkHyperTreeGrid, the topology is implicit
  return true;
}

//----------------------------------------------------------------------------
std::string GenerateRandomHexaString()
{
  static std::minstd_rand gen;
  int n = gen();
  std::stringstream ss;
  ss << std::hex << n;
  return ss.str();
}

//----------------------------------------------------------------------------
void GenerateNewRandomArrayName(std::string nameRoot, vtkAbstractArray* array1,
  vtkAbstractArray* array2, vtkFieldData* fd1, vtkFieldData* fd2)
{
  std::string name;
  do
  {
    // The name is an integer over 2 bytes written in hexadecimal. We are unlikely not to find one.
    name = nameRoot + GenerateRandomHexaString();
  } while (fd1->GetAbstractArray(name.c_str()) && fd2->GetAbstractArray(name.c_str()));

  array1->SetName(name.c_str());
  array2->SetName(name.c_str());
}

//----------------------------------------------------------------------------
void AddArrayCopyWithUniqueNameToFieldData(std::string nameRoot, vtkDataArray* array1,
  vtkDataArray* array2, vtkFieldData* fd1, vtkFieldData* fd2)
{
  auto impl = [](vtkDataArray* in)
  {
    auto out = vtkSmartPointer<vtkDataArray>::Take(in->NewInstance());
    out->ShallowCopy(in);
    return out;
  };

  auto out1 = impl(array1);
  auto out2 = impl(array2);

  // We have to name the meta data arrays. We do that by generating a random name.
  GenerateNewRandomArrayName(std::move(nameRoot), out1, out2, fd1, fd2);

  fd1->AddArray(out1);
  fd2->AddArray(out2);
}

//----------------------------------------------------------------------------
void AddCellMetaDataToCellDataImpl(
  vtkHyperTreeGrid* vtkNotUsed(out1), vtkHyperTreeGrid* vtkNotUsed(out2))
{
  // No special array to handle for HTG, the mask being used during tree iteration,
  // it does not need to be compared value-by-value
}

//----------------------------------------------------------------------------
void AddCellMetaDataToCellDataImpl(vtkUnstructuredGrid* out1, vtkUnstructuredGrid* out2)
{
  // We only need to shallow copy CellTypes
  AddArrayCopyWithUniqueNameToFieldData("cell_types_", out1->GetCellTypesArray(),
    out2->GetCellTypesArray(), out1->GetCellData(), out2->GetCellData());
}

//----------------------------------------------------------------------------
void AddCellMetaDataToCellDataImpl(vtkPolyData* out1, vtkPolyData* out2)
{
  // We need to generate a cell types array.
  auto impl = [](vtkPolyData* out)
  {
    out->BuildCells();

    vtkNew<vtkUnsignedCharArray> cellTypes;
    cellTypes->SetNumberOfValues(out->GetNumberOfCells());

    auto array = vtk::DataArrayValueRange<1>(cellTypes);

    vtkSMPTools::For(0, cellTypes->GetNumberOfValues(),
      [&array, &out](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          array[cellId] = out->GetCellType(cellId);
        }
      });
    return cellTypes;
  };

  auto cellTypes1 = impl(out1);
  auto cellTypes2 = impl(out2);

  vtkCellData* cd1 = out1->GetCellData();
  vtkCellData* cd2 = out2->GetCellData();

  GenerateNewRandomArrayName("cell_types_", cellTypes1, cellTypes2, cd1, cd2);

  cd1->AddArray(cellTypes1);
  cd2->AddArray(cellTypes2);
}

//----------------------------------------------------------------------------
void AddCellMetaDataToCellDataImpl(vtkPointSet* out1, vtkPointSet* out2)
{
  if (auto pd1 = vtkPolyData::SafeDownCast(out1))
  {
    AddCellMetaDataToCellDataImpl(pd1, vtkPolyData::SafeDownCast(out2));
  }
  else if (auto ug1 = vtkUnstructuredGrid::SafeDownCast(out1))
  {
    AddCellMetaDataToCellDataImpl(ug1, vtkUnstructuredGrid::SafeDownCast(out2));
  }
}

//----------------------------------------------------------------------------
template <class DataObjectT>
std::pair<vtkSmartPointer<DataObjectT>, vtkSmartPointer<DataObjectT>> AddCellMetaDataToCellData(
  DataObjectT* in1, DataObjectT* in2)
{
  auto createOutput = [](DataObjectT* in)
  {
    auto out = vtkSmartPointer<DataObjectT>::Take(in->NewInstance());
    out->CopyStructure(in);
    out->ShallowCopy(in);
    return out;
  };

  auto out1 = createOutput(in1);
  auto out2 = createOutput(in2);

  AddCellMetaDataToCellDataImpl(out1, out2);

  return std::make_pair(out1, out2);
}

//============================================================================
template <class DataObjectT, class = void>
struct CellsTester;

//============================================================================
// When the dataset is not structured, it is sufficient for us to test the cell centers
// and the edge centers instead of the cells themselves.
// Indeed, even if the points match, a wrong topology will
// be caught when computing those centers. It allows us to reuse the point matching functions,
// as centers form a point cloud.
template <class DataObjectT>
struct CellsTester<DataObjectT,
  typename std::enable_if<DataObjectTraits<DataObjectT>::IsStructured == false>::type>
{
  static bool Execute(DataObjectT* do1, DataObjectT* do2, double toleranceFactor)
  {
    bool retVal = true;

    // We want to add cell meta data (cell types for vtkPointSet, bit mask for htg)
    // into a vtkCellData. We create a new instance of the 2 inputs, shallow copy them,
    // and add the meta data to the vtkCellData
    vtkSmartPointer<DataObjectT> enhancedDO1, enhancedDO2;
    std::tie(enhancedDO1, enhancedDO2) = AddCellMetaDataToCellData(do1, do2);

    auto computeCellCenters = [](DataObjectT* ds)
    {
      using CellCenters = typename DataObjectTraits<DataObjectT>::CellCenters;

      vtkNew<CellCenters> centers;
      centers->SetInputData(ds);
      centers->ConvertGhostCellsToGhostPointsOff();
      centers->Update();

      return vtkSmartPointer<vtkPointSet>(
        vtkPointSet::SafeDownCast(centers->GetOutputDataObject(0)));
    };

    auto centers1 = computeCellCenters(enhancedDO1);
    auto centers2 = computeCellCenters(enhancedDO2);

    // We want to output errors using vtkCellData in the log rather than vtkPointData
    if (!TestPoints<vtkCellData>(centers1.GetPointer(), centers2.GetPointer(), toleranceFactor))
    {
      vtkLog(ERROR, "Cells of input of type " << do1->GetClassName() << " do not match.");
      retVal = false;
    }

    if (!TestEdgeCenters(do1, do2, toleranceFactor))
    {
      vtkLog(ERROR, "Cell connectivity is wrong between the 2 datasets.");
      retVal = false;
    }

    return retVal;
  }
};

//============================================================================
template <class DataObjectT>
struct CellsTester<DataObjectT,
  typename std::enable_if<DataObjectTraits<DataObjectT>::IsStructured == true>::type>
{
  static bool Execute(DataObjectT* do1, DataObjectT* do2, double toleranceFactor)
  {
    if (!TestFieldData(do1->GetCellData(), do2->GetCellData(),
          IdentityMapper(do1->GetNumberOfCells()), toleranceFactor))
    {
      vtkLog(ERROR, "Cells of input of type " << do1->GetClassName() << " do not match.");
      return false;
    }
    return true;
  }
};

//============================================================================
template <class DataObjectT>
struct TestDataObjectsImpl
{
  static bool Execute(DataObjectT* do1, DataObjectT* do2, double toleranceFactor)
  {
    if (!TestFieldData(do1->GetFieldData(), do2->GetFieldData(),
          IdentityMapper(do1->GetFieldData()->GetNumberOfTuples()), toleranceFactor, true) ||
      !TestPoints(do1, do2, toleranceFactor) ||
      !CellsTester<DataObjectT>::Execute(do1, do2, toleranceFactor))
    {
      vtkLog(
        ERROR, "Failed to match the 2 input data objects of type " << do1->GetClassName() << ".");
      return false;
    }
    return true;
  }
};

//============================================================================
template <>
struct TestDataObjectsImpl<vtkTable>
{
  static bool Execute(vtkTable* t1, vtkTable* t2, double toleranceFactor)
  {
    IdentityMapper identity(t1->GetNumberOfRows());

    if (!TestFieldData(t1->GetFieldData(), t2->GetFieldData(), identity, toleranceFactor, true) ||
      !TestFieldData(t1->GetRowData(), t2->GetRowData(), identity, toleranceFactor))
    {
      vtkLog(ERROR, "Failed to match the 2 input data objects of type vtkTable.");
      return false;
    }
    return true;
  }
};

//============================================================================
/**
 * Recursively check if the trees pointed by the 2 cursors have
 * the same structure and associated data.
 */
bool CheckTreeEqual(vtkHyperTreeGridNonOrientedGeometryCursor* cursor1,
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor2, vtkCellData* data1, vtkCellData* data2)
{
  vtkIdType currentId1 = cursor1->GetGlobalNodeIndex();
  vtkIdType currentId2 = cursor2->GetGlobalNodeIndex();

  // Match mask status
  if (cursor1->IsMasked() != cursor2->IsMasked())
  {
    vtkLog(ERROR, "Mismatched mask status for ids " << currentId1 << "/" << currentId2);
    return false;
  }

  if (cursor1->IsMasked())
  {
    // Ignore masked cell
    return true;
  }

  for (int id = 0; id < data1->GetNumberOfArrays(); ++id)
  {
    vtkDataArray* array1 = vtkArrayDownCast<vtkDataArray>(data1->GetAbstractArray(id));
    vtkDataArray* array2 =
      array1 ? vtkArrayDownCast<vtkDataArray>(data2->GetAbstractArray(array1->GetName())) : nullptr;
    if (!array1 || !array2)
    {
      vtkLog(ERROR, "Cannot process arrays.");
      return false;
    }

    // Compare a single value using the vtkDataArray API
    int ncomps = array1->GetNumberOfComponents();
    if (ncomps != array2->GetNumberOfComponents())
    {
      vtkLog(ERROR, "Mismatched number of composants in array " << array1->GetName());
      return false;
    }
    double* tuple1 = array1->GetTuple(currentId1);
    double* tuple2 = array2->GetTuple(currentId2);
    for (int comp = 0; comp < ncomps; comp++)
    {
      if (tuple1[comp] != tuple2[comp])
      {
        vtkLog(ERROR,
          "Array mismatch for " << array1->GetName() << " in input HyperTreeGrid "
                                << " for tuple " << currentId1 << "/" << currentId2 << " component "
                                << comp << " : " << tuple1[comp] << " != " << tuple2[comp]);
        return false;
      }
    }
  }

  // Match leaf status
  if (cursor1->IsLeaf() != cursor2->IsLeaf())
  {
    vtkLog(ERROR, "Mismatched leaves" << currentId1 << "/" << currentId2);
    return false;
  }

  if (cursor1->IsLeaf())
  {
    return true;
  }

  if (cursor1->GetNumberOfChildren() != cursor2->GetNumberOfChildren())
  {
    vtkLog(ERROR, "Mismatched number of children");
    return false;
  }

  // Recurse over children
  bool result = true;
  for (int child = 0; child < cursor1->GetNumberOfChildren(); ++child)
  {
    cursor1->ToChild(child);
    cursor2->ToChild(child);
    result &= ::CheckTreeEqual(cursor1, cursor2, data1, data2);
    cursor1->ToParent();
    cursor2->ToParent();
  }
  return result;
}

/**
 * HyperTreeGrid needs special comparison, because 2 equivalent HTGs can have a different internal
 * structure and memory layout. Comparison needs to be done using cursors over each HyperTree.
 */
template <>
struct TestDataObjectsImpl<vtkHyperTreeGrid>
{
  static bool Execute(vtkHyperTreeGrid* htg1, vtkHyperTreeGrid* htg2, double toleranceFactor)
  {
    if (!TestFieldData(htg1->GetFieldData(), htg2->GetFieldData(),
          IdentityMapper(htg1->GetFieldData()->GetNumberOfTuples()), toleranceFactor, true))
    {
      return false;
    }

    vtkCellData* data1 = htg1->GetCellData();
    vtkCellData* data2 = htg2->GetCellData();

    // Compare extent
    const int* extent1 = htg1->GetExtent();
    const int* extent2 = htg2->GetExtent();
    for (int i = 0; i < 5; i++)
    {
      if (extent1[i] != extent2[i])
      {
        vtkLog(ERROR, "Extent doesn't match between the 2 input vtkHyperTreeGrid");
        return false;
      }
    }

    // Compare dimensions
    const unsigned int* dims1 = htg1->GetDimensions();
    const unsigned int* dims2 = htg2->GetDimensions();
    for (int i = 0; i < 3; i++)
    {
      if (dims1[i] != dims2[i])
      {
        vtkLog(ERROR, "Dimension doesn't match between the 2 input vtkHyperTreeGrid");
        return false;
      }
    }

    if (htg1->GetOrientation() != htg2->GetOrientation())
    {
      vtkLog(ERROR, "Orientation doesn't match between the 2 input vtkHyperTreeGrid");
      return false;
    }

    if (data1->GetNumberOfArrays() != data2->GetNumberOfArrays())
    {
      vtkLog(ERROR, "Number of arrays doesn't match between the 2 input vtkHyperTreeGrid");
      return false;
    }

    if (htg1->GetInterfaceInterceptsName() && htg2->GetInterfaceInterceptsName() &&
      std::string(htg1->GetInterfaceInterceptsName()) !=
        std::string(htg2->GetInterfaceInterceptsName()))
    {
      vtkLog(ERROR, "Interface Intercepts Name doesn't match between the 2 input vtkHyperTreeGrid");
      return false;
    }

    if (htg1->GetInterfaceNormalsName() && htg2->GetInterfaceNormalsName() &&
      std::string(htg1->GetInterfaceNormalsName()) != std::string(htg2->GetInterfaceNormalsName()))
    {
      vtkLog(ERROR, "Interface Normals Name doesn't match between the 2 input vtkHyperTreeGrid");
      return false;
    }

    if (htg1->GetHasInterface() != htg2->GetHasInterface())
    {
      vtkLog(ERROR, "Only one out of the 2 HTG has the interface flag set");
      return false;
    }

    if (htg1->GetHasInterface() != htg2->GetHasInterface())
    {
      vtkLog(ERROR, "Only one out of the 2 HTG has the interface flag set");
      return false;
    }

    if (htg1->GetDepthLimiter() != htg2->GetDepthLimiter())
    {
      vtkLog(ERROR, "Depth Limiter value doesn't match between the 2 input vtkHyperTreeGrid");
      return false;
    }

    if (htg1->GetTransposedRootIndexing() != htg2->GetTransposedRootIndexing())
    {
      vtkLog(
        ERROR, "Transposed Root Indexing value doesn't match between the 2 input vtkHyperTreeGrid");
      return false;
    }

    // Iterate over HTGs
    vtkIdType indexHTG1 = 0, indexHTG2 = 0;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator1, iterator2;
    htg1->InitializeTreeIterator(iterator1);
    htg2->InitializeTreeIterator(iterator2);
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor1, cursor2;
    while (iterator1.GetNextTree(indexHTG1) && iterator2.GetNextTree(indexHTG2))
    {
      htg1->InitializeNonOrientedGeometryCursor(cursor1, indexHTG1);
      htg2->InitializeNonOrientedGeometryCursor(cursor2, indexHTG2);

      if (!::CheckTreeEqual(cursor1, cursor2, data1, data2))
      {
        return false;
      }
    }
    return true;
  }
};

//============================================================================
/**
 * Check the equality of both partitioned collections.
 * Test both the equality of each partitioned dataset and the strict equality of the assembly.
 */
template <>
struct TestDataObjectsImpl<vtkPartitionedDataSetCollection>
{
  static bool Execute(vtkPartitionedDataSetCollection* t1, vtkPartitionedDataSetCollection* t2,
    double toleranceFactor)
  {
    if (t1->GetNumberOfPartitionedDataSets() != t2->GetNumberOfPartitionedDataSets())
    {
      vtkLog(ERROR,
        "Each vtkPartitionedDataSetCollection should have the same number of PartitionedDataSet. "
        "Got "
          << t1->GetNumberOfPartitionedDataSets() << " and " << t2->GetNumberOfPartitionedDataSets()
          << ".");
      return false;
    }

    for (unsigned int index = 0; index < t1->GetNumberOfPartitionedDataSets(); ++index)
    {
      vtkPartitionedDataSet* t1Block = t1->GetPartitionedDataSet(index);
      vtkPartitionedDataSet* t2Block = t2->GetPartitionedDataSet(index);

      if (!vtkTestUtilities::CompareDataObjects(t1Block, t2Block, toleranceFactor))
      {
        vtkLog(ERROR,
          "vtkPartitionedDataSetCollection Partitioned datasets " << index << "do not match");
        return false;
      }
    }

    vtkDataAssembly* assembly1 = t1->GetDataAssembly();
    vtkDataAssembly* assembly2 = t2->GetDataAssembly();
    if (!assembly1 && !assembly2)
    {
      return true;
    }
    if (!assembly1 || !assembly2 || assembly1->GetChildNodes(0) != assembly2->GetChildNodes(0))
    {
      vtkLog(ERROR, "vtkPartitionedDataSetCollection Assembly tree structures do not match");
      return false;
    }
    for (auto node : assembly1->GetChildNodes(0))
    {
      if (assembly1->GetDataSetIndices(node) != assembly2->GetDataSetIndices(node))
      {
        vtkLog(ERROR,
          "vtkPartitionedDataSetCollection Assembly dataset indices for node " << node
                                                                               << " do not match.");
        return false;
      }
    }

    return true;
  }
};

//============================================================================
/**
 * Check each partition from inputs.
 * For the structure itself, only the number of partition is checked.
 */
template <>
struct TestDataObjectsImpl<vtkPartitionedDataSet>
{
  static bool Execute(vtkPartitionedDataSet* t1, vtkPartitionedDataSet* t2, double toleranceFactor)
  {
    if (!t1 || !t2)
    {
      return true;
    }

    if (t1->GetNumberOfPartitions() != t2->GetNumberOfPartitions())
    {
      vtkLog(ERROR,
        "Each partitioned dataset should have the same number of partitions. Got "
          << t1->GetNumberOfPartitions() << " and " << t2->GetNumberOfPartitions() << ".");
      return false;
    }

    for (unsigned int index = 0; index < t1->GetNumberOfPartitions(); index++)
    {
      vtkDataObject* t1Block = t1->GetPartitionAsDataObject(index);
      vtkDataObject* t2Block = t2->GetPartitionAsDataObject(index);

      if (!t1Block || !t2Block)
      {
        continue;
      }

      if (!vtkTestUtilities::CompareDataObjects(t1Block, t2Block, toleranceFactor))
      {
        return false;
      }
    }

    return true;
  }
};

//============================================================================
/**
 * Check each block from inputs.
 * For the structure itself, only the number of blocks is checked.
 */
template <>
struct TestDataObjectsImpl<vtkMultiBlockDataSet>
{
  static bool Execute(vtkMultiBlockDataSet* mb1, vtkMultiBlockDataSet* mb2, double toleranceFactor)
  {
    if (!mb1 || !mb2)
    {
      return true;
    }

    if (mb1->GetNumberOfBlocks() != mb2->GetNumberOfBlocks())
    {
      vtkLog(ERROR,
        "Each multiBlockDataSet should have the same number of blocks. Got "
          << mb1->GetNumberOfBlocks() << " and " << mb2->GetNumberOfBlocks() << ".");
      return false;
    }

    for (unsigned int index = 0; index < mb1->GetNumberOfBlocks(); index++)
    {
      vtkDataObject* mb1Block = mb1->GetBlock(index);
      vtkDataObject* mb2Block = mb2->GetBlock(index);

      if (!mb1Block || !mb2Block)
      {
        continue;
      }

      if (!vtkTestUtilities::CompareDataObjects(mb1Block, mb2Block, toleranceFactor))
      {
        return false;
      }
    }

    return true;
  }
};
//============================================================================
template <class DataObjectT, class = void>
struct TestPointsImpl;

//============================================================================
template <class DataObjectT>
struct TestPointsImpl<DataObjectT,
  typename std::enable_if<DataObjectTraits<DataObjectT>::HasPoints == true>::type>
{
  static bool Execute(DataObjectT* ds1, DataObjectT* ds2, double toleranceFactor)
  {
    if (!TestPoints(ds1, ds2, toleranceFactor))
    {
      vtkLog(ERROR,
        "Could not match points between the 2 input datasets of type " << ds1->GetClassName()
                                                                       << ".");
      return false;
    }
    return true;
  }
};

//============================================================================
template <class DataObjectT>
struct TestPointsImpl<DataObjectT,
  typename std::enable_if<DataObjectTraits<DataObjectT>::HasPoints == false>::type>
{
  static bool Execute(DataObjectT*, DataObjectT*, double)
  {
    vtkLog(ERROR, "There are no points in " << vtk::TypeName<DataObjectT>() << ".");
    return false;
  }
};

//============================================================================
template <class DataObjectT, class = void>
struct TestCellsImpl;

//============================================================================
template <class DataObjectT>
struct TestCellsImpl<DataObjectT,
  typename std::enable_if<DataObjectTraits<DataObjectT>::HasCells == true>::type>
{
  static bool Execute(DataObjectT* do1, DataObjectT* do2, double toleranceFactor)
  {
    if (!CellsTester<DataObjectT>::Execute(do1, do2, toleranceFactor))
    {
      vtkLog(ERROR,
        "Could not match cells between the 2 input data objects of type " << do1->GetClassName()
                                                                          << ".");
      return false;
    }
    return true;
  }
};

//============================================================================
template <class DataObjectT>
struct TestCellsImpl<DataObjectT,
  typename std::enable_if<DataObjectTraits<DataObjectT>::HasCells == false>::type>
{
  static bool Execute(DataObjectT*, DataObjectT*, double)
  {
    vtkLog(ERROR, "There are no cells in " << vtk::TypeName<DataObjectT>() << ".");
    return false;
  }
};

//----------------------------------------------------------------------------
template <template <class...> class ImplT, class DataObjectT>
bool DispatchDataObjectImpl(
  vtkDataObject* do1, vtkDataObject* do2, double toleranceFactor, bool& retVal)
{
  if (auto do1T = DataObjectT::SafeDownCast(do1))
  {
    if (auto do2T = DataObjectT::SafeDownCast(do2))
    {
      retVal = ImplT<DataObjectT>::Execute(do1T, DataObjectT::SafeDownCast(do2), toleranceFactor);
      return true;
    }
    vtkLog(ERROR,
      "Input dataset types do not match: " << do1->GetClassName() << " != " << do2->GetClassName());
  }
  return false;
}

//----------------------------------------------------------------------------
template <template <class...> class ImplT, class DataObjectT>
bool DispatchDataObjectImpl(
  vtkCompositeDataSet* do1, vtkCompositeDataSet* do2, double toleranceFactor, bool& retVal)
{
  if (auto pd1 = vtkPartitionedDataSet::SafeDownCast(do1))
  {
    if (auto pd2 = vtkPartitionedDataSet::SafeDownCast(do2))
    {
      retVal = ImplT<vtkPartitionedDataSet>::Execute(pd1, pd2, toleranceFactor);
      return true;
    }
    vtkLog(ERROR,
      "Input dataset types do not match: " << do1->GetClassName() << " != " << do2->GetClassName());
  }
  else if (auto pdc1 = vtkPartitionedDataSetCollection::SafeDownCast(do1))
  {
    if (auto pdc2 = vtkPartitionedDataSetCollection::SafeDownCast(do2))
    {
      retVal = ImplT<vtkPartitionedDataSetCollection>::Execute(pdc1, pdc2, toleranceFactor);
      return true;
    }
    vtkLog(ERROR,
      "Input dataset types do not match: " << do1->GetClassName() << " != " << do2->GetClassName());
  }
  else if (auto mb1 = vtkMultiBlockDataSet::SafeDownCast(do1))
  {
    if (auto mb2 = vtkMultiBlockDataSet::SafeDownCast(do2))
    {
      retVal = ImplT<vtkMultiBlockDataSet>::Execute(mb1, mb2, toleranceFactor);
      return true;
    }
    vtkLog(ERROR,
      "Input dataset types do not match: " << do1->GetClassName() << " != " << do2->GetClassName());
  }

  vtkLog(ERROR, << "Only vtkPartitionedDataSet, vtkPartitionedDataSetCollection and "
                   "vtkMultiBlockDataSet are supported "
                   "for now.");
  return false;
}

//----------------------------------------------------------------------------
template <template <class...> class ImplT>
bool DispatchDataObject(vtkDataObject* do1, vtkDataObject* do2, double toleranceFactor)
{
  bool retVal = false;
  if (DispatchDataObjectImpl<ImplT, vtkImageData>(do1, do2, toleranceFactor, retVal) ||
    DispatchDataObjectImpl<ImplT, vtkRectilinearGrid>(do1, do2, toleranceFactor, retVal) ||
    DispatchDataObjectImpl<ImplT, vtkStructuredGrid>(do1, do2, toleranceFactor, retVal) ||
    DispatchDataObjectImpl<ImplT, vtkExplicitStructuredGrid>(do1, do2, toleranceFactor, retVal) ||
    DispatchDataObjectImpl<ImplT, vtkPointSet>(do1, do2, toleranceFactor, retVal) ||
    DispatchDataObjectImpl<ImplT, vtkHyperTreeGrid>(do1, do2, toleranceFactor, retVal) ||
    DispatchDataObjectImpl<ImplT, vtkTable>(do1, do2, toleranceFactor, retVal))
  {
    return retVal;
  }
  vtkLog(ERROR, "vtkDataObject subtype is not supported or the 2 input types don't match.");
  return false;
}

//----------------------------------------------------------------------------
void FixToleranceFactorIfNeeded(double& toleranceFactor)
{
  if (toleranceFactor < 1.0)
  {
    vtkLog(WARNING, "toleranceFactor is below 1.0 which is not allowed... Setting it to 1.0.");
    toleranceFactor = 1.0;
  }
}
} // anonymous namespace

//----------------------------------------------------------------------------
bool vtkTestUtilities::CompareDataObjects(
  vtkDataObject* do1, vtkDataObject* do2, double toleranceFactor)
{
  ::FixToleranceFactorIfNeeded(toleranceFactor);

  if (auto cds = vtkCompositeDataSet::SafeDownCast(do1))
  {
    if (auto cds2 = vtkCompositeDataSet::SafeDownCast(do2))
    {
      bool retVal = false;
      if (::DispatchDataObjectImpl<TestDataObjectsImpl, vtkCompositeDataSet>(
            cds, cds2, toleranceFactor, retVal))
      {
        return retVal;
      }
    }

    return false;
  }

  return ::DispatchDataObject<TestDataObjectsImpl>(do1, do2, toleranceFactor);
}

//----------------------------------------------------------------------------
bool vtkTestUtilities::ComparePoints(vtkDataSet* ds1, vtkDataSet* ds2, double toleranceFactor)
{
  ::FixToleranceFactorIfNeeded(toleranceFactor);
  return ::DispatchDataObject<TestPointsImpl>(ds1, ds2, toleranceFactor);
}

//----------------------------------------------------------------------------
bool vtkTestUtilities::CompareCells(vtkDataObject* do1, vtkDataObject* do2, double toleranceFactor)
{
  ::FixToleranceFactorIfNeeded(toleranceFactor);
  return ::DispatchDataObject<TestCellsImpl>(do1, do2, toleranceFactor);
}

//----------------------------------------------------------------------------
bool vtkTestUtilities::CompareFieldData(
  vtkFieldData* fd1, vtkFieldData* fd2, double toleranceFactor)
{
  ::FixToleranceFactorIfNeeded(toleranceFactor);
  return ::TestFieldData(fd1, fd2, IdentityMapper(fd1->GetNumberOfTuples()), toleranceFactor);
}

//----------------------------------------------------------------------------
bool vtkTestUtilities::CompareAbstractArray(vtkAbstractArray* array1, vtkAbstractArray* array2,
  double toleranceFactor, vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip)
{
  ::FixToleranceFactorIfNeeded(toleranceFactor);
  return ::TestAbstractArray(array1, array2, ::IdentityMapper(array1->GetNumberOfTuples()),
    toleranceFactor, ghosts, ghostsToSkip);
}

VTK_ABI_NAMESPACE_END
