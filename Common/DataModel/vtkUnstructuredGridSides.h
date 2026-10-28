// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridSides
 * @brief   the sides of the cells of a vtkUnstructuredGrid, matched without cell links
 *
 * vtkUnstructuredGridSides finds the sides of the cells of a vtkUnstructuredGrid (their faces,
 * edges or vertices), and keeps some of them depending on how many cells share them, without
 * building cell links. As in vtkCellGridComputeSides (see vtkCellGridSidesQuery), the sides to find
 * are selected with SideFlags, e.g., the faces of the 3D cells, and the sides to keep with
 * SummaryStrategy, e.g., the boundary. The sides are scattered by their key, their smallest point
 * id, as with vtk::keys, and are built in one of three ways:
 * - BuildScatteredSides(): all the sides, by key, not matched, e.g., to match them in another way;
 * - BuildReducedSides(): the kept sides, by key, see vtk::keys::Reduce(), e.g., the boundary;
 * - BuildGroupedSides(): all the sides, by key, the equal sides of every key contiguous, in groups,
 *   see vtk::keys::Group(), e.g., a face of a mesh and the cells that share it.
 * Each side is given by a cell that has it, its local index among the sides of its dimension in
 * the cell, or -1 for the cell itself (see SetPreserveRenderableInputs()), and its dimension (0 for
 * a vertex, 1 for an edge, 2 for a face). The sides are built again only when this object or the
 * dataset is modified, or to build them in another way, as by vtkLocator::BuildLocator().
 *
 * @code{.cpp}
 * // The boundary faces of the 3D cells of a grid.
 * using Sides = vtkUnstructuredGridSides<vtkTypeInt32>;
 * vtkNew<Sides> sides;
 * sides->SetDataSet(grid);
 * sides->SetOutputDimensionControl(Sides::SideFlags::SurfacesOfVolumes);
 * sides->BuildReducedSides();
 * for (vtkTypeInt32 keyId = 0; keyId < sides->GetNumberOfKeys(); ++keyId)
 * {
 *   for (vtkIdType k = 0; k < sides->GetNumberOfSidesOfKey(keyId); ++k)
 *   {
 *     // The face sides->GetSideIdOfSide(keyId, k) of the cell sides->GetCellIdOfSide(keyId, k),
 *     // whose type and size are given by sides->GetSideInformation(keyId, k, cell), and whose
 *     // points are given by sides->GetSidePoints(keyId, k, ...).
 *   }
 * }
 * // The faces of the 3D cells of a grid, each with the cells that share it.
 * sides->BuildGroupedSides();
 * for (vtkTypeInt32 keyId = 0; keyId < sides->GetNumberOfKeys(); ++keyId)
 * {
 *   for (vtkIdType group = sides->GetGroupOffsetOfKey(keyId);
 *        group < sides->GetGroupOffsetOfKey(keyId + 1); ++group)
 *   {
 *     // The face `group`, whose sides are k in [first, first + sides->GetNumberOfSidesOfGroup(
 *     // group)) of the key, where first = sides->GetSideOffsetOfGroup(group) -
 *     // sides->GetSideOffsetOfKey(keyId).
 *   }
 * }
 * @endcode
 *
 * The sides are scattered by their key, their smallest point id, with vtk::keys::ScatterDense(), as
 * the faces of DP-Hash-Count (see below): only their cell and local index are stored, and their
 * points are computed again from the cell arrays of the dataset. The sides of a key are in the
 * order of their cells and of their local index, whatever the threads, so that the scattered,
 * reduced and grouped sides are the same from one run to the next. Two sides of a key match if they
 * have the same corners in the same cyclic order, in either direction, and, for the nonlinear
 * sides, the same other points, e.g., the mid-edge points of the faces of the quadratic cells,
 * unless SetMatchSidePoints() is off. A degenerate side whose smallest corner is repeated, e.g., a
 * face of a collapsed hexahedron, is compared in a canonical form of its corners.
 *
 * Cells can be masked, as in vtkGeometryFilter: the cells clipped by their ids, by their point ids
 * or by an extent (see SetCellClipping(), SetPointClipping() and SetExtentClipping()), and the
 * masked ghost cells, hidden or refined by default (see SetMaskedGhostTypes()), take no part. With
 * SetRemoveGhostInterfaces(), the duplicate ghost cells match the sides of the other cells, so that
 * the interfaces between processes are not boundaries, but their own sides are not kept;
 * otherwise, they are ordinary cells.
 *
 * The class is templated on the type of the ids, TId, vtkTypeInt32 or vtkTypeInt64, which must
 * hold the number of points and the number of cells, e.g., vtkTypeInt32 halves the memory when
 * they fit, and the type of the local side indices, TSideId, vtkTypeInt8 or vtkTypeInt32, which
 * must hold the local index of every side among the sides of its dimension, which is only asserted:
 * vtkTypeInt32 is needed when a cell can have more than 128 sides of a dimension, i.e., the faces
 * or edges of polyhedra and the faces of convex point sets, or the edges of polygons and triangle
 * strips and the vertices of polylines and poly-vertices with many points. These 4 combinations
 * are instantiated. The positions of the sides in their keys, and the groups, are vtkIdType.
 *
 * @warning
 * The edges and vertices of a cell are those of its vtkCell, e.g., all the points of a nonlinear
 * cell are its vertices. The faces of a 3D cell are those of
 * vtkUnstructuredGrid::GetCellFacePoints(), e.g., read from the face arrays for a polyhedron.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other non-sequential type (set in
 * the CMake variable VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @par Reference:
 * S. Tsalikis, W. Schroeder, D. Szafir, and K. Moreland, "Memory-Aware External
 * Facelist Calculation: A Data-Parallel Atomic Hash Counting Approach," IEEE Transactions
 * on Visualization and Computer Graphics, vol. 32, no. 6, pp. 4862-4871, 2026,
 * doi: 10.1109/TVCG.2026.3694435.
 *
 * @sa
 * vtkCellGridComputeSides vtkCellGridSidesQuery
 */
#ifndef vtkUnstructuredGridSides_h
#define vtkUnstructuredGridSides_h

#include "vtkCellGridSidesQuery.h"    // For SideFlags and SummaryStrategy
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSetAttributes.h"     // For HIDDENCELL and REFINEDCELL
#include "vtkKeys.h"                  // For DenseKeys and ReducedKeys
#include "vtkObject.h"
#include "vtkType.h" // For vtkIdType, VTK_ID_MAX and VTK_DOUBLE_MAX

#include <cstdint> // For std::uint8_t
#include <memory>  // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericCell;
class vtkIdList;
class vtkUnstructuredGrid;

template <typename TId = vtkIdType, typename TSideId = vtkTypeInt8>
class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridSides : public vtkObject
{
public:
  using SelfType = vtkUnstructuredGridSides<TId, TSideId>;
  static SelfType* New();
  vtkTemplateTypeMacro(SelfType, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Which sides of which cells to find, see vtkCellGridSidesQuery::SideFlags, whose bits can be
   * combined.
   */
  using SideFlags = vtkCellGridSidesQuery::SideFlags;

  /**
   * Which sides to keep, see vtkCellGridSidesQuery::SummaryStrategy.
   */
  using SummaryStrategy = vtkCellGridSidesQuery::SummaryStrategy;

  ///@{
  /**
   * The dataset whose sides are built, which is referenced. The sides are built again when it is
   * modified.
   */
  virtual void SetDataSet(vtkUnstructuredGrid* dataSet);
  vtkGetObjectMacro(DataSet, vtkUnstructuredGrid);
  ///@}

  ///@{
  /**
   * Whether the cells of dimension 2 or less are preserved, as sides with a side id of -1, in the
   * last key, in the order of their ids, without being matched, and in no group. Off by default.
   */
  vtkSetMacro(PreserveRenderableInputs, bool);
  vtkGetMacro(PreserveRenderableInputs, bool);
  vtkBooleanMacro(PreserveRenderableInputs, bool);
  ///@}

  ///@{
  /**
   * Whether no side is found for the cells of dimension 2 or less. Off by default.
   */
  vtkSetMacro(OmitSidesForRenderableInputs, bool);
  vtkGetMacro(OmitSidesForRenderableInputs, bool);
  vtkBooleanMacro(OmitSidesForRenderableInputs, bool);
  ///@}

  ///@{
  /**
   * Which sides of which cells to find: any combination of the bits of SideFlags, e.g.,
   * SideFlags::VerticesOfEdges | SideFlags::EdgesOfVolumes, as
   * vtkCellGridSidesQuery::SetOutputDimensionControl(). NextLowestDimension by default.
   */
  vtkSetClampMacro(OutputDimensionControl, int, 0, SideFlags::AllSides);
  vtkGetMacro(OutputDimensionControl, int);
  ///@}

  ///@{
  /**
   * Which sides to keep, see BuildReducedSides(). Boundary by default.
   */
  vtkSetMacro(Strategy, SummaryStrategy);
  void SetStrategyToWinding() { this->SetStrategy(SummaryStrategy::Winding); }
  void SetStrategyToAnyOccurrence() { this->SetStrategy(SummaryStrategy::AnyOccurrence); }
  void SetStrategyToBoundary() { this->SetStrategy(SummaryStrategy::Boundary); }
  /// This method exists for ParaView to set the strategy.
  virtual void SetStrategy(int strategy)
  {
    this->SetStrategy(static_cast<SummaryStrategy>(strategy));
  }
  vtkGetMacro(Strategy, SummaryStrategy);
  ///@}

  ///@{
  /**
   * Turn on/off the clipping of the cells by their point ids: a cell takes no part if one of its
   * points is not in [PointMinimum, PointMaximum]. Off by default.
   */
  vtkSetMacro(PointClipping, bool);
  vtkGetMacro(PointClipping, bool);
  vtkBooleanMacro(PointClipping, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the clipping of the cells by their ids: a cell takes no part if its id is not in
   * [CellMinimum, CellMaximum]. Off by default.
   */
  vtkSetMacro(CellClipping, bool);
  vtkGetMacro(CellClipping, bool);
  vtkBooleanMacro(CellClipping, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the clipping of the cells by an extent: a cell takes no part if one of its points
   * is not in the Extent. Off by default.
   */
  vtkSetMacro(ExtentClipping, bool);
  vtkGetMacro(ExtentClipping, bool);
  vtkBooleanMacro(ExtentClipping, bool);
  ///@}

  ///@{
  /**
   * The range of the point ids of the point clipping, see SetPointClipping(). [0, VTK_ID_MAX] by
   * default.
   */
  vtkSetClampMacro(PointMinimum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(PointMinimum, vtkIdType);
  vtkSetClampMacro(PointMaximum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(PointMaximum, vtkIdType);
  ///@}

  ///@{
  /**
   * The range of the cell ids of the cell clipping, see SetCellClipping(). [0, VTK_ID_MAX] by
   * default.
   */
  vtkSetClampMacro(CellMinimum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(CellMinimum, vtkIdType);
  vtkSetClampMacro(CellMaximum, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(CellMaximum, vtkIdType);
  ///@}

  ///@{
  /**
   * The (xmin, xmax, ymin, ymax, zmin, zmax) bounding box of the extent clipping, see
   * SetExtentClipping(). Unbounded by default.
   */
  vtkSetVector6Macro(Extent, double);
  vtkGetVector6Macro(Extent, double);
  ///@}

  ///@{
  /**
   * Whether the duplicate ghost cells match the sides of the other cells, so that the interfaces
   * between processes are not boundaries. On by default.
   */
  vtkSetMacro(RemoveGhostInterfaces, bool);
  vtkGetMacro(RemoveGhostInterfaces, bool);
  vtkBooleanMacro(RemoveGhostInterfaces, bool);
  ///@}

  ///@{
  /**
   * The ghost types that mask a cell, see vtkDataSetAttributes::CellGhostTypes: a cell with one of
   * them takes no part. vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL by
   * default, as in vtkGeometryFilter; 0 masks no ghost cell.
   */
  vtkSetMacro(MaskedGhostTypes, unsigned char);
  vtkGetMacro(MaskedGhostTypes, unsigned char);
  ///@}

  ///@{
  /**
   * Whether the matched nonlinear sides must also have the same other points than their corners,
   * in any order, e.g., their mid-edge points: a face of a quadratic cell then does not match a
   * face of a linear cell with the same corners, nor faces of Lagrange cells of different orders or
   * with distinct mid-edge points, so that both are kept by Boundary, since their interface is not
   * conforming. The linear sides match by their corners, which are all their points. On by
   * default. Off, the sides match by their corners only, e.g., those of cells of different orders.
   */
  vtkSetMacro(MatchSidePoints, bool);
  vtkGetMacro(MatchSidePoints, bool);
  vtkBooleanMacro(MatchSidePoints, bool);
  ///@}

  /**
   * How the sides were built, see GetBuildMode().
   */
  enum class BuildMode : std::uint8_t
  {
    None,      //!< Not built, e.g., after Reset().
    Scattered, //!< BuildScatteredSides().
    Reduced,   //!< BuildReducedSides().
    Grouped    //!< BuildGroupedSides().
  };

  /**
   * Find the sides of the cells given by OutputDimensionControl, and scatter all of them by their
   * key, their smallest point id, not matched, e.g., to match them in another way. Nothing is done
   * if they were scattered after the last modification of this object and of the dataset.
   */
  void BuildScatteredSides();

  /**
   * Same as BuildScatteredSides(), then match the sides of every key, and keep them as given by the
   * summary strategy: e.g., the boundary of the dataset. The keys then hold the kept sides only.
   * Nothing is done if they were reduced after the last modification of this object and of the
   * dataset.
   */
  void BuildReducedSides();

  /**
   * Same as BuildScatteredSides(), then match the sides of every key, and keep all of them, the
   * equal sides of a key contiguous, as a group: e.g., a face of the dataset and the cells that
   * share it. The groups of a key are in the order of their first sides, and the sides of a group
   * in the order of their cells. The summary strategy is not used, and the sides of the duplicate
   * ghost cells are in the groups like the others. Nothing is done if they were grouped after the
   * last modification of this object and of the dataset.
   */
  void BuildGroupedSides();

  /**
   * How the sides were last built, None if they were not, e.g., after Reset().
   */
  BuildMode GetBuildMode() const { return this->Mode; }

  /**
   * Release the memory, so that the sides are built again.
   */
  void Reset();

  /**
   * The time at which the sides were built.
   */
  vtkGetMacro(BuildTime, vtkMTimeType);

  /**
   * The dataset is reported to the garbage collector, see ReportReferences().
   */
  bool UsesGarbageCollector() const override { return true; }

  //------------------------------------------------------------------------------
  // The sides, after any build: by key, and by their index k in their key. BuildScatteredSides()
  // and BuildReducedSides() have no getters of their own.
  //------------------------------------------------------------------------------

  /**
   * The number of sides: the kept sides after BuildReducedSides(), all the sides otherwise.
   */
  vtkIdType GetNumberOfSides() const;

  /**
   * The number of keys: the number of points plus one if some sides are found, otherwise one,
   * e.g., before the sides are built. A key is the smallest point id of its sides, and its key id,
   * as in vtk::keys, its position, which is the same, since the keys start at point 0: key id p
   * has the sides whose smallest point id is p. The last key id, GetNumberOfKeys() - 1, has the
   * preserved cells (see SetPreserveRenderableInputs()), which have no key.
   */
  TId GetNumberOfKeys() const;

  /**
   * The number of sides of a key, as GetNumberOfSides(), 0 <= keyId < GetNumberOfKeys().
   */
  vtkIdType GetNumberOfSidesOfKey(TId keyId) const;

  /**
   * The offset of a key, 0 <= keyId <= GetNumberOfKeys(): the number of sides of the keys before
   * it, e.g., to write the sides of the keys in parallel, as the offsets of a vtkCellArray.
   */
  vtkIdType GetSideOffsetOfKey(TId keyId) const;

  /**
   * The cells of the sides of a key, of GetNumberOfSidesOfKey() values, 0 <= keyId <
   * GetNumberOfKeys().
   */
  const TId* GetCellIdsOfKey(TId keyId) const;

  /**
   * The local indices of the sides of a key in their cell, of GetNumberOfSidesOfKey() values, -1
   * for a preserved cell.
   */
  const TSideId* GetSideIdsOfKey(TId keyId) const;

  ///@{
  /**
   * The cell, the local index in its cell, and the dimension of the side k of a key,
   * 0 <= k < GetNumberOfSidesOfKey(keyId).
   */
  TId GetCellIdOfSide(TId keyId, vtkIdType k) const;
  TSideId GetSideIdOfSide(TId keyId, vtkIdType k) const;
  int GetDimensionOfSide(TId keyId, vtkIdType k) const;
  ///@}

  /**
   * The information of a side, see GetSideInformation().
   */
  struct SideInformation
  {
    vtkIdType CellId;       // the cell of the side
    unsigned char CellType; // the cell type of its cell
    int SideId;             // its local index in its cell, -1 for a preserved cell
    int SideType;           // its cell type, e.g., VTK_QUADRATIC_QUAD for a face of a quadratic hex
    int SideDimension;      // its dimension, the one of its cell for a preserved cell
    int SideSize;           // its number of points
  };

  /**
   * The information of the side k of a key, without reading its points: its type and size are
   * given by vtkCellTypeSides::GetCellSideType() and vtkCellTypeSides::GetCellSideSize(), those of
   * a preserved cell being its own. It is thread-safe, with a scratch cell of the calling thread.
   *
   * @param keyId The key id of the side, 0 <= keyId < GetNumberOfKeys().
   * @param k The index of the side in its key, 0 <= k < GetNumberOfSidesOfKey(keyId).
   * @param tempCell Scratch vtkGenericCell, for the cells whose sides are built, e.g., higher-order
   * cells.
   */
  SideInformation GetSideInformation(TId keyId, vtkIdType k, vtkGenericCell* tempCell) const;

  /**
   * Get the point ids of the side k of a key, in the order of the side in its cell, from the cell
   * arrays of the dataset: a vertex is a point of its cell, an edge and a face are given as by
   * vtkUnstructuredGrid::GetCellEdgePoints() and vtkUnstructuredGrid::GetCellFacePoints(), and a
   * preserved cell by its points. It is thread-safe, with scratch objects of the calling thread.
   *
   * @param keyId The key id of the side, 0 <= keyId < GetNumberOfKeys().
   * @param k The index of the side in its key, 0 <= k < GetNumberOfSidesOfKey(keyId).
   * @param sidePtIds Output vtkIdList populated with the point ids of the side.
   * @param cellType Output parameter returning the type of the cell of the side.
   * @param tempCellPointIds Scratch vtkIdList used to retrieve the point ids of the cell.
   * @param tempCell Scratch vtkGenericCell, for the cells whose sides are built, e.g., higher-order
   * cells and the edges of the polyhedra.
   */
  void GetSidePoints(TId keyId, vtkIdType k, vtkIdList* sidePtIds, unsigned char& cellType,
    vtkIdList* tempCellPointIds, vtkGenericCell* tempCell) const;

  //------------------------------------------------------------------------------
  // The groups of equal sides, after BuildGroupedSides() only, none otherwise.
  //------------------------------------------------------------------------------

  ///@{
  /**
   * The groups of equal sides, ordered by key: their number, e.g., the number of distinct faces,
   * and the first group of a key, 0 <= keyId <= GetNumberOfKeys(), so that the groups of a key are
   * [GetGroupOffsetOfKey(keyId), GetGroupOffsetOfKey(keyId + 1)). The preserved cells, in the last
   * key, are in no group.
   */
  vtkIdType GetNumberOfGroups() const;
  vtkIdType GetGroupOffsetOfKey(TId keyId) const;
  ///@}

  ///@{
  /**
   * The number of sides of a group, 0 <= group < GetNumberOfGroups(), and its first side,
   * 0 <= group <= GetNumberOfGroups(), among all the sides: the first side of the group is
   * k = GetSideOffsetOfGroup(group) - GetSideOffsetOfKey(keyId) of its key.
   */
  vtkIdType GetNumberOfSidesOfGroup(vtkIdType group) const;
  vtkIdType GetSideOffsetOfGroup(vtkIdType group) const;
  ///@}

  ///@{
  /**
   * The cells and the local indices in their cell of the sides of a group, of
   * GetNumberOfSidesOfGroup() values, 0 <= group < GetNumberOfGroups(), e.g., the cells that share
   * a face, read without its key. The sides of all the groups are contiguous, so that
   * GetCellIdsOfGroup(0)[GetSideOffsetOfGroup(group) + i] is side i of a group.
   */
  const TId* GetCellIdsOfGroup(vtkIdType group) const;
  const TSideId* GetSideIdsOfGroup(vtkIdType group) const;
  ///@}

  /**
   * The dimension of the sides of a group, 0 <= group < GetNumberOfGroups(), which all have the
   * same one, since equal sides have the same number of points.
   */
  int GetDimensionOfGroup(vtkIdType group) const;

protected:
  vtkUnstructuredGridSides();
  ~vtkUnstructuredGridSides() override;

  vtkTimeStamp BuildTime; // time at which the sides were built

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkUnstructuredGridSides(const vtkUnstructuredGridSides&) = delete;
  void operator=(const vtkUnstructuredGridSides&) = delete;

  // Find and scatter the sides, see BuildScatteredSides(), and reduce or group them as given by
  // the mode.
  void BuildSides(BuildMode mode);

  // Where the sides of a key start in their arrays, which keep the kept sides of a key at the
  // front of its sides, with gaps between the keys, unlike GetSideOffsetOfKey(): the last key, of
  // the preserved cells, starts after all the found sides.
  vtkIdType GetStorageOffsetOfKey(TId keyId) const;

  // The steps of BuildSides(), defined in the .cxx file, nested so that they read the settings
  // directly.
  template <class OffsetsT, class ConnectivityT>
  struct InputCells;
  template <class InputCellsT>
  struct SideMapper;
  template <class InputCellsT, bool Faces>
  struct MatchingFunctor;
  struct PreservedCells;
  struct BuildSidesWorker;

  // The settings.
  vtkUnstructuredGrid* DataSet = nullptr;
  bool PreserveRenderableInputs = false;
  bool OmitSidesForRenderableInputs = false;
  int OutputDimensionControl = SideFlags::NextLowestDimension;
  SummaryStrategy Strategy = SummaryStrategy::Boundary;
  bool PointClipping = false;
  bool CellClipping = false;
  bool ExtentClipping = false;
  vtkIdType PointMinimum = 0;
  vtkIdType PointMaximum = VTK_ID_MAX;
  vtkIdType CellMinimum = 0;
  vtkIdType CellMaximum = VTK_ID_MAX;
  double Extent[6] = { -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX,
    -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  bool RemoveGhostInterfaces = true;
  unsigned char MaskedGhostTypes =
    vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;
  bool MatchSidePoints = true;

  // The sides, scattered by their key, their smallest point id (ScatteredKeys), the number of kept
  // sides at the front of the sides of every key once reduced (ReducedKeys), and the groups of
  // equal sides and the number of groups of every key once grouped (Groups): their cell, local
  // index and dimension, unless they all have the dimension SideDimension. The cell and local index
  // of the preserved cells are after them. BuiltKeys, the keys of the built sides, which the
  // getters read, is ScatteredKeys, or ReducedKeys once reduced.
  using KeysType = vtk::keys::DenseKeys<TId>;
  KeysType ScatteredKeys;
  vtk::keys::ReducedKeys<KeysType> ReducedKeys;
  vtk::keys::Grouping<KeysType> Groups;
  const KeysType* BuiltKeys = &this->ScatteredKeys;
  BuildMode Mode = BuildMode::None;
  std::unique_ptr<TId[]> CellIds;
  std::unique_ptr<TSideId[]> SideIds;
  std::unique_ptr<std::uint8_t[]> Dimensions;
  std::uint8_t SideDimension = 0;

  // The number of preserved cells, in the last key, in the order of their ids: they are not
  // scattered, since they are not matched.
  vtkIdType NumberOfPreservedCells = 0;
};

VTK_ABI_NAMESPACE_END
#endif // vtkUnstructuredGridSides_h

#if defined(VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATING)

#define VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATE(TId, TSideId)                                      \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridSides<TId, TSideId>;                 \
  VTK_ABI_NAMESPACE_END

// MinGW GCC does not reliably export the inline member functions of vtkTemplateTypeMacro with
// extern template, see vtkAOSDataArrayTemplate.h.
#elif defined(VTK_USE_EXTERN_TEMPLATE) && !defined(__MINGW32__)

#ifndef VTK_UNSTRUCTURED_GRID_SIDES_TEMPLATE_EXTERN
#define VTK_UNSTRUCTURED_GRID_SIDES_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when vtkUnstructuredGridSides is declared dllexport and is used from
// another class in vtkCommonDataModel.
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
extern template class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridSides<vtkTypeInt32, vtkTypeInt8>;
extern template class VTKCOMMONDATAMODEL_EXPORT
  vtkUnstructuredGridSides<vtkTypeInt32, vtkTypeInt32>;
extern template class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridSides<vtkTypeInt64, vtkTypeInt8>;
extern template class VTKCOMMONDATAMODEL_EXPORT
  vtkUnstructuredGridSides<vtkTypeInt64, vtkTypeInt32>;
VTK_ABI_NAMESPACE_END
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_UNSTRUCTURED_GRID_SIDES_TEMPLATE_EXTERN

#endif
// VTK-HeaderTest-Exclude: vtkUnstructuredGridSides.h
