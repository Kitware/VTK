// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResolveHangingNodes
 * @brief   make a 2D or 3D multi-resolution mesh conforming by inserting hanging points
 *
 * vtkResolveHangingNodes takes a vtkPolyData, vtkUnstructuredGrid or vtkCartesianGrid, or a
 * vtkPartitionedDataSet, vtkPartitionedDataSetCollection or vtkNonOverlappingAMR of them, made of
 * quads (2D) or hexahedra (3D), or pixels or voxels for a vtkCartesianGrid, that tile space at
 * different refinement levels, e.g., as produced by block-structured adaptive mesh refinement
 * (M. J. Berger, P. Colella, "Local adaptive mesh refinement for shock hydrodynamics", Journal of
 * Computational Physics 82, pp. 64-84, 1989). Where a coarse cell borders finer cells, the corners
 * of the finer cells lie on the edges or faces of the coarse cell without being points of it: they
 * are hanging nodes. This filter resolves them by inserting these hanging points into the coarse
 * cells, which keeps them as points, so that neighboring cells share their points along their
 * common boundary, which makes the interpolation of point data continuous across levels.
 *
 * The cells do not need to be axis aligned, and may have different lengths in each direction.
 *
 * ### Algorithm
 *
 * 1. Find the candidate cells: the cells with an edge (2D) or face (3D) without neighbor, i.e.,
 *    the cells on both sides of a level transition and at the boundary of the data. The other
 *    cells are copied unchanged. In 3D, a cell can also touch finer cells only along an edge, with
 *    all its faces matched: the cells with an edge of an unmatched face are candidates too.
 * 2. Insert in the candidate cells the existing points that lie strictly inside these edges and
 *    faces, within Tolerance. All the cells sharing an edge search it, so they find the same
 *    points.
 * 3. The edges and faces that still have no neighbor are the true boundary of the data. In a
 *    vtkPartitionedDataSet or in distributed data, the points that hang on them belong to other
 *    partitions: see "Points of other partitions" below.
 *
 * ### Number of levels
 *
 * Consecutive levels differ by a refinement ratio of 2 in each direction, i.e., their cell areas
 * (2D) or volumes (3D) differ by a factor of 4 or 8. The number of levels is computed from the
 * ratio of the largest to the smallest measure of the candidate cells, which include the
 * coarsest and the finest cells of a connected mesh. A warning is issued if the measures are not
 * consistent with this ratio.
 *
 * ### Points of other partitions
 *
 * The partitions of a vtkPartitionedDataSet are resolved together, and every vtkPartitionedDataSet
 * of the collection on its own, since it is the one whose partitions may share hanging nodes. The
 * blocks of all the levels of a vtkNonOverlappingAMR are resolved together, but a level is
 * conforming, so a block only receives the points of the blocks of other levels. A
 * vtkCartesianGrid is conforming too: its candidate cells are its boundary cells, and it has no
 * hanging node by itself. vtkOverlappingAMR is not supported, since its levels overlap. The points
 * of the other partitions (remote points) that lie on the true boundary of a partition are inserted
 * in its cells, and added with their point data to its output, so that the partitions end up with
 * identical points along their common boundary.
 * The partitions whose boundary bounds overlap are neighbors, and they share which bins of their
 * bounds their boundary edges or faces occupy, so each partition receives only the points of its
 * neighbors that are near its own boundary (see GetRemotePoints()). A parallel subclass
 * (vtkPResolveHangingNodes) exchanges them between the partitions of all the processes, by
 * overriding UseRemotePoints() and GetRemotePoints().
 *
 * ### Output
 *
 * Only the candidate cells with hanging points change, and the cells keep their order, so the
 * cell data stays valid. The points are the input points, followed by the remote points that are
 * used, if any, and the center points of the 3D decomposition below. The point and cell data are
 * passed: the remote points keep the global ids of the points they copy, and the center points get
 * new ones (see below), but the cell global ids are removed if cells are decomposed, since the new
 * cells have none.
 *
 * The output has the structure of the input. A vtkCartesianGrid gives a vtkUnstructuredGrid of
 * quads or hexahedra, and a vtkNonOverlappingAMR a vtkPartitionedDataSetCollection of them with the
 * same structure.
 *
 * When Decompose is true (the default), each cell with hanging points is replaced, at the same
 * place in the cell order, by several cells that copy its cell data: triangles and quads in 2D,
 * tetrahedra and pyramids in 3D. A quad, or a face of a hexahedron, with hanging points on its
 * edges is split into triangles and quads in a way that only depends on which of its edges are
 * split, so that the 2 cells sharing a face split it the same way, even on different processes. In
 * 3D, a face with a hanging point at its center is split into the 4 faces of the finer neighbors,
 * and the cell is decomposed by pulling (coning) a point over its faces (see the pulling
 * triangulations in J. A. De Loera, J. Rambau, F. Santos, "Triangulations: Structures for
 * Algorithms and Applications", Springer, 2010). A coarse cell next to finer cells across one face
 * (5 pyramids and 4 tetrahedra) or along one edge (4 pyramids) is decomposed without a new point.
 * The other cases pull a new point at the center of the cell, whose point data is the average of
 * the points of the cell, for the arrays that are interpolated (see vtkDataSetAttributes), while
 * the others, e.g., the pedigree ids, get null values. With point global ids, the center points get
 * new global ids, after the largest one of the input, unique over all the partitions and processes,
 * and over all the vtkPartitionedDataSet of a vtkPartitionedDataSetCollection.
 *
 * When Decompose is false, each cell with hanging points becomes one polygon (2D) or one
 * polyhedron (3D).
 *
 * If ComputeInverseMeasure is true, an "InverseArea" (2D) or "InverseVolume" (3D) cell array is
 * added to the output. If PassThroughCellIds is true, a "vtkOriginalResolvedCellIds" cell array
 * (see OriginalCellIdsName) with the input cell of every output cell is added to the output. The
 * partitions without cells get these arrays too, empty.
 *
 * @warning
 * The input, or every partition of a collection, must have only quads (2D) or only hexahedra (3D),
 * or be a 2D or 3D vtkCartesianGrid without blanking, with the same dimension for all the
 * partitions resolved together, otherwise the filter reports an error and fails. A
 * vtkUnstructuredGrid of pixels or voxels, e.g., from vtkThreshold of a vtkImageData, is not
 * supported. Neighboring cells must share their points where they touch at corners, i.e.,
 * coincident points must be merged, and the mesh must be manifold: apart from the hanging nodes, an
 * edge (2D) or face (3D) has at most one neighbor. Cells sharing an edge or a face must differ by
 * at most one level.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other non-sequential execution type
 * (set in the CMake variable VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPResolveHangingNodes vtkCellDataToPointData
 */

#ifndef vtkResolveHangingNodes_h
#define vtkResolveHangingNodes_h

#include "vtkBoundingBox.h" // For BoundaryBins
#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkSmartPointer.h"      // For vtkSmartPointer
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO

#include <bitset>      // For BoundaryBins
#include <map>         // For MergeReceivedPoints
#include <string>      // For OriginalCellIdsName
#include <type_traits> // For BoundaryBins
#include <vector>      // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkPartitionedDataSet;
class vtkPointSet;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT VTK_MARSHALAUTO vtkResolveHangingNodes : public vtkDataObjectAlgorithm
{
public:
  static vtkResolveHangingNodes* New();
  vtkTypeMacro(vtkResolveHangingNodes, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Tolerance to decide whether a point lies on an edge or face, as a fraction of the length of
   * the edge or of the diagonal of the face. It is clamped to [0, 0.1]. Default is 1e-6.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 0.1);
  vtkGetMacro(Tolerance, double);
  ///@}

  /**
   * Number of refinement levels of the mesh, computed by the last execution: the largest one over
   * the vtkPartitionedDataSet of a vtkPartitionedDataSetCollection. It is 0 without candidate
   * cells.
   */
  vtkGetMacro(NumberOfLevels, int);

  ///@{
  /**
   * Option to decompose each cell with hanging points into conforming triangles and quads (2D) or
   * tetrahedra and pyramids (3D), instead of turning it into a single polygon (2D) or polyhedron
   * (3D). Default is true.
   */
  vtkSetMacro(Decompose, bool);
  vtkGetMacro(Decompose, bool);
  vtkBooleanMacro(Decompose, bool);
  ///@}

  ///@{
  /**
   * Option to add an "InverseArea" (2D) or "InverseVolume" (3D) cell array to the output, the
   * inverse of the measure of each cell, to weight the interpolation of cell data to points (see
   * vtkCellDataToPointData::SetEnableWeightedAverage). Default is false.
   */
  vtkSetMacro(ComputeInverseMeasure, bool);
  vtkGetMacro(ComputeInverseMeasure, bool);
  vtkBooleanMacro(ComputeInverseMeasure, bool);
  ///@}

  ///@{
  /**
   * Option to add a cell array, named OriginalCellIdsName, to the output, with the id of the input
   * cell of every output cell, in its partition: several output cells have the same input cell when
   * Decompose is true. Default is false.
   */
  vtkSetMacro(PassThroughCellIds, bool);
  vtkGetMacro(PassThroughCellIds, bool);
  vtkBooleanMacro(PassThroughCellIds, bool);
  ///@}

  ///@{
  /**
   * The name of the cell array of PassThroughCellIds. Default is "vtkOriginalResolvedCellIds".
   */
  vtkSetStdStringFromCharMacro(OriginalCellIdsName);
  vtkGetCharFromStdStringMacro(OriginalCellIdsName);
  ///@}

protected:
  vtkResolveHangingNodes() = default;
  ~vtkResolveHangingNodes() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Whether the points of the partitions of other processes are needed to resolve the boundary of
   * the local partitions, see GetRemotePoints(). The default is false. It must return the same
   * value on all the processes.
   */
  virtual bool UseRemotePoints() { return false; }

  /**
   * Called with the range (min, max) of the measures of the candidate cells of the local
   * partitions, and the dimension of the cells (2, 3, or 0 without cells), which it may replace by
   * the ones of the whole data set. The default does nothing. Subclasses working on distributed
   * data reduce them over all the processes, so that they agree on the number of levels. It is
   * called on all the processes, even on those without cells.
   */
  virtual void GetGlobalMeasureRange(double vtkNotUsed(range)[2], int& vtkNotUsed(dimension)) {}

  /**
   * Called with the true boundary of every local partition: the edges (2D) or faces (3D) without
   * neighbor that no point of the partition splits, as lines or quads, with their end points and
   * point data, or nullptr if there is none. Returns, for every partition, the points of the other
   * partitions that may lie on its boundary, with their point data, which must have the same arrays
   * as the point data of the partition (see MergeReceivedPoints()), or nullptr if there is none.
   * The default exchanges the points between the local partitions: the partitions whose boundary
   * bounds overlap are neighbors, and every partition receives the points of its neighbors that are
   * in the bins its boundary occupies (see BoundaryBins). `partitionedDataSetIds` tells, for every
   * partition, which partitions it is resolved with, the same way on all the processes:
   * - -1: all the other partitions, here and on the other processes, e.g., for the partitions of a
   *   vtkPartitionedDataSet, which may have hanging nodes between them;
   * - otherwise, the index of its vtkPartitionedDataSet, e.g., the level of a block of a
   *   vtkNonOverlappingAMR: only the partitions of the other vtkPartitionedDataSet, since the
   *   partitions of its own are conforming with it.
   *
   * It is called if there are several local partitions with cells or if UseRemotePoints() is
   * true, then on all the processes, even on those without cells, unless no process has candidate
   * cells.
   */
  virtual std::vector<vtkSmartPointer<vtkPolyData>> GetRemotePoints(
    const std::vector<vtkPolyData*>& boundaries, const std::vector<int>& partitionedDataSetIds);

  /**
   * Called with the largest point global id of the local partitions, -1 without point global ids,
   * and the number of center points of the local partitions with point global ids (see
   * Decompose), which get new global ids after the largest one of all the processes. Sets
   * `maxGlobalId` to the largest point global id of all the processes, `offset` to the number of
   * these center points on the processes of lower rank, so that the new global ids are unique, and
   * `totalNumberOfCentroidPoints` to their number on all the processes. The default, for a single
   * process, keeps `maxGlobalId`, sets `offset` to 0 and `totalNumberOfCentroidPoints` to
   * `numberOfCentroidPoints`. It is called if Decompose is true, on all the processes, even on
   * those without cells, unless no process has candidate cells.
   */
  virtual void GetCentroidGlobalIdsOffset(vtkIdType& vtkNotUsed(maxGlobalId),
    vtkIdType numberOfCentroidPoints, vtkIdType& offset, vtkIdType& totalNumberOfCentroidPoints)
  {
    offset = 0;
    totalNumberOfCentroidPoints = numberOfCentroidPoints;
  }

#ifndef __VTK_WRAP__
  /**
   * Helper for the exchange of points between partitions: the bounds of the true boundary
   * `boundary` of a partition (see GetRemotePoints()), inflated by `tolerance` times their
   * diagonal, so that the points within the tolerance of the boundary are not missed. They are
   * invalid without boundary points.
   */
  static vtkBoundingBox GetBoundaryBounds(vtkPolyData* boundary, double tolerance);

  /**
   * Helper for the exchange of points between partitions: the bins of the boundary of a
   * partition, laid out as in vtkStaticCellLocator (Bounds, Divisions, bin indices ijk and bin
   * index), with one bit per bin telling if a boundary element (edge in 2D, face in 3D) of the
   * partition overlaps the bin. The bounds alone cover the whole partition, interface and domain
   * boundary included, so most of the boundary points of a neighbor are within them; the occupied
   * bins keep only those near the boundary elements.
   */
  struct VTKFILTERSCORE_EXPORT BoundaryBins
  {
    // Target number of bins (see vtkBoundingBox::ComputeDivisions).
    static constexpr vtkIdType TargetNumberOfBins = 32 * 32 * 32;

    // Bounds of the bins, which may be larger than the bounds the bins were initialized with along
    // an axis where they are flat (see vtkBoundingBox::ComputeDivisions).
    vtkBoundingBox Bounds;
    int Divisions[3] = { 1, 1, 1 };
    // One bit per bin, by bin index. ComputeDivisions never makes more bins than the target.
    using OccupiedBitset = std::bitset<TargetNumberOfBins>;
    // diy sends it as raw bytes.
    static_assert(
      std::is_trivially_copyable_v<OccupiedBitset>, "the bitset must be trivially copyable");
    OccupiedBitset Occupied;

    // Set the bounds and the divisions, with bins as close to cubes as possible, all unoccupied.
    // The result only depends on `bounds`, so a neighbor initialized with the same bounds has the
    // same bins, and only the occupied bits need to be exchanged.
    void Initialize(const vtkBoundingBox& bounds);

    vtkIdType GetNumberOfBins() const;

    // The bin indices ijk of the point `x`, clamped to the bins.
    void GetBinIndices(const double* x, int ijk[3]) const;

    vtkIdType GetBinIndex(const int ijk[3]) const;

    void SetBinOccupied(vtkIdType binId);

    vtkIdType GetBinIndex(const double* x) const;

    bool IsBinOccupied(vtkIdType binId) const;

    // Whether `x` is within the bounds, in an occupied bin.
    bool IsInOccupiedBin(const double* x) const;
  };

  /**
   * Helper for the exchange of points between partitions: bin the boundary elements of `boundary`
   * (its cells, see GetRemotePoints()), marking the bins that their bounds overlap, as
   * vtkStaticCellLocator does with the bounds of the cells. The bounds of each element are
   * inflated by `tolerance` times their diagonal, as the points that lie on an element are within
   * `tolerance` times its size from it. Without cells, all the bins are occupied.
   */
  static void BinBoundaryElements(BoundaryBins& bins, vtkPolyData* boundary, double tolerance);

  /**
   * Helper for the exchange of points between partitions: the points of the boundary `source` of
   * a partition that are in the occupied bins of a neighbor, with their point data, as a
   * vtkPolyData without cells.
   */
  static vtkSmartPointer<vtkPolyData> ExtractPointsInBins(
    vtkPolyData* source, const BoundaryBins& bins);
#endif

  /**
   * Helper for GetRemotePoints(): merge the points `received` from other partitions, by partition
   * id, in one point set. `reference` is the true boundary of the partition that receives them:
   * the received points must have point data arrays with the same names and numbers of components,
   * in the same order, otherwise they are ignored, with a warning. Their numeric types may differ,
   * e.g., after a serialization, which turns char into signed char: they are converted. Returns
   * nullptr if there is none.
   */
  vtkSmartPointer<vtkPolyData> MergeReceivedPoints(
    const std::map<int, vtkSmartPointer<vtkPolyData>>& received, vtkPolyData* reference) const;

  double Tolerance = 1e-6;
  int NumberOfLevels = 0;
  bool Decompose = true;
  bool ComputeInverseMeasure = false;
  bool PassThroughCellIds = false;
  std::string OriginalCellIdsName = "vtkOriginalResolvedCellIds";

private:
  vtkResolveHangingNodes(const vtkResolveHangingNodes&) = delete;
  void operator=(const vtkResolveHangingNodes&) = delete;

  /**
   * Check the partitions `inputs`, whose results are written in `outputs` (see
   * ResolvePartitionedDataSet), and resolve them together with ResolvePartitions() for the
   * dimension of their cells. `partitionedDataSetIds` is as in ResolvePartitions(). Returns 0 if a
   * partition is not valid: it is then resolved as an empty one, since the other partitions and
   * processes still expect it in the parallel calls.
   */
  int CheckAndResolvePartitions(std::vector<vtkSmartPointer<vtkDataSet>> inputs,
    std::vector<vtkSmartPointer<vtkPointSet>> outputs,
    const std::vector<int>& partitionedDataSetIds);

  /**
   * Resolve together the partitions `inputs`, whose cells have dimension Dim (2 for quads, 3 for
   * hexahedra), vtkPolyData, vtkUnstructuredGrid or vtkCartesianGrid, and write the results in
   * `outputs`, of the type of the input, but vtkUnstructuredGrid for a vtkCartesianGrid. The
   * candidate cells of a vtkCartesianGrid are its boundary cells, and it is not resolved by
   * itself. `partitionedDataSetIds` gives, for every partition, -1 to resolve it with all the other
   * partitions, or the index of its vtkPartitionedDataSet to resolve it only with the partitions
   * of other vtkPartitionedDataSet, see GetRemotePoints().
   */
  template <int Dim>
  int ResolvePartitions(const std::vector<vtkSmartPointer<vtkDataSet>>& inputs,
    const std::vector<vtkSmartPointer<vtkPointSet>>& outputs,
    const std::vector<int>& partitionedDataSetIds);

  /**
   * Resolve together the partitions of `inputPDS`, which may be null, into the partitions of
   * `outputPDS`, which has its structure. Every resolution makes parallel calls that all the
   * processes must take part in, even without partitions.
   */
  int ResolvePartitionedDataSet(vtkPartitionedDataSet* inputPDS, vtkPartitionedDataSet* outputPDS);

  // The largest point global id that the new global ids of the center points follow: the largest
  // one of the whole input, then of the center points numbered so far, so that the
  // vtkPartitionedDataSet of a collection, resolved one after the other, get unique global ids.
  vtkIdType MaxPointGlobalId = -1;
};

VTK_ABI_NAMESPACE_END
#endif
