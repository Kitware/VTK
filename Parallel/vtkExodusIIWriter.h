/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusIIWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkExodusIIWriter - Write Exodus II files
// .SECTION Description
//     This is a vtkWriter that writes it's vtkUnstructuredGrid 
//     input out to an Exodus II file.  Go to http://endo.sandia.gov/SEACAS/
//     for more information about the Exodus II format.
//
//     Exodus files contain much information that is not captured
//     in a vtkUnstructuredGrid, such as time steps, information
//     lines, node sets, and side sets.  This information can be
//     stored in a vtkModelMetadata object.
//
//     The vtkExodusReader and vtkPExodusReader can create
//     a vtkModelMetadata object and embed it in a vtkUnstructuredGrid
//     in a series of field arrays.  This writer searches for these
//     field arrays and will use the metadata contained in them
//     when creating the new Exodus II file. 
//
//     You can also explicitly give the vtkExodusIIWriter a
//     vtkModelMetadata object to use when writing the file.
//
//     In the absence of the information provided by vtkModelMetadata,
//     if this writer is not part of a parallel application, we will use
//     reasonable defaults for all the values in the output Exodus file.
//     If you don't provide a block ID element array, we'll create a
//     block for each cell type that appears in the unstructured grid.
//
//     However if this writer is part of a parallel application (hence 
//     writing out a distributed Exodus file), then we need at the very 
//     least a list of all the block IDs that appear in the file.  And 
//     we need the element array of block IDs for the input unstructured grid.
//
//     In the absense of a vtkModelMetadata object, you can also provide
//     time step information which we will include in the output Exodus
//     file.
//
//  .SECTION Caveats
//     If the input floating point field arrays and point locations are all
//     floats or all doubles, this class will operate more efficiently.
//     Mixing floats and doubles will slow you down, because Exodus II
//     requires that we write only floats or only doubles.
//
//     We use the terms "point" and "node" interchangeably.
//     Also, we use the terms "element" and "cell" interchangeably.

#ifndef __vtkExodusIIWriter_h
#define __vtkExodusIIWriter_h

#include "vtkWriter.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <vtkstd/vector> // STL Header
#include <vtkstd/map>    // STL Header
#include <vtkstd/string> // STL Header

class vtkModelMetadata;
class vtkDoubleArray;
class vtkIntArray;
class vtkUnstructuredGrid;

class VTK_PARALLEL_EXPORT vtkExodusIIWriter : public vtkWriter
{
public:
  static vtkExodusIIWriter *New ();
  vtkTypeMacro(vtkExodusIIWriter,vtkWriter);
  void PrintSelf (ostream& os, vtkIndent indent);

  // Description:
  // Specify the vtkModelMetadata object which contains the Exodus file
  // model information (metadata) absent in the vtkUnstructuredGrid.  If you 
  // have this object, you don't need to set any other values before writing.
  // (Just the FileName and the Input.)
  // Note that the vtkExodusReader can create and attach a vtkModelMetadata
  // object to it's output.  If this has happened, the ExodusIIWriter will
  // find it and use it.

  void SetModelMetadata (vtkModelMetadata*);
  vtkGetObjectMacro(ModelMetadata, vtkModelMetadata);

  // Description:
  //   Name for the output file.  If writing in parallel, the number
  //   of processes and the process rank will be appended to the name,
  //   so each process is writing out a separate file.
  //   If not set, this class will make up a file name.

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  //   If StoreDoubles is ON, the floating point fields in the Exodus file
  //   will be double precision fields.  The default is determined by the
  //   max precision of the input.  If the field data appears to be doubles,
  //   then StoreDoubles will be ON, otherwise StoreDoubles will be OFF.

  vtkSetMacro(StoreDoubles, int);
  vtkGetMacro(StoreDoubles, int);

  // Description:
  //   We never write out ghost cells.  This variable is here to satisfy
  //   the behavior of ParaView on invoking a parallel writer.

  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

   // Description:
  //   By default, the integer array containing the global Block Ids of the
  //   cells is not included when the new Exodus II file is written out.  If
  //   you do want to include this array, set WriteOutBlockIdArray to ON.

  vtkSetMacro(WriteOutBlockIdArray, int);
  vtkGetMacro(WriteOutBlockIdArray, int);
  vtkBooleanMacro(WriteOutBlockIdArray, int);

  // Description:
  //   By default, the integer array containing the global Node Ids 
  //   is not included when the new Exodus II file is written out.  If
  //   you do want to include this array, set WriteOutGlobalNodeIdArray to ON.

  vtkSetMacro(WriteOutGlobalNodeIdArray, int);
  vtkGetMacro(WriteOutGlobalNodeIdArray, int);
  vtkBooleanMacro(WriteOutGlobalNodeIdArray, int);

  // Description:
  //   By default, the integer array containing the global Element Ids 
  //   is not included when the new Exodus II file is written out.  If you
  //   do want to include this array, set WriteOutGlobalElementIdArray to ON.

  vtkSetMacro(WriteOutGlobalElementIdArray, int);
  vtkGetMacro(WriteOutGlobalElementIdArray, int);
  vtkBooleanMacro(WriteOutGlobalElementIdArray, int);

  // Description:
  //   When WriteAllTimeSteps is turned ON, the writer is executed once for
  //    each timestep available from the reader.

  vtkSetMacro(WriteAllTimeSteps, int);
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);

  vtkSetStringMacro(BlockIdArrayName);
  vtkGetStringMacro(BlockIdArrayName);

protected:
  vtkExodusIIWriter ();
  ~vtkExodusIIWriter ();

  vtkModelMetadata* ModelMetadata;

  char *BlockIdArrayName;

  char *FileName;
  int fid;

  int NumberOfProcesses;
  int MyRank;

  int PassDoubles;

  int StoreDoubles;
  int GhostLevel;
  int WriteOutBlockIdArray;
  int WriteOutGlobalNodeIdArray;
  int WriteOutGlobalElementIdArray;
  int WriteAllTimeSteps;
  int NumberOfTimeSteps;

  vtkDoubleArray* TimeValues;
  int CurrentTimeIndex;
  int FileTimeOffset;

//BTX 
  vtkDataObject *OriginalInput;
  vtkstd::vector< vtkSmartPointer<vtkUnstructuredGrid> > FlattenedInput;
  vtkstd::vector< vtkSmartPointer<vtkUnstructuredGrid> > NewFlattenedInput;

  vtkstd::vector< vtkIntArray* > BlockIdList;

  struct Block
  {
    Block () 
      {
      this->Type = 0;
      this->NumElements = 0;
      this->ElementStartIndex = -1;
      this->NodesPerElement = 0;
      this->EntityCounts = std::vector<int>();
      this->EntityNodeOffsets = std::vector<int>();
      this->GridIndex = 0;
      this->OutputIndex = -1;
      this->NumAttributes = 0;
      this->BlockAttributes = 0;
      };
    int Type;
    int NumElements;
    int ElementStartIndex;
    int NodesPerElement;
    std::vector<int> EntityCounts;
    std::vector<int> EntityNodeOffsets;
    size_t GridIndex;
    // vtkstd::vector<int> CellIndex;
    int OutputIndex;
    int NumAttributes;
    float *BlockAttributes; // Owned by metamodel or null.  Don't delete.
  };
  vtkstd::map<int, Block> BlockInfoMap;
  int NumCells, NumPoints, MaxId;

  vtkstd::vector<vtkIdType*> GlobalElementIdList;
  vtkstd::vector<vtkIdType*> GlobalNodeIdList;
//ETX
  int AtLeastOneGlobalElementIdList;
  int AtLeastOneGlobalNodeIdList;

//BTX
  struct VariableInfo
  {
    int NumComponents;
    int InIndex;
    int ScalarOutOffset;
    vtkstd::vector<vtkstd::string> OutNames;
  };
  vtkstd::map<vtkstd::string, VariableInfo> GlobalVariableMap;
  vtkstd::map<vtkstd::string, VariableInfo> BlockVariableMap;
  vtkstd::map<vtkstd::string, VariableInfo> NodeVariableMap;
  int NumberOfScalarGlobalArrays;
  int NumberOfScalarElementArrays;
  int NumberOfScalarNodeArrays;
//ETX
  
//BTX
  vtkstd::vector< vtkstd::vector<int> > CellToElementOffset;
//ETX
  // By BlockId, and within block ID by element variable, with variables
  // appearing in the same order in which they appear in OutputElementArrayNames
  
  int *BlockElementVariableTruthTable;
  int AllVariablesDefinedInAllBlocks;

  int BlockVariableTruthValue(int blockIdx, int varIdx);
  
//BTX
  char *StrDupWithNew (const char *s);
  void StringUppercase (vtkstd::string& str);
//ETX

  int ProcessRequest (vtkInformation* request,
                      vtkInformationVector** inputVector,
                      vtkInformationVector* outputVector);

  int RequestInformation (vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  int FillInputPortInformation (int port, vtkInformation* info);

  int RequestData (vtkInformation* request,
                   vtkInformationVector** inputVector,
                   vtkInformationVector* outputVector);

  void WriteData ();
  
  int FlattenHierarchy (vtkDataObject* input, bool& changed);

  int CreateNewExodusFile ();
  void CloseExodusFile ();

  int IsDouble ();
  void RemoveGhostCells ();
  int CheckParameters ();
  int CheckInputArrays ();
  int ConstructBlockInfoMap ();
  int ConstructVariableInfoMaps ();
  int ParseMetadata ();
  int CreateDefaultMetadata ();
  char *GetCellTypeName (int t);
  
  int CreateBlockIdMetadata(vtkModelMetadata *em);
  int CreateBlockVariableMetadata (vtkModelMetadata* em);
  
//BTX
  void ConvertVariableNames (vtkstd::map<vtkstd::string, VariableInfo>& variableMap);
  char **FlattenOutVariableNames (
            int nScalarArrays, 
            const vtkstd::map<vtkstd::string, VariableInfo>& variableMap);
  vtkstd::string CreateNameForScalarArray (const char *root,
                                           int component,
                                           int numComponents);

  vtkstd::map<vtkIdType, vtkIdType> *LocalNodeIdMap;
  vtkstd::map<vtkIdType, vtkIdType> *LocalElementIdMap;
//ETX
  vtkIdType GetNodeLocalId(vtkIdType id);
  vtkIdType GetElementLocalId(vtkIdType id);

  int WriteInitializationParameters ();
  int WriteQARecords ();
  int WriteInformationRecords ();
  int WritePoints ();
  int WriteCoordinateNames ();
  int WriteGlobalPointIds ();
  int WriteBlockInformation ();
  int WriteGlobalElementIds ();
  int WriteVariableArrayNames ();
  int WriteNodeSetInformation ();
  int WriteSideSetInformation ();
  int WriteProperties ();
  int WriteNextTimeStep ();

//BTX
  double ExtractGlobalData (const char *name, int comp, int ts);
  int WriteGlobalData (int timestep, vtkDataArray *buffer);
  void ExtractCellData (const char *name, int comp, vtkDataArray *buffer);
  int WriteCellData (int timestep, vtkDataArray *buffer);
  void ExtractPointData (const char *name, int comp, vtkDataArray *buffer);
  int WritePointData (int timestep, vtkDataArray *buffer);
//ETX


private:
  vtkExodusIIWriter (const vtkExodusIIWriter&); // Not Implemented
  void operator= (const vtkExodusIIWriter&); // Not Implemented
};

#endif
