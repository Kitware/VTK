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
 *
 * @note Currently, only `vtkImageData` and `vtkRectilinearGrid` are implemented.
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
class vtkRectilinearGrid;
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

  /**
   * Structure to inherit from for data sets having a grid topology.
   */
  struct GridBlockStructure
  {
    /**
     * `GridBlockStructure` constructor. It takes the extent of a neighbor block as input.
     */
    GridBlockStructure(const int* extent);

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
    ImageDataBlockStructure(const int extent[6], const double origin[3], const double spacing[3],
      const double orientationQuaternion[4]);
    ImageDataBlockStructure(const int extent[6], const double origin[3], const double spacing[3],
      vtkMatrix3x3* directionMatrix);
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
    RectilinearGridBlockStructure(const int extent[6], vtkDataArray* xCoordinates,
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

  // Block structure used for diy communication
  template <class BlockStructureT, class InformationT>
  struct Block : public diy::Serialization<vtkFieldData*>
  {
    /**
     * `BlockStructureType` is a structure that englobes all the information needed from connected
     * blocks to know which data to send and receive.
     */
    typedef BlockStructureT BlockStructureType;

    /**
     * `BlockStructures` maps a neighboring block globald id to its block structure.
     */
    BlockMapType<BlockStructureT> BlockStructures;

    /**
     * `InformationT` holds any information from the current block that is necessary to exchange
     * ghosts. This is typically used when sending ghosts to neighboring blocks.
     */
    InformationT Information;

    //@{
    /**
     * These map a neighbor block global id to a cell data / point data that is to be sent by this
     * neighbor.
     */
    BlockMapType<vtkSmartPointer<vtkFieldData>> CellDatas;
    BlockMapType<vtkSmartPointer<vtkFieldData>> PointDatas;
    //@}
  };

  //@{
  /**
   * Block typedefs.
   */
  using ImageDataBlock = Block<ImageDataBlockStructure, GridInformation>;
  using RectilinearGridBlock = Block<RectilinearGridBlockStructure, RectilinearGridInformation>;
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
  static void InitializeGhostArrays(std::vector<DataSetT*>& outputs,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays);

  /**
   * This method sets every ghost array where data was exchanged to a duplicate ghost.
   */
  template <class DataSetT>
  static void FillDuplicateGhosts(const diy::Master& master, std::vector<DataSetT*>& outputs,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays);

  //@{
  /**
   * This method sets the ghost arrays in the output. Ghosts have to be already allocated.
   */
  static void FillGhostArrays(const diy::Master& master, std::vector<vtkImageData*>& outputs,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays);
  static void FillGhostArrays(const diy::Master& master, std::vector<vtkRectilinearGrid*>& outputs,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays);
  //@}

private:
  vtkDIYGhostUtilities(const vtkDIYGhostUtilities&) = delete;
  void operator=(const vtkDIYGhostUtilities&) = delete;
};

#include "vtkDIYGhostUtilities.txx"

#endif
