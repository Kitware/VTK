// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * - `vtkStructuredGrid`: Blocks connect if external faces of the grids are mangled together,
 *   regardless of relative inner orientation. In other words, if grid 1 is spanned by (i1, j1, k1)
 *   indexing, and grid 2 is spanned by (i2, j2, k2) indexing, if on one face, (j1, k1) connects
 *   with (-i2, -j2), those 2 grids are connected and will exchange ghosts. Two grids partially
 *   fitting are discarded. In order for 2 grids to fit, one corner from one face of one grid needs
 *   to be an existing point in one face of the second grid. Additionally, every points from this
 *   corner on both grids need to match until the opposite corner (opposite w.r.t. each dimension)
 *   is reached, and this opposite corner of the grid junction needs to be a corner of either grid.
 * - `vtkUnstructuredGrid`: Blocks connect if the external surface of neighboring grids match.
 *   To do so, only points are looked at. If at least one point matches a point in a neighboring
 *   grid, then they are connected. If there are no point global ids in the input, the 3D position
 *   of the points is used to determine if grids match, up to floating point precision in the
 *   coordinates. Note that integer coordinates can be used with this pipeline. If global ids are
 *   present in the input point data, then the pipeline will only look at matching global ids, and
 *   ignore point positions.
 * - `vtkPolyData`: Blocks connect if the boundary edges of neighboring poly data match. The filter
 *   behaves the same way it does with `vtkUnstructuredGrid`. Point positions are used to match
 *   points if point global ids are not present, and point global ids are used instead if they are
 *   present.
 *
 * Points at the interface between 2 partitions are edited depending on the ownership of the point
 * after the ghost points are generated. One can keep track of which process owns a non-ghost copy
 * of the point if an array associating each point with its process id is available in the input.
 *
 * @note Currently, only `vtkImageData`, `vtkRectilinearGrid`, `vtkStructuredGrid`,
 * `vtkUnstructuredGrid` and `vtkPolyData` are
 * implemented. Unless there is determining structural data added to subclasses of those classes,
 * this filter should work well on subclasses of supported types.
 *
 * @warning This warning only applies for `vtkUnstructuredGrid` and `vtkPolyData` inputs. If
 * there are duplicate points in the outer shell of an input partition, then this filter cannot
 * decide on how to connect the cells properly when generating ghosts. The same phenomenon occurs
 * when the outer shell of the partition has 2 points with the same global id. In such
 * circumstances, use the `vtkStaticCleanUnstructuredGrid`
 * or `vtkStaticCleanPolyData` filter first in order to have a clean input.
 */
#ifndef vtkDIYGhostUtilities_h
#define vtkDIYGhostUtilities_h

#include "vtkBoundingBox.h"         // For ComputeLinkMap
#include "vtkDIYExplicitAssigner.h" // For DIY assigner
#include "vtkDIYUtilities.h"        // For Block
#include "vtkNew.h"                 // For vtkNew
#include "vtkObject.h"
#include "vtkParallelDIYModule.h" // For export macros
#include "vtkQuaternion.h"        // For vtkImageData
#include "vtkSmartPointer.h"      // For vtkSmartPointer

#include <array>         // For VectorType and ExtentType
#include <map>           // For BlockMapType
#include <set>           // For Link
#include <unordered_map> // For BlockMapType
#include <vector>        // For LinkMap

// clang-format off
#include "vtk_diy2.h" // Third party include
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/partners/all-reduce.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractPointLocator;
class vtkAlgorithm;
class vtkCellArray;
class vtkDataArray;
class vtkDataSet;
class vtkFieldData;
class vtkIdList;
class vtkIdTypeArray;
class vtkImageData;
class vtkMatrix3x3;
class vtkMultiProcessController;
class vtkPoints;
class vtkPointSet;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnsignedCharArray;
class vtkUnstructuredGrid;

class VTKPARALLELDIY_EXPORT vtkDIYGhostUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkDIYGhostUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
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
  ///@}

  /**
   * This helper structure owns a typedef to the block type of `DataSetT` used with diy to generate
   * ghosts. This block type is defined as DataSetTypeToBlockTypeConverter<DataSetT>::BlockType.
   */
  template <class DataSetT>
  struct DataSetTypeToBlockTypeConverter;

protected:
  /**
   * Base block structure for data sets.
   */
  struct DataSetBlockStructure
  {
    vtkSmartPointer<vtkFieldData> GhostCellData = nullptr;
    vtkSmartPointer<vtkFieldData> GhostPointData = nullptr;
  };

  struct DataSetInformation
  {
    virtual ~DataSetInformation() = default;

    /**
     * @warning This method does not work before the link map between blocks is computed.
     */
    virtual bool InputNeedsGhostsPeeledOff() const = 0;
  };

  /**
   * Structure to inherit from for data sets having a structured grid topology.
   */
  struct GridBlockStructure : public DataSetBlockStructure
  {
    /**
     * `GridBlockStructure` constructor. It takes the extent of a neighbor block as input.
     */
    GridBlockStructure(const int* extent, int dim);

    /**
     * Extent of neighboring block with no ghosts.
     */
    ExtentType Extent = ExtentType{ 1, -1, 1, -1, 1, -1 };

    /**
     * Extent of neighboring block that include ghost layers, shifted to match our mapping of the
     * extent in the 3D world.
     */
    ExtentType ShiftedExtentWithNewGhosts;

    /**
     * Extent of the neighboring block, shifted to match our mapping of the extent in the 3D world.
     */
    ExtentType ShiftedExtent;

    ExtentType ReceivedGhostExtent = ExtentType{ 1, -1, 1, -1, 1, -1 };

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
  struct GridInformation : public DataSetInformation
  {
    ~GridInformation() override = default;

    bool InputNeedsGhostsPeeledOff() const override { return this->Extent != this->InputExtent; }

    /**
     * Extent without ghost layers.
     */
    ExtentType Extent = ExtentType{ 1, -1, 1, -1, 1, -1 };

    /**
     * Input extent without any modification.
     */
    ExtentType InputExtent = ExtentType{ 1, -1, 1, -1, 1, -1 };

    ExtentType ExtentGhostThickness;
  };

  struct ImageDataInformation : public GridInformation
  {
    ~ImageDataInformation() override = default;

    vtkImageData* Input;
  };

  /**
   * Block structure storing information being communicated by neighboring blocks for
   * `vtkImageData`.
   */
  struct ImageDataBlockStructure : public GridBlockStructure
  {
    ///@{
    /**
     * Constructor taking the extent (without ghosts) of the neighboring `vtkImageData`, as well
     * as well as the image's origin, spacing, and orientation.
     */
    ImageDataBlockStructure(const int extent[6], int dim, const double origin[3],
      const double spacing[3], const double orientationQuaternion[4]);
    ImageDataBlockStructure(const int extent[6], int dim, const double origin[3],
      const double spacing[3], vtkMatrix3x3* directionMatrix);
    ///@}

    /**
     * Copy constructor.
     */
    ImageDataBlockStructure(vtkImageData* image, const ImageDataInformation& info);

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
    ~RectilinearGridInformation() override = default;

    ///@{
    /**
     * Point coordinates without ghosts.
     */
    vtkSmartPointer<vtkDataArray> XCoordinates;
    vtkSmartPointer<vtkDataArray> YCoordinates;
    vtkSmartPointer<vtkDataArray> ZCoordinates;
    ///@}

    /**
     * Coordinates of ghosts copied from connected neighbors.
     * This array maps to extents, i.e. the first element of this array stores the coordinates of
     * the left side of the grid, the second the right side of the grid, and so on.
     */
    vtkSmartPointer<vtkDataArray> CoordinateGhosts[6];

    vtkRectilinearGrid* Input;
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

    ///@{
    /**
     * Point coordinate arrays of the rectilinear grid.
     */
    vtkSmartPointer<vtkDataArray> XCoordinates;
    vtkSmartPointer<vtkDataArray> YCoordinates;
    vtkSmartPointer<vtkDataArray> ZCoordinates;
    ///@}
  };

  struct StructuredGridInformation : public GridInformation
  {
    ~StructuredGridInformation() override = default;

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

    vtkStructuredGrid* Input;
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

    /**
     * Buffer to store received ghost points from neighboring blocks.
     */
    vtkNew<vtkPoints> GhostPoints;
  };

  struct UnstructuredDataInformation
  {
    bool InputNeedsGhostsPeeledOff() const
    {
      return this->OutputToInputCellIdRedirectionMap != nullptr;
    }

    /**
     * Bounding box of input.
     */
    vtkBoundingBox BoundingBox;

    /**
     * When the input has ghosts, this map is being used to copy input cells / cell data into
     * the output (with input ghosts peeled off).
     */
    vtkSmartPointer<vtkIdList> OutputToInputCellIdRedirectionMap = nullptr;

    ///@{
    /**
     * When the input has ghosts, this map is being used to copy input points / point data into
     * the output (with input ghosts peeled off).
     */
    vtkSmartPointer<vtkIdList> InputToOutputPointIdRedirectionMap = nullptr;
    vtkSmartPointer<vtkIdList> OutputToInputPointIdRedirectionMap = nullptr;
    ///@}

    /**
     * Filter that is being used to extract the surface of the input.
     * The surface is used to check how the interface of the input matches the ones of its
     * neighboring blocks.
     */
    vtkSmartPointer<vtkAlgorithm> InterfaceExtractor;

    /**
     * Handle to the local point ids of the surface of the input.
     * Those point ids are being used to rapidly match points of the surface to their original
     * location in the input.
     */
    vtkIdTypeArray* InterfacePointIds;

    /**
     * Handle to the points of the surface of the input.
     */
    vtkDataArray* InterfacePoints;

    /**
     * Handle to the point ids of the input surface, if present.
     */
    vtkSmartPointer<vtkIdTypeArray> InterfaceGlobalPointIds;

    ///@{
    /*
     * This is a cursor telling the amount of points / cells information,
     * that has
     * already been added to the output. This variable is used at the very end of the pipeline.
     */
    vtkIdType CurrentMaxPointId;
    vtkIdType CurrentMaxCellId;
    ///@}

    ///@{
    /**
     * Number of input points / cell in the input when ghosts are removed.
     */
    vtkIdType NumberOfInputPoints;
    vtkIdType NumberOfInputCells;
    ///@}
  };

  struct UnstructuredDataBlockStructure : public DataSetBlockStructure
  {
    /**
     * This lists the matching point ids to the interfacing points that are exchanged with current
     * neighboring block. Those ids correspond to local point ordering as indexed in the input.
     */
    vtkNew<vtkIdList> MatchingReceivedPointIds;

    /**
     * This array describes the same points as `MatchingReceivedPointIds`, but points are ordered
     * like in the current neighboring block. Point ids stored in this array map to the output.
     */
    vtkNew<vtkIdList> RemappedMatchingReceivedPointIdsSortedLikeTarget;

    /**
     * These are the interfacing points sent by the current neighboring block. They should match
     * a subset of the output of the surface filter which is in `UnstructuredDataInformation`.
     */
    vtkNew<vtkPoints> InterfacingPoints;

    /**
     * Point global ids of the interfacing surface sent to us by corresponding block, if present.
     */
    vtkSmartPointer<vtkIdTypeArray> InterfacingGlobalPointIds = nullptr;

    /**
     * Point global ids sent to us by neighboring block, if present. This array has the same
     * ordering as `GhostPoints`.
     */
    vtkSmartPointer<vtkIdTypeArray> GhostGlobalPointIds = nullptr;

    /**
     * Ghost points sent by the current neighboring block.
     */
    vtkNew<vtkPoints> GhostPoints;

    /**
     * This lists the ids of the points that we own and need to send to the current neighboring
     * block.
     */
    vtkNew<vtkIdList> PointIdsToSend;

    ///@{
    /**
     * It can happen that a point can be sent by multiple blocks. If those points are not carefully
     * tracked down, we can end up instantiating multiple times a point that should be created only
     * once. This array lists the potential duplicate point ids that are being send / received for
     * the current neighboring block.
     */
    vtkNew<vtkIdTypeArray> SharedPointIds;
    vtkSmartPointer<vtkIdTypeArray> ReceivedSharedPointIds;
    ///@}

    /**
     * This is a mapping from points that have been sent by the current neighboring block and have
     * already been added in the output points, to their location in the output point array.
     */
    std::map<vtkIdType, vtkIdType> RedirectionMapForDuplicatePointIds;

    /**
     * This lists the ids of the cells that we own and need to send to the current neighboring
     * block.
     */
    vtkNew<vtkIdList> CellIdsToSend;

    /**
     * Point data at the interface sent by our neighbor. We only receive point data from neighbors
     * of lower block id than us.
     */
    vtkSmartPointer<vtkFieldData> InterfacingPointData;
  };

  struct UnstructuredGridInformation : public UnstructuredDataInformation
  {
    /**
     * This is a cursor telling the amount of faces information, that has
     * already been added to the output. This variable is used at the very end of the pipeline.
     */
    vtkIdType CurrentFacesSize = 0;

    /**
     * This is a cursor telling the amount of faces information, that has
     * already been added to the output. This variable is used at the very end of the pipeline.
     */
    vtkIdType CurrentMaxFaceId = 0;

    /**
     * This is a cursor telling how much the output connectivity array is filled.
     * This is used at the very end of the pipeline.
     */
    vtkIdType CurrentConnectivitySize = 0;

    vtkCellArray* Faces = nullptr;
    vtkCellArray* FaceLocations = nullptr;

    vtkUnstructuredGrid* Input;

    /**
     * Cell connectivity array size of the input if the ghost cells are removed.
     */
    vtkIdType InputConnectivitySize = 0;

    /**
     * Faces array size of the input if the ghost cells are removed.
     */
    vtkIdType InputFacesSize = 0;

    /**
     * Number of Faces stored in Faces array of the input if the ghost cells are removed.
     */
    vtkIdType InputNumberOfFaces = 0;
  };

  struct UnstructuredGridBlockStructure : public UnstructuredDataBlockStructure
  {
    /**
     * Topology information for cells to be exchanged.
     */
    struct TopologyBufferType
    {
      vtkSmartPointer<vtkUnsignedCharArray> Types = nullptr;
      vtkSmartPointer<vtkCellArray> Faces = nullptr;
      vtkSmartPointer<vtkCellArray> FaceLocations = nullptr;
      vtkNew<vtkCellArray> CellArray;
    };

    TopologyBufferType SendBuffer;
    TopologyBufferType ReceiveBuffer;

    ///@{
    /**
     * Handle to the faces / connectivity size that we have to send to the neighboring block.
     */
    vtkIdType FacesSize = 0;
    vtkIdType FacesNum = 0;
    vtkIdType ConnectivitySize = 0;
    ///@}
  };

  struct PolyDataInformation : public UnstructuredDataInformation
  {
    vtkPolyData* Input;

    ///@{
    /**
     * This is a cursor telling how much the corresponding output connectivity array is filled.
     * This is used at the very end of the pipeline.
     */
    vtkIdType CurrentPolyConnectivitySize;
    vtkIdType CurrentStripConnectivitySize;
    vtkIdType CurrentLineConnectivitySize;
    ///@}

    ///@{
    /**
     * This is a cursor telling how many cells of corresponding types have been added so far.
     * This is used at the very end of the pipeline.
     */
    vtkIdType CurrentMaxPolyId = 0;
    vtkIdType CurrentMaxStripId = 0;
    vtkIdType CurrentMaxLineId = 0;
    ///@}

    ///@{
    /**
     * Number of cells of respective type when the input has its ghost cells removed
     */
    vtkIdType NumberOfInputVerts;
    vtkIdType NumberOfInputPolys;
    vtkIdType NumberOfInputStrips;
    vtkIdType NumberOfInputLines;
    ///@}

    ///@{
    /**
     * Cell connectivity array size of the input if ghost cells are removed.
     */
    vtkIdType InputVertConnectivitySize;
    vtkIdType InputPolyConnectivitySize;
    vtkIdType InputStripConnectivitySize;
    vtkIdType InputLineConnectivitySize;
    ///@{

    ///@{
    /**
     * In the event that the input has ghost cells, this maps the output cells (with input ghosts
     * removed) to the input cells.
     */
    vtkNew<vtkIdList> OutputToInputVertCellIdRedirectionMap;
    vtkNew<vtkIdList> OutputToInputLineCellIdRedirectionMap;
    vtkNew<vtkIdList> OutputToInputPolyCellIdRedirectionMap;
    vtkNew<vtkIdList> OutputToInputStripCellIdRedirectionMap;
    ///@}
  };

  struct PolyDataBlockStructure : public UnstructuredDataBlockStructure
  {
    ///@{
    /**
     * This lists the ids of the cells that we own and need to send to the current neighboring
     * block.
     */
    vtkNew<vtkIdList> PolyIdsToSend;
    vtkNew<vtkIdList> StripIdsToSend;
    vtkNew<vtkIdList> LineIdsToSend;
    ///@}

    struct TopologyBufferType
    {
      vtkNew<vtkCellArray> Polys;
      vtkNew<vtkCellArray> Strips;
      vtkNew<vtkCellArray> Lines;
    };

    TopologyBufferType SendBuffer;
    TopologyBufferType ReceiveBuffer;

    ///@{
    /**
     * Handle on the number of cells to send of the corresponding type.
     */
    vtkIdType NumberOfPolysToSend = 0;
    vtkIdType NumberOfStripsToSend = 0;
    vtkIdType NumberOfLinesToSend = 0;
    ///@}

    ///@{
    /**
     * Handle on the number of cells of corresponding type to be sent to the neighbor block.
     */
    vtkIdType PolyConnectivitySize = 0;
    vtkIdType StripConnectivitySize = 0;
    vtkIdType LineConnectivitySize = 0;
    ///@}
  };

private:
  struct DataSetBlock
  {
    std::unordered_map<int, std::unordered_map<vtkIdType, vtkIdType>>
      GlobalToLocalIds; //  Per attribute
    std::unordered_map<int, std::unordered_map<vtkIdType, vtkNew<vtkIdTypeArray>>>
      NeededGidsForBlocks; // Ghosts of this block per process id (per attribute)
    std::unordered_map<int, std::unordered_map<vtkIdType, vtkNew<vtkIdTypeArray>>>
      GhostGidsFromBlocks; // Ghosts of other blocks per partition id (per attribute)
  };

public:
  /**
   * Block structure to be used for diy communication.
   * This is a generic structure split into 3 main components:
   *   - `BlockStructureType`: This holds all information sent by another block that are necessary
   * to determine whether this block is to be connected with ourself.
   *   - `InformationType`: This holds all information from the input necessary to allocate the
   *   output for this block. This can include bounds, extent, etc.
   */
  template <class BlockStructureT, class InformationT>
  struct Block
  {
    ///@{
    /**
     * Typedef handle on block structure and block information
     */
    typedef BlockStructureT BlockStructureType;
    typedef InformationT InformationType;
    ///@}

    /**
     * `BlockStructures` maps a neighboring block globald id to its block structure.
     */
    BlockMapType<BlockStructureType> BlockStructures;

    /**
     * `InformationT` holds any information from the current block that is necessary to exchange
     * ghosts. This is typically used when sending ghosts to neighboring blocks.
     */
    InformationType Information;

    BlockMapType<vtkBoundingBox> NeighborBoundingBoxes;

    vtkBoundingBox BoundingBox;

    vtkSmartPointer<vtkUnsignedCharArray> GhostCellArray;
    vtkSmartPointer<vtkUnsignedCharArray> GhostPointArray;
  };

  ///@{
  /**
   * Block typedefs.
   */
  using ImageDataBlock = Block<ImageDataBlockStructure, ImageDataInformation>;
  using RectilinearGridBlock = Block<RectilinearGridBlockStructure, RectilinearGridInformation>;
  using StructuredGridBlock = Block<StructuredGridBlockStructure, StructuredGridInformation>;
  using UnstructuredDataBlock = Block<UnstructuredDataBlockStructure, UnstructuredDataInformation>;
  using UnstructuredGridBlock = Block<UnstructuredGridBlockStructure, UnstructuredGridInformation>;
  using PolyDataBlock = Block<PolyDataBlockStructure, PolyDataInformation>;
  ///@}

  /**
   * Synchronize ghost data to match non-ghost data.
   * Please see `vtkGhostCellsGenerator` for a finer description of what this method does, as it is
   * being used as a backend for this filter.
   *
   * `outputs` need to be already allocated and be of same size as `inputs`.
   */
  static int SynchronizeGhostData(std::vector<vtkDataSet*>& inputsDS,
    std::vector<vtkDataSet*>& outputsDS, vtkMultiProcessController* controller, bool syncCell,
    bool SyncPoint);

  /**
   * Main pipeline generating ghosts. It takes as parameters a list of `DataSetT` for the `inputs`
   * and the `outputs`.
   * Please see `vtkGhostCellsGenerator` for a finer description of what this method does, as it is
   * being used as a backend for this filter.
   *
   * `outputs` need to be already allocated and be of same size as `inputs`.
   */
  template <class DataSetT>
  static int GenerateGhostCells(std::vector<DataSetT*>& inputsDS, std::vector<DataSetT*>& outputsDS,
    int outputGhostLevels, vtkMultiProcessController* controller);

  ///@{
  /**
   * Method that can be used to avoid the compile-time overhead of the templated method
   * `GenerateGhostCells`. All this method does is call `GenerateGhostCells` with the appropriate
   * template parameter.
   */
  static int GenerateGhostCellsImageData(std::vector<vtkImageData*>& inputs,
    std::vector<vtkImageData*>& outputs, int outputGhostLevels,
    vtkMultiProcessController* controller);
  static int GenerateGhostCellsRectilinearGrid(std::vector<vtkRectilinearGrid*>& inputs,
    std::vector<vtkRectilinearGrid*>& outputs, int outputGhostLevels,
    vtkMultiProcessController* controller);
  static int GenerateGhostCellsStructuredGrid(std::vector<vtkStructuredGrid*>& inputs,
    std::vector<vtkStructuredGrid*>& outputs, int outputGhostLevels,
    vtkMultiProcessController* controller);
  static int GenerateGhostCellsPolyData(std::vector<vtkPolyData*>& inputs,
    std::vector<vtkPolyData*>& outputs, int outputGhostLevels,
    vtkMultiProcessController* controller);
  static int GenerateGhostCellsUnstructuredGrid(std::vector<vtkUnstructuredGrid*>& inputs,
    std::vector<vtkUnstructuredGrid*>& outputs, int outputGhostLevels,
    vtkMultiProcessController* controller);
  ///@}

protected:
  vtkDIYGhostUtilities();
  ~vtkDIYGhostUtilities() override;

  ///@{
  /**
   * Clone input data into output. The ghost array and the attributes GlobalIds and ProcessIds
   * are shallow copied as they're not updated. Others are deep copied.
   */
  static void CloneInputData(std::vector<vtkDataSet*>& inputs, std::vector<vtkDataSet*>& outputs,
    bool syncCell, bool syncPoint);
  static void CloneInputData(vtkDataSet* input, vtkDataSet* output, int fieldType);
  ///@}

  ///@{
  /**
   * Initialize vtkDataSet blocks for synchronizing ghost data.
   */
  static void InitializeBlocks(
    diy::Master& master, std::vector<vtkDataSet*>& inputs, bool syncCell, bool syncPoint);
  static void InitializeBlocks(
    diy::Master& master, std::vector<vtkDataSet*>& inputs, int fieldType, unsigned char ghostFlag);
  ///@}

  ///@{
  /**
   * Exchange global ids of data that needs to be synced to
   * owners of the data.
   */
  static void ExchangeNeededIds(
    diy::Master& master, const vtkDIYExplicitAssigner& assigner, bool syncCell, bool syncPoint);
  static void ExchangeNeededIds(
    diy::Master& master, const vtkDIYExplicitAssigner& assigner, int fieldType);
  ///@}

  ///@{
  /**
   * Compute link map using known information from blocks, eg. to whom it needs
   * to send data, and from whom it needs to receive data.
   */
  static LinkMap ComputeLinkMapUsingNeededIds(
    const diy::Master& master, bool syncCell, bool syncPoint);
  static void ComputeLinksUsingNeededIds(
    Links& links, vtkDIYGhostUtilities::DataSetBlock* block, int fieldType);
  ///@}

  ///@{
  /**
   * This method exchanges ghost data across partitions.
   */
  static void ExchangeFieldData(diy::Master& master, std::vector<vtkDataSet*>& inputs,
    std::vector<vtkDataSet*>& outputs, bool syncCell, bool syncPoint);
  static void ExchangeFieldData(diy::Master& master, std::vector<vtkDataSet*>& inputs,
    std::vector<vtkDataSet*>& outputs, int fieldType);
  ///@}

  /**
   * Reinitializes the bits that match the input bit mask in the input array to zero.
   */
  static void ReinitializeSelectedBits(vtkUnsignedCharArray* ghosts, unsigned char mask);

  /**
   * This method will set all ghosts points in `output` to zero. It will also
   * allocate a new ghost array if none is already present.
   */
  template <class DataSetT>
  static void InitializeGhostPointArray(
    typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output);

  /**
   * This method will set all ghosts cells in `output` to zero. It will also
   * allocate a new ghost array if none is already present.
   */
  template <class DataSetT>
  static void InitializeGhostCellArray(
    typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output);

  ///@{
  /**
   *
   */
  static void CloneGeometricStructures(
    std::vector<vtkImageData*>& inputs, std::vector<vtkImageData*>& outputs);
  static void CloneGeometricStructures(
    std::vector<vtkRectilinearGrid*>& inputs, std::vector<vtkRectilinearGrid*>& outputs);
  static void CloneGeometricStructures(
    std::vector<vtkStructuredGrid*>& inputs, std::vector<vtkStructuredGrid*>& outputs);
  static void CloneGeometricStructures(
    std::vector<vtkUnstructuredGrid*>& inputs, std::vector<vtkUnstructuredGrid*>& outputs);
  static void CloneGeometricStructures(
    std::vector<vtkPolyData*>& inputs, std::vector<vtkPolyData*>& outputs);
  ///@}

  ///@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * In this method, given `BlockType` the block type for the data set type,
   * `BlockType::InformationType` is filled with all information needed from the input to create the
   * output.
   */
  static void InitializeBlocks(diy::Master& master, std::vector<vtkImageData*>& inputs);
  static void InitializeBlocks(diy::Master& master, std::vector<vtkRectilinearGrid*>& inputs);
  static void InitializeBlocks(diy::Master& master, std::vector<vtkStructuredGrid*>& inputs);
  static void InitializeBlocks(diy::Master& master, std::vector<vtkUnstructuredGrid*>& inputs);
  static void InitializeBlocks(diy::Master& master, std::vector<vtkPolyData*>& inputs);
  ///@}

  /**
   * This method exchanges the bounding boxes among blocks.
   */
  template <class DataSetT>
  static void ExchangeBoundingBoxes(
    diy::Master& master, const vtkDIYExplicitAssigner& assigner, std::vector<DataSetT*>& inputs);

  template <class BlockT>
  static LinkMap ComputeLinkMapUsingBoundingBoxes(const diy::Master& master);

  ///@{
  /**
   * Method to be overloaded for each supported type of input data set.
   * This method is called in the pipeline right after diy environment
   * has been set up. It exchanges between every information needed for the data set type `DataSetT`
   * in order to be able to set link connections.
   */
  static void ExchangeBlockStructures(diy::Master& master, std::vector<vtkImageData*>& inputs);
  static void ExchangeBlockStructures(
    diy::Master& master, std::vector<vtkRectilinearGrid*>& inputs);
  static void ExchangeBlockStructures(diy::Master& master, std::vector<vtkStructuredGrid*>& inputs);
  static void ExchangeBlockStructures(
    diy::Master& master, std::vector<vtkUnstructuredGrid*>& inputs);
  static void ExchangeBlockStructures(diy::Master& master, std::vector<vtkPolyData*>& inputs);
  ///@}

  ///@{
  /**
   * Method to be overloaded for each supported input data set type,
   * that computes the minimal link map being necessary to exchange ghosts.
   * This method is called after `master` has been relinked.
   */
  static LinkMap ComputeLinkMap(
    const diy::Master& master, std::vector<vtkImageData*>& inputs, int outputGhostLevels);
  static LinkMap ComputeLinkMap(
    const diy::Master& master, std::vector<vtkRectilinearGrid*>& inputs, int outputGhostLevels);
  static LinkMap ComputeLinkMap(
    const diy::Master& master, std::vector<vtkStructuredGrid*>& inputs, int outputGhostLevels);
  static LinkMap ComputeLinkMap(
    const diy::Master& master, std::vector<vtkUnstructuredGrid*>& inputs, int outputGhostLevels);
  static LinkMap ComputeLinkMap(
    const diy::Master& master, std::vector<vtkPolyData*>& inputs, int outputGhostLevels);
  ///@}

  ///@{
  /**
   * This method enqueues ghosts between communicating blocks. One version of this method
   * is implemented per supported input types. It enqueues ghosts to block of id `blockId`.
   */
  static void EnqueueGhosts(const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId,
    vtkImageData* input, ImageDataBlock* block);
  static void EnqueueGhosts(const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId,
    vtkRectilinearGrid* input, RectilinearGridBlock* block);
  static void EnqueueGhosts(const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId,
    vtkStructuredGrid* input, StructuredGridBlock* block);
  static void EnqueueGhosts(const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId,
    vtkUnstructuredGrid* input, UnstructuredGridBlock* block);
  static void EnqueueGhosts(const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId,
    vtkPolyData* input, PolyDataBlock* block);
  ///@}

  ///@{
  /**
   * This method dequeues ghosts sent between communicating blocks. One version of this method
   * is implemented per supported input types. It receives data from block of id `gid` and
   * stores is inside `blockStructured`.
   */
  static void DequeueGhosts(
    const diy::Master::ProxyWithLink& cp, int gid, ImageDataBlockStructure& blockStructure);
  static void DequeueGhosts(
    const diy::Master::ProxyWithLink& cp, int gid, RectilinearGridBlockStructure& blockStructure);
  static void DequeueGhosts(
    const diy::Master::ProxyWithLink& cp, int gid, StructuredGridBlockStructure& blockStructure);
  static void DequeueGhosts(
    const diy::Master::ProxyWithLink& cp, int gid, UnstructuredGridBlockStructure& blockStructure);
  static void DequeueGhosts(
    const diy::Master::ProxyWithLink& cp, int gid, PolyDataBlockStructure& blockStructure);
  ///@}

  /**
   * Copy the inputs into the outputs. Shallow copying is performed if possible. If not,
   * a deep copy is done.
   */
  template <class DataSetT>
  static void CopyInputsAndAllocateGhosts(diy::Master& master, diy::Assigner& assigner,
    diy::RegularAllReducePartners& partners, std::vector<DataSetT*>& inputs,
    std::vector<DataSetT*>& outputs, int outputGhostLevels);

  ///@{
  /**
   * Method to be overloaded for each supported input data set type,
   * This method allocates ghosts in the output. At the point of calling this method,
   * ghosts should have already been exchanged (see `ExchangeGhosts`).
   */
  static void DeepCopyInputAndAllocateGhosts(
    ImageDataBlock* block, vtkImageData* input, vtkImageData* outputs);
  static void DeepCopyInputAndAllocateGhosts(
    RectilinearGridBlock* block, vtkRectilinearGrid* input, vtkRectilinearGrid* outputs);
  static void DeepCopyInputAndAllocateGhosts(
    StructuredGridBlock* block, vtkStructuredGrid* input, vtkStructuredGrid* outputs);
  static void DeepCopyInputAndAllocateGhosts(
    UnstructuredGridBlock* block, vtkUnstructuredGrid* input, vtkUnstructuredGrid* outputs);
  static void DeepCopyInputAndAllocateGhosts(
    PolyDataBlock* block, vtkPolyData* input, vtkPolyData* outputs);
  ///@}

  /**
   * This method exchanges ghosts between connected blocks.
   */
  template <class DataSetT>
  static bool ExchangeGhosts(diy::Master& master, diy::Assigner& assigner,
    diy::RegularAllReducePartners& partners, std::vector<DataSetT*>& inputs);

  /**
   * This methods allocate a point and cell ghost array and fills it with 0.
   */
  template <class DataSetT>
  static void InitializeGhostArrays(
    diy::Master& master, std::vector<DataSetT*>& outputs, int outputGhostLevels);

  /**
   * Adds ghost arrays, which are present in blocks of `master`, to `outputs` point and / or cell
   * data.
   */
  template <class DataSetT>
  static void AddGhostArrays(diy::Master& master, std::vector<DataSetT*>& outputs);

  ///@{
  /**
   * This method sets the ghost arrays in the output. Ghosts have to be already allocated.
   */
  static void FillGhostArrays(
    const diy::Master& master, std::vector<vtkImageData*>& outputs, int outputGhostLevels);
  static void FillGhostArrays(
    const diy::Master& master, std::vector<vtkRectilinearGrid*>& outputs, int outputGhostLevels);
  static void FillGhostArrays(
    const diy::Master& master, std::vector<vtkStructuredGrid*>& outputs, int outputGhostLevels);
  static void FillGhostArrays(
    const diy::Master& master, std::vector<vtkUnstructuredGrid*>& outputs, int outputGhostLevels);
  static void FillGhostArrays(
    const diy::Master& master, std::vector<vtkPolyData*>& outputs, int outputGhostLevels);
  ///@}

private:
  vtkDIYGhostUtilities(const vtkDIYGhostUtilities&) = delete;
  void operator=(const vtkDIYGhostUtilities&) = delete;

  ///@{
  /**
   * Internal method that inflates exchanged bounding boxes to better treat floating point precision
   * for points that are on the boundary of the bounding box.
   *
   * @note This method only does something for vtkUnstructuredGrid and vtkPolyData. The vtkDataSet
   * version is empty;
   */
  static void InflateBoundingBoxIfNecessary(
    vtkDataSet* vtkNotUsed(input), vtkBoundingBox& vtkNotUsed(bb));
  static void InflateBoundingBoxIfNecessary(vtkPointSet* input, vtkBoundingBox& bb);
  ///@}
};

VTK_ABI_NAMESPACE_END
#include "vtkDIYGhostUtilities.txx" // for template implementations

#endif
