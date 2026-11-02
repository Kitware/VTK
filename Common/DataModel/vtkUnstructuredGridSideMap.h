// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridSideMap
 * @brief   the numbering of the sides shared between the cells of a vtkUnstructuredGrid
 *
 * vtkUnstructuredGridSideMap gives a vtkUnstructuredGrid a dense numbering of
 * its sides, so that a side shared by two cells has one id and therefore one
 * tuple in the grid's side data, a vtkSideData reached through
 * vtkUnstructuredGrid::GetSideData(). It does this without storing any side-to-point
 * connectivity: a side's points are recovered from the cell that owns it,
 * through vtkCellTypeSides.
 *
 * The addressing is by half-side. A half-side is a (cell, local side) pair,
 * and its id is arithmetic:
 *
 *   halfSide = CellSideOffsets[cellId] + localSide
 *
 * Half-sides are implicit in a vtkUnstructuredGrid already; what has to be
 * stored is which of them name the same side. That is HalfSideToSide, one
 * dense id per half-side, and the inverse, which lists the half-sides of each
 * side and whose first entry is the side's representative.
 *
 * Every query is O(1):
 *
 *   GetCellSide(cellId, localSide)  -> side id
 *   GetSidePoints(sideId, ptIds)    -> the representative's winding
 *   GetSideCells(sideId, cellIds)   -> the incident cells
 *   GetSideOrientation(cellId, localSide) -> +1 or -1
 *
 * VTK's canonical face and edge templates are consistently outward-oriented, so
 * the two cells sharing a side traverse it in opposite directions.
 * GetSidePoints() returns the representative cell's traversal, which is that
 * cell's outward winding; the other cell sees it reversed.
 * GetSideOrientation() reports which case a given cell is in, so no
 * orientation array is stored.
 *
 * Side data with a sign, such as a flux or the normal component of a velocity,
 * is relative to the representative cell's outward normal, the winding
 * GetSidePoints() returns. A reader whose file measures it from another cell
 * negates the values of the sides where GetSideOrientation() is -1 for that
 * cell.
 *
 * The structure costs nothing until asked for. BuildMap() runs on demand and is
 * re-run when the grid's topology changes. Two modes:
 *
 * - Derived. Nobody supplies a numbering, so BuildMap() takes the grouped sides
 *   of vtkUnstructuredGridSides: each group of equal cell sides is one side
 *   here, and its members are that side's half-sides. Two half-sides match when
 *   they have the same corners in cyclic order, either way, and the same other
 *   points, e.g., the mid-edge nodes of a quadratic face. Purely a cache: the
 *   numbering is unobservable, and may be dropped and recomputed freely.
 * - Authored. SetSideIds() supplies the numbering, typically a reader passing
 *   through the side ids the file already uses. No matching pass runs. The
 *   numbering is now load-bearing, because the side data is indexed by it, so
 *   it is data rather than cache and must be carried, not recomputed.
 *
 * Once the side data holds anything, the numbering is pinned either way: a fresh
 * derived build would be a different permutation and would silently transpose
 * every value. A change of topology drops the numbering and the side data
 * together, with a warning.
 *
 * Both 3D and 2D cells take part. A 3D cell's sides are its faces; a 2D cell's
 * sides are its edges. Either way they are the codimension-one boundary, so a
 * side id means the same kind of thing in a volume mesh and in a surface mesh,
 * and a surface mesh carries data on its edges the same way a volume mesh
 * carries data on its faces. The sides are those of vtkCellTypeSides, read from
 * its static tables, or from the cell for the higher-order cells. The edges of
 * a pixel and of a higher-order quadrilateral, which follow its parametric
 * axes, are reversed where needed to run round its boundary.
 *
 * The structure depends on the topology of the grid only: moving its points
 * keeps it, and the side data indexed by it.
 *
 * Ghost cells take part like any other cell, so the numbering is per grid:
 * a side on the boundary between two processes has an id on each of them, and
 * there is no global side numbering.
 *
 * BuildMap() is not thread safe, and every query calls it. Build the structure
 * from one thread, e.g., by calling GetNumberOfSides(), before reading it from
 * several; the queries are then thread safe as long as the grid's topology is
 * not modified.
 *
 * @warning
 * The higher-order pyramids have no sides here, since vtkCellTypeSides has
 * none for them.
 *
 * @sa
 * vtkUnstructuredGrid vtkUnstructuredGridSides vtkCellTypeSides
 */

#ifndef vtkUnstructuredGridSideMap_h
#define vtkUnstructuredGridSideMap_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkTimeStamp.h"    // For vtkTimeStamp
#include "vtkType.h"         // For vtkTypeInt32

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkGarbageCollector;
class vtkGenericCell;
class vtkIdList;
class vtkUnstructuredGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridSideMap : public vtkObject
{
public:
  static vtkUnstructuredGridSideMap* New();
  vtkTypeMacro(vtkUnstructuredGridSideMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The grid these sides belong to. Setting another grid drops the structure.
   * The grid is reported to the garbage collector, since a grid holds its side
   * map, see ReportReferences().
   */
  void SetDataSet(vtkUnstructuredGrid* dataSet);
  vtkUnstructuredGrid* GetDataSet();
  ///@}

  bool UsesGarbageCollector() const override { return true; }

  /**
   * Build the side map if it is not up to date with the grid's topology.
   * Called by every query, so there is normally no reason to call it
   * directly.
   */
  void BuildMap();

  /**
   * Whether the side map is built and up to date. Does not build it.
   */
  bool IsMapBuilt();

  /**
   * Supply the side numbering rather than deriving one. halfSideToSide holds
   * one side id per half-side, in half-side order, and the ids must cover
   * [0, numberOfSides). No matching pass runs, so the caller is trusted: two
   * half-sides given the same id are taken to be the same side.
   *
   * This is how a reader keeps the file's own side numbering, so that data
   * dimensioned on sides reads straight in with no permutation. The
   * representative of each side is still its lowest half-side, so signed side
   * data has to be measured from that cell, as the class description says.
   */
  void SetSideIds(vtkDataArray* halfSideToSide, vtkIdType numberOfSides);

  /**
   * Whether the numbering was supplied rather than derived. An authored
   * numbering is data: it has to be copied and serialized, not recomputed.
   */
  vtkGetMacro(Authored, bool);

  ///@{
  /**
   * Sizes.
   */
  vtkIdType GetNumberOfSides();
  vtkIdType GetNumberOfHalfSides();
  ///@}

  /**
   * The number of sides of a cell: its faces for a 3D cell, its edges for a
   * 2D cell, 0 otherwise.
   */
  vtkIdType GetNumberOfCellSides(vtkIdType cellId);

  /**
   * The half-side id of side localSide of cell cellId, i.e.,
   * CellSideOffsets[cellId] + localSide, or -1 if there is no such side.
   * Each cell has its own half-sides, so two cells sharing a side have
   * different half-side ids for it. A half-side id indexes HalfSideToSide.
   */
  vtkIdType GetHalfSideId(vtkIdType cellId, vtkIdType localSide);

  ///@{
  /**
   * The side id of side localSide of cell cellId, i.e.,
   * HalfSideToSide[GetHalfSideId(cellId, localSide)], or -1 if there is no
   * such side. Two cells sharing a side get the same side id, which indexes
   * the side data. GetCellSides() gives the side ids of all the sides of a
   * cell, in local side order.
   */
  vtkIdType GetCellSide(vtkIdType cellId, vtkIdType localSide);
  void GetCellSides(vtkIdType cellId, vtkIdList* sideIds);
  ///@}

  /**
   * The points of a side, in the winding of the cell that represents it, which
   * is that cell's outward winding. A cell holding the side the other way sees
   * these points reversed; GetSideOrientation() says which. tempCellPointIds
   * and tempCell are scratch objects of the caller, e.g., one per thread.
   */
  void GetSidePoints(
    vtkIdType sideId, vtkIdList* ptIds, vtkIdList* tempCellPointIds, vtkGenericCell* tempCell);

  /**
   * The cells incident on a side: one for a boundary side, two for an interior
   * side of a conforming mesh, more where the mesh is non-manifold. The first
   * is the representative, whose winding GetSidePoints() returns.
   */
  void GetSideCells(vtkIdType sideId, vtkIdList* cellIds);

  /**
   * +1 when this cell traverses the side the way GetSidePoints() returns it,
   * -1 when it traverses it the other way. Derived from the structure rather
   * than stored.
   *
   * For the sign to mean outward, the two cells sharing a side have to
   * traverse it opposite ways. For 3D cells that follows from the type: VTK's
   * canonical face templates are outward-wound, so a correctly ordered cell
   * cannot get it wrong. For 2D cells it does not: an edge's direction only
   * means anything relative to the neighbouring cell, so it is the mesh that
   * has to be consistently wound, not each cell in isolation. On a surface
   * mesh that comes down to the cells all being wound the same way round, which
   * is a property of the data. CheckWindingConsistency() says whether it holds.
   */
  int GetSideOrientation(vtkIdType cellId, vtkIdType localSide);

  /**
   * Count the sides whose two cells traverse them the same way round, which
   * they should not, and optionally list them. Returns 0 for a consistently
   * wound mesh.
   *
   * This is a diagnostic, not part of the build, because it costs a second
   * pass over the half-sides. It is worth running once on 2D data of unknown
   * provenance: where it reports sides, GetSideOrientation() still says which
   * cell represents a side, but its sign no longer means outward, and a signed
   * quantity read through it will not cancel between neighbours. Boundary and
   * non-manifold sides are skipped, having no two-sided convention to break.
   */
  vtkIdType CheckWindingConsistency(vtkIdList* offendingSides = nullptr);

  /**
   * The cells that share a side with this one.
   */
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* neighborCellIds);

  /**
   * Drop the side map and any supplied numbering. The next query builds it
   * again. Refused, with a warning, while the grid's side data holds any
   * array, because the data is indexed by this numbering.
   */
  void Initialize();

  ///@{
  /**
   * ShallowCopy shares the structure; DeepCopy takes its own. Neither sets the
   * grid, because the copy belongs to whichever grid is taking it: call
   * SetDataSet() before.
   */
  void ShallowCopy(vtkUnstructuredGridSideMap* other);
  void DeepCopy(vtkUnstructuredGridSideMap* other);
  ///@}

  ///@{
  /**
   * Direct access to the arrays, for filters that walk them rather than call
   * the accessors. Each has one component and is a vtkUnsignedCharArray,
   * vtkTypeInt32Array or vtkTypeInt64Array, the narrowest that holds its
   * values, so a filter can dispatch on these three types only, e.g., with
   * vtkArrayDispatch::DispatchByArray and vtkTypeList::Create<vtkUnsignedCharArray,
   * vtkTypeInt32Array, vtkTypeInt64Array>. A numbering given to SetSideIds()
   * in one of these types is returned by GetHalfSideToSide() as given;
   * one given in any other type is copied into the narrowest of them.
   */
  vtkDataArray* GetCellSideOffsets();
  vtkDataArray* GetHalfSideToSide();
  vtkDataArray* GetSideOffsets();
  vtkDataArray* GetSideCellsArray();
  vtkDataArray* GetSideLocalSidesArray();
  ///@}

  /**
   * Kibibytes held by the side structure.
   */
  unsigned long GetActualMemorySize();

protected:
  vtkUnstructuredGridSideMap();
  ~vtkUnstructuredGridSideMap() override;

  void ReportReferences(vtkGarbageCollector*) override;

  /**
   * One column of the structure, kept in the narrowest integer type its values
   * fit in and read through a raw pointer.
   *
   * The columns hold small numbers -- a local side index is single digits, and
   * ids on any mesh short of two billion sides fit in 32 bits -- so storing
   * them all as vtkIdType wastes most of the structure. The type is chosen once
   * when the column is packed and the pointer resolved then, so reading stays a
   * load and a widening rather than a virtual call, which matters because these
   * are read at random, one value at a time, on the O(1) paths.
   */
  struct IdColumn
  {
    vtkSmartPointer<vtkDataArray> Array;
    const unsigned char* Tiny = nullptr;
    const vtkTypeInt32* Narrow = nullptr;
    const vtkTypeInt64* Wide = nullptr;

    vtkIdType operator[](vtkIdType i) const
    {
      if (this->Narrow)
      {
        return static_cast<vtkIdType>(this->Narrow[i]);
      }
      if (this->Tiny)
      {
        return static_cast<vtkIdType>(this->Tiny[i]);
      }
      return static_cast<vtkIdType>(this->Wide[i]);
    }

    bool Empty() const
    {
      return this->Narrow == nullptr && this->Tiny == nullptr && this->Wide == nullptr;
    }
    vtkIdType GetNumberOfValues() const;
    unsigned long GetActualMemorySize() const;
    void Clear();
    // Resolve the raw pointer from Array, after packing or copying.
    void Bind();
    // Store these values in the narrowest type that holds them.
    void Pack(const std::vector<vtkIdType>& values);
    // Take a caller's array as it stands, for a supplied numbering.
    void Adopt(vtkDataArray* array);
  };

  // Count the half-sides of every cell and prefix-sum them.
  bool BuildCellSideOffsets();
  // Number the sides from the grouped sides of vtkUnstructuredGridSides, with
  // their inverse.
  void DeriveSideIds();
  // Invert a supplied HalfSideToSide into SideOffsets/SideCells/SideLocalSides.
  void BuildInverse();
  // Drop every column, leaving the side data alone.
  void ClearStructure();
  // Whether the grid's side data holds any array, which pins the numbering.
  bool HasSideData();

  vtkUnstructuredGrid* DataSet = nullptr;

  // One entry per cell, plus a trailing total: where a cell's half-sides start.
  IdColumn CellSideOffsets;
  // One entry per half-side: the dense side id it names.
  IdColumn HalfSideToSide;
  // One entry per side, plus a trailing total: where a side's incident
  // half-sides start in SideCells/SideLocalSides. The first of them is the
  // side's representative.
  IdColumn SideOffsets;
  // One entry per half-side, grouped by side. Kept as a (cell, local side)
  // pair rather than a packed half-side id, so that going from a side back to
  // a cell needs no search through CellSideOffsets.
  IdColumn SideCells;
  IdColumn SideLocalSides;

  vtkIdType NumberOfSides = 0;
  vtkIdType NumberOfHalfSides = 0;
  bool Authored = false;
  vtkTimeStamp BuildTime;

private:
  vtkUnstructuredGridSideMap(const vtkUnstructuredGridSideMap&) = delete;
  void operator=(const vtkUnstructuredGridSideMap&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
