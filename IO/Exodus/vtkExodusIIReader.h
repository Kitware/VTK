/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusIIReader.h

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

// .NAME vtkExodusIIReader - Read exodus 2 files .ex2
// .SECTION Description
// vtkExodusIIReader is a unstructured grid source object that reads ExodusII
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


#ifndef __vtkExodusIIReader_h
#define __vtkExodusIIReader_h

#include "vtkIOExodusModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkDataArray;
class vtkDataSet;
class vtkExodusIICache;
class vtkExodusIIReaderPrivate;
class vtkExodusModel;
class vtkFloatArray;
class vtkGraph;
class vtkIntArray;
class vtkPoints;
class vtkUnstructuredGrid;

class VTKIOEXODUS_EXPORT vtkExodusIIReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExodusIIReader *New();
  vtkTypeMacro(vtkExodusIIReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

  //virtual void Modified();

  // Description:
  // Return the object's MTime. This is overridden to include the timestamp of its internal class.
  virtual unsigned long GetMTime();

  // Description:
  // Return the MTime of the internal data structure.
  // This is really only intended for use by vtkPExodusIIReader in order
  // to determine if the filename is newer than the metadata.
  virtual unsigned long GetMetadataMTime();

  // Description:
  // Specify file name of the Exodus file.
  virtual void SetFileName( const char* fname );
  vtkGetStringMacro(FileName);

  // Description:
  // Specify file name of the xml file.
  virtual void SetXMLFileName( const char* fname );
  vtkGetStringMacro(XMLFileName);

  // Description:
  // Which TimeStep to read.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Convenience method to set the mode-shape which is same as
  // this->SetTimeStep(val-1);
  void SetModeShape(int val)
    {
    this->SetTimeStep(val-1);
    }

  // Description:
  // Returns the available range of valid integer time steps.
  vtkGetVector2Macro(TimeStepRange,int);
  vtkSetVector2Macro(TimeStepRange,int);

  // Description:
  // Extra cell data array that can be generated.  By default, this array
  // is ON.  The value of the array is the integer id found
  // in the exodus file. The name of the array is returned by
  // GetBlockIdArrayName(). For cells representing elements from
  // an Exodus element block, this is set to the element block ID. For
  // cells representing edges from an Exodus edge block, this is the
  // edge block ID. Similarly, this is the face block ID for cells
  // representing faces from an Exodus face block. The same holds
  // for cells representing entries of node, edge, face, side, and element sets.
  virtual void SetGenerateObjectIdCellArray( int g );
  int GetGenerateObjectIdCellArray();
  vtkBooleanMacro(GenerateObjectIdCellArray, int);
  static const char *GetObjectIdArrayName() { return "ObjectId"; }

  virtual void SetGenerateGlobalElementIdArray( int g );
  int GetGenerateGlobalElementIdArray();
  vtkBooleanMacro(GenerateGlobalElementIdArray, int);

  virtual void SetGenerateGlobalNodeIdArray( int g );
  int GetGenerateGlobalNodeIdArray();
  vtkBooleanMacro(GenerateGlobalNodeIdArray, int);

  virtual void SetGenerateImplicitElementIdArray( int g );
  int GetGenerateImplicitElementIdArray();
  vtkBooleanMacro(GenerateImplicitElementIdArray, int);

  virtual void SetGenerateImplicitNodeIdArray( int g );
  int GetGenerateImplicitNodeIdArray();
  vtkBooleanMacro(GenerateImplicitNodeIdArray, int);

  virtual void SetGenerateFileIdArray( int f );
  int GetGenerateFileIdArray();
  vtkBooleanMacro(GenerateFileIdArray, int);
  virtual void SetFileId( int f );
  int GetFileId();

//BTX
  // Description:
  // Extra cell data array that can be generated.  By default, this array
  // is off.  The value of the array is the integer global id of the cell.
  // The name of the array is returned by GetGlobalElementIdArrayName()
  // ***NOTE*** No more "unique" global ID. Instead we have an arbitrary number of maps.
  enum {
    SEARCH_TYPE_ELEMENT=0,
    SEARCH_TYPE_NODE,
    SEARCH_TYPE_ELEMENT_THEN_NODE,
    SEARCH_TYPE_NODE_THEN_ELEMENT,
    ID_NOT_FOUND=-234121312
  };
  // NOTE: GetNumberOfObjectTypes must be updated whenever you add an entry to enum ObjectType {...}
  enum ObjectType {
    // match Exodus macros from exodusII.h and exodusII_ext.h
    EDGE_BLOCK = 6,
    FACE_BLOCK = 8,
    ELEM_BLOCK = 1,
    NODE_SET = 2,
    EDGE_SET = 7,
    FACE_SET = 9,
    SIDE_SET = 3,
    ELEM_SET = 10,
    NODE_MAP = 5,
    EDGE_MAP = 11,
    FACE_MAP = 12,
    ELEM_MAP = 4,
    GLOBAL = 13,
    NODAL = 14,
    // extended values (not in Exodus headers) for use with SetAllArrayStatus:
    ASSEMBLY = 60,
    PART = 61,
    MATERIAL = 62,
    HIERARCHY = 63,
    // extended values (not in Exodus headers) for use in cache keys:
    QA_RECORDS = 103,          //!< Exodus II Quality Assurance (QA) string metadata
    INFO_RECORDS = 104,        //!< Exodus II Information Records string metadata
    GLOBAL_TEMPORAL = 102,  //!< global data across timesteps
    NODAL_TEMPORAL = 101,      //!< nodal data across timesteps
    ELEM_BLOCK_TEMPORAL = 100,  //!< element data across timesteps
    GLOBAL_CONN = 99,          //!< connectivity assembled from all blocks+sets to be loaded
    ELEM_BLOCK_ELEM_CONN = 98, //!< raw element block connectivity for elements (not edges/faces)
    ELEM_BLOCK_FACE_CONN = 97, //!< raw element block connectivity for faces (references face blocks)
    ELEM_BLOCK_EDGE_CONN = 96, //!< raw element block connectivity for edges (references edge blocks)
    FACE_BLOCK_CONN = 95,      //!< raw face block connectivity (references nodes)
    EDGE_BLOCK_CONN = 94,      //!< raw edge block connectivity (references nodes)
    ELEM_SET_CONN = 93,        //!< element set connectivity
    SIDE_SET_CONN = 92,        //!< side set connectivity
    FACE_SET_CONN = 91,        //!< face set connectivity
    EDGE_SET_CONN = 90,        //!< edge set connectivity
    NODE_SET_CONN = 89,        //!< node set connectivity
    NODAL_COORDS = 88,         //!< raw nodal coordinates (not the "squeezed" version)
    OBJECT_ID = 87,            //!< object id (old BlockId) array
    IMPLICIT_ELEMENT_ID = 108, //!< the implicit global index of each element given by exodus
    IMPLICIT_NODE_ID = 107,    //!< the implicit global index of each node given by exodus
    GLOBAL_ELEMENT_ID = 86,    //!< element id array extracted for a particular block (yes, this is a bad name)
    GLOBAL_NODE_ID = 85,       //!< nodal id array extracted for a particular block (yes, this is a bad name)
    ELEMENT_ID = 84,           //!< element id map (old-style elem_num_map or first new-style elem map) array
    NODE_ID = 83,              //!< nodal id map (old-style node_num_map or first new-style node map) array
    NODAL_SQUEEZEMAP = 82,     //!< the integer map use to "squeeze" coordinates and nodal arrays/maps
    ELEM_BLOCK_ATTRIB = 81,    //!< an element block attribute array (time-constant scalar per element)
    FACE_BLOCK_ATTRIB = 80,    //!< a face block attribute array (time-constant scalar per element)
    EDGE_BLOCK_ATTRIB = 79,    //!< an edge block attribute array (time-constant scalar per element)
    FACE_ID = 105,             //!< face id map (old-style face_num_map or first new-style face map) array
    EDGE_ID = 106,             //!< edge id map (old-style edge_num_map or first new-style edge map) array
    ENTITY_COUNTS = 109        //!< polyhedra per-entity count ex_get_block returns the sum for polyhedra
  };
//ETX
  static const char* GetGlobalElementIdArrayName() { return "GlobalElementId"; }
  static const char* GetPedigreeElementIdArrayName() { return "PedigreeElementId"; }
  static int GetGlobalElementID( vtkDataSet *data, int localID );
  static int GetGlobalElementID ( vtkDataSet *data, int localID,
      int searchType );
  static const char* GetImplicitElementIdArrayName() { return "ImplicitElementId"; }

  static const char* GetGlobalFaceIdArrayName() { return "GlobalFaceId"; }
  static const char* GetPedigreeFaceIdArrayName() { return "PedigreeFaceId"; }
  static int GetGlobalFaceID( vtkDataSet *data, int localID );
  static int GetGlobalFaceID ( vtkDataSet *data, int localID,
      int searchType );
  static const char* GetImplicitFaceIdArrayName() { return "ImplicitFaceId"; }

  static const char* GetGlobalEdgeIdArrayName() { return "GlobalEdgeId"; }
  static const char* GetPedigreeEdgeIdArrayName() { return "PedigreeEdgeId"; }
  static int GetGlobalEdgeID( vtkDataSet *data, int localID );
  static int GetGlobalEdgeID ( vtkDataSet *data, int localID,
      int searchType );
  static const char* GetImplicitEdgeIdArrayName() { return "ImplicitEdgeId"; }

  // Description:
  // Extra point data array that can be generated.  By default, this array
  // is ON.  The value of the array is the integer id of the node.
  // The id is relative to the entire data set.
  // The name of the array is returned by GlobalNodeIdArrayName().
  static const char* GetGlobalNodeIdArrayName() { return "GlobalNodeId"; }
  static const char* GetPedigreeNodeIdArrayName() { return "PedigreeNodeId"; }
  static int GetGlobalNodeID( vtkDataSet *data, int localID );
  static int GetGlobalNodeID( vtkDataSet *data, int localID,
      int searchType );
  static const char* GetImplicitNodeIdArrayName() { return "ImplicitNodeId"; }

  // Description:
  // Get the name of the array that stores the mapping from side set
  // cells back to the global id of the elements they bound.
  static const char* GetSideSetSourceElementIdArrayName() { return "SourceElementId"; }

  // Description:
  // Get the name of the array that stores the mapping from side set
  // cells back to the canonical side of the elements they bound.
  static const char* GetSideSetSourceElementSideArrayName() { return "SourceElementSide"; }
  // Description:
  // Geometric locations can include displacements.  By default,
  // this is ON.  The nodal positions are 'displaced' by the
  // standard exodus displacment vector. If displacements
  // are turned 'off', the user can explicitly add them by
  // applying a warp filter.
  virtual void SetApplyDisplacements( int d );
  int GetApplyDisplacements();
  vtkBooleanMacro(ApplyDisplacements, int);
  virtual void SetDisplacementMagnitude( float s );
  float GetDisplacementMagnitude();

  // Description:
  // Set/Get whether the Exodus sequence number corresponds to time steps or mode shapes.
  // By default, HasModeShapes is false unless two time values in the Exodus file are identical,
  // in which case it is true.
  virtual void SetHasModeShapes( int ms );
  int GetHasModeShapes();
  vtkBooleanMacro(HasModeShapes,int);

  // Description:
  // Set/Get the time used to animate mode shapes.
  // This is a number between 0 and 1 that is used to scale the \a DisplacementMagnitude
  // in a sinusoidal pattern. Specifically, the displacement vector for each vertex is scaled by
  // \f$ \mathrm{DisplacementMagnitude} cos( 2\pi \mathrm{ModeShapeTime} ) \f$ before it is
  // added to the vertex coordinates.
  virtual void SetModeShapeTime( double phase );
  double GetModeShapeTime();

  // Description:
  // If this flag is on (the default) and HasModeShapes is also on, then this
  // reader will report a continuous time range [0,1] and animate the
  // displacements in a periodic sinusoid.  If this flag is off and
  // HasModeShapes is on, this reader ignores time.  This flag has no effect if
  // HasModeShapes is off.
  virtual void SetAnimateModeShapes(int flag);
  int GetAnimateModeShapes();
  vtkBooleanMacro(AnimateModeShapes, int);

  // Description:
  // Access to meta data generated by UpdateInformation.
  const char* GetTitle();
  int GetDimensionality();
  int GetNumberOfTimeSteps();

  int GetNumberOfNodesInFile();
  int GetNumberOfEdgesInFile();
  int GetNumberOfFacesInFile();
  int GetNumberOfElementsInFile();

  int GetObjectTypeFromName( const char* name );
  const char* GetObjectTypeName( int );

  int GetNumberOfNodes();
  int GetNumberOfObjects( int objectType );
  int GetNumberOfEntriesInObject( int objectType, int objectIndex );
  int GetObjectId( int objectType, int objectIndex );
  const char* GetObjectName( int objectType, int objectIndex );
  int GetObjectIndex( int objectType, const char* objectName );
  int GetObjectIndex( int objectType, int id );
  int GetObjectStatus( int objectType, int objectIndex );
  int GetObjectStatus( int objectType, const char* objectName )
    { return this->GetObjectStatus( objectType, this->GetObjectIndex( objectType, objectName ) ); }
  void SetObjectStatus( int objectType, int objectIndex, int status );
  void SetObjectStatus( int objectType, const char* objectName, int status );

  // Descriptions:
  // By default arrays are not loaded.  These methods allow the user to select
  // which arrays they want to load.  You can get information about the arrays
  // by first caling UpdateInformation, and using GetPointArrayName ...
  // (Developer Note) This meta data is all accessed through vtkExodusMetadata
  int GetNumberOfObjectArrays( int objectType );
  const char* GetObjectArrayName( int objectType, int arrayIndex );
  int GetObjectArrayIndex( int objectType, const char* arrayName );
  int GetNumberOfObjectArrayComponents( int objectType, int arrayIndex );
  int GetObjectArrayStatus( int objectType, int arrayIndex );
  int GetObjectArrayStatus( int objectType, const char* arrayName )
    { return this->GetObjectArrayStatus( objectType, this->GetObjectArrayIndex( objectType, arrayName ) ); }
  void SetObjectArrayStatus( int objectType, int arrayIndex, int status );
  void SetObjectArrayStatus( int objectType, const char* arrayName, int status );

  // Descriptions:
  // By default attributes are not loaded.  These methods allow the user to select
  // which attributes they want to load.  You can get information about the attributes
  // by first caling UpdateInformation, and using GetObjectAttributeName ...
  // (Developer Note) This meta data is all accessed through vtkExodusMetadata
  int GetNumberOfObjectAttributes( int objectType, int objectIndex );
  const char* GetObjectAttributeName( int objectType, int objectIndex, int attribIndex );
  int GetObjectAttributeIndex( int objectType, int objectIndex, const char* attribName );
  int GetObjectAttributeStatus( int objectType, int objectIndex, int attribIndex );
  int GetObjectAttributeStatus( int objectType, int objectIndex, const char* attribName )
    { return this->GetObjectAttributeStatus( objectType, objectIndex,
      this->GetObjectAttributeIndex( objectType, objectIndex, attribName ) ); }
  void SetObjectAttributeStatus( int objectType, int objectIndex, int attribIndex, int status );
  void SetObjectAttributeStatus( int objectType, int objectIndex, const char* attribName, int status )
    { this->SetObjectAttributeStatus( objectType, objectIndex,
      this->GetObjectAttributeIndex( objectType, objectIndex, attribName ), status ); }

  virtual vtkIdType GetTotalNumberOfNodes();
  virtual vtkIdType GetTotalNumberOfEdges();
  virtual vtkIdType GetTotalNumberOfFaces();
  virtual vtkIdType GetTotalNumberOfElements();

  // Descriptions:
  // By default all parts are loaded. These methods allow the user to select
  // which parts they want to load.  You can get information about the parts
  // by first caling UpdateInformation, and using GetPartArrayName ...
  int GetNumberOfPartArrays();
  const char* GetPartArrayName(int arrayIdx);
  int GetPartArrayID( const char *name );
  const char* GetPartBlockInfo(int arrayIdx);
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
  const char* GetMaterialArrayName(int arrayIdx);
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
  const char* GetAssemblyArrayName(int arrayIdx);
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
  const char* GetHierarchyArrayName(int arrayIdx);
  void SetHierarchyArrayStatus(int index, int flag);
  void SetHierarchyArrayStatus(const char*, int flag);
  int GetHierarchyArrayStatus(int index);
  int GetHierarchyArrayStatus(const char*);

  vtkGetMacro(DisplayType,int);
  virtual void SetDisplayType(int type);

  // Description:
  //   There is a great deal of model information lost when an Exodus II
  //   file is read in to a vtkMultiBlockDataSet.  Turn this option ON
  //   if you want this metadata to be read in to a vtkExodusModel object.
  //   The default is OFF.

  vtkBooleanMacro(ExodusModelMetadata, int);
  vtkSetMacro(ExodusModelMetadata, int);
  vtkGetMacro(ExodusModelMetadata, int);

  // Description:
  //   Returns the object which encapsulates the model metadata.
  vtkGetObjectMacro(ExodusModel,vtkExodusModel);

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

  // Descriptions:
  // return boolean indicating whether the type,name is a valid variable
  int IsValidVariable( const char *type, const char *name );

  // Descriptions:
  // Return the id of the type,name variable
  int GetVariableID ( const char *type, const char *name );

  void SetAllArrayStatus( int otype, int status );
  // Helper functions
  //static int StringsEqual(const char* s1, char* s2);
  //static void StringUppercase(const char* str, char* upperstr);
  //static char *StrDupWithNew(const char *s);

  // time series query functions
  int GetTimeSeriesData( int ID, const char *vName, const char *vType,
                         vtkFloatArray *result );



  int GetNumberOfEdgeBlockArrays()
    { return this->GetNumberOfObjects(EDGE_BLOCK); }
  const char* GetEdgeBlockArrayName(int index)
    { return this->GetObjectName(EDGE_BLOCK, index); }
  int GetEdgeBlockArrayStatus(const char* name)
    { return this->GetObjectStatus(EDGE_BLOCK, name); }
  void SetEdgeBlockArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(EDGE_BLOCK, name, flag); }

  int GetNumberOfFaceBlockArrays()
    { return this->GetNumberOfObjects(FACE_BLOCK); }
  const char* GetFaceBlockArrayName(int index)
    { return this->GetObjectName(FACE_BLOCK, index); }
  int GetFaceBlockArrayStatus(const char* name)
    { return this->GetObjectStatus(FACE_BLOCK, name); }
  void SetFaceBlockArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(FACE_BLOCK, name, flag); }

  int GetNumberOfElementBlockArrays()
    { return this->GetNumberOfObjects(ELEM_BLOCK); }
  const char* GetElementBlockArrayName(int index)
    { return this->GetObjectName(ELEM_BLOCK, index); }
  int GetElementBlockArrayStatus(const char* name)
    { return this->GetObjectStatus(ELEM_BLOCK, name); }
  void SetElementBlockArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(ELEM_BLOCK, name, flag); }

  int GetNumberOfGlobalResultArrays()
    { return this->GetNumberOfObjectArrays(GLOBAL); }
  const char* GetGlobalResultArrayName(int index)
    { return this->GetObjectArrayName(GLOBAL, index); }
  int GetGlobalResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(GLOBAL, name); }
  void SetGlobalResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(GLOBAL, name, flag); }

  int GetNumberOfPointResultArrays()
    { return this->GetNumberOfObjectArrays(NODAL); }
  const char* GetPointResultArrayName(int index)
    { return this->GetObjectArrayName(NODAL, index); }
  int GetPointResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(NODAL, name); }
  void SetPointResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(NODAL, name, flag); }

  int GetNumberOfEdgeResultArrays()
    { return this->GetNumberOfObjectArrays(EDGE_BLOCK); }
  const char* GetEdgeResultArrayName(int index)
    { return this->GetObjectArrayName(EDGE_BLOCK, index); }
  int GetEdgeResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(EDGE_BLOCK, name); }
  void SetEdgeResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(EDGE_BLOCK, name, flag); }

  int GetNumberOfFaceResultArrays()
    { return this->GetNumberOfObjectArrays(FACE_BLOCK); }
  const char* GetFaceResultArrayName(int index)
    { return this->GetObjectArrayName(FACE_BLOCK, index); }
  int GetFaceResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(FACE_BLOCK, name); }
  void SetFaceResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(FACE_BLOCK, name, flag); }

  int GetNumberOfElementResultArrays()
    { return this->GetNumberOfObjectArrays(ELEM_BLOCK); }
  const char* GetElementResultArrayName(int index)
    { return this->GetObjectArrayName(ELEM_BLOCK, index); }
  int GetElementResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(ELEM_BLOCK, name); }
  void SetElementResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(ELEM_BLOCK, name, flag); }


  int GetNumberOfNodeMapArrays()
    { return this->GetNumberOfObjects(NODE_MAP); }
  const char* GetNodeMapArrayName(int index)
    { return this->GetObjectName(NODE_MAP, index); }
  int GetNodeMapArrayStatus(const char* name)
    { return this->GetObjectStatus(NODE_MAP, name); }
  void SetNodeMapArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(NODE_MAP, name, flag); }

  int GetNumberOfEdgeMapArrays()
    { return this->GetNumberOfObjects(EDGE_MAP); }
  const char* GetEdgeMapArrayName(int index)
    { return this->GetObjectName(EDGE_MAP, index); }
  int GetEdgeMapArrayStatus(const char* name)
    { return this->GetObjectStatus(EDGE_MAP, name); }
  void SetEdgeMapArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(EDGE_MAP, name, flag); }

  int GetNumberOfFaceMapArrays()
    { return this->GetNumberOfObjects(FACE_MAP); }
  const char* GetFaceMapArrayName(int index)
    { return this->GetObjectName(FACE_MAP, index); }
  int GetFaceMapArrayStatus(const char* name)
    { return this->GetObjectStatus(FACE_MAP, name); }
  void SetFaceMapArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(FACE_MAP, name, flag); }

  int GetNumberOfElementMapArrays()
    { return this->GetNumberOfObjects(ELEM_MAP); }
  const char* GetElementMapArrayName(int index)
    { return this->GetObjectName(ELEM_MAP, index); }
  int GetElementMapArrayStatus(const char* name)
    { return this->GetObjectStatus(ELEM_MAP, name); }
  void SetElementMapArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(ELEM_MAP, name, flag); }

  int GetNumberOfNodeSetArrays()
    { return this->GetNumberOfObjects(NODE_SET); }
  const char* GetNodeSetArrayName(int index)
    { return this->GetObjectName(NODE_SET, index); }
  int GetNodeSetArrayStatus(const char* name)
    { return this->GetObjectStatus(NODE_SET, name); }
  void SetNodeSetArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(NODE_SET, name, flag); }

  int GetNumberOfSideSetArrays()
    { return this->GetNumberOfObjects(SIDE_SET); }
  const char* GetSideSetArrayName(int index)
    { return this->GetObjectName(SIDE_SET, index); }
  int GetSideSetArrayStatus(const char* name)
    { return this->GetObjectStatus(SIDE_SET, name); }
  void SetSideSetArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(SIDE_SET, name, flag); }

  int GetNumberOfEdgeSetArrays()
    { return this->GetNumberOfObjects(EDGE_SET); }
  const char* GetEdgeSetArrayName(int index)
    { return this->GetObjectName(EDGE_SET, index); }
  int GetEdgeSetArrayStatus(const char* name)
    { return this->GetObjectStatus(EDGE_SET, name); }
  void SetEdgeSetArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(EDGE_SET, name, flag); }

  int GetNumberOfFaceSetArrays()
    { return this->GetNumberOfObjects(FACE_SET); }
  const char* GetFaceSetArrayName(int index)
    { return this->GetObjectName(FACE_SET, index); }
  int GetFaceSetArrayStatus(const char* name)
    { return this->GetObjectStatus(FACE_SET, name); }
  void SetFaceSetArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(FACE_SET, name, flag); }

  int GetNumberOfElementSetArrays()
    { return this->GetNumberOfObjects(ELEM_SET); }
  const char* GetElementSetArrayName(int index)
    { return this->GetObjectName(ELEM_SET, index); }
  int GetElementSetArrayStatus(const char* name)
    { return this->GetObjectStatus(ELEM_SET, name); }
  void SetElementSetArrayStatus(const char* name, int flag)
    { this->SetObjectStatus(ELEM_SET, name, flag); }


  int GetNumberOfNodeSetResultArrays()
    { return this->GetNumberOfObjectArrays(NODE_SET); }
  const char* GetNodeSetResultArrayName(int index)
    { return this->GetObjectArrayName(NODE_SET, index); }
  int GetNodeSetResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(NODE_SET, name); }
  void SetNodeSetResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(NODE_SET, name, flag); }

  int GetNumberOfSideSetResultArrays()
    { return this->GetNumberOfObjectArrays(SIDE_SET); }
  const char* GetSideSetResultArrayName(int index)
    { return this->GetObjectArrayName(SIDE_SET, index); }
  int GetSideSetResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(SIDE_SET, name); }
  void SetSideSetResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(SIDE_SET, name, flag); }

  int GetNumberOfEdgeSetResultArrays()
    { return this->GetNumberOfObjectArrays(EDGE_SET); }
  const char* GetEdgeSetResultArrayName(int index)
    { return this->GetObjectArrayName(EDGE_SET, index); }
  int GetEdgeSetResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(EDGE_SET, name); }
  void SetEdgeSetResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(EDGE_SET, name, flag); }

  int GetNumberOfFaceSetResultArrays()
    { return this->GetNumberOfObjectArrays(FACE_SET); }
  const char* GetFaceSetResultArrayName(int index)
    { return this->GetObjectArrayName(FACE_SET, index); }
  int GetFaceSetResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(FACE_SET, name); }
  void SetFaceSetResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(FACE_SET, name, flag); }

  int GetNumberOfElementSetResultArrays()
    { return this->GetNumberOfObjectArrays(ELEM_SET); }
  const char* GetElementSetResultArrayName(int index)
    { return this->GetObjectArrayName(ELEM_SET, index); }
  int GetElementSetResultArrayStatus(const char* name)
    { return this->GetObjectArrayStatus(ELEM_SET, name); }
  void SetElementSetResultArrayStatus(const char* name, int flag)
    { this->SetObjectArrayStatus(ELEM_SET, name, flag); }

  /**!\brief Fast path
    *
    * The following are set using the fast-path keys found in
    * vtkPExodusIIReader's input information.
    * Fast-path keys are meant to be used by an filter that
    * works with temporal data. Rather than re-executing the pipeline
    * for each timestep, since the exodus reader, as part of its API, contains
    * a faster way to read temporal data, algorithms may use these
    * keys to request temporal data.
    * See also: vtkExtractArraysOverTime.
    */
  //@{
  // Description:
  // Set the fast-path keys. All three must be set for the fast-path
  // option to work.
  // Possible argument values: "POINT","CELL","EDGE","FACE"
  void SetFastPathObjectType(const char *type);
  // Description:
  // Possible argument values: "INDEX","GLOBAL"
  // "GLOBAL" means the id refers to a global id
  // "INDEX" means the id refers to an index into the VTK array
  void SetFastPathIdType(const char *type);
  void SetFastPathObjectId(vtkIdType id);
  //@}

  // Description:
  // Reset the user-specified parameters and flush internal arrays
  // so that the reader state is just as it was after the reader was
  // instantiated.
  //
  // It doesn't make sense to let users reset only the internal state;
  // both the settings and the state are changed by this call.
  void Reset();

  // Description:
  // Reset the user-specified parameters to their default values.
  // The only settings not affected are the filename and/or pattern
  // because these have no default.
  //
  // Resetting the settings but not the state allows users to
  // keep the active cache but return to initial array selections, etc.
  void ResetSettings();

  // Description:
  // Clears out the cache entries.
  void ResetCache();

  // Description:
  // Set the size of the cache in MiB.
  void SetCacheSize(double CacheSize);

  // Description:
  // Get the size of the cache in MiB.
  double GetCacheSize();

  // Description:
  // Should the reader output only points used by elements in the output mesh,
  // or all the points. Outputting all the points is much faster since the
  // point array can be read straight from disk and the mesh connectivity need
  // not be altered. Squeezing the points down to the minimum set needed to
  // produce the output mesh is useful for glyphing and other point-based
  // operations. On large parallel datasets, loading all the points implies
  // loading all the points on all processes and performing subsequent
  // filtering on a much larger set.
  //
  // By default, SqueezePoints is true for backwards compatibility.
  void SetSqueezePoints(bool sp);
  bool GetSqueezePoints();


  // Description:
  // Re-reads time information from the exodus file and updates
  // TimeStepRange accordingly.
  virtual void UpdateTimeInformation();

  virtual void Dump();

  // Description:
  // SIL describes organization of/relationships between classifications
  // eg. blocks/materials/hierarchies.
  vtkGraph* GetSIL();

  // Description:
  // Every time the SIL is updated a this will return a different value.
  vtkGetMacro(SILUpdateStamp, int);

  // Description:
  // HACK: Used by vtkPExodusIIReader to tell is the reader produced a valid
  // fast path output.
  vtkGetMacro(ProducedFastPathOutput, bool);

protected:
  vtkExodusIIReader();
  ~vtkExodusIIReader();

  // Description:
  // Reset or create an ExodusModel and turn on arrays that must be present for the ExodusIIWriter
  virtual void NewExodusModel();

  // helper for finding IDs
  static int GetIDHelper ( const char *arrayName, vtkDataSet *data, int localID, int searchType );
  static int GetGlobalID( const char *arrayName, vtkDataSet *data, int localID, int searchType );

  virtual void SetMetadata( vtkExodusIIReaderPrivate* );
  vtkGetObjectMacro(Metadata,vtkExodusIIReaderPrivate);

  // Description:
  // Returns true if XMLFileName has already been set. Otherwise, look for the XML
  // metadata file in the same directory as the data file(s) using the following
  // possible file names:
  //     DATA_FILE_NAME.xml
  //     DATA_FILE_NAME.dart
  //     artifact.dta
  //  Return true if found, false otherwise
  bool FindXMLFile();

  // Time query function. Called by ExecuteInformation().
  // Fills the TimestepValues array.
  void GetAllTimes(vtkInformationVector*);

  // Description:
  // Populates the TIME_STEPS and TIME_RANGE keys based on file metadata.
  void AdvertiseTimeSteps( vtkInformation* outputInfo );

  virtual void SetExodusModel( vtkExodusModel* em );

  int ProcessRequest( vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation( vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData( vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  //int RequestDataOverTime( vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Parameters for controlling what is read in.
  char* FileName;
  char* XMLFileName;
  int TimeStep;
  int TimeStepRange[2];
  vtkTimeStamp FileNameMTime;
  vtkTimeStamp XMLFileNameMTime;

  // Information specific for exodus files.

  //1=display Block names
  //2=display Part names
  //3=display Material names
  int DisplayType;

  // Metadata containing a description of the currently open file.
  vtkExodusIIReaderPrivate* Metadata;

  vtkExodusModel *ExodusModel;
  int PackExodusModelOntoOutput;
  int ExodusModelMetadata;

  int SILUpdateStamp;
  bool ProducedFastPathOutput;
private:
  vtkExodusIIReader(const vtkExodusIIReader&); // Not implemented
  void operator=(const vtkExodusIIReader&); // Not implemented

  void AddDisplacements(vtkUnstructuredGrid* output);
};

#endif
