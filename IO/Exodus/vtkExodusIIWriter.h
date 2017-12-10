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

/**
 * @class   vtkExodusIIWriter
 * @brief   Write Exodus II files
 *
 *     This is a vtkWriter that writes it's vtkUnstructuredGrid
 *     input out to an Exodus II file.  Go to http://endo.sandia.gov/SEACAS/
 *     for more information about the Exodus II format.
 *
 *     Exodus files contain much information that is not captured
 *     in a vtkUnstructuredGrid, such as time steps, information
 *     lines, node sets, and side sets.  This information can be
 *     stored in a vtkModelMetadata object.
 *
 *     The vtkExodusReader and vtkPExodusReader can create
 *     a vtkModelMetadata object and embed it in a vtkUnstructuredGrid
 *     in a series of field arrays.  This writer searches for these
 *     field arrays and will use the metadata contained in them
 *     when creating the new Exodus II file.
 *
 *     You can also explicitly give the vtkExodusIIWriter a
 *     vtkModelMetadata object to use when writing the file.
 *
 *     In the absence of the information provided by vtkModelMetadata,
 *     if this writer is not part of a parallel application, we will use
 *     reasonable defaults for all the values in the output Exodus file.
 *     If you don't provide a block ID element array, we'll create a
 *     block for each cell type that appears in the unstructured grid.
 *
 *     However if this writer is part of a parallel application (hence
 *     writing out a distributed Exodus file), then we need at the very
 *     least a list of all the block IDs that appear in the file.  And
 *     we need the element array of block IDs for the input unstructured grid.
 *
 *     In the absence of a vtkModelMetadata object, you can also provide
 *     time step information which we will include in the output Exodus
 *     file.
 *
 * @warning
 *     If the input floating point field arrays and point locations are all
 *     floats or all doubles, this class will operate more efficiently.
 *     Mixing floats and doubles will slow you down, because Exodus II
 *     requires that we write only floats or only doubles.
 *
 * @warning
 *     We use the terms "point" and "node" interchangeably.
 *     Also, we use the terms "element" and "cell" interchangeably.
*/

#ifndef vtkExodusIIWriter_h
#define vtkExodusIIWriter_h

#include "vtkIOExodusModule.h" // For export macro
#include "vtkWriter.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <vector> // STL Header
#include <map>    // STL Header
#include <string> // STL Header

class vtkModelMetadata;
class vtkDoubleArray;
class vtkIntArray;
class vtkUnstructuredGrid;

class VTKIOEXODUS_EXPORT vtkExodusIIWriter : public vtkWriter
{
public:
  static vtkExodusIIWriter *New ();
  vtkTypeMacro(vtkExodusIIWriter,vtkWriter);
  void PrintSelf (ostream& os, vtkIndent indent) override;

  /**
   * Specify the vtkModelMetadata object which contains the Exodus file
   * model information (metadata) absent in the vtkUnstructuredGrid.  If you
   * have this object, you don't need to set any other values before writing.
   * (Just the FileName and the Input.)
   * Note that the vtkExodusReader can create and attach a vtkModelMetadata
   * object to it's output.  If this has happened, the ExodusIIWriter will
   * find it and use it.
   */

  void SetModelMetadata (vtkModelMetadata*);
  vtkGetObjectMacro(ModelMetadata, vtkModelMetadata);

  /**
   * Name for the output file.  If writing in parallel, the number
   * of processes and the process rank will be appended to the name,
   * so each process is writing out a separate file.
   * If not set, this class will make up a file name.
   */

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  /**
   * If StoreDoubles is ON, the floating point fields in the Exodus file
   * will be double precision fields.  The default is determined by the
   * max precision of the input.  If the field data appears to be doubles,
   * then StoreDoubles will be ON, otherwise StoreDoubles will be OFF.
   */

  vtkSetMacro(StoreDoubles, int);
  vtkGetMacro(StoreDoubles, int);

  /**
   * We never write out ghost cells.  This variable is here to satisfy
   * the behavior of ParaView on invoking a parallel writer.
   */

  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

   /**
    * By default, the integer array containing the global Block Ids of the
    * cells is not included when the new Exodus II file is written out.  If
    * you do want to include this array, set WriteOutBlockIdArray to ON.
    */

  vtkSetMacro(WriteOutBlockIdArray, vtkTypeBool);
  vtkGetMacro(WriteOutBlockIdArray, vtkTypeBool);
  vtkBooleanMacro(WriteOutBlockIdArray, vtkTypeBool);

  /**
   * By default, the integer array containing the global Node Ids
   * is not included when the new Exodus II file is written out.  If
   * you do want to include this array, set WriteOutGlobalNodeIdArray to ON.
   */

  vtkSetMacro(WriteOutGlobalNodeIdArray, vtkTypeBool);
  vtkGetMacro(WriteOutGlobalNodeIdArray, vtkTypeBool);
  vtkBooleanMacro(WriteOutGlobalNodeIdArray, vtkTypeBool);

  /**
   * By default, the integer array containing the global Element Ids
   * is not included when the new Exodus II file is written out.  If you
   * do want to include this array, set WriteOutGlobalElementIdArray to ON.
   */

  vtkSetMacro(WriteOutGlobalElementIdArray, vtkTypeBool);
  vtkGetMacro(WriteOutGlobalElementIdArray, vtkTypeBool);
  vtkBooleanMacro(WriteOutGlobalElementIdArray, vtkTypeBool);

  /**
   * When WriteAllTimeSteps is turned ON, the writer is executed once for
   * each timestep available from the reader.
   */

  vtkSetMacro(WriteAllTimeSteps, vtkTypeBool);
  vtkGetMacro(WriteAllTimeSteps, vtkTypeBool);
  vtkBooleanMacro(WriteAllTimeSteps, vtkTypeBool);

  vtkSetStringMacro(BlockIdArrayName);
  vtkGetStringMacro(BlockIdArrayName);

  /**
   * In certain cases we know that metadata doesn't exist and
   * we want to ignore that warning.
   */

  vtkSetMacro(IgnoreMetaDataWarning, bool);
  vtkGetMacro(IgnoreMetaDataWarning, bool);
  vtkBooleanMacro(IgnoreMetaDataWarning, bool);

protected:
  vtkExodusIIWriter ();
  ~vtkExodusIIWriter () override;

  vtkModelMetadata* ModelMetadata;

  char *BlockIdArrayName;

  char *FileName;
  int fid;

  int NumberOfProcesses;
  int MyRank;

  int PassDoubles;

  int StoreDoubles;
  int GhostLevel;
  vtkTypeBool WriteOutBlockIdArray;
  vtkTypeBool WriteOutGlobalNodeIdArray;
  vtkTypeBool WriteOutGlobalElementIdArray;
  vtkTypeBool WriteAllTimeSteps;
  int NumberOfTimeSteps;

  int CurrentTimeIndex;
  int FileTimeOffset;
  bool TopologyChanged;
  bool IgnoreMetaDataWarning;

  vtkDataObject *OriginalInput;
  std::vector< vtkSmartPointer<vtkUnstructuredGrid> > FlattenedInput;
  std::vector< vtkSmartPointer<vtkUnstructuredGrid> > NewFlattenedInput;

  std::vector< vtkStdString > FlattenedNames;
  std::vector< vtkStdString > NewFlattenedNames;

  std::vector< vtkIntArray* > BlockIdList;

  struct Block
  {
    Block ()
    {
      this->Name = nullptr;
      this->Type = 0;
      this->NumElements = 0;
      this->ElementStartIndex = -1;
      this->NodesPerElement = 0;
      this->EntityCounts = std::vector<int>();
      this->EntityNodeOffsets = std::vector<int>();
      this->GridIndex = 0;
      this->OutputIndex = -1;
      this->NumAttributes = 0;
      this->BlockAttributes = nullptr;
    };
    const char *Name;
    int Type;
    int NumElements;
    int ElementStartIndex;
    int NodesPerElement;
    std::vector<int> EntityCounts;
    std::vector<int> EntityNodeOffsets;
    size_t GridIndex;
    // std::vector<int> CellIndex;
    int OutputIndex;
    int NumAttributes;
    float *BlockAttributes; // Owned by metamodel or null.  Don't delete.
  };
  std::map<int, Block> BlockInfoMap;
  int NumCells, NumPoints, MaxId;

  std::vector<vtkIdType*> GlobalElementIdList;
  std::vector<vtkIdType*> GlobalNodeIdList;

  int AtLeastOneGlobalElementIdList;
  int AtLeastOneGlobalNodeIdList;

  struct VariableInfo
  {
    int NumComponents;
    int InIndex;
    int ScalarOutOffset;
    std::vector<std::string> OutNames;
  };
  std::map<std::string, VariableInfo> GlobalVariableMap;
  std::map<std::string, VariableInfo> BlockVariableMap;
  std::map<std::string, VariableInfo> NodeVariableMap;
  int NumberOfScalarGlobalArrays;
  int NumberOfScalarElementArrays;
  int NumberOfScalarNodeArrays;

  std::vector< std::vector<int> > CellToElementOffset;

  // By BlockId, and within block ID by element variable, with variables
  // appearing in the same order in which they appear in OutputElementArrayNames

  int *BlockElementVariableTruthTable;
  int AllVariablesDefinedInAllBlocks;

  int BlockVariableTruthValue(int blockIdx, int varIdx);

  char *StrDupWithNew (const char *s);
  void StringUppercase (std::string& str);

  int ProcessRequest (vtkInformation* request,
                      vtkInformationVector** inputVector,
                      vtkInformationVector* outputVector) override;

  int RequestInformation (vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent (vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector);

  int FillInputPortInformation (int port, vtkInformation* info) override;

  int RequestData (vtkInformation* request,
                   vtkInformationVector** inputVector,
                   vtkInformationVector* outputVector) override;

  void WriteData () override;

  int FlattenHierarchy (vtkDataObject* input, const char *name, bool& changed);

  int CreateNewExodusFile ();
  void CloseExodusFile ();

  int IsDouble ();
  void RemoveGhostCells ();
  int CheckParametersInternal (int NumberOfProcesses, int MyRank);
  virtual int CheckParameters ();
  // If writing in parallel multiple time steps exchange after each time step
  // if we should continue the execution. Pass local continueExecution as a
  // parameter and return the global continueExecution.
  virtual int GlobalContinueExecuting(int localContinueExecution);
  int CheckInputArrays ();
  virtual void CheckBlockInfoMap();
  int ConstructBlockInfoMap ();
  int ConstructVariableInfoMaps ();
  int ParseMetadata ();
  int CreateDefaultMetadata ();
  char *GetCellTypeName (int t);

  int CreateBlockIdMetadata(vtkModelMetadata *em);
  int CreateBlockVariableMetadata (vtkModelMetadata* em);
  int CreateSetsMetadata (vtkModelMetadata* em);

  void ConvertVariableNames (std::map<std::string, VariableInfo>& variableMap);
  char **FlattenOutVariableNames (
            int nScalarArrays,
            const std::map<std::string, VariableInfo>& variableMap);
  std::string CreateNameForScalarArray (const char *root,
                                           int component,
                                           int numComponents);

  std::map<vtkIdType, vtkIdType> *LocalNodeIdMap;
  std::map<vtkIdType, vtkIdType> *LocalElementIdMap;

  vtkIdType GetNodeLocalId(vtkIdType id);
  vtkIdType GetElementLocalId(vtkIdType id);
  int GetElementType(vtkIdType id);

  int WriteInitializationParameters ();
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
  vtkIntArray* GetBlockIdArray (
    const char* BlockIdArrayName, vtkUnstructuredGrid* input);
  static bool SameTypeOfCells (vtkIntArray* cellToBlockId,
                               vtkUnstructuredGrid* input);

  double ExtractGlobalData (const char *name, int comp, int ts);
  int WriteGlobalData (int timestep, vtkDataArray *buffer);
  void ExtractCellData (const char *name, int comp, vtkDataArray *buffer);
  int WriteCellData (int timestep, vtkDataArray *buffer);
  void ExtractPointData (const char *name, int comp, vtkDataArray *buffer);
  int WritePointData (int timestep, vtkDataArray *buffer);

  /**
   * Get the maximum length name in the input data set. If it is smaller
   * than 32 characters long we just return the ExodusII default of 32.
   */
  virtual unsigned int GetMaxNameLength();

private:
  vtkExodusIIWriter (const vtkExodusIIWriter&) = delete;
  void operator= (const vtkExodusIIWriter&) = delete;
};

#endif
