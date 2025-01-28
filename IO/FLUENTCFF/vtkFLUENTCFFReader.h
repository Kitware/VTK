// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFLUENTCFFReader
 * @brief   reads a dataset in Fluent CFF file format
 *
 * vtkFLUENTCFFReader creates an unstructured grid dataset. It reads .cas.h5 and
 * .dat.h5 files stored in FLUENT CFF format (hdf5).
 *
 * @par Thanks:
 * Original author : Arthur Piquet
 *
 * This class is based on the vtkFLUENTReader class from Brian W. Dotson &
 * Terry E. Jordan (Department of Energy, National Energy Technology
 * Laboratory) & Douglas McCorkle (Iowa State University)
 *
 * This class reads the HDF5 data in Fluent Format (face type structure)
 * and converts it to VTK Format (cell type structure).
 * This class could be improved for memory performance but the developer
 * will need to rewrite entirely the structure of the class.
 * Some piece of functionality lack in the HDF reading (overset, AMR tree,
 * interface), no file available in order to code/test the structure.
 *
 *
 * @sa
 * vtkFLUENTReader
 */

#ifndef vtkFLUENTCFFReader_h
#define vtkFLUENTCFFReader_h

#include <memory> // std::unique_ptr

#include "vtkIOFLUENTCFFModule.h" // For export macro

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h" // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkPoints;
class vtkTriangle;
class vtkTetra;
class vtkQuad;
class vtkHexahedron;
class vtkPyramid;
class vtkWedge;

class VTKIOFLUENTCFF_EXPORT vtkFLUENTCFFReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFLUENTCFFReader* New();
  vtkTypeMacro(vtkFLUENTCFFReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the file name of the Fluent case file to read.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  //@}

  //@{
  /**
   * Get the total number of cells. The number of cells is only valid after a
   * successful read of the data file is performed. Initial value is 0.
   */
  vtkGetMacro(NumberOfCells, vtkIdType);
  //@}

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfCellArrays();

  /**
   * Get the name of the cell array with the given index in
   * the input.
   */
  const char* GetCellArrayName(int index);

  //@{
  /**
   * Get/Set whether the cell array with the given name is to
   * be read.
   */
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  //@}

  //@{
  /**
   * Turn on/off all cell arrays.
   */
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  //@}

  //@{
  //
  //  Structures
  //
  struct Cell
  {
    int type;
    int zone;
    std::vector<int> faces;
    int parent;
    int child;
    std::vector<int> nodes;
    std::vector<int> nodesOffset;
    std::vector<int> childId;
  };
  struct Face
  {
    int type;
    unsigned int zone;
    std::vector<int> nodes;
    int c0;
    int c1;
    int periodicShadow;
    int parent;
    int child;
    int interfaceFaceParent;
    int interfaceFaceChild;
    int ncgParent;
    int ncgChild;
  };
  struct ScalarDataChunk
  {
    std::string variableName;
    vtkIdType zoneId;
    std::vector<double> scalarData;
  };
  struct VectorDataChunk
  {
    std::string variableName;
    vtkIdType zoneId;
    size_t dim;
    std::vector<double> vectorData;
  };
  //@}

protected:
  vtkFLUENTCFFReader();
  ~vtkFLUENTCFFReader() override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Enumerate
   */
  enum DataState
  {
    NOT_LOADED = 0,
    AVAILABLE = 1,
    LOADED = 2,
    ERROR = 3
  };

  //@{
  /**
   * Open the HDF5 file structure
   */
  virtual bool OpenCaseFile(const std::string& filename);
  virtual DataState OpenDataFile(const std::string& filename);
  //@}

  /**
   * Retrieve the number of cell zones
   */
  virtual void GetNumberOfCellZones();

  /**
   * Reads necessary information from the .cas file
   */
  virtual int ParseCaseFile();

  /**
   * Get the dimension of the file (2D/3D)
   */
  virtual int GetDimension();

  //@{
  /**
   * Get the total number of nodes/cells/faces
   */
  virtual void GetNodesGlobal();
  virtual void GetCellsGlobal();
  virtual void GetFacesGlobal();
  //@}

  /**
   * Get the size and index of node per zone
   */
  virtual void GetNodes();

  /**
   * Get the topology of cell per zone
   */
  virtual void GetCells();

  /**
   * Get the topology of face per zone
   */
  virtual void GetFaces();

  /**
   * Get the periodic shadown faces information
   * !!! NOT IMPLEMENTED YET !!!
   */
  virtual void GetPeriodicShadowFaces();

  /**
   * Get the overset cells information
   * !!! NOT IMPLEMENTED YET !!!
   */
  virtual void GetCellOverset();

  /**
   * Get the tree (AMR) cell topology
   */
  virtual void GetCellTree();

  /**
   * Get the tree (AMR) face topology
   */
  virtual void GetFaceTree();

  /**
   * Get the interface id of parent faces
   */
  virtual void GetInterfaceFaceParents();

  /**
   * Get the non conformal grid interface information
   * !!! NOT IMPLEMENTED YET !!!
   */
  virtual void GetNonconformalGridInterfaceFaceInformation();

  /**
   * Removes unnecessary faces from the cells
   */
  virtual void CleanCells();

  //@{
  /**
   * Reconstruct and convert the Fluent data format
   * to the VTK format
   */
  virtual void PopulateCellNodes();
  virtual void PopulateCellTree();
  //@}

  //@{
  /**
   * Reconstruct VTK cell topology from Fluent format
   */
  virtual void PopulateTriangleCell(int i);
  virtual void PopulateTetraCell(int i);
  virtual void PopulateQuadCell(int i);
  virtual void PopulateHexahedronCell(int i);
  virtual void PopulatePyramidCell(int i);
  virtual void PopulateWedgeCell(int i);
  virtual void PopulatePolyhedronCell(int i);
  //@}

  /**
   * Read and reconstruct data from .dat.h5 file
   */
  virtual int GetData();

  /**
   * Pre-read variable name data available for selection
   */
  virtual int GetMetaData();

  //
  //  Variables
  //
  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  std::string FileName;
  vtkIdType NumberOfCells = 0;
  int NumberOfCellArrays = 0;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> HDFImpl;

  vtkNew<vtkPoints> Points;
  vtkNew<vtkTriangle> Triangle;
  vtkNew<vtkTetra> Tetra;
  vtkNew<vtkQuad> Quad;
  vtkNew<vtkHexahedron> Hexahedron;
  vtkNew<vtkPyramid> Pyramid;
  vtkNew<vtkWedge> Wedge;

  std::vector<Cell> Cells;
  std::vector<Face> Faces;
  std::vector<int> CellZones;
  std::vector<ScalarDataChunk> ScalarDataChunks;
  std::vector<VectorDataChunk> VectorDataChunks;
  std::vector<std::string> PreReadScalarData;
  std::vector<std::string> PreReadVectorData;

  vtkTypeBool SwapBytes = 0;
  int GridDimension = 0;
  DataState FileState = DataState::NOT_LOADED;
  int NumberOfScalars = 0;
  int NumberOfVectors = 0;

private:
  vtkFLUENTCFFReader(const vtkFLUENTCFFReader&) = delete;
  void operator=(const vtkFLUENTCFFReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
