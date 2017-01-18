/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFLUENTReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFLUENTReader
 * @brief   reads a dataset in Fluent file format
 *
 * vtkFLUENTReader creates an unstructured grid dataset. It reads .cas and
 * .dat files stored in FLUENT native format.
 *
 * @par Thanks:
 * Thanks to Brian W. Dotson & Terry E. Jordan (Department of Energy, National
 * Energy Technology Laboratory) & Douglas McCorkle (Iowa State University)
 * who developed this class.
 * Please address all comments to Brian Dotson (brian.dotson@netl.doe.gov) &
 * Terry Jordan (terry.jordan@sa.netl.doe.gov)
 * & Doug McCorkle (mccdo@iastate.edu)
 *
 *
 * @sa
 * vtkGAMBITReader
*/

#ifndef vtkFLUENTReader_h
#define vtkFLUENTReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkDataArraySelection;
class vtkPoints;
class vtkTriangle;
class vtkTetra;
class vtkQuad;
class vtkHexahedron;
class vtkPyramid;
class vtkWedge;
class vtkConvexPointSet;

class VTKIOGEOMETRY_EXPORT vtkFLUENTReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFLUENTReader *New();
  vtkTypeMacro(vtkFLUENTReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
  vtkGetMacro(NumberOfCells,int);
  //@}

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfCellArrays(void);

  /**
   * Get the name of the  cell array with the given index in
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
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian
   * otherwise SetDataByteOrderToBigEndian. Not used when reading
   * text files.
   */
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int  GetDataByteOrder();
  void SetDataByteOrder(int);
  const char *GetDataByteOrderAsString();
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
  vtkFLUENTReader();
  ~vtkFLUENTReader() VTK_OVERRIDE;
  int RequestInformation(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the byte swapping to explicitly swap the bytes of a file.
   * Not used when reading text files.
   */
  vtkSetMacro(SwapBytes,int);
  int GetSwapBytes() {return this->SwapBytes;}
  vtkBooleanMacro(SwapBytes,int);
  //@}

  vtkDataArraySelection* CellDataArraySelection;
  char * FileName;
  int NumberOfCells;
  int NumberOfCellArrays;
  virtual bool                   OpenCaseFile(const char *filename);
  virtual bool                   OpenDataFile(const char *filename);
  virtual int                    GetCaseChunk ();
  virtual void                   GetNumberOfCellZones();
  virtual int                    GetCaseIndex();
  virtual void                   LoadVariableNames();
  virtual int                    GetDataIndex();
  virtual int                    GetDataChunk();
  virtual void                   GetSpeciesVariableNames();

  virtual void                   ParseCaseFile();
  virtual int                    GetDimension();
  virtual void                   GetLittleEndianFlag();
  virtual void                   GetNodesAscii();
  virtual void                   GetNodesSinglePrecision();
  virtual void                   GetNodesDoublePrecision();
  virtual void                   GetCellsAscii();
  virtual void                   GetCellsBinary();
  virtual void                   GetFacesAscii();
  virtual void                   GetFacesBinary();
  virtual void                   GetPeriodicShadowFacesAscii();
  virtual void                   GetPeriodicShadowFacesBinary();
  virtual void                   GetCellTreeAscii();
  virtual void                   GetCellTreeBinary();
  virtual void                   GetFaceTreeAscii();
  virtual void                   GetFaceTreeBinary();
  virtual void                   GetInterfaceFaceParentsAscii();
  virtual void                   GetInterfaceFaceParentsBinary();
  virtual void                   GetNonconformalGridInterfaceFaceInformationAscii();
  virtual void                   GetNonconformalGridInterfaceFaceInformationBinary();
  virtual void                   GetPartitionInfo() {}
  virtual void                   CleanCells();
  virtual void                   PopulateCellNodes();
  virtual int                    GetCaseBufferInt(int ptr);
  virtual float                  GetCaseBufferFloat(int ptr);
  virtual double                 GetCaseBufferDouble(int ptr);
  virtual void                   PopulateTriangleCell(int i);
  virtual void                   PopulateTetraCell(int i);
  virtual void                   PopulateQuadCell(int i);
  virtual void                   PopulateHexahedronCell(int i);
  virtual void                   PopulatePyramidCell(int i);
  virtual void                   PopulateWedgeCell(int i);
  virtual void                   PopulatePolyhedronCell(int i);
  virtual void                   ParseDataFile();
  virtual int                    GetDataBufferInt(int ptr);
  virtual float                  GetDataBufferFloat(int ptr);
  virtual double                 GetDataBufferDouble(int ptr);
  virtual void                   GetData(int dataType);
  virtual bool                   ParallelCheckCell(int vtkNotUsed(i)) { return true; }

  //
  //  Variables
  //
  ifstream *FluentCaseFile;
  ifstream *FluentDataFile;
  stdString *CaseBuffer;
  stdString *DataBuffer;

  vtkPoints           *Points;
  vtkTriangle         *Triangle;
  vtkTetra            *Tetra;
  vtkQuad             *Quad;
  vtkHexahedron       *Hexahedron;
  vtkPyramid          *Pyramid;
  vtkWedge            *Wedge;
  vtkConvexPointSet   *ConvexPointSet;

  cellVector *Cells;
  faceVector *Faces;
  stdMap *VariableNames;
  intVector  *CellZones;
  scalarDataVector *ScalarDataChunks;
  vectorDataVector *VectorDataChunks;

  intVectorVector *SubSectionZones;
  intVector *SubSectionIds;
  intVector *SubSectionSize;

  stringVector *ScalarVariableNames;
  intVector *ScalarSubSectionIds;
  stringVector *VectorVariableNames;
  intVector *VectorSubSectionIds;

  int SwapBytes;
  int GridDimension;
  int DataPass;
  int NumberOfScalars;
  int NumberOfVectors;

private:
  vtkFLUENTReader(const vtkFLUENTReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFLUENTReader&) VTK_DELETE_FUNCTION;
};
#endif
