// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSurfaceNetsAtlas
 * @brief   query-driven extraction of regions and patches from any SurfaceNets filter output
 *
 * vtkSurfaceNetsAtlas consumes the polygonal output of any SurfaceNets filter
 * — vtkSurfaceNets2D, vtkSurfaceNets3D, or vtkGeneralizedSurfaceNets3D — and
 * exposes it as a queryable label atlas: a database of Regions (one per
 * label) and Patches (one per ordered pair of adjacent labels) that can be
 * efficiently re-queried without re-running the upstream surface extraction.
 *
 * @section Terminology
 *
 * - **Label**: the integral annotation value used to segment a point.
 *   The input volume passed to the upstream SurfaceNets filter
 *   contains N distinct labels.
 * - **LID** (Label Identifier): an internal compact index assigned to each
 *   distinct label. The LID space is dense ([0, N)) and is used for
 *   indexing internal data structures. Label and LID are generally
 *   different; the public API speaks in Labels.
 * - **Label Set**: an unordered collection of Labels used to specify which
 *   regions to extract.
 * - **Region**: the object implicitly defined by a single Label. A Region is
 *   the union of all Patches that touch its Label.
 * - **Patch**: the shared mesh boundary / interface between two adjacent
 *   regions. A Patch is identified by the ordered pair (L0, L1) with L0 < L1
 *   (sorted by numeric label value) for stable identification and naming.
 * - **Adjacent**: two Labels are adjacent if their Regions share a Patch.
 * - **BoundaryLabels**: the 2-component cell-data array written by the
 *   upstream SurfaceNets filter on its output vtkPolyData. Each cell carries
 *   an *ordered* pair (Label0, Label1) describing the labels on either side
 *   of the cell.
 *   The ordering follows the upstream filter's convention:
 *   - If one side is the background, the tuple is (non-background,
 *     BackgroundLabel) (background is always the second component).
 *   - Otherwise, the tuple is sorted so that Label0 < Label1.
 *
 *   The mesh is wound such that the cell normal points from Label0 to Label1.
 *
 * @section Output
 *
 * The filter produces a vtkPartitionedDataSetCollection (PDC). Each requested
 * Region or Patch is emitted as a vtkPartitionedDataSet (PDS) containing a
 * single vtkPolyData partition (in the serial case; distributed builds
 * may produce multiple partitions per PDS). A vtkDataAssembly organizes
 * the output into two top-level subtrees:
 *
 * @code
 *   <root>
 *     <Regions>                    (only if GenerateRegions is on)
 *       <Name|Label_N>             -> PDS containing the region polydata
 *       ...
 *     <Patches>                    (only if GeneratePatches is on)
 *       <Name0_Name1|Label_N_Label_M>  -> PDS containing the patch polydata
 *       ...
 * @endcode
 *
 * Node names default to `Label_N` / `Label_N_Label_M` but are replaced by the name
 * set via SetLabelName() when one is present.
 * Assembly node attributes — Region: `Name`, `Label`, `LID`.
 * Patch: `Name0`, `Name1`, `Label0`, `Label1`, `LID0`, `LID1`.
 *
 * Each partition's vtkPolyData also carries arrays so downstream consumers
 * can identify it without the assembly. The scalar type of `"BoundaryLabels"`
 * matches the scalar type of the upstream filter's `"BoundaryLabels"` input array.
 *
 * **Region cell data**:
 * - `"BoundaryLabels"` (2-component) — `[Label0, Label1]` for each cell.
 *   Label0 is always the region's own label, so the cell normal points outward
 *   (from the region into its neighbour). Active scalar array. Varies per cell.
 * - `"Label"` (`vtkIdType`, constant) — the region's label value, same for every cell.
 *   Backed by `vtkConstantArray` (O(1) storage).
 * - `"LID"` (`int`, constant) — the region's dense index, same for every cell.
 *   Backed by `vtkConstantArray` (O(1) storage).
 *
 * **Region field data** (partition-level metadata):
 * - `"AdjacentLabels"` (`vtkIdType[]`) — label values of all neighbouring regions.
 * - `"PatchIDs"` (`int[]`) — atlas patch IDs of all patches touching this region.
 *
 * **Patch cell data**:
 * - `"BoundaryLabels"` (2-component) — `[Label0, Label1]`, with Label0 < Label1
 *   by label value (background label is always Label1 when present). Active scalar array.
 *   Varies per cell.
 * - `"PatchID"` (`int`, constant) — the atlas ID of this patch, same for every cell.
 *   Backed by `vtkConstantArray` (O(1) storage).
 *
 * **Patch field data**: none.
 *
 * **PDC field data** (always present):
 * - `"LIDToLabel"` (`vtkIdType[]`) — the full Label↔LID mapping for the current atlas.
 * - `"PatchLIDs"` (2-component `int`) — maps each patch ID to its `[LID0, LID1]` pair.
 * - `"PatchLabels"` (2-component `vtkIdType`) — maps each patch ID to its
 *   `[Label0, Label1]` pair (with Label0 < Label1).
 *
 * @section Execution Model
 *
 * The atlas (the internal database of label mappings, the patch table,
 * and per-cell patch indices) is built once per input change, gated on
 * the input vtkPolyData's MTime. Changing extraction parameters
 * (SelectedLabels, ExtractionMode, OutputStyle, GeneratePatches,
 * ResolveNonManifoldPoints, BackgroundLabel) does not trigger
 * a rebuild of the atlas; it only rebuilds the output PDC by walking
 * the cached patch table. This makes iterative queries (e.g.,
 * interactive toggling of labels in a viewer) cheap.
 *
 * Output partitions are deep-copied: the output PDC has no lifetime
 * dependency on the input mesh.
 *
 * @section Inputs
 *
 * The filter accepts the vtkPolyData output of any SurfaceNets filter:
 *
 * | Upstream filter                | Cell type |
 * |--------------------------------|-----------|
 * | vtkSurfaceNets2D               | Lines     |
 * | vtkSurfaceNets3D               | Quads     |
 * | vtkSurfaceNets3D               | Triangles |
 * | vtkGeneralizedSurfaceNets3D    | Polygons  |
 * | vtkGeneralizedSurfaceNets3D    | Triangles |
 *
 * The point-data array "NonManifoldTableIndices" (signed int8), produced
 * only by vtkSurfaceNets3D, is used as a hint by ResolveNonManifoldPoints
 * to identify candidate points; when absent, all points are scanned.
 * The array is never present in output partitions.
 *
 * @sa vtkSurfaceNets2D vtkSurfaceNets3D vtkGeneralizedSurfaceNets3D
 *     vtkPartitionedDataSetCollection vtkDataAssembly
 */

#ifndef vtkSurfaceNetsAtlas_h
#define vtkSurfaceNetsAtlas_h

#include "vtkFiltersCoreModule.h" // for export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h"  // for vtkSmartPointer
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <atomic> // for std::atomic
#include <memory> // for std::unique_ptr
#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkIdTypeArray;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT VTK_MARSHALAUTO vtkSurfaceNetsAtlas
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to instantiate, print, and provide type information.
   */
  static vtkSurfaceNetsAtlas* New();
  vtkTypeMacro(vtkSurfaceNetsAtlas, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  // ==========================================================================
  // Extraction mode
  // ==========================================================================

  enum ExtractionModes
  {
    EXTRACT_ALL = 0,
    EXTRACT_LABEL_SET = 1,
  };

  ///@{
  /**
   * Specify which regions are emitted in the output:
   * - All:      every distinct label found in the input.
   * - LabelSet: the labels currently in the SelectedLabels list.
   *
   * In LabelSet mode, an empty selection produces an empty output (it is
   * not interpreted as "all labels"). Use All for that.
   *
   * Default: EXTRACT_ALL.
   */
  vtkSetClampMacro(ExtractionMode, int, EXTRACT_ALL, EXTRACT_LABEL_SET);
  vtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToAll() { this->SetExtractionMode(EXTRACT_ALL); }
  void SetExtractionModeToLabelSet() { this->SetExtractionMode(EXTRACT_LABEL_SET); }
  ///@}

  // ==========================================================================
  // Output style
  // ==========================================================================

  enum OutputStyles
  {
    OUTPUT_STYLE_ALL = 0,
    OUTPUT_STYLE_BOUNDARY = 1
  };

  ///@{
  /**
   * Specify which cells are included in each emitted Region partition.
   * - All:      every cell of the region surface, regardless of whether it
   *             borders background or another label. Cells on interior interfaces
   *             appear in both adjacent Region partitions (with opposite winding),
   *             so the total cell count across all partitions exceeds the upstream
   *             SurfaceNets output.
   * - Boundary  (default): only cells whose interface touches the BackgroundLabel
   *             (the outer shell of each labeled region). When GeneratePatches is
   *             also on (the default), the complementary interior label-to-label
   *             cells are emitted as Patches, so that Regions and Patches together
   *             form an exact partition of the upstream SurfaceNets output — same
   *             total cell count, no duplication, no omission. When GeneratePatches
   *             is off, interior cells are simply omitted.
   *
   * Default: OUTPUT_STYLE_BOUNDARY.
   */
  vtkSetClampMacro(OutputStyle, int, OUTPUT_STYLE_ALL, OUTPUT_STYLE_BOUNDARY);
  vtkGetMacro(OutputStyle, int);
  void SetOutputStyleToAll() { this->SetOutputStyle(OUTPUT_STYLE_ALL); }
  void SetOutputStyleToBoundary() { this->SetOutputStyle(OUTPUT_STYLE_BOUNDARY); }
  ///@}

  ///@{
  /**
   * The label value that represents background voxels. Used by
   * OUTPUT_STYLE_BOUNDARY to identify the outer shell of each region (cells
   * touching background) and, when GeneratePatches is on, to identify the
   * interior label-to-label interfaces emitted as patches.
   *
   * @note Should match the BackgroundLabel set on the upstream vtkSurfaceNets3D.
   *
   * Default: 0.
   */
  vtkSetMacro(BackgroundLabel, vtkIdType);
  vtkGetMacro(BackgroundLabel, vtkIdType);
  ///@}

  // ==========================================================================
  // Selected labels (used when ExtractionMode is EXTRACT_LABEL_SET)
  // ==========================================================================

  ///@{
  /**
   * Add, remove, or replace a single label in the selection list.
   * AddSelectedLabel is idempotent, i.e. adding a label already in the list
   * is a no-op.
   */
  void AddSelectedLabel(vtkIdType label);
  void RemoveSelectedLabel(vtkIdType label);
  void ClearSelectedLabels();
  ///@}

  ///@{
  /**
   * Set the selection list to exactly n entries. Excess entries are discarded
   * when shrinking. SetSelectedLabel(i, label) grows the list automatically
   * when i is past the current end.
   */
  void SetNumberOfSelectedLabels(int n);
  int GetNumberOfSelectedLabels() const;
  ///@}

  ///@{
  /**
   * Index-based access to the selection list. The index i ranges over
   * [0, GetNumberOfSelectedLabels()).
   *
   * SetSelectedLabel grow the list automatically when i is past the
   * current end.
   */
  void SetSelectedLabel(int i, vtkIdType label);
  vtkIdType GetSelectedLabel(int i) const;
  ///@}

  ///@{
  /**
   * Set/Get the complete selection list.
   */
  std::vector<vtkIdType> GetSelectedLabels() const;
  void SetSelectedLabels(const std::vector<vtkIdType>& labels);
  ///@}

  ///@{
  /**
   * Populate the selection list with numLabels equally spaced integer labels
   * between rangeStart and rangeEnd (inclusive). Replaces any existing selection.
   *
   * The 2-argument overload selects every consecutive integer in [rangeStart,
   * rangeEnd], which covers the common case of sequential segmentation labels.
   * For non-uniform label sets use AddSelectedLabel / SetSelectedLabel directly.
   */
  void GenerateSelectedLabels(int numLabels, vtkIdType rangeStart, vtkIdType rangeEnd);
  void GenerateSelectedLabels(int numLabels, vtkIdType range[2]);
  void GenerateSelectedLabels(vtkIdType rangeStart, vtkIdType rangeEnd);
  ///@}

  // ==========================================================================
  // Label names
  // ==========================================================================

  ///@{
  /**
   * Human-readable names for labels. Names are independent of the SelectedLabels
   * list and can be assigned to any label value, including when ExtractionMode is
   * EXTRACT_ALL. When a name is set it is used as the assembly node name instead of
   * the default "Label_N" / "Label_N_Label_M", and exposed as a "Name" / "Name0" /
   * "Name1" attribute on the assembly node.
   *
   * Names are not validated for XML identifier legality; the caller is
   * responsible for avoiding characters that are invalid in XML node names
   * (spaces, colons, leading digits, etc.).
   */
  int GetNumberOfLabelNames() const;
  void SetLabelName(vtkIdType label, const std::string& name);
  std::string GetLabelName(vtkIdType label) const;
  void AddLabelName(vtkIdType label, const std::string& name);
  void RemoveLabelName(vtkIdType label);
  void ClearLabelNames();
  ///@}

  // ==========================================================================
  // Optional outputs
  // ==========================================================================

  ///@{
  /**
   * When on (the default), the output PDC contains a "Regions" subtree with
   * one PDS per selected label.
   *
   * Turn off to produce only the "Patches" subtree (requires GeneratePatches
   * to also be on).
   *
   * Default: on.
   */
  vtkSetMacro(GenerateRegions, bool);
  vtkGetMacro(GenerateRegions, bool);
  vtkBooleanMacro(GenerateRegions, bool);
  ///@}

  ///@{
  /**
   * When on, the output PDC also contains a "Patches" subtree in its
   * assembly, with one PDS per patch (L0, L1) whose at-least-one-label
   * is in the current selection.
   *
   * In OUTPUT_STYLE_BOUNDARY mode (the default), only interior label-to-label
   * patches (those not involving the BackgroundLabel) are emitted. Together
   * with the boundary-only Region partitions, this produces an exact partition
   * of the upstream SurfaceNets output with no cell duplication. In
   * OUTPUT_STYLE_ALL mode, all patches (including those touching the
   * BackgroundLabel) are emitted.
   *
   * Default: on.
   */
  vtkSetMacro(GeneratePatches, bool);
  vtkGetMacro(GeneratePatches, bool);
  vtkBooleanMacro(GeneratePatches, bool);
  ///@}

  ///@{
  /**
   * When on, post-processes each output partition (both Regions and
   * Patches) to resolve non-manifold points by splitting the local cell
   * fan by connected component. Because each partition contains only a
   * subset of the original cells, configurations that were globally
   * unresolvable in the source mesh may be locally resolvable here.
   *
   * If the input carries a NonManifoldTableIndices point-data array
   * (produced by vtkSurfaceNets3D), only points marked -1 in that array
   * are candidates; otherwise every point is a candidate.
   *
   * In neither case does a NonManifoldTableIndices array appear in the output partitions.
   *
   * Default: off.
   */
  vtkSetMacro(ResolveNonManifoldPoints, bool);
  vtkGetMacro(ResolveNonManifoldPoints, bool);
  vtkBooleanMacro(ResolveNonManifoldPoints, bool);
  ///@}

  // ==========================================================================
  // Atlas inspection (Phase 2: queries against the built atlas)
  //
  // These methods inspect the atlas built from the most recent Update().
  // They require the filter to have been executed at least once with a
  // valid input; otherwise they return zero / empty results (the
  // "invalid" sentinel for index-returning methods is -1).
  //
  // None of these methods trigger re-execution.
  // ==========================================================================

  // -- Label queries ---------------------------------------------------------

  /**
   * Return the number of distinct labels in the input.
   */
  int GetNumberOfLabels() const;

  ///@{
  /**
   * Return whether `label` exists in the atlas.
   */
  bool HasLabel(vtkIdType label) const;
  bool HasLabel(const std::string& name) const;
  ///@}

  /**
   * Reverse-lookup a label value by its name. Returns -1 if the name
   * is not found in the atlas.
   */
  vtkIdType GetLabelForName(const std::string& name) const;

  ///@{
  /**
   * Translate between Label and LID. LIDs are the dense [0, N) indices
   * used internally and as PDS indices in the output. Returns -1 if
   * the label or LID is not found / out of range.
   */
  int GetLIDForLabel(vtkIdType label) const;
  int GetLIDForLabel(const std::string& name) const;
  vtkIdType GetLabelForLID(int lid) const;
  ///@}

  // -- Adjacency queries -----------------------------------------------------

  ///@{
  /**
   * Return whether two labels share a patch. False if either label is
   * not in the atlas.
   */
  bool AreAdjacent(vtkIdType label0, vtkIdType label1) const;
  bool AreAdjacent(const std::string& name0, const std::string& name1) const;
  ///@}

  ///@{
  /**
   * Return a list of labels that share a patch with `label`.
   * Output is empty if `label` does not exist in the atlas.
   */
  std::vector<vtkIdType> GetAdjacentLabels(vtkIdType label) const;
  std::vector<vtkIdType> GetAdjacentLabels(const std::string& name) const;
  ///@}

  // -- Patch queries ---------------------------------------------------------

  /**
   * Return the number of patches (unordered {L0, L1} pairs with
   * L0 < L1) in the input.
   */
  vtkIdType GetNumberOfPatches() const;

  /**
   * Fill `labels` with the (L0, L1) pair defining the patch at `patchID`,
   * with L0 < L1. `patchID` ranges over [0, GetNumberOfPatches()).
   * Sets both entries to -1 if out of range.
   */
  void GetPatchLabels(int patchID, vtkIdType labels[2]) const;

  ///@{
  /**
   * Return the patch ID for the patch defined by the (unordered)
   * pair (label0, label1), or -1 if no such patch exists. The argument
   * order is irrelevant: GetPatchID(a, b) == GetPatchID(b, a).
   */
  int GetPatchID(vtkIdType label0, vtkIdType label1) const;
  int GetPatchID(const std::string& name0, const std::string& name1) const;
  ///@}

  /**
   * Return the number of cells on the patch at `patchID`.
   * Useful for diagnostics (identifying degenerate or trivially small
   * patches) without materializing the patch polydata. Returns 0 if
   * `patchID` is out of range.
   */
  vtkIdType GetPatchCellCount(int patchID) const;

  ///@{
  /**
   * Returns the indices of every patch touching `label`. The
   * Output is empty if `label` does not exist in the atlas.
   */
  std::vector<int> GetPatchesForLabel(vtkIdType label) const;
  std::vector<int> GetPatchesForLabel(const std::string& name) const;
  ///@}

protected:
  vtkSurfaceNetsAtlas();
  ~vtkSurfaceNetsAtlas() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkSurfaceNetsAtlas(const vtkSurfaceNetsAtlas&) = delete;
  void operator=(const vtkSurfaceNetsAtlas&) = delete;

  /**
   * Build the atlas from the input mesh if the input has changed since the last build. The atlas is
   * cached across updates and is only rebuilt when the input MTime changes.
   */
  void BuildAtlas(vtkPolyData* input);

  template <bool IsPatch>
  vtkSmartPointer<vtkPolyData> ExtractLabel(vtkPolyData* source,
    const std::vector<const std::vector<vtkIdType>*>& groups, vtkIdType label,
    std::atomic<unsigned char>* ptUses, std::vector<vtkIdType>& pointMap) const;

  /**
   * Walk the cached atlas and assemble the output PDC according to the
   * current extraction parameters.
   */
  void BuildOutput(vtkPolyData* input, vtkPartitionedDataSetCollection* output) const;

  // -- Parameters ------------------------------------------------------------

  int ExtractionMode = EXTRACT_ALL;
  int OutputStyle = OUTPUT_STYLE_BOUNDARY;
  vtkIdType BackgroundLabel = 0;
  bool GenerateRegions = true;
  bool GeneratePatches = true;
  bool ResolveNonManifoldPoints = false;

  std::vector<vtkIdType> Labels;

  // The atlas information
  struct AtlasData;
  std::unique_ptr<AtlasData> Atlas;

  vtkTimeStamp AtlasBuildTime;
};

VTK_ABI_NAMESPACE_END
#endif
