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
// .NAME vtkFLUENTReader - reads a dataset in Fluent file format
// .SECTION Description
// vtkFLUENTReader creates an unstructured grid dataset. It reads .cas and
// .dat files stored in FLUENT native format.
//
// .SECTION Thanks
// Thanks to Brian W. Dotson & Terry E. Jordan (Department of Energy, National
// Energy Technology Laboratory) & Douglas McCorkle (Iowa State University)
// who developed this class.
// Please address all comments to Brian Dotson (brian.dotson@netl.doe.gov) &
// Terry Jordan (terry.jordan@sa.netl.doe.gov)
// & Doug McCorkle (mccdo@iastate.edu)
//

// .SECTION See Also
// vtkGAMBITReader

#ifndef __vtkFLUENTReader_h
#define __vtkFLUENTReader_h

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

class VTK_IO_EXPORT vtkFLUENTReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFLUENTReader *New();
  vtkTypeRevisionMacro(vtkFLUENTReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the file name of the Fluent case file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the total number of cells. The number of cells is only valid after a
  // successful read of the data file is performed.
  vtkGetMacro(NumberOfCells,int);

  // Description:
  // Get the number of cell arrays available in the input.
  int GetNumberOfCellArrays(void);

  // Description:
  // Get the name of the  cell array with the given index in
  // the input.
  const char* GetCellArrayName(int index);

  // Description:
  // Get/Set whether the cell array with the given name is to
  // be read.
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);

  // Description:
  // Turn on/off all cell arrays.
  void DisableAllCellArrays();
  void EnableAllCellArrays();

protected:
  vtkFLUENTReader();
  ~vtkFLUENTReader();
  int RequestInformation(vtkInformation *, 
    vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, 
    vtkInformationVector *);
  vtkDataArraySelection* CellDataArraySelection;
  char * FileName;
  int NumberOfCells;
  int NumberOfCellArrays;
  int                    OpenCaseFile(const char *filename);
  int                    OpenDataFile(const char *filename);
  int                    GetCaseChunk ();
  void                   GetNumberOfCellZones();
  int                    GetCaseIndex();
  void                   LoadVariableNames();
  int                    GetDataIndex();
  int                    GetDataChunk();

  void                   ParseCaseFile();
  int                    GetDimension();
  void                   GetLittleEndianFlag();
  void                   GetNodesAscii();
  void                   GetNodesSinglePrecision();
  void                   GetNodesDoublePrecision();
  void                   GetCellsAscii();
  void                   GetCellsBinary();
  void                   GetFacesAscii();
  void                   GetFacesBinary();
  void                   GetPeriodicShadowFacesAscii();
  void                   GetPeriodicShadowFacesBinary();
  void                   GetCellTreeAscii();
  void                   GetCellTreeBinary();
  void                   GetFaceTreeAscii();
  void                   GetFaceTreeBinary();
  void                   GetInterfaceFaceParentsAscii();
  void                   GetInterfaceFaceParentsBinary();
  void                   GetNonconformalGridInterfaceFaceInformationAscii();
  void                   GetNonconformalGridInterfaceFaceInformationBinary();
  void                   CleanCells();
  void                   PopulateCellNodes();
  int                    GetCaseBufferInt(int ptr);
  float                  GetCaseBufferFloat(int ptr);
  double                 GetCaseBufferDouble(int ptr);
  void                   PopulateTriangleCell(int i);
  void                   PopulateTetraCell(int i);
  void                   PopulateQuadCell(int i);
  void                   PopulateHexahedronCell(int i);
  void                   PopulatePyramidCell(int i);
  void                   PopulateWedgeCell(int i);
  void                   PopulatePolyhedronCell(int i);
  void                   ParseDataFile();
  int                    GetDataBufferInt(int ptr);
  float                  GetDataBufferFloat(int ptr);
  double                 GetDataBufferDouble(int ptr);
  void                   GetData(int dataType);

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

  int LittleEndianFlag;
  int GridDimension;
  int DataPass;
  int NumberOfScalars;
  int NumberOfVectors;

private:
  vtkFLUENTReader(const vtkFLUENTReader&);  // Not implemented.
  void operator=(const vtkFLUENTReader&);  // Not implemented.
};
#endif
