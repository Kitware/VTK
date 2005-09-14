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
//

#ifndef __vtkExodusIIWriter_h
#define __vtkExodusIIWriter_h


#include "vtkWriter.h"

#include <vtkstd/map> // For the map

class vtkUnstructuredGrid;
class vtkFloatArray;
class vtkDoubleArray;
class vtkDataArray;
class vtkUnsignedCharArray;
class vtkIntArray;
class vtkModelMetadata;

class VTK_PARALLEL_EXPORT vtkExodusIIWriter : public vtkWriter
{
public:
  static vtkExodusIIWriter *New();
  vtkTypeRevisionMacro(vtkExodusIIWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify input which will be written out to the Exodus II file.

  void SetInput(vtkUnstructuredGrid *ug);
  vtkUnstructuredGrid *GetInput();

  // Description:
  // Specify the vtkModelMetadata object which contains the Exodus file
  // model information (metadata) absent in the vtkUnstructuredGrid.  If you 
  // have this object, you don't need to set any other values before writing.
  // (Just the FileName and the Input.)
  // Note that the vtkExodusReader can create and attach a vtkModelMetadata
  // object to it's output.  If this has happened, the ExodusIIWriter will
  // find it and use it.

  virtual void SetModelMetadata(vtkModelMetadata*);
  vtkGetObjectMacro(ModelMetadata, vtkModelMetadata);

  // Description:
  // By default, ModelMetadata is NULL until the Write() method is called,
  // at which point the vtkExodusIIWriter will create a default metadata
  // object. If you would like to obtain the metadata and modify it, rather
  // than creating it yourself, you may call this function. If the
  // metadata already exists (because it has been set by a call to SetModelMetadata
  // or because it has been packed into the FieldData of the input mesh),
  // that metadata will be returned. Otherwise, the vtkExodusIIWriter will
  // create metadata using the input mesh as needed, set the metadata to
  // that object, and return it.
  vtkModelMetadata* GetOrCreateModelMetadata();

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
  //   input vtkUnstructuredGrid.  If the field data appears to be doubles,
  //   then StoreDoubles will be ON, otherwise StoreDoubles will be OFF.

  vtkSetMacro(StoreDoubles, int);
  vtkGetMacro(StoreDoubles, int);

  // Description:
  //   We never write out ghost cells.  This variable is here to satisfy
  //   the behavior of ParaView on invoking a parallel writer.

  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

  // Description:
  //   Exodus files group cells into blocks.  There are no blocks in a
  //   vtkUnstructuredGrid, but you may have stored block IDs in a
  //   cell array.  If so provide that name here.  If you don't provide
  //   it, we'll look for an array named "BlockId", and we'll assume
  //   it's a vtkIntArray.  If we find no block ID array, and this is not
  //   a multi-process application, we will create
  //   one Exodus block for every cell type found in the vtkUnstructuredGrid.

  vtkSetStringMacro(BlockIdArrayName);
  vtkGetStringMacro(BlockIdArrayName);

  // Description:
  //   By default, the integer array containing the global Block Ids of the
  //   cells is not included when the new Exodus II file is written out.  If
  //   you do want to include this array, set WriteOutBlockIdArray to ON.

  vtkSetMacro(WriteOutBlockIdArray, int);
  vtkGetMacro(WriteOutBlockIdArray, int);
  vtkBooleanMacro(WriteOutBlockIdArray, int);

  // Description:
  //   The name of a point array that gives the global node IDs.
  //   We will look for an array called "GlobalNodeId" if you
  //   don't provide a different name here.  It must be an integer
  //   array.  This array is optional.

  vtkSetStringMacro(GlobalNodeIdArrayName);
  vtkGetStringMacro(GlobalNodeIdArrayName);

  // Description:
  //   By default, the integer array containing the global Node Ids 
  //   is not included when the new Exodus II file is written out.  If
  //   you do want to include this array, set WriteOutGlobalNodeIdArray to ON.

  vtkSetMacro(WriteOutGlobalNodeIdArray, int);
  vtkGetMacro(WriteOutGlobalNodeIdArray, int);
  vtkBooleanMacro(WriteOutGlobalNodeIdArray, int);

  // Description:
  //   The name of a cell array that gives the global cell IDs.
  //   We will look for an array called "GlobalElementId" if you
  //   don't provide a different name here.  It must be an integer array.
  //   This array is optional.

  vtkSetStringMacro(GlobalElementIdArrayName);
  vtkGetStringMacro(GlobalElementIdArrayName);

  // Description:
  //   By default, the integer array containing the global Element Ids 
  //   is not included when the new Exodus II file is written out.  If you
  //   do want to include this array, set WriteOutGlobalElementIdArray to ON.

  vtkSetMacro(WriteOutGlobalElementIdArray, int);
  vtkGetMacro(WriteOutGlobalElementIdArray, int);
  vtkBooleanMacro(WriteOutGlobalElementIdArray, int);

  // Description:
  //   If there is no vtkModelMetadata object, then you can
  //   input time step values here.  We copy your array.  This is
  //   not required, the writer can use sensible defaults.  If you
  //   only give one time step value (say 1.0), we'll increment
  //   each successive time step by that amount (2.0, 3.0, ...).

  void SetTimeStepValues(int NumberOfTimeSteps, float *v);
  float *GetTimeStepValues(){return this->InputTimeStepValues;}
  int GetNumberOfTimeSteps(){return this->InputNumberOfTimeSteps;}

  // Description:
  //   You can set the time step index for the next write with
  //   SetCurrentTimeStep.  If this is not set, the writer will
  //   use the time step index found in the vtkModelMetadata object,
  //   or else a sensible default (one more than the last time step
  //   written).  (You may want to set a different
  //   time step index when you have a vtkModelMetadata object if,
  //   for example, you are writing out only every tenth time
  //   step.  The input to the writer may be time step 10, but you
  //   want it written out as time step 1.)
  //   The first index is 0.

  void SetCurrentTimeStep(int ts);
  int GetCurrentTimeStep(){return this->InputCurrentTimeStep;}

  // Description:
  //   Provide a list of all blockIds that appear in the file.  If
  //   this is a distributed file, and there is no vtkModelMetadata,
  //   we need all block Ids that appear in any of the files.
  //
  //   We make a copy of your array of IDs.

  void SetAllBlockIds(int numEntries, int *blockIds);

  // Description:
  //   The writer will set the ErrorStatus to a non-zero value
  //   each time a serious error occurs.  Usually this would be
  //   a problem with memory allocation, invalid values in the
  //   input file's metadata, or an inability to write the output
  //   file.

  vtkSetMacro(ErrorStatus, int);
  vtkGetMacro(ErrorStatus, int);

  // ATTRIBUTE EDITOR
  // Description:
  //    If this writer is writing to the original data file, set 
  //    this flag so that it will only write out the variable array. 
  //    If a vtkAttributeEditor filter is the input to this writer, set the attribute name here
  vtkSetMacro(EditorFlag,int);
  vtkGetMacro(EditorFlag,int);
  vtkSetStringMacro(EditedVariableName);
  vtkGetStringMacro(EditedVariableName);


protected:

  vtkExodusIIWriter();
  ~vtkExodusIIWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  void WriteData();     

  vtkExodusIIWriter(const vtkExodusIIWriter&); // Not implemented
  void operator=(const vtkExodusIIWriter&); // Not implemented

  vtkSetStringMacro(MyFileName);
  vtkGetStringMacro(MyFileName);

  // Description:
  //   Get/Set the rank of the writer in a set of parallel processes
  //   so that it may determine which piece of the dataset it is
  //   responsible for writing.
  vtkSetMacro(MyRank, int);
  vtkGetMacro(MyRank, int);

private:
  static char *StrDupWithNew(const char *s);

  void RemoveGhostCells();

  void SetPassDoubles();

  int CheckParameters();
  int CreateExodusModel();
  int CreateBlockIdInformationFromCellTypes(vtkModelMetadata *em);
  int CreateBlockIdInformation(vtkModelMetadata *em);

  static char **FlattenOutVariableNames(int narrays, int nScalarArrays, 
                                  char **nms, int *numComponents);
  static void CreateNamesForScalarArrays(const char *root, char **nms, 
                                         int numComponents);
  static char *GetCellTypeName(int t);
  static int FindCellType(int blockId, int *blockIdList, unsigned char *cellTypeList, 
                 int nCells);

  int CreateNewExodusFile();
  int OpenExodusFile();
  void CloseExodusFile();

  void InitializeVariableArrayNames();
  void ClearVariableArrayNames();
  void SetNewNodeVariableNames(vtkDataArray *da, char **nm);
  void SetNewElementVariableNames(vtkDataArray *da, char **nm);

  void InitializeBlockLists();
  void ClearBlockLists();
  int WriteBlockVariables();
//BTX
  vtkstd::map<int, int> *BuildBlockElementSearchStructure(int block);
//ETX

  int WriteInitializationParameters();
  int WriteQARecords();
  int WriteInformationRecords();
  int WritePoints();
  int WriteCoordinateNames();
  int WriteGlobalPointIds();
  int WriteGlobalElementIds();
  int WriteBlockInformation();
  int WriteVariableArrayNames();
  int WriteNodeSetInformation();
  int WriteSideSetInformation();
  int WriteProperties();

  int GetTimeStepIndex();
  float GetTimeStepValue(int timeStepIndex);
  int WriteNextTimeStep();
  float *ExtractComponentF(vtkDataArray *da, int comp, int *idx);
  double *ExtractComponentD(vtkDataArray *da, int comp, int *idx);

  vtkModelMetadata *ModelMetadata;

  int PassDoubles; // If set, we have to pass doubles to exodus library.
                   // If not set, we have to pass floats.
  int StoreDoubles;// If set, Exodus library will store doubles.
                   // If not set, Exodus library will store floats.
  int fid;

  char *FileName;    // base file name
  char *MyFileName;  // base plus number of processes and my rank  

  // The block IDs, the time step values, and the current time step index
  // may be provided if there is not vtkModelMetadata object.  The time
  // step may also be provided if only a subset (like every other one)
  // of the time steps are being written out.

  int *InputBlockIds;
  int InputBlockIdsLength;

  int InputNumberOfTimeSteps;
  float *InputTimeStepValues;
  int InputCurrentTimeStep;   
  int LastTimeStepWritten;

  // List of the global element ID of each cell in input

  char *GlobalElementIdArrayName;
  int *GlobalElementIdList;
//BTX
  vtkstd::map<int, int> *LocalElementIdMap;
//ETX
  int GetElementLocalId(int i);
  int WriteOutGlobalElementIdArray;

  // List of the global node ID of each cell in input

  char *GlobalNodeIdArrayName;
  int *GlobalNodeIdList;
//BTX
  vtkstd::map<int, int> *LocalNodeIdMap;
//ETX
  int GetNodeLocalId(int i);
  int WriteOutGlobalNodeIdArray;

  // Exodus II element blocks

  char *BlockIdArrayName;    // List of block ID of each cell in input
  int *BlockIdList;
  int WriteOutBlockIdArray;

  int NumberOfElementBlocks;
  int *BlockIds;             // list of every block ID in dataset
//BTX
  vtkstd::map<int, int> *LocalBlockIndexMap; // block ID -> block index
//ETX
  int GetBlockLocalIndex(int i);
  int *BlockElementStart;
  int *ElementIndex;

  char **BlockElementType;
  int *NumberOfElementsPerBlock;
  int *NumberOfNodesPerElementInBlock;
  int *NumberOfAttributesPerElementInBlock;
  float **BlockElementAttributesF;
  double **BlockElementAttributesD;
  int **BlockElementConnectivity;

  // By BlockId, and within block ID by element variable, with variables
  // appearing in the same order in which they appear in OutputElementArrayNames

  int *BlockElementVariableTruthTable;
  int AllVariablesDefinedInAllBlocks;

  int BlockVariableTruthValue(int blockIdx, int varIdx);

  // Element variable arrays

  int NumberOfScalarElementArrays;
  char **InputElementArrayNames;    // input names (including vectors)
  char **OutputElementArrayNames;         // output names (all scalars)
  int *InputElementArrayComponent;

  // Point variable arrays

  int NumberOfScalarNodeArrays;
  char **InputNodeArrayNames;
  char **OutputNodeArrayNames;
  int *InputNodeArrayComponent;

  // Global application information

  int NumberOfProcesses;
  int MyRank;

  // The input less ghost cells

  vtkUnstructuredGrid *MyInput;

  // we don't use this variable - it's for ParaView

  int GhostLevel;

  int ErrorStatus;

  // ATTRIBUTE EDITOR
  int ExtractComponentForEditorF(vtkDataArray *da, vtkFloatArray *fa, vtkIntArray *ids, int comp, int *idx);
  int ExtractComponentForEditorD(vtkDataArray *da, vtkDoubleArray *dba, vtkIntArray *ids, int comp, int *idx);
  char *EditedVariableName;
  int EditorFlag;
};

#endif
