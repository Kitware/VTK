/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYGhostUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDIYGhostUtilities
 * @brief Utilities to produce ghost cells between a collection of data sets of same type.
 *
 * vtkDIYGhostUtilities is a set of utilities which produce ghost cells between
 * a collection of data sets of same type, using DIY.
 *
 * Ghosts are computed from scratch, even if parts of the input already own ghosts. In such
 * instance, ghosts are treated as if they didn't exist in the input.
 *
 * Mixed types in the input `vtkCompositeDataSet` are supported, in the sense that ghost cells are
 * only exchanged between blocks of same type. Similarly, ghosts are exchanged between blocks that
 * connect.
 *
 * Blocks connect with different criteria depending on their types:
 *
 * - `vtkImageData`: Blocks connect if they share face / edge / corner, if they share same
 *   dimension (1D / 2D / 3D), and if they have same orientation matrix and same spacing.
 * - `vtkRectilinearGrid`: Blocks connect if the x, y, and z coordinate arrays match at their
 *   interfaces.
 * - `vtkStructuredGrid`: Blocks connect if external faces of the grids are mangled, regardless of
 *   relative inner orientation. In other words, if grid 1 is spanned by (i1, j1, k1) indexing, and
 *   grid 2 is spanned by (i2, j2, k2) indexing, if on one face, (j1, k1) connects with (-i2, -j2),
 *   those 2 grids are connected and will exchange ghosts. Two grids partially fitting are
 *   discarded. In order for 2 grids to fit, one corner from one face of one grid needs to be an
 *   existing point in one face of the second grid. Additionally, every points from this corner on
 *   both grids need to match until the opposite corner (opposite w.r.t. each dimension) is reached,
 *   and this opposite corner of the grid junction needs to be a corner of either grid.
 *
 * @note Currently, only `vtkImageData`, `vtkRectilinearGrid` and `vtkStructuredGrid` are
 * implemented. Unless there is determining structural data added to subclasses of those classes,
 * this filter should work well on subclasses of supported types.
 */
#ifndef vtkDIYGhostUtilities_h
#define vtkDIYGhostUtilities_h

#include "vtkDIYExplicitAssigner.h" // For DIY assigner
#include "vtkDIYUtilities.h"        // For Block
#include "vtkObject.h"
#include "vtkParallelDIYModule.h" // For export macros
#include "vtkQuaternion.h"        // For vtkImageData
#include "vtkSmartPointer.h"      // For vtkSmartPointer

#include <array>  // For VectorType and ExtentType
#include <map>    // For BlockMapType
#include <set>    // For Link
#include <vector> // For LinkMap

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/master.hpp)
// clang-format on

#include "vtkDataArray.h"

class vtkDataArray;
class vtkDataSet;
class vtkFieldData;
class vtkIdList;
class vtkImageData;
class vtkMatrix3x3;
class vtkMultiProcessController;
class vtkPoints;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnsignedCharArray;

class VTKPARALLELDIY_EXPORT vtkDIYGhostUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkDIYGhostUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Convenient typedefs.
   */
  using VectorType = std::array<double, 3>;
  using QuaternionType = vtkQuaternion<double>;
  using ExtentType = std::array<int, 6>;
  template <class T>
  using BlockMapType = std::map<int, T>;
  using Links = std::set<int>;
  using LinkMap = std::vector<Links>;
  //@}

  /**
   * This helper structure owns a typedef to the block type of `DataSetT` used with diy to generate
   * ghosts. This block type is defined as `DataSetTypeToBlockTypeConverter<DataSetT>::BlockType`.
   */
  template <class DataSetT>
  struct DataSetTypeToBlockTypeConverter;

protected:
  /**
   * Structure to inherit from for data sets having a grid topology.
   */
  struct GridBlockStructure
  {
    /**
     * `GridBlockStructure` constructor. It takes the extent of a neighbor block as input.
     */
    GridBlockStructure(const int* extent, int dim);

    /**
     * Extent of neighboring block with no ghosts.
     */
    ExtentType Extent;

    /**
     * Extent of neighboring block that include ghost layers.
     */
    ExtentType ExtentWithNewGhosts;

    /**
     * Binary mask encoding the adjacency of the neighboring block w.r.t. current block.
     * This mask shall be written / read using Adjacency enumeration.
     */
    unsigned char AdjacencyMask;

    /**
     * This stores the dimension of the grid (1D, 2D, or 3D).
     */
    int DataDimension;
  };

  /**
   * Structure storing information needed by a block on it's own grid structure.
   */
  struct GridInformation
  {
    /**
     * Extent without ghost layers.
     */
    ExtentType Extent;

    ExtentType ExtentGhostThickness;
  };

  /**
   * Block structure storing information being communicated by neighboring blocks for
   * `vtkImageData`.
   */
  struct ImageDataBlockStructure : public GridBlockStructure
  {
    //@{
    /**
     * Constructor taking the extent (without ghosts) of the neighboring `vtkImageData`, as well
     * as well as the image's origin, spacing, and orientation.
     */
    ImageDataBlockStructure(const int extent[6], int dim, const double origin[3],
      const double spacing[3], const double orientationQuaternion[4]);
    ImageDataBlockStructure(const int extent[6], int dim, const double origin[3],
      const double spacing[3], vtkMatrix3x3* directionMatrix);
    //@}

    /**
     * Copy constructor.
     */
    ImageDataBlockStructure(vtkImageData* image, const GridInformation& info);

    /**
     * Origin of the neighboring `vtkImageData`.
     */
    VectorType Origin;

    /**
     * Spacing of the neighboring `vtkImageData`.
     */
    VectorType Spacing;

    /**
     * Orientation of the neighboring `vtkImageData`.
     */
    QuaternionType OrientationQuaternion;
  };

  struct RectilinearGridInformation : public GridInformation
  {
    //@{
    /**
     * Point coordinates without ghosts.
     */
    vtkSmartPointer<vtkDataArray> XCoordinates;
    vtkSmartPointer<vtkDataArray> YCoordinates;
    vtkSmartPointer<vtkDataArray> ZCoordinates;
    //@}

    /**
     * Coordinates of ghosts copied from connected neighbors.
     * This array maps to extents, i.e. the first element of this array stores the coordinates of
     * the left side of the grid, the second the right side of the grid, and so on.
     */
    vtkSmartPointer<vtkDataArray> CoordinateGhosts[6];
  };

  /**
   * Block structure storing information being communicated by neighboring blocks for
   * `vtkRectilinearGrid`.
   */
  struct RectilinearGridBlockStructure : public GridBlockStructure
  {
    /**
     * Constructor taking the extent (without ghosts) of the neighboring `vtkRectilinearGrid`,
     * as well as its point coordinates.
     */
    RectilinearGridBlockStructure(const int extent[6], int dim, vtkDataArray* xCoordinates,
      vtkDataArray* yCoordinates, vtkDataArray* zCoordinates);

    /**
     * Copy constructor.
     */
    RectilinearGridBlockStructure(vtkRectilinearGrid* grid, const RectilinearGridInformation& info);

    //@{
    /**
     * Point coordinate arrays of the rectilinear grid.
     */
    vtkSmartPointer<vtkDataArray> XCoordinates;
    vtkSmartPointer<vtkDataArray> YCoordinates;
    vtkSmartPointer<vtkDataArray> ZCoordinates;
    //@}
  };

  struct StructuredGridInformation : public GridInformation
  {
    /**
     * This structure represents the set of points and their corresponding extent
     * of an external face of the structured grid.
     */
    struct OuterPointLayersType
    {
      /**
       * Points of an external face.
       */
      vtkSmartPointer<vtkPoints> Points;

      /**
       * Extent (which represents a 2D, 1D, or 0D grid), of an external face.
       */
      ExtentType Extent;
    };

    /**
     * Array of 6 external faces. Each outer point layer holds a copy of the points of the external
     * face, as well as its extent. This array is indexed in the same fashion as grid extents.
     */
    OuterPointLayersType OuterPointLayers[6];

    /**
     * Handle on input points for current block.
     */
    vtkPoints* InputPoints;
  };

  /**
   * Block structure storing information being communicated by neighboring blocks for
   * `vtkStructuredGrid`.
   */
  struct StructuredGridBlockStructure : public GridBlockStructure
  {
    /**
     * Constructor taking the extent (without ghosts) of the neighboring `vtkStructuredGrid`,
     * as well as its point array.
     */
    StructuredGridBlockStructure(const int extent[6], int dim, vtkDataArray* points[6]);

    /**
     * Copy constructor.
     */
    StructuredGridBlockStructure(vtkStructuredGrid* grid, const StructuredGridInformation& info);

    /**
     * Point coordinate arrays of the structured grid.
     */
    vtkSmartPointer<vtkPoints> OuterPointLayers[6];

    /**
     * Grid interfacing with block's grid.
     * This grid is a 2D grid and can be arbritrarely oriented depending on how grids connect.
     */
    struct Grid2D
    {
      /**
       * Start point extent coordinate in the x dimension
       */
      int StartX = 0;

      /**
       * Start point extent coordinate in the y dimension
       */
      int StartY = 0;

      /**
       * End point extent coordinate in the x dimension
       */
      int EndX = 0;

      /**
       * End point extent coordinate in the y dimension
       */
      int EndY = 0;

      /**
       * Orientation of the x dimension. (Either +1 or -1)
       */
      int XOrientation = 0;

      /**
       * Orientation of the y dimension. (Either +1 or -1)
       */
      int YOrientation = 0;

      /**
       * Index of the extent of the current grid. This is a value between 0 and 5, describing which
       * external face of the structured grid this 2D grid represents.
       */
      int ExtentId = -1;
    };

    /**
     * 2D grid interfacing 2 blocks.
     *
     * @note This grid can be 1D or 0D.
     */
    Grid2D GridInterface;
  };

  /**
   * Base ghost buffer structure holding point data and cell data.
   */
  template <class DataSetT>
  struct ImplicitGeometryGhosts
  {
    /**
     * Convenient typedef.
     */
    using BlockType =
      typename vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

    virtual ~ImplicitGeometryGhosts() = default;

    //@{
    /**
     * Methods to be called by `GhostBuffersHelper::Enqueue`.
     * `pointIds` / `cellIds` are the set of ids mapping to the data to be enqueued.
     */
    static void EnqueuePointBuffers(const diy::Master::ProxyWithLink& cp,
      const diy::BlockID& blockId, DataSetT* input, vtkIdList* pointIds);
    static void EnqueueCellBuffers(const diy::Master::ProxyWithLink& cp,
      const diy::BlockID& blockId, DataSetT* input, vtkIdList* cellIds);
    //@}

    //@{
    /**
     * Methods to be called by `GhostBuffersHelper::Dequeue`.
     * `pointIds` / `cellIds` are the set of ids mapping to the data to be dequeued.
     */
    virtual void DequeuePointBuffers(const diy::Master::ProxyWithLink& cp, int gid);
    virtual void DequeueCellBuffers(const diy::Master::ProxyWithLink& cp, int gid);
    //@}

    //@{
    /**
     * Methods to be called by `GhostBuffersHelper::FillReceivedGhosts`.
     * This method fills the points / cells of ids `pointIds` / `cellIds` with the received buffers
     * when exchanging ghosts.
     */
    virtual void FillReceivedGhostPoints(
      BlockType* block, DataSetT* output, vtkIdList* pointIds) const;
    virtual void FillReceivedGhostCells(
      BlockType* block, DataSetT* output, vtkIdList* cellIds) const;
    //@}

    //@{
    /**
     * Point and cell data that have been sent by this block's neighbor.
     */
    vtkSmartPointer<vtkFieldData> PointData;
    vtkSmartPointer<vtkFieldData> CellData;
    //@}
  };

  /**
   * Ghost buffer holding point data, cell data, as well as points.
   */
  template <class DataSetT>
  struct ExplicitPointGeometryGhosts : public ImplicitGeometryGhosts<DataSetT>
  {
    //@{
    /**
     * Convenient typedefs.
     */
    using Superclass = ImplicitGeometryGhosts<DataSetT>;
    using BlockType = typename Superclass::BlockType;
    //@}

    ~ExplicitPointGeometryGhosts() override = default;

    /**
     * See `Superclass`. In addition to enqueueing point and cell data, this method enqueues point
     * positions as well.
     */
    static void EnqueuePointBuffers(const diy::Master::ProxyWithLink& cp,
      const diy::BlockID& blockId, DataSetT* input, vtkIdList* pointIds);

    /**
     * See `Superclass`. In addition to dequeueing point and cell data, this method dequeues point
     * positions as well.
     */
    void DequeuePointBuffers(const diy::Master::ProxyWithLink& cp, int gid) override;

    /**
     * See `Superclass`. In addition to filling point and cell data, this method filles point
     * positions as well.
     */
    void FillReceivedGhostPoints(
      BlockType* block, DataSetT* output, vtkIdList* pointIds) const override;

    /**
     * Points that have been sent by this block's neighbor.
     */
    vtkNew<vtkPoints> Points;
  };

public:
  /**
   * Block structure to be used for diy communication.
   * This is a generic structure split into 3 main components:
   *   - `BlockStructureType`: This holds all information sent by another block that are necessary
   * to determine whether this block is to be connected with ourself.
   *   - `InformationType`: This holds all information from the input necessary to allocate the
   *   output for this block. This can include bounds, extent, etc.
   *   - `GhostBuffersType`: This is a list of buffers receiving ghost information from connected
   * blocks. Typically, cell data and / or point data and / or explicit geometry.
   */
  template <class BlockStructureT, class InformationT, class GhostBuffersT>
  struct Block : public diy::Serialization<vtkFieldData*>
  {
    //@{
    /**
     * Typedef handle on block structure and block information
     */
    typedef BlockStructureT BlockStructureType;
    typedef InformationT InformationType;
    typedef GhostBuffersT GhostBuffersType;
    //@}

    /**
     * `BlockStructures` maps a neighboring block globald id to its block structure.
     */
    BlockMapType<BlockStructureType> BlockStructures;

    /**
     * `InformationT` holds any information from the current block that is necessary to exchange
     * ghosts. This is typically used when sending ghosts to neighboring blocks.
     */
    InformationType Information;

    /**
     * `GhostsT` holds buffers used to receive data sent by other blocks.
     */
    BlockMapType<GhostBuffersType> GhostBuffers;
    //@}

    vtkSmartPointer<vtkUnsignedCharArray> GhostCellArray;
    vtkSmartPointer<vtkUnsignedCharArray> GhostPointArray;
  };

  //@{
  /**
   * Block typedefs.
   */
  using ImageDataBlock =
    Block<ImageDataBlockStructure, GridInformation, ImplicitGeometryGhosts<vtkImageData>>;
  using RectilinearGridBlock = Block<RectilinearGridBlockStructure, RectilinearGridInformation,
    ImplicitGeometryGhosts<vtkRectilinearGrid>>;
  using StructuredGridBlock = Block<StructuredGridBlockStructure, StructuredGridInformation,
    ExplicitPointGeometryGhosts<vtkStructuredGrid>>;
  //@}

  /**
   * Main pipeline generating ghosts. It takes as parameters a list of `DataSetT` for the `inputs`
   * and the `outputs`.
   *
   * `outputs` need to be already allocated and be of same size as `inputs`.
   */
  template <class DataSetT>
  static int GenerateGhostCells(std::vector<DataSetT*>& inputsDS, std::vector<DataSetT*>& outputsDS,
    int inputGhostLevels, int outputGhostLevels, vtkMultiProcessController* controller);

protected:
  vtkDIYGhostUtilities();
  ~vtkDIYGhostUtilities() override;

  /**
   * This helper class automatizes enqueueing ghosts depending on the ghost data needed to be
   * shared. This includes point data and / or cell data and / or geometry.
   */
  template <class DataSetT>
  class GhostBuffersHelper
  {
  public:
    //@{
    /**
     * Convenient typedefs.
     */
    using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;
    using GhostBuffersType = typename BlockType::GhostBuffersType;
    //@}

    /**
     * This method enqueues ghosts from local block and sends them to block of id `blockId`.
     * `input` corresponds to the input data set mapped to input `block`.
     */
    static void Enqueue(const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId,
      DataSetT* input, BlockType* block);

    /**
     * This method dequeues the ghost data that was sent by block of global id `gid` using
     * `GhostBuffersHelper::Enqueue`.
     */
    static void Dequeue(
      const diy::Master::ProxyWithLink& cp, int gid, GhostBuffersType& ghostBuffers);

    /**
     * This method initializes the ghost arrays for the output of current block.
     * After calling this method, the current block will own an allocated buffer of ghost arrays for
     * cell and point data, filled by default with zero everywhere.
     */
    static void InitializeGhostArrays(BlockType* block, DataSetT* output);

    /**
     * This method fills already initialized cell and point ghost arrays with the correct ghost
     * types.
     */
    static void FillReceivedGhosts(BlockType* block, int gid, DataSetT* output);

    /**
     * This method adds the ghost arrays from current blocks into the output.
     */
    static void AddGhostArrays(BlockType* block, DataSetT* output);
  };

  template <class DataSetT>
  static void InitializeGhostPointArray(
    typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output);

  template <class DataSetT>
  static void InitializeGhostCellArray(
    typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output);

  //@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * In this method, given `BlockType` the block type for the data set type,
   * `BlockType::InformationType` is filled with all information needed from the input to create the
   * output.
   */
  static void SetupBlockSelfInformation(diy::Master& master, std::vector<vtkImageData*>& inputs);
  static void SetupBlockSelfInformation(
    diy::Master& master, std::vector<vtkRectilinearGrid*>& inputs);
  static void SetupBlockSelfInformation(
    diy::Master& master, std::vector<vtkStructuredGrid*>& inputs);
  //@}

  //@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * It computes the list of cell ids in `input` to be sent to current `block`'s neighbor of global
   * id `gid`.
   */
  static vtkSmartPointer<vtkIdList> ComputeInputInterfaceCellIds(
    const ImageDataBlock* block, int gid, vtkImageData* input);
  static vtkSmartPointer<vtkIdList> ComputeInputInterfaceCellIds(
    const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input);
  static vtkSmartPointer<vtkIdList> ComputeInputInterfaceCellIds(
    const StructuredGridBlock* block, int gid, vtkStructuredGrid* input);
  //@}

  //@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * It computes the list of cell ids in `output` that should be filled with what was sent by
   * `block` of id `gid`.
   */
  static vtkSmartPointer<vtkIdList> ComputeOutputInterfaceCellIds(
    const ImageDataBlock* block, int gid, vtkImageData* input);
  static vtkSmartPointer<vtkIdList> ComputeOutputInterfaceCellIds(
    const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input);
  static vtkSmartPointer<vtkIdList> ComputeOutputInterfaceCellIds(
    const StructuredGridBlock* block, int gid, vtkStructuredGrid* input);
  //@}

  //@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * It computes the list of point ids in `input` to be sent to current `block`'s neighbor of global
   * id `gid`.
   */
  static vtkSmartPointer<vtkIdList> ComputeInputInterfacePointIds(
    const ImageDataBlock* block, int gid, vtkImageData* input);
  static vtkSmartPointer<vtkIdList> ComputeInputInterfacePointIds(
    const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input);
  static vtkSmartPointer<vtkIdList> ComputeInputInterfacePointIds(
    const StructuredGridBlock* block, int gid, vtkStructuredGrid* input);
  //@}

  //@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * It computes the list of point ids in `output` that should be filled with what was sent by
   * `block` of id `gid`.
   */
  static vtkSmartPointer<vtkIdList> ComputeOutputInterfacePointIds(
    const ImageDataBlock* block, int gid, vtkImageData* input);
  static vtkSmartPointer<vtkIdList> ComputeOutputInterfacePointIds(
    const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input);
  static vtkSmartPointer<vtkIdList> ComputeOutputInterfacePointIds(
    const StructuredGridBlock* block, int gid, vtkStructuredGrid* input);
  //@}

  //@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * This method is called in the pipeline right after diy environment
   * has been set up. It exchanges between every information needed for the data set type `DataSetT`
   * in order to be able to set link connections.
   */
  static void ExchangeBlockStructures(diy::Master& master, const vtkDIYExplicitAssigner& assigner,
    std::vector<vtkImageData*>& inputs, int inputGhostLevels);
  static void ExchangeBlockStructures(diy::Master& master, const vtkDIYExplicitAssigner& assigner,
    std::vector<vtkRectilinearGrid*>& inputs, int inputGhostLevels);
  static void ExchangeBlockStructures(diy::Master& master, const vtkDIYExplicitAssigner& assigner,
    std::vector<vtkStructuredGrid*>& inputs, int inputGhostLevels);
  //@}

  //@{
  /**
   * Method to be overloaded for each supported input data saet type,
   * that computes the minimal link map being necessary to exchange ghosts.
   * This method is called after `master` has been relinked.
   */
  static LinkMap ComputeLinkMapAndAllocateGhosts(const diy::Master& master,
    std::vector<vtkImageData*>& inputs, std::vector<vtkImageData*>& outputs, int outputGhostLevels);
  static LinkMap ComputeLinkMapAndAllocateGhosts(const diy::Master& master,
    std::vector<vtkRectilinearGrid*>& inputs, std::vector<vtkRectilinearGrid*>& outputs,
    int outputGhostLevels);
  static LinkMap ComputeLinkMapAndAllocateGhosts(const diy::Master& master,
    std::vector<vtkStructuredGrid*>& inputs, std::vector<vtkStructuredGrid*>& outputs,
    int outputGhostLevels);
  //@}

  /**
   * This method exchanges ghosts between connected blocks.
   */
  template <class DataSetT>
  static void ExchangeGhosts(diy::Master& master, std::vector<DataSetT*>& inputs);

  /**
   * This methods allocate a point and cell ghost array and fills it with 0.
   */
  template <class DataSetT>
  static void InitializeGhostArrays(diy::Master& master, std::vector<DataSetT*>& outputs);

  /**
   * This method sets every ghost array where data was exchanged to a duplicate ghost.
   */
  template <class DataSetT>
  static void FillReceivedGhosts(const diy::Master& master, std::vector<DataSetT*>& outputs);

  /**
   * Adds ghost arrays, which are present in blocks of `master`, to `outputs` point and / or cell
   * data.
   */
  template <class DataSetT>
  static void AddGhostArrays(diy::Master& master, std::vector<DataSetT*>& outputs);

  //@{
  /**
   * This method sets the ghost arrays in the output. Ghosts have to be already allocated.
   */
  static void FillGhostArrays(const diy::Master& master, std::vector<vtkImageData*>& outputs);
  static void FillGhostArrays(const diy::Master& master, std::vector<vtkRectilinearGrid*>& outputs);
  static void FillGhostArrays(const diy::Master& master, std::vector<vtkStructuredGrid*>& outputs);
  //@}

private:
  vtkDIYGhostUtilities(const vtkDIYGhostUtilities&) = delete;
  void operator=(const vtkDIYGhostUtilities&) = delete;
};

#include "vtkDIYGhostUtilities.txx"

#endif
