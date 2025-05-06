//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_CellSet_h
#define viskores_cont_CellSet_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/StaticAssert.h>

#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace cont
{

/// @brief Defines the topological structure of the data in a `DataSet`.
///
/// Fundamentally, any cell set is a collection of cells, which typically (but not always)
/// represent some region in space.
class VISKORES_CONT_EXPORT CellSet
{
public:
  CellSet() = default;
  CellSet(const CellSet&) = default;
  CellSet(CellSet&&) noexcept = default;

  CellSet& operator=(const CellSet&) = default;
  CellSet& operator=(CellSet&&) noexcept = default;

  virtual ~CellSet();

  /// @brief Get the number of cells in the topology.
  virtual viskores::Id GetNumberOfCells() const = 0;

  virtual viskores::Id GetNumberOfFaces() const = 0;

  virtual viskores::Id GetNumberOfEdges() const = 0;

  /// @brief Get the number of points in the topology.
  virtual viskores::Id GetNumberOfPoints() const = 0;

  /// @brief Get the shell shape of a particular cell.
  virtual viskores::UInt8 GetCellShape(viskores::Id id) const = 0;
  /// @brief Get the number of points incident to a particular cell.
  virtual viskores::IdComponent GetNumberOfPointsInCell(viskores::Id id) const = 0;
  /// @brief Get a list of points incident to a particular cell.
  virtual void GetCellPointIds(viskores::Id id, viskores::Id* ptids) const = 0;

  /// @brief Return a new `CellSet` that is the same derived class.
  virtual std::shared_ptr<CellSet> NewInstance() const = 0;
  /// @brief Copy the provided `CellSet` into this object.
  ///
  /// The provided `CellSet` must be the same type as this one.
  virtual void DeepCopy(const CellSet* src) = 0;

  /// @brief Print a summary of this cell set.
  virtual void PrintSummary(std::ostream&) const = 0;

  /// @brief Remove the `CellSet` from any devices.
  ///
  /// Any memory used on a device to store this object will be deleted.
  /// However, the data will still remain on the host.
  virtual void ReleaseResourcesExecution() = 0;
};

namespace internal
{

/// Checks to see if the given object is a cell set. It contains a
/// typedef named \c type that is either std::true_type or
/// std::false_type. Both of these have a typedef named value with the
/// respective boolean value.
///
template <typename T>
struct CellSetCheck
{
  using U = typename std::remove_pointer<T>::type;
  using type = typename std::is_base_of<viskores::cont::CellSet, U>::type;
};

#define VISKORES_IS_CELL_SET(T) \
  VISKORES_STATIC_ASSERT(::viskores::cont::internal::CellSetCheck<T>::type::value)

} // namespace internal
}
} // namespace viskores::cont

#endif //viskores_cont_CellSet_h
