/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusReader.h

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

// .NAME vtkExodusReader - Read exodus 2 files .ex2
// .SECTION Description
// vtkExodusReader is a unstructured grid source object that reads ExodusII
// files.  Most of the meta data associated with the file is loaded when 
// UpdateInformation is called.  This includes information like Title, number
// of blocks, number and names of arrays. This data can be retrieved from 
// methods in this reader. Separate arrays that are meant to be a single 
// vector, are combined internally for convenience.  To be combined, the array 
// names have to be identical except for a trailing X,Y and Z (or x,y,z).  By 
// default cell and point arrays are not loaded.  However, the user can flag 
// arrays to load with the methods "SetPointArrayStatus" and
// "SetCellArrayStatus".  The reader DOES NOT respond to piece requests
// 


#ifndef __vtkExodusReader_h
#define __vtkExodusReader_h

#define ARRAY_TYPE_NAMES_IN_CXX_FILE

#include "vtkUnstructuredGridAlgorithm.h"

class vtkIntArray;
class vtkFloatArray;
class vtkDataArray;
class vtkDataSet;
class vtkPoints;
class vtkExodusMetadata;
class vtkExodusModel;
class vtkExodusXMLParser;


#include "vtkDSPFilterGroup.h" //for USE_EXO_DSP_FILTERS


class VTK_HYBRID_EXPORT vtkExodusReader : public vtkUnstructuredGridAlgorithm 
{
public:
  static vtkExodusReader *New();
  vtkTypeMacro(vtkExodusReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

  // Description:
  // Specify file name of the Exodus file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify file name of the xml file.
  vtkSetStringMacro(XMLFileName);
  vtkGetStringMacro(XMLFileName);

  // Description:
  // Which TimeStep to read.    
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Extra cell data array that can be generated.  By default, this array
  // is ON.  The value of the array is the integer id found
  // in the exodus file. The name of the array is returned by 
  // GetBlockIdArrayName()
  vtkSetMacro(GenerateBlockIdCellArray, int);
  vtkGetMacro(GenerateBlockIdCellArray, int);
  vtkBooleanMacro(GenerateBlockIdCellArray, int);
  const char *GetBlockIdArrayName() { return "BlockId"; }  


  // Description:
  // Extra cell data array that can be generated.  By default, this array
  // is off.  The value of the array is the integer global id of the cell.
  // The name of the array is returned by GetGlobalElementIdArrayName()
  vtkSetMacro(GenerateGlobalElementIdArray, int);
  vtkGetMacro(GenerateGlobalElementIdArray, int);
  vtkBooleanMacro(GenerateGlobalElementIdArray, int);
//BTX
  enum {
    SEARCH_TYPE_ELEMENT=0,
    SEARCH_TYPE_NODE,
    SEARCH_TYPE_ELEMENT_THEN_NODE,
    SEARCH_TYPE_NODE_THEN_ELEMENT,
    ID_NOT_FOUND=-234121312
  };
//ETX
  static const char *GetGlobalElementIdArrayName() { return "GlobalElementId"; }
  static const char *GetPedigreeElementIdArrayName() { return "PedigreeElementId"; }
  static int GetGlobalElementID( vtkDataSet *data, int localID );
  static int GetGlobalElementID ( vtkDataSet *data, int localID, 
      int searchType );
  
  // Description:
  // Extra point data array that can be generated.  By default, this array
  // is ON.  The value of the array is the integer id of the node.
  // The id is relative to the entire data set.
  // The name of the array is returned by GlobalNodeIdArrayName().
  vtkSetMacro(GenerateGlobalNodeIdArray, int);
  vtkGetMacro(GenerateGlobalNodeIdArray, int);
  vtkBooleanMacro(GenerateGlobalNodeIdArray, int);
  static const char *GetGlobalNodeIdArrayName() { return "GlobalNodeId"; }  
  static const char *GetPedigreeNodeIdArrayName() { return "PedigreeNodeId"; }  
  static int GetGlobalNodeID( vtkDataSet *data, int localID );
  static int GetGlobalNodeID( vtkDataSet *data, int localID, 
      int searchType );

  // Description:
  // Geometric locations can include displacements.  By default, 
  // this is ON.  The nodal positions are 'displaced' by the
  // standard exodus displacment vector. If displacements
  // are turned 'off', the user can explicitly add them by
  // applying a warp filter.
  vtkSetMacro(ApplyDisplacements, int);
  vtkGetMacro(ApplyDisplacements, int);
  vtkBooleanMacro(ApplyDisplacements, int);
  vtkSetMacro(DisplacementMagnitude, float);
  vtkGetMacro(DisplacementMagnitude, float);
  
  // Description:
  // Access to meta data generated by UpdateInformation.
  vtkGetStringMacro(Title);
  vtkGetMacro(Dimensionality, int);
  vtkGetMacro(NumberOfTimeSteps, int);
  int GetNumberOfElements() { return this->NumberOfUsedElements; }
  vtkGetMacro(NumberOfNodeSets, int);
  vtkGetMacro(NumberOfSideSets, int);
  vtkGetMacro(NumberOfBlocks, int);
  vtkGetVector2Macro(TimeStepRange, int);
  vtkSetVector2Macro(TimeStepRange, int);
  int GetNumberOfNodes() { return this->NumberOfUsedNodes; }
  int GetNumberOfElementsInBlock(int block_idx);
  int GetBlockId(int block_idx);
  virtual int GetTotalNumberOfNodes() { return this->NumberOfNodesInFile; }
  
  
  // Descriptions:
  // By default arrays are not loaded.  These methods allow the user to select
  // which arrays they want to load.  You can get information about the arrays
  // by first caling UpdateInformation, and using GetPointArrayName ...
  // (Developer Note) This meta data is all accessed through vtkExodusMetadata
  int GetNumberOfPointArrays();
  const char *GetPointArrayName(int index);
  int GetPointArrayID( const char *name );
  int GetPointArrayNumberOfComponents(int index);
  void SetPointArrayStatus(int index, int flag);
  void SetPointArrayStatus(const char*, int flag);
  int GetPointArrayStatus(int index);
  int GetPointArrayStatus(const char*);

  int GetNumberOfCellArrays();
  const char *GetCellArrayName(int index);
  int GetCellArrayID( const char *name );
  int GetCellArrayNumberOfComponents(int index);
  void SetCellArrayStatus(int index, int flag);
  void SetCellArrayStatus(const char*, int flag);
  int GetCellArrayStatus(int index);
  int GetCellArrayStatus(const char*);
  virtual int GetTotalNumberOfElements() 
      { return this->NumberOfElementsInFile; }

  // Descriptions:
  // By default all blocks are loaded. These methods allow the user to select
  // which blocks they want to load.  You can get information about the blocks
  // by first caling UpdateInformation, and using GetBlockArrayName ... 
  int GetNumberOfBlockArrays();
  const char *GetBlockArrayName(int index);
  int GetBlockArrayID( const char *name );
  void SetBlockArrayStatus(int index, int flag);
  void SetBlockArrayStatus(const char*, int flag);
  int GetBlockArrayStatus(int index);
  int GetBlockArrayStatus(const char*);  


  // Description:
  // By default Node/Side sets are not loaded, These methods allow the user to
  // select which Node/Side sets they want to load. NumberOfNodeSets and
  // NumberOfSideSets (set by vtk macros) are stored in vtkExodusReader 
  // but other Node/Side set metadata are stored in vtkExodusMetaData
  // Note: GetNumberOfNodeSetArrays and GetNumberOfSideSetArrays are
  // just syntatic sugar for paraview server xml
  int GetNumberOfNodeSetArrays(){return this->GetNumberOfNodeSets();}
  int GetNodeSetArrayStatus(int index);
  int GetNodeSetArrayStatus(const char* name);
  void SetNodeSetArrayStatus(int index, int flag);
  void SetNodeSetArrayStatus(const char* name, int flag);
  const char *GetNodeSetArrayName(int index);
  
  int GetNumberOfSideSetArrays(){return this->GetNumberOfSideSets();}
  int GetSideSetArrayStatus(int index);
  int GetSideSetArrayStatus(const char* name);
  void SetSideSetArrayStatus(int index, int flag);
  void SetSideSetArrayStatus(const char* name, int flag);
  const char *GetSideSetArrayName(int index);

  // Descriptions:
  // By default all parts are loaded. These methods allow the user to select
  // which parts they want to load.  You can get information about the parts
  // by first caling UpdateInformation, and using GetPartArrayName ... 
  int GetNumberOfPartArrays();
  const char *GetPartArrayName(int arrayIdx);
  int GetPartArrayID( const char *name );
  const char *GetPartBlockInfo(int arrayIdx);
  void SetPartArrayStatus(int index, int flag);
  void SetPartArrayStatus(const char*, int flag);
  int GetPartArrayStatus(int index);
  int GetPartArrayStatus(const char*);
  

  // Descriptions:
  // By default all materials are loaded. These methods allow the user to 
  // select which materials they want to load.  You can get information 
  // about the materials by first caling UpdateInformation, and using 
  // GetMaterialArrayName ... 
  int GetNumberOfMaterialArrays();
  const char *GetMaterialArrayName(int arrayIdx);
  int GetMaterialArrayID( const char *name );
  void SetMaterialArrayStatus(int index, int flag);
  void SetMaterialArrayStatus(const char*, int flag);
  int GetMaterialArrayStatus(int index);
  int GetMaterialArrayStatus(const char*);

  // Descriptions:
  // By default all assemblies are loaded. These methods allow the user to 
  // select which assemblies they want to load.  You can get information 
  // about the assemblies by first caling UpdateInformation, and using 
  // GetAssemblyArrayName ... 
  int GetNumberOfAssemblyArrays();
  const char *GetAssemblyArrayName(int arrayIdx);
  int GetAssemblyArrayID( const char *name );
  void SetAssemblyArrayStatus(int index, int flag);
  void SetAssemblyArrayStatus(const char*, int flag);
  int GetAssemblyArrayStatus(int index);
  int GetAssemblyArrayStatus(const char*);

  // Descriptions:
  // By default all hierarchy entries are loaded. These methods allow 
  //the user to 
  // select which hierarchy entries they want to load.  You can get information 
  // about the hierarchy entries by first caling UpdateInformation, and using 
  // GetHierarchyArrayName ... 
  //these methods do not call functions in metaData. They call functions on
  //the ExodusXMLParser since it seemed silly to duplicate all the information
  int GetNumberOfHierarchyArrays();
  const char *GetHierarchyArrayName(int arrayIdx);
  void SetHierarchyArrayStatus(int index, int flag);
  void SetHierarchyArrayStatus(const char*, int flag);
  int GetHierarchyArrayStatus(int index);
  int GetHierarchyArrayStatus(const char*);

  // Description:
  // Some simulations overload the Exodus time steps to represent mode shapes.
  // In this case, it does not make sense to iterate over the "time steps",
  // because they are not meant to be played in order.  Rather, each represents
  // the vibration at a different "mode."  Setting this to 1 changes the
  // semantics of the reader to not report the time steps to downstream filters.
  // By default, this is off, which is the case for most Exodus files.
  vtkGetMacro(HasModeShapes, int);
  vtkSetMacro(HasModeShapes, int);
  vtkBooleanMacro(HasModeShapes, int);

  vtkGetMacro(DisplayType,int);
  virtual void SetDisplayType(int type);

  // Description:
  //   There is a great deal of model information lost when an Exodus II
  //   file is read in to a vtkUnstructuredGrid.  Turn this option ON 
  //   if you want this metadata to be read in to a vtkExodusModel object.
  //   The default is OFF.

  vtkBooleanMacro(ExodusModelMetadata, int);
  vtkSetMacro(ExodusModelMetadata, int);
  vtkGetMacro(ExodusModelMetadata, int);

  // Description:
  //   Returns the object which encapsulates the model metadata.

  vtkExodusModel *GetExodusModel(){return this->ExodusModel;}

  // Description:
  //  By default, the ExodusModel metadata (if requested with
  //  ExodusModelMetadataOn()) is also encoded into field arrays 
  //  and attached to the output unstructured grid.  Set this OFF
  //  if you don't want this to happen.  (The vtkExodusIIWriter and
  //  the vtkEnSightWriter can unpack this metadata from the field
  //  arrays and use it when writing out Exodus or EnSight files.)

  vtkSetMacro(PackExodusModelOntoOutput, int);
  vtkGetMacro(PackExodusModelOntoOutput, int);
  vtkBooleanMacro(PackExodusModelOntoOutput, int);

//BTX
  // Descriptions:
  // Syntactic sugar funtions.
  enum ArrayType {
    CELL=0,
    POINT,
    BLOCK,
    PART,
    MATERIAL,
    ASSEMBLY,
    HIERARCHY,
    NUM_ARRAY_TYPES,
    UNKNOWN_TYPE
  };
//ETX

  // Descriptions:
  // return boolean indicating whether the type,name is a valid variable 
  int IsValidVariable( const char *type, const char *name );

//BTX
  // Description:
  // Named type accessor for array information
  int GetNumberOfArrays( vtkExodusReader::ArrayType type );
  const char *GetArrayName( vtkExodusReader::ArrayType type, int id );
//ETX

  // Descriptions:
  // Return the id of the type,name variable
  int GetVariableID ( const char *type, const char *name );

  void SetAllAssemblyArrayStatus( int status );
  void SetAllBlockArrayStatus( int status );
  void SetAllCellArrayStatus( int status );
  void SetAllHierarchyArrayStatus( int status );
  void SetAllMaterialArrayStatus( int status );
  void SetAllPartArrayStatus( int status );
  void SetAllPointArrayStatus( int status );
//BTX
  void SetAllArrayStatus ( vtkExodusReader::ArrayType type, int flag );
  void SetArrayStatus ( vtkExodusReader::ArrayType type, const char *name, 
      int flag );
//ETX
  void SetArrayStatus ( const char *type, const char *name, int flag )
    {
    this->SetArrayStatus( this->GetArrayTypeID(type), name, flag );
    }
//BTX
  int  GetArrayStatus ( vtkExodusReader::ArrayType type, const char *name );
//ETX
  int  GetArrayStatus ( const char *type, const char *name )
    {
    return this->GetArrayStatus( this->GetArrayTypeID( type ), name );
    }

  // Helper functions
  static int StringsEqual(const char* s1, char* s2);
  static void StringUppercase(const char* str, char* upperstr);
  static char *StrDupWithNew(const char *s);

  // time series query functions
  int GetTimeSeriesData( int ID, const char *vName, const char *vType, 
                         vtkFloatArray *result );
  

  //begin USE_EXO_DSP_FILTERS
  int GetNumberOfVariableArrays();
  const char *GetVariableArrayName(int a_which);
  void EnableDSPFiltering(); 
  void AddFilter(vtkDSPFilterDefinition *a_filter);
  void StartAddingFilter();
  void AddFilterInputVar(char *name);
  void AddFilterOutputVar(char *name);
  void AddFilterNumeratorWeight(double weight);
  void AddFilterForwardNumeratorWeight(double weight);
  void AddFilterDenominatorWeight(double weight);
  void FinishAddingFilter();
  void RemoveFilter(char *a_outputVariableName);
  void GetDSPOutputArrays(int exoid, vtkUnstructuredGrid* output);
//BTX
  vtkExodusReader::ArrayType GetArrayTypeID( const char *type ); 

#ifdef ARRAY_TYPE_NAMES_IN_CXX_FILE
  static const char *GetArrayTypeName( vtkExodusReader::ArrayType type );
#else
  static const char *ArrayTypeNames[NUM_ARRAY_TYPES];

  static const char *GetArrayTypeName( vtkExodusReader::ArrayType type )
    {
    return ArrayTypeNames[type];
    }
#endif
//ETX

  vtkDSPFilterDefinition *AddingFilter;
  int DSPFilteringIsEnabled;
  vtkDSPFilterGroup **DSPFilters;
  //end USE_EXO_DSP_FILTERS



protected:
  vtkExodusReader();
  ~vtkExodusReader();

  void NewExodusModel();

  void ReadGeometry(int exoid, vtkUnstructuredGrid* output);
  void ReadCells(int exoid, vtkUnstructuredGrid* output);
  void ReadPoints(int exoid, vtkUnstructuredGrid* output);
  void ReadArrays(int exoid, vtkUnstructuredGrid* output);
  void ReadNodeAndSideSets(int exoid, vtkUnstructuredGrid* output);
  vtkDataArray *ReadPointArray(int exoid, int varIndex);
  vtkDataArray *ReadPointVector(int handle, int varIndex, int dim);
  vtkDataArray *ReadCellArray(int exoid, int varIndex);
  vtkDataArray *ReadCellVector(int handle, int varIndex, int dim);
  void ReadNodeSetMetadata();
  void ReadSideSetMetadata();

  // helper for finding IDs
  static int GetIDHelper ( const char *arrayName, vtkDataSet *data, int localID,
      int searchType );
  static int GetGlobalID( const char *arrayName, vtkDataSet *data, int localID, 
      int searchType );
  
  // This method is a helper for determining the
  // number of additional cell scalar field
  // values needed to 'pad' for node and side sets
  int GetExtraCellCountForNodeSideSets();
  
  // This method generates arrays like blockid, global nodeid
  // and global element id
  void GenerateExtraArrays(vtkUnstructuredGrid* output);

  // Parameters for controlling what is read in.
  char *FileName;
  char *XMLFileName;
  int TimeStep;
  int ActualTimeStep;
  double TimeValue;
  int GenerateBlockIdCellArray;
  int GenerateGlobalElementIdArray;
  int GenerateGlobalNodeIdArray;
  int ApplyDisplacements;
  double DisplacementMagnitude;
  
  // Information specific for exodus files.
  vtkSetStringMacro(Title);
  char *Title;
  int Dimensionality;
  int NumberOfNodeSets;
  int NumberOfSideSets;
  int NumberOfBlocks;
  int NumberOfUsedNodes;
  int NumberOfNodesInFile;
  int NumberOfUsedElements;
  int NumberOfElementsInFile;
  int NumberOfTimeSteps;
  int ExodusCPUWordSize;
  int ExodusIOWordSize;
  float ExodusVersion;
  vtkIntArray *CellVarTruthTable;

  //1=display Block names
  //2=display Part names
  //3=display Material names
  int DisplayType;
  
  //Parser that understands the xml part and material file
  vtkExodusXMLParser *Parser;

  // **KEN** By VTK convention, metaData should be Metadata.
 
  ////////////////////////////////////////
  // Scalar Array and Block Info
  ////////////////////////////////////////
  vtkExodusMetadata *MetaData;
  
 
  // Descriptions:
  // Store current file name and current handle.
  int CurrentHandle;
  char* CurrentFileName;
  char* CurrentXMLFileName;
  vtkSetStringMacro(CurrentFileName);
  vtkSetStringMacro(CurrentXMLFileName);

  // Open the exodus file, and set some basic information
  int OpenCurrentFile();

  // Close the exodus file
  void CloseCurrentFile();

  
  // Descriptions:
  // Store the range of time steps
  int TimeStepRange[2];

  // DataCache: this object keeps the points and cells
  // around so they don't need to be re-read when the
  // timestep changes or an scalar array is switched
  vtkUnstructuredGrid *DataCache;

  // Should I re-read in the geometry and topology of the dataset
  int RemakeDataCacheFlag;

  // vtkExodusModel needs to count changes in geometry, so it knows
  //   if geometry has changed since it last updated model data.

  int NewGeometryCount;

  // PointMap keeps track of which points are actually
  // used by the cells that are read in (blocks)
  vtkIntArray *PointMap;
  vtkIntArray *ReversePointMap;
  void SetUpPointMap(int num_points);
  int GetPointMapIndex(int point_id);

  // Global element ID cache
  int *GlobalElementIdCache;
  void SetGlobalElementIdCache(int *list);

  // Time query function. Called by ExecuteInformation().
  // Fills the TimestepValues array.
  void GetAllTimes(vtkInformationVector *);

  int HasModeShapes;

  vtkExodusModel *ExodusModel;
  int PackExodusModelOntoOutput;
  int ExodusModelMetadata;

  double *TimeSteps;

  int RequestInformation(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Used to determine current progress.
  double ProgressOffset;
  double ProgressScale;

private:
  vtkExodusReader(const vtkExodusReader&); // Not implemented
  void operator=(const vtkExodusReader&); // Not implemented

  void AddDisplacements(vtkUnstructuredGrid* output);
  void RemoveBeginningAndTrailingSpaces(char **names, int len);
  
  void FixMetadataTruthTable(int *table, int len);
};

#endif
