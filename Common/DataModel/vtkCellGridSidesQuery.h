// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellGridSidesQuery_h
#define vtkCellGridSidesQuery_h

#include "vtkCellGridQuery.h"

#include "vtkStringToken.h" // For API.

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkCellGridSidesCache;
class vtkIdTypeArray;

/**\brief A cell-grid query for enumerating sides of cells.
 *
 * This query runs in 3 passes (see vtkCellGridSidesQuery::PassWork):
 *
 * + In the first pass, responders invoke the AddSides() method on
 *   this query, entries are added to this->Hashes storage indicating
 *   the cells which are bounded by a given shape + connectivity.
 * + In the second pass, responders mark the entries created above and
 *   add entries in this->Sides. This reorganizes the hashes into groups
 *   more amenable to output as side arrays. This pass is called
 *   "Summarization," since not every input side identified will be
 *   output.
 * + In the third and final pass, responders create new cells in
 *   the output cell-grid that correspond to the selected sides of
 *   the input.
 */
class VTKCOMMONDATAMODEL_EXPORT vtkCellGridSidesQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridSidesQuery* New();
  vtkTypeMacro(vtkCellGridSidesQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// An enum specifying which side(s) each responder should generate.
  enum SideFlags : int
  {
    // Individual bits
    VerticesOfEdges = 0x01,    //!< Input edges should produce endpoint vertices.
    VerticesOfSurfaces = 0x02, //!< Input surfaces should produce corner vertices.
    EdgesOfSurfaces = 0x04,    //!< Input surfaces should produce bounding edges.
    VerticesOfVolumes = 0x08,  //!< Input volumes should produce corner vertices.
    EdgesOfVolumes = 0x10,     //!< Input volumes should produce edges bounding faces.
    SurfacesOfVolumes = 0x20,  //!< Input volumes should produce bounding surfaces.

    // Useful (but not exhaustive) combinations
    SurfacesOfInputs = 0x20,   //!< Produce surfaces (i.e., omit sides of renderable geometry).
    EdgesOfInputs = 0x14,      //!< Produce edges of inputs of higher dimension.
    VerticesOfInputs = 0x0b,   //!< Produce vertices of inputs of higher dimension.
    AllSides = 0x3f,           //!< Produce all sides of inputs that have any sides.
    NextLowestDimension = 0x25 //!< Given an input of dimension D, produce sides of dimension D-1.
  };

  /// An enum specifying the work responders should perform for each pass.
  enum PassWork : int
  {
    /// Responders should call AddSide() on each cell's sides to insert
    /// entries into this->SideCache.
    HashSides = 0,
    /// Responders should claim entries in this->SideCache by transcribing them
    /// to this->Sides and then deleting the entry in this->SideCache to prevent
    /// multiple responders from processing them.
    Summarize = 1,
    /// Responders should insert new side sets into their parent cell-grid
    /// by examining this->Sides.
    GenerateSideSets = 2
  };

  /// An enum specifying the strategy by which input hashes are summarized
  /// into output Sides entries.
  enum SummaryStrategy
  {
    /// The number of hash entries for a given side should be used to
    /// decide whether to include a hash's side in the output.
    /// If a side occurs an odd number of times, it should be included
    /// in the output.
    Winding,
    /// If a hash entry entry exists, a single side should be included
    /// in the output.
    AnyOccurrence,
    /// Hashes for shapes (a) of dimension (d-1) that occur an odd number of
    /// times and (b) of dimension < (d-1) that occur once or more should
    /// be included in the output. (Dimension d is the dimension of the
    /// input shapes, whether they are cells or sides.
    Boundary
  };

  /// Indicate how output should be generated or marked so selection works as expected.
  enum SelectionMode
  {
    Input, //!< Input shapes should be selected when output sides are picked.
    Output //!< Output sides should be selected when they are picked.
  };

  /// A structure created by the GetSideSetArrays() method for responders to use.
  struct SideSetArray
  {
    /// The type of parent cell which created the sides.
    vtkStringToken CellType;
    /// The shape of all the sides in the \a Sides array.
    vtkStringToken SideShape;
    /// An array of tuples of (cell-id, side-id) specifying sides.
    vtkSmartPointer<vtkIdTypeArray> Sides;
  };

  /// Set/get whether renderable cells should be included in the output
  /// or the output should strictly contain sides of cells.
  ///
  /// A cell is renderable if it is of dimension 2 or less (i.e., surfaces,
  /// edges, and vertices are all renderable; volumetric cells are not).
  ///
  /// The default is false.
  vtkSetMacro(PreserveRenderableInputs, vtkTypeBool);
  vtkGetMacro(PreserveRenderableInputs, vtkTypeBool);
  vtkBooleanMacro(PreserveRenderableInputs, vtkTypeBool);

  /// Set/get whether to omit computation of sides for renderable cells.
  ///
  /// A cell is renderable if it is of dimension 2 or less (i.e., surfaces,
  /// edges, and vertices are all renderable; volumetric cells are not).
  /// This setting, when used in combination with PreserveRenderableInputs,
  /// allows the filter to behave similar to vtkPolyData surface extraction
  /// filters; volumetric cells will have sides computed but others will be
  /// passed through from the input unaltered.
  ///
  /// The default is false.
  vtkSetMacro(OmitSidesForRenderableInputs, vtkTypeBool);
  vtkGetMacro(OmitSidesForRenderableInputs, vtkTypeBool);
  vtkBooleanMacro(OmitSidesForRenderableInputs, vtkTypeBool);

  /// Set/get which sides to generate given input cells/sides.
  ///
  /// OutputDimensionControl is a bit-vector taking values from the SideFlags
  /// enumeration. It determines which sides of the input should be generated.
  /// The default is SideFlags::SurfacesOfInputs, which will only emit surfaces
  /// of volumetric cells.
  vtkSetMacro(OutputDimensionControl, int);
  vtkGetMacro(OutputDimensionControl, int);
  vtkBooleanMacro(OutputDimensionControl, int);

  /// Set/get the strategy responders should use to generate entries in
  /// Sides from entries in SideCache.
  ///
  /// The default is BoundaryStrategy.
  vtkSetEnumMacro(Strategy, SummaryStrategy);
  vtkGetEnumMacro(Strategy, SummaryStrategy);
  void SetStrategyToWinding() { this->SetStrategy(SummaryStrategy::Winding); }
  void SetStrategyToAnyOccurrence() { this->SetStrategy(SummaryStrategy::AnyOccurrence); }
  void SetStrategyToBoundary() { this->SetStrategy(SummaryStrategy::Boundary); }
  /// This method exists for ParaView to set the strategy.
  virtual void SetStrategy(int strategy)
  {
    this->SetStrategy(static_cast<SummaryStrategy>(strategy));
  }

  /// Set/get whether the extracted sides should be marked as selectable
  /// or whether their originating data should be selectable.
  ///
  /// Responders should use this to *either*:
  /// (a) mark the output to indicate what shapes should be selected upon being picked; or
  /// (b) output different shapes so that picking implicitly results in the proper shape
  ///     being picked.
  ///
  /// The default SelectionMode::Input indicates the input data should be selected.
  /// Other values indicate the generated output sides should be selected.
  vtkSetEnumMacro(SelectionType, SelectionMode);
  vtkGetEnumMacro(SelectionType, SelectionMode);
  /// This method exists for ParaView to set the selection mode.
  virtual void SetSelectionType(int selnType)
  {
    this->SetSelectionType(static_cast<SelectionMode>(selnType));
  }

  bool Initialize() override;
  void StartPass() override;
  bool IsAnotherPassRequired() override;
  bool Finalize() override;

  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, std::unordered_map<vtkIdType, std::set<int>>>>&
  GetSides()
  {
    return this->Sides;
  }

  /// Return arrays of cell+side IDs for the given \a cellType.
  std::vector<SideSetArray> GetSideSetArrays(vtkStringToken cellType);

  /// Return a string-token with the given selection mode or vice-versa.
  static vtkStringToken SelectionModeToLabel(SelectionMode mode);
  static SelectionMode SelectionModeFromLabel(vtkStringToken token);

  /// Return a string-token with the given summarization strategy or vice-versa.
  static vtkStringToken SummaryStrategyToLabel(SummaryStrategy strategy);
  static SummaryStrategy SummaryStrategyFromLabel(vtkStringToken token);

  /// Set/get cached hashtable of sides.
  ///
  /// The idea is that vtkCellGridSidesCache is generic enough to accommodate
  /// a wide variety of cell types and that many of them will be capable of
  /// having sides that are conformal to cells of different types that may
  /// reside in the same cell-grid. Filters may own this cache or they may
  /// attach it to a collection of cell-grid objects that participate by inserting
  /// their cells' sides into the cache. (For example, all the cell-grids within
  /// a partitioned dataset collection may wish to insert sides in the same cache.)
  vtkGetObjectMacro(SideCache, vtkCellGridSidesCache);
  virtual void SetSideCache(vtkCellGridSidesCache* cache);

protected:
  vtkCellGridSidesQuery() = default;
  ~vtkCellGridSidesQuery() override;

  vtkTypeBool PreserveRenderableInputs{ false };
  vtkTypeBool OmitSidesForRenderableInputs{ false };
  int OutputDimensionControl{ SideFlags::SurfacesOfInputs };
  SelectionMode SelectionType{ SelectionMode::Input };
  SummaryStrategy Strategy{ SummaryStrategy::Boundary };
  vtkCellGridSidesCache* SideCache{ nullptr };
  bool TemporarySideCache{ false };
  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, std::unordered_map<vtkIdType, std::set<int>>>>
    Sides;

private:
  vtkCellGridSidesQuery(const vtkCellGridSidesQuery&) = delete;
  void operator=(const vtkCellGridSidesQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridSidesQuery_h
