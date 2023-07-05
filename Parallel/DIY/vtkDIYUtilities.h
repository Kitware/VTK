// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDIYUtilities
 * @brief collection of helper functions for working with DIY
 *
 * vtkDIYUtilities provides a set of utility functions when using DIY in a VTK
 * filters.
 */
#ifndef vtkDIYUtilities_h
#define vtkDIYUtilities_h

#include "vtkObject.h"
#include "vtkParallelDIYModule.h" // for export macros
#include "vtkSmartPointer.h"      // needed for vtkSmartPointer

#include <map>    // For Link
#include <set>    // For Link
#include <vector> // For GetDataSets

// clang-format off
#include "vtk_diy2.h" // needed for DIY
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/serialization.hpp)
#include VTK_DIY2(diy/types.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkBoundingBox;
class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkMultiProcessController;
class vtkPoints;
class vtkStringArray;

class VTKPARALLELDIY_EXPORT vtkDIYUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkDIYUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * In MPI-enabled builds, DIY filters need MPI to be initialized.
   * Calling this method in such filters will ensure that that's the case.
   */
  static void InitializeEnvironmentForDIY();

  /**
   * Converts a vtkMultiProcessController to a diy::mpi::communicator.
   * If controller is nullptr or not a vtkMPIController, then
   * diy::mpi::communicator(MPI_COMM_NULL) is created.
   */
  static diy::mpi::communicator GetCommunicator(vtkMultiProcessController* controller);

  ///@{
  /**
   * Load/Save a vtkDataSet in a diy::BinaryBuffer.
   */
  static void Save(diy::BinaryBuffer& bb, vtkDataSet*);
  static void Load(diy::BinaryBuffer& bb, vtkDataSet*&);
  ///@}

  ///@{
  /**
   * Load/Save a vtkFieldData in a diy::BinaryBuffer.
   */
  static void Save(diy::BinaryBuffer& bb, vtkFieldData*);
  static void Load(diy::BinaryBuffer& bb, vtkFieldData*&);
  ///@}

  ///@{
  /**
   * Load/Save a vtkStringArray in a diy::BinaryBuffer.
   */
  static void Save(diy::BinaryBuffer& bb, vtkStringArray*);
  static void Load(diy::BinaryBuffer& bb, vtkStringArray*&);
  ///@}

  ///@{
  /**
   * Load/Save a vtkDataArray in a diy::BinaryBuffer.
   */
  static void Save(diy::BinaryBuffer& bb, vtkDataArray*);
  static void Load(diy::BinaryBuffer& bb, vtkDataArray*&);
  ///@}

  /**
   * Reduce bounding box.
   */
  static void AllReduce(diy::mpi::communicator& comm, vtkBoundingBox& bbox);

  /**
   * Convert vtkBoundingBox to diy::ContinuousBounds.
   *
   * Note, there is a loss of precision since vtkBoundingBox uses `double` while
   * diy::ContinuousBounds uses float.
   */
  static diy::ContinuousBounds Convert(const vtkBoundingBox& bbox);

  /**
   * Convert diy::ContinuousBounds to vtkBoundingBox.
   *
   * Note, there is a change of precision since vtkBoundingBox uses `double` while
   * diy::ContinuousBounds uses float.
   */
  static vtkBoundingBox Convert(const diy::ContinuousBounds& bds);

  /**
   * Broadcast a vector of bounding boxes. Only the source vector needs to have
   * a valid size.
   */
  static void Broadcast(
    diy::mpi::communicator& comm, std::vector<vtkBoundingBox>& boxes, int source);

  ///@{
  /**
   * Extracts points from the input. If input is not a vtkPointSet, it will use
   * an appropriate filter to extract the vtkPoints. If use_cell_centers is
   * true, cell-centers will be computed and extracted instead of the dataset
   * points.
   */
  static std::vector<vtkSmartPointer<vtkPoints>> ExtractPoints(
    const std::vector<vtkDataSet*>& datasets, bool use_cell_centers);
  ///@}

  /**
   * Convenience method to get local bounds for the data object.
   */
  static vtkBoundingBox GetLocalBounds(vtkDataObject* dobj);

  /**
   * Links master such that there is communication between ranks as given
   * in `linksMap`.
   * `linksMap` is a vector of a list of global ids. The size of this vector should be the same as
   * the number of blocks in the current rank and should map to the block of same local id.
   * The associated list of global ids will tell which block is to be connected with the local
   * block.
   */
  template <class DummyT>
  static void Link(diy::Master& master, const diy::Assigner& assigner,
    const std::vector<std::map<int, DummyT>>& linksMap);

  static void Link(
    diy::Master& master, const diy::Assigner& assigner, const std::vector<std::set<int>>& linksMap);

protected:
  vtkDIYUtilities();
  ~vtkDIYUtilities() override;

private:
  vtkDIYUtilities(const vtkDIYUtilities&) = delete;
  void operator=(const vtkDIYUtilities&) = delete;
};
VTK_ABI_NAMESPACE_END

namespace diy
{
template <>
struct Serialization<vtkDataSet*>
{
  static void save(BinaryBuffer& bb, vtkDataSet* const& p) { vtkDIYUtilities::Save(bb, p); }
  static void load(BinaryBuffer& bb, vtkDataSet*& p) { vtkDIYUtilities::Load(bb, p); }
};

template <>
struct Serialization<vtkDataArray*>
{
  static void save(BinaryBuffer& bb, vtkDataArray* const& da) { vtkDIYUtilities::Save(bb, da); }
  static void load(BinaryBuffer& bb, vtkDataArray*& da) { vtkDIYUtilities::Load(bb, da); }
};

template <>
struct Serialization<vtkFieldData*>
{
  static void save(BinaryBuffer& bb, vtkFieldData* const& fd) { vtkDIYUtilities::Save(bb, fd); }
  static void load(BinaryBuffer& bb, vtkFieldData*& fd) { vtkDIYUtilities::Load(bb, fd); }
};
}

VTK_ABI_NAMESPACE_BEGIN
// Implementation detail for Schwarz counter idiom.
class VTKPARALLELDIY_EXPORT vtkDIYUtilitiesCleanup
{
public:
  vtkDIYUtilitiesCleanup();
  ~vtkDIYUtilitiesCleanup();

private:
  vtkDIYUtilitiesCleanup(const vtkDIYUtilitiesCleanup&) = delete;
  void operator=(const vtkDIYUtilitiesCleanup&) = delete;
};
static vtkDIYUtilitiesCleanup vtkDIYUtilitiesCleanupInstance;

VTK_ABI_NAMESPACE_END
#include "vtkDIYUtilities.txx" // for template implementations

#endif
