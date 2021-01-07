/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFLUENTCFFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * This class could be improved for memory performance but the developper
 * will need to rewrite entirely the structure of the class.
 *
 *
 * @sa
 * vtkFLUENTReader
 */

#ifndef vtkFLUENTCFFReader_h
#define vtkFLUENTCFFReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtk_hdf5.h" // For hdf5 library (hid_t type)

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkPoints;
class vtkTriangle;
class vtkTetra;
class vtkQuad;
class vtkHexahedron;
class vtkPyramid;
class vtkWedge;
class vtkConvexPointSet;

class VTKIOGEOMETRY_EXPORT vtkFLUENTCFFReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFLUENTCFFReader* New();
  vtkTypeMacro(vtkFLUENTCFFReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the file name of the Fluent case file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
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
  int GetNumberOfCellArrays(void);

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
  struct Cell;
  struct Face;
  struct ScalarDataChunk;
  struct VectorDataChunk;
  struct stdString;
  struct intVector;
  struct doubleVector;
  struct stringVector;
  struct cellVector;
  struct faceVector;
  struct stdMap;
  struct scalarDataVector;
  struct vectorDataVector;
  struct intVectorVector;
  //@}

protected:
  vtkFLUENTCFFReader();
  ~vtkFLUENTCFFReader() override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual bool OpenCaseFile(const char* filename);
  virtual bool OpenDataFile(const char* filename);
  virtual void GetNumberOfCellZones();

  virtual void ParseCaseFile();
  virtual int GetDimension();
  virtual void GetNodesGlobal();
  virtual void GetCellsGlobal();
  virtual void GetFacesGlobal();
  virtual void GetNodes();
  virtual void GetCells();
  virtual void GetFaces();
  virtual void GetPeriodicShadowFaces();
  virtual void GetCellOverset();
  virtual void GetCellTree();
  virtual void GetFaceTree();
  virtual void GetInterfaceFaceParents();
  virtual void GetNonconformalGridInterfaceFaceInformation();
  virtual void GetPartitionInfo() {}
  virtual void CleanCells();
  virtual void PopulateCellNodes();
  virtual void PopulateCellTree();
  virtual void PopulateTriangleCell(int i);
  virtual void PopulateTetraCell(int i);
  virtual void PopulateQuadCell(int i);
  virtual void PopulateHexahedronCell(int i);
  virtual void PopulatePyramidCell(int i);
  virtual void PopulateWedgeCell(int i);
  virtual void PopulatePolyhedronCell(int i);

  virtual void GetData();
  virtual bool ParallelCheckCell(int vtkNotUsed(i)) { return true; }

  //
  //  Variables
  //
  vtkDataArraySelection* CellDataArraySelection;
  char* FileName;
  vtkIdType NumberOfCells;
  int NumberOfCellArrays;

  hid_t FluentCaseFile;
  hid_t FluentDataFile;
  herr_t status;

  vtkPoints* Points;
  vtkTriangle* Triangle;
  vtkTetra* Tetra;
  vtkQuad* Quad;
  vtkHexahedron* Hexahedron;
  vtkPyramid* Pyramid;
  vtkWedge* Wedge;
  vtkConvexPointSet* ConvexPointSet;

  cellVector* Cells;
  faceVector* Faces;
  intVector* CellZones;
  scalarDataVector* ScalarDataChunks;
  vectorDataVector* VectorDataChunks;

  vtkTypeBool SwapBytes;
  int GridDimension;
  int DataPass;
  int NumberOfScalars;
  int NumberOfVectors;

private:
  vtkFLUENTCFFReader(const vtkFLUENTCFFReader&) = delete;
  void operator=(const vtkFLUENTCFFReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
