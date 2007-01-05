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

#include "vtkUnstructuredGridAlgorithm.h"

class vtkIntArray;
class vtkFloatArray;
class vtkDataArray;
class vtkDataSet;
class vtkPoints;
class vtkExodusIIReaderPrivate;
class vtkExodusModel;
class vtkExodusIIXMLParser;
class vtkExodusIICache;

#include "vtkDSPFilterGroup.h" //for USE_EXO_DSP_FILTERS


class VTK_HYBRID_EXPORT vtkExodusIIReader : public vtkUnstructuredGridAlgorithm 
{
public:
  static vtkExodusIIReader *New();
  vtkTypeRevisionMacro(vtkExodusIIReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

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
  // edge block ID. Similarly, ths is the face block ID for cells
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


  // Description:
  // Extra cell data array that can be generated.  By default, this array
  // is off.  The value of the array is the integer global id of the cell.
  // The name of the array is returned by GetGlobalElementIdArrayName()
  // ***NOTE*** No more "unique" global ID. Instead we have an arbitrary number of maps.
//BTX
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
    GLOBAL_OBJECT_ID = 87,     //!< assembled object id (old BlockId) array
    GLOBAL_ELEMENT_ID = 86,    //!< assembled, zero-padded element id array
    GLOBAL_NODE_ID = 85,       //!< assembled, zero-padded nodal id array
    ELEMENT_ID = 84,           //!< element id map (old-style elem_num_map or first new-style elem map) array
    NODE_ID = 83,              //!< nodal id map (old-style node_num_map or first new-style node map) array
    NODAL_SQUEEZEMAP = 82,     //!< the integer map use to "squeeze" coordinates and nodal arrays/maps
    ELEM_BLOCK_ATTRIB = 81,    //!< an element block attribute array (time-constant scalar per element)
    FACE_BLOCK_ATTRIB = 80,    //!< a face block attribute array (time-constant scalar per element)
    EDGE_BLOCK_ATTRIB = 79     //!< an edge block attribute array (time-constant scalar per element)
  };
//ETX
  static const char* GetGlobalElementIdArrayName() { return "GlobalElementId"; }
  static const char* GetPedigreeElementIdArrayName() { return "pedigreeElementId"; }
  static int GetGlobalElementID( vtkDataSet *data, int localID );
  static int GetGlobalElementID ( vtkDataSet *data, int localID, 
      int searchType );

  static const char* GetGlobalFaceIdArrayName() { return "GlobalFaceId"; }
  static const char* GetPedigreeFaceIdArrayName() { return "pedigreeFaceId"; }
  static int GetGlobalFaceID( vtkDataSet *data, int localID );
  static int GetGlobalFaceID ( vtkDataSet *data, int localID, 
      int searchType );

  static const char* GetGlobalEdgeIdArrayName() { return "GlobalEdgeId"; }
  static const char* GetPedigreeEdgeIdArrayName() { return "pedigreeEdgeId"; }
  static int GetGlobalEdgeID( vtkDataSet *data, int localID );
  static int GetGlobalEdgeID ( vtkDataSet *data, int localID, 
      int searchType );

  // Description:
  // Extra point data array that can be generated.  By default, this array
  // is ON.  The value of the array is the integer id of the node.
  // The id is relative to the entire data set.
  // The name of the array is returned by GlobalNodeIdArrayName().
  static const char* GetGlobalNodeIdArrayName() { return "GlobalNodeId"; }  
  static const char* GetPedigreeNodeIdArrayName() { return "pedigreeNodeId"; }  
  static int GetGlobalNodeID( vtkDataSet *data, int localID );
  static int GetGlobalNodeID( vtkDataSet *data, int localID, 
      int searchType );

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

  int GetNumberOfObjects( int objectType );
  int GetNumberOfNodeSets() { return this->GetNumberOfObjects( NODE_SET ); }
  int GetNumberOfEdgeSets() { return this->GetNumberOfObjects( EDGE_SET ); }
  int GetNumberOfFaceSets() { return this->GetNumberOfObjects( FACE_SET ); }
  int GetNumberOfSideSets() { return this->GetNumberOfObjects( SIDE_SET ); }
  int GetNumberOfElementSets() { return this->GetNumberOfObjects( ELEM_SET ); }
  int GetNumberOfEdgeBlocks() { return this->GetNumberOfObjects( EDGE_BLOCK ); }
  int GetNumberOfFaceBlocks() { return this->GetNumberOfObjects( FACE_BLOCK ); }
  int GetNumberOfElementBlocks() { return this->GetNumberOfObjects( ELEM_BLOCK ); }
  int GetNumberOfNodeMaps() { return this->GetNumberOfObjects( NODE_MAP ); }
  int GetNumberOfEdgeMaps() { return this->GetNumberOfObjects( EDGE_MAP ); }
  int GetNumberOfFaceMaps() { return this->GetNumberOfObjects( FACE_MAP ); }
  int GetNumberOfElementMaps() { return this->GetNumberOfObjects( ELEM_MAP ); }

  int GetNumberOfNodes();

  int GetNumberOfEntriesInObject( int objectType, int objectIndex );
  int GetNumberOfEdgesInBlock(int blockIdx) { return this->GetNumberOfEntriesInObject( EDGE_BLOCK, blockIdx ); }
  int GetNumberOfFacesInBlock(int blockIdx) { return this->GetNumberOfEntriesInObject( FACE_BLOCK, blockIdx ); }
  int GetNumberOfElementsInBlock(int blockIdx) { return this->GetNumberOfEntriesInObject( ELEM_BLOCK, blockIdx ); }
  int GetNumberOfNodesInSet( int setIdx ) { return this->GetNumberOfEntriesInObject( NODE_SET, setIdx ); }
  int GetNumberOfEdgesInSet( int setIdx ) { return this->GetNumberOfEntriesInObject( EDGE_SET, setIdx ); }
  int GetNumberOfFacesInSet( int setIdx ) { return this->GetNumberOfEntriesInObject( FACE_SET, setIdx ); }
  int GetNumberOfSidesInSet( int setIdx ) { return this->GetNumberOfEntriesInObject( SIDE_SET, setIdx ); }
  int GetNumberOfElementsInSet( int setIdx ) { return this->GetNumberOfEntriesInObject( ELEM_SET, setIdx ); }

  int GetObjectId( int objectType, int objectIndex );
  int GetEdgeBlockId(int blockIdx) { return this->GetObjectId( EDGE_BLOCK, blockIdx ); }
  int GetFaceBlockId(int blockIdx) { return this->GetObjectId( FACE_BLOCK, blockIdx ); }
  int GetElementBlockId(int blockIdx) { return this->GetObjectId( ELEM_BLOCK, blockIdx ); }
  int GetNodeSetId(int setIdx) { return this->GetObjectId( NODE_SET, setIdx ); }
  int GetEdgeSetId(int setIdx) { return this->GetObjectId( EDGE_SET, setIdx ); }
  int GetFaceSetId(int setIdx) { return this->GetObjectId( FACE_SET, setIdx ); }
  int GetSideSetId(int setIdx) { return this->GetObjectId( SIDE_SET, setIdx ); }
  int GetElementSetId(int setIdx) { return this->GetObjectId( ELEM_SET, setIdx ); }
  int GetNodeMapId(int mapIdx) { return this->GetObjectId( NODE_MAP, mapIdx ); }
  int GetEdgeMapId(int mapIdx) { return this->GetObjectId( EDGE_MAP, mapIdx ); }
  int GetFaceMapId(int mapIdx) { return this->GetObjectId( FACE_MAP, mapIdx ); }
  int GetElementMapId(int mapIdx) { return this->GetObjectId( ELEM_MAP, mapIdx ); }

  const char* GetObjectName( int objectType, int objectIndex );
  int GetObjectIndex( int objectType, const char* objectName );
  int GetObjectStatus( int objectType, int objectIndex );
  int GetObjectStatus( int objectType, const char* objectName )
    { return this->GetObjectStatus( objectType, this->GetObjectIndex( objectType, objectName ) ); }
  void SetObjectStatus( int objectType, int objectIndex, int status );
  void SetObjectStatus( int objectType, const char* objectName, int status ) 
    { this->SetObjectStatus( objectType, this->GetObjectIndex( objectType, objectName ), status ); }

  int GetNumberOfObjectArrays( int objectType );
  const char* GetObjectArrayName( int objectType, int arrayIndex );
  int GetObjectArrayIndex( int objectType, const char* arrayName );
  int GetNumberOfObjectArrayComponents( int objectType, int arrayIndex );
  int GetObjectArrayStatus( int objectType, int arrayIndex );
  int GetObjectArrayStatus( int objectType, const char* arrayName )
    { return this->GetObjectArrayStatus( objectType, this->GetObjectArrayIndex( objectType, arrayName ) ); }
  void SetObjectArrayStatus( int objectType, int arrayIndex, int status );
  void SetObjectArrayStatus( int objectType, const char* arrayName, int status ) 
    { this->SetObjectArrayStatus( objectType, this->GetObjectArrayIndex( objectType, arrayName ), status ); }

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

  // Descriptions:
  // By default arrays are not loaded.  These methods allow the user to select
  // which arrays they want to load.  You can get information about the arrays
  // by first caling UpdateInformation, and using GetPointArrayName ...
  // (Developer Note) This meta data is all accessed through vtkExodusMetadata
  int GetNumberOfPointResultArrays() { return this->GetNumberOfObjectArrays( NODAL ); }
  const char *GetPointResultArrayName(int index) { return this->GetObjectArrayName( NODAL, index ); }
  int GetPointResultArrayID( const char* name ) { return this->GetObjectArrayIndex( NODAL, name ); }
  int GetPointResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( NODAL, index ); }
  void SetPointResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( NODAL, index, flag ); }
  void SetPointResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( NODAL, name, flag ); }
  int GetPointResultArrayStatus(int index) { return this->GetObjectArrayStatus( NODAL, index ); }
  int GetPointResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( NODAL, name ); }
  virtual int GetTotalNumberOfNodes();

  int GetNumberOfEdgeResultArrays() { return this->GetNumberOfObjectArrays( EDGE_BLOCK ); }
  const char* GetEdgeResultArrayName(int index) { return this->GetObjectArrayName( EDGE_BLOCK, index ); }
  int GetEdgeResultArrayID( const char *name ) { return this->GetObjectArrayIndex( EDGE_BLOCK, name ); }
  int GetEdgeResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( EDGE_BLOCK, index ); }
  void SetEdgeResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( EDGE_BLOCK, index, flag ); }
  void SetEdgeResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( EDGE_BLOCK, name, flag ); }
  int GetEdgeResultArrayStatus(int index) { return this->GetObjectArrayStatus( EDGE_BLOCK, index ); }
  int GetEdgeResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( EDGE_BLOCK, name ); }
  virtual int GetTotalNumberOfEdges();

  int GetNumberOfFaceResultArrays() { return this->GetNumberOfObjectArrays( FACE_BLOCK ); }
  const char* GetFaceResultArrayName(int index) { return this->GetObjectArrayName( FACE_BLOCK, index ); }
  int GetFaceResultArrayID( const char *name ) { return this->GetObjectArrayIndex( FACE_BLOCK, name ); }
  int GetFaceResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( FACE_BLOCK, index ); }
  void SetFaceResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( FACE_BLOCK, index, flag ); }
  void SetFaceResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( FACE_BLOCK, name, flag ); }
  int GetFaceResultArrayStatus(int index) { return this->GetObjectArrayStatus( FACE_BLOCK, index ); }
  int GetFaceResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( FACE_BLOCK, name ); }
  virtual int GetTotalNumberOfFaces();

  int GetNumberOfElementResultArrays() { return this->GetNumberOfObjectArrays( ELEM_BLOCK ); }
  const char* GetElementResultArrayName(int index) { return this->GetObjectArrayName( ELEM_BLOCK, index ); }
  int GetElementResultArrayID( const char *name ) { return this->GetObjectArrayIndex( ELEM_BLOCK, name ); }
  int GetElementResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( ELEM_BLOCK, index ); }
  void SetElementResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( ELEM_BLOCK, index, flag ); }
  void SetElementResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( ELEM_BLOCK, name, flag ); }
  int GetElementResultArrayStatus(int index) { return this->GetObjectArrayStatus( ELEM_BLOCK, index ); }
  int GetElementResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( ELEM_BLOCK, name ); }
  virtual int GetTotalNumberOfElements();

  // Descriptions:
  // By default no edge blocks are loaded. These methods allow the user to select
  // which blocks they want to load.  You can get information about the blocks
  // by first caling UpdateInformation, and using GetEdgeBlockArrayName ... 
  int GetNumberOfEdgeBlockArrays() { return this->GetNumberOfEdgeBlocks(); }
  const char* GetEdgeBlockArrayName(int index) { return this->GetObjectName( EDGE_BLOCK, index ); }
  int GetEdgeBlockArrayID( const char *name ) { return this->GetObjectIndex( EDGE_BLOCK, name ); }
  void SetEdgeBlockArrayStatus(int index, int flag) { this->SetObjectStatus( EDGE_BLOCK, index, flag ); }
  void SetEdgeBlockArrayStatus(const char* name, int flag) { this->SetObjectStatus( EDGE_BLOCK, name, flag ); }
  int GetEdgeBlockArrayStatus(int index) { return this->GetObjectStatus( EDGE_BLOCK, index ); }
  int GetEdgeBlockArrayStatus(const char* name ) { return this->GetObjectStatus( EDGE_BLOCK, name ); }

  // Descriptions:
  // By default all face blocks are loaded. These methods allow the user to select
  // which blocks they want to load.  You can get information about the blocks
  // by first caling UpdateInformation, and using GetFaceBlockArrayName ... 
  int GetNumberOfFaceBlockArrays() { return this->GetNumberOfFaceBlocks(); }
  const char* GetFaceBlockArrayName(int index) { return this->GetObjectName( FACE_BLOCK, index ); }
  int GetFaceBlockArrayID( const char *name ) { return this->GetObjectIndex( FACE_BLOCK, name ); }
  void SetFaceBlockArrayStatus(int index, int flag) { this->SetObjectStatus( FACE_BLOCK, index, flag ); }
  void SetFaceBlockArrayStatus(const char* name, int flag) { this->SetObjectStatus( FACE_BLOCK, name, flag ); }
  int GetFaceBlockArrayStatus(int index) { return this->GetObjectStatus( FACE_BLOCK, index ); }
  int GetFaceBlockArrayStatus(const char* name) { return this->GetObjectStatus( FACE_BLOCK, name ); }

  // Descriptions:
  // By default all element blocks are loaded. These methods allow the user to select
  // which blocks they want to load.  You can get information about the blocks
  // by first caling UpdateInformation, and using GetElementBlockArrayName ... 
  int GetNumberOfElementBlockArrays() { return this->GetNumberOfElementBlocks(); }
  const char* GetElementBlockArrayName(int index) { return this->GetObjectName( ELEM_BLOCK, index ); }
  int GetElementBlockArrayID( const char* name ) { return this->GetObjectIndex( ELEM_BLOCK, name ); }
  void SetElementBlockArrayStatus(int index, int flag) { this->SetObjectStatus( ELEM_BLOCK, index, flag ); }
  void SetElementBlockArrayStatus(const char* name, int flag) { this->SetObjectStatus( ELEM_BLOCK, name, flag ); }
  int GetElementBlockArrayStatus(int index) { return this->GetObjectStatus( ELEM_BLOCK, index ); }
  int GetElementBlockArrayStatus(const char* name) { return this->GetObjectStatus( ELEM_BLOCK, name ); }


  // Description:
  // By default Node/Side sets are not loaded, These methods allow the user to
  // select which Node/Side sets they want to load. NumberOfNodeSets and
  // NumberOfSideSets (set by vtk macros) are stored in vtkExodusIIReader 
  // but other Node/Side set metadata are stored in vtkExodusMetaData
  // Note: GetNumberOfNodeSetArrays and GetNumberOfSideSetArrays are
  // just syntatic sugar for paraview server xml
  int GetNumberOfNodeSetArrays() { return this->GetNumberOfNodeSets(); }
  int GetNodeSetArrayStatus(int index) { return this->GetObjectStatus( NODE_SET, index ); }
  int GetNodeSetArrayStatus(const char* name) { return this->GetObjectStatus( NODE_SET, name ); }
  void SetNodeSetArrayStatus(int index, int flag) { this->SetObjectStatus( NODE_SET, index, flag ); }
  void SetNodeSetArrayStatus(const char* name, int flag) { this->SetObjectStatus( NODE_SET, name, flag ); }
  const char* GetNodeSetArrayName(int index) { return this->GetObjectName( NODE_SET, index ); }
  int GetNodeSetArrayID( const char* name ) { return this->GetObjectIndex( NODE_SET, name ); }
  
  int GetNumberOfEdgeSetArrays() { return this->GetNumberOfEdgeSets(); }
  int GetEdgeSetArrayStatus(int index) { return this->GetObjectStatus( EDGE_SET, index ); }
  int GetEdgeSetArrayStatus(const char* name) { return this->GetObjectStatus( EDGE_SET, name ); }
  void SetEdgeSetArrayStatus(int index, int flag) { this->SetObjectStatus( EDGE_SET, index, flag ); }
  void SetEdgeSetArrayStatus(const char* name, int flag) { this->SetObjectStatus( EDGE_SET, name, flag ); }
  const char* GetEdgeSetArrayName(int index) { return this->GetObjectName( EDGE_SET, index ); }
  int GetEdgeSetArrayID( const char* name ) { return this->GetObjectIndex( EDGE_SET, name ); }
  
  int GetNumberOfFaceSetArrays() { return this->GetNumberOfFaceSets(); }
  int GetFaceSetArrayStatus(int index) { return this->GetObjectStatus( FACE_SET, index ); }
  int GetFaceSetArrayStatus(const char* name) { return this->GetObjectStatus( FACE_SET, name ); }
  void SetFaceSetArrayStatus(int index, int flag) { this->SetObjectStatus( FACE_SET, index, flag ); }
  void SetFaceSetArrayStatus(const char* name, int flag) { this->SetObjectStatus( FACE_SET, name, flag ); }
  const char* GetFaceSetArrayName(int index) { return this->GetObjectName( FACE_SET, index ); }
  int GetFaceSetArrayID( const char* name ) { return this->GetObjectIndex( FACE_SET, name ); }
  
  int GetNumberOfSideSetArrays() { return this->GetNumberOfSideSets(); }
  int GetSideSetArrayStatus(int index) { return this->GetObjectStatus( SIDE_SET, index ); }
  int GetSideSetArrayStatus(const char* name) { return this->GetObjectStatus( SIDE_SET, name ); }
  void SetSideSetArrayStatus(int index, int flag) { this->SetObjectStatus( SIDE_SET, index, flag ); }
  void SetSideSetArrayStatus(const char* name, int flag) { this->SetObjectStatus( SIDE_SET, name, flag ); }
  const char* GetSideSetArrayName(int index) { return this->GetObjectName( SIDE_SET, index ); }
  int GetSideSetArrayID( const char* name ) { return this->GetObjectIndex( SIDE_SET, name ); }
  
  int GetNumberOfElementSetArrays() { return this->GetNumberOfElementSets(); }
  int GetElementSetArrayStatus(int index) { return this->GetObjectStatus( ELEM_SET, index ); }
  int GetElementSetArrayStatus(const char* name) { return this->GetObjectStatus( ELEM_SET, name ); }
  void SetElementSetArrayStatus(int index, int flag) { this->SetObjectStatus( ELEM_SET, index, flag ); }
  void SetElementSetArrayStatus(const char* name, int flag) { this->SetObjectStatus( ELEM_SET, name, flag ); }
  const char* GetElementSetArrayName(int index) { return this->GetObjectName( ELEM_SET, index ); }
  int GetElementSetArrayID( const char* name ) { return this->GetObjectIndex( ELEM_SET, name ); }
  
  int GetNumberOfNodeSetResultArrays() { return this->GetNumberOfObjectArrays( NODE_SET ); }
  const char *GetNodeSetResultArrayName(int index) { return this->GetObjectArrayName( NODE_SET, index ); }
  int GetNodeSetResultArrayID( const char* name ) { return this->GetObjectArrayIndex( NODE_SET, name ); }
  int GetNodeSetResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( NODE_SET, index ); }
  void SetNodeSetResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( NODE_SET, index, flag ); }
  void SetNodeSetResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( NODE_SET, name, flag ); }
  int GetNodeSetResultArrayStatus(int index) { return this->GetObjectArrayStatus( NODE_SET, index ); }
  int GetNodeSetResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( NODE_SET, name ); }

  int GetNumberOfEdgeSetResultArrays() { return this->GetNumberOfObjectArrays( EDGE_SET ); }
  const char* GetEdgeSetResultArrayName(int index) { return this->GetObjectArrayName( EDGE_SET, index ); }
  int GetEdgeSetResultArrayID( const char *name ) { return this->GetObjectArrayIndex( EDGE_SET, name ); }
  int GetEdgeSetResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( EDGE_SET, index ); }
  void SetEdgeSetResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( EDGE_SET, index, flag ); }
  void SetEdgeSetResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( EDGE_SET, name, flag ); }
  int GetEdgeSetResultArrayStatus(int index) { return this->GetObjectArrayStatus( EDGE_SET, index ); }
  int GetEdgeSetResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( EDGE_SET, name ); }

  int GetNumberOfFaceSetResultArrays() { return this->GetNumberOfObjectArrays( FACE_SET ); }
  const char* GetFaceSetResultArrayName(int index) { return this->GetObjectArrayName( FACE_SET, index ); }
  int GetFaceSetResultArrayID( const char *name ) { return this->GetObjectArrayIndex( FACE_SET, name ); }
  int GetFaceSetResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( FACE_SET, index ); }
  void SetFaceSetResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( FACE_SET, index, flag ); }
  void SetFaceSetResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( FACE_SET, name, flag ); }
  int GetFaceSetResultArrayStatus(int index) { return this->GetObjectArrayStatus( FACE_SET, index ); }
  int GetFaceSetResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( FACE_SET, name ); }

  int GetNumberOfSideSetResultArrays() { return this->GetNumberOfObjectArrays( SIDE_SET ); }
  const char* GetSideSetResultArrayName(int index) { return this->GetObjectArrayName( SIDE_SET, index ); }
  int GetSideSetResultArrayID( const char *name ) { return this->GetObjectArrayIndex( SIDE_SET, name ); }
  int GetSideSetResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( SIDE_SET, index ); }
  void SetSideSetResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( SIDE_SET, index, flag ); }
  void SetSideSetResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( SIDE_SET, name, flag ); }
  int GetSideSetResultArrayStatus(int index) { return this->GetObjectArrayStatus( SIDE_SET, index ); }
  int GetSideSetResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( SIDE_SET, name ); }

  int GetNumberOfElementSetResultArrays() { return this->GetNumberOfObjectArrays( ELEM_SET ); }
  const char* GetElementSetResultArrayName(int index) { return this->GetObjectArrayName( ELEM_SET, index ); }
  int GetElementSetResultArrayID( const char *name ) { return this->GetObjectArrayIndex( ELEM_SET, name ); }
  int GetElementSetResultArrayNumberOfComponents(int index) { return this->GetNumberOfObjectArrayComponents( ELEM_SET, index ); }
  void SetElementSetResultArrayStatus(int index, int flag) { this->SetObjectArrayStatus( ELEM_SET, index, flag ); }
  void SetElementSetResultArrayStatus(const char* name, int flag) { this->SetObjectArrayStatus( ELEM_SET, name, flag ); }
  int GetElementSetResultArrayStatus(int index) { return this->GetObjectArrayStatus( ELEM_SET, index ); }
  int GetElementSetResultArrayStatus(const char* name) { return this->GetObjectArrayStatus( ELEM_SET, name ); }

  int GetNumberOfNodeMapArrays() { return this->GetNumberOfNodeMaps(); }
  int GetNodeMapArrayStatus(int index) { return this->GetObjectStatus( NODE_MAP, index ); }
  int GetNodeMapArrayStatus(const char* name) { return this->GetObjectStatus( NODE_MAP, name ); }
  void SetNodeMapArrayStatus(int index, int flag) { this->SetObjectStatus( NODE_MAP, index, flag ); }
  void SetNodeMapArrayStatus(const char* name, int flag) { this->SetObjectStatus( NODE_MAP, name, flag ); }
  const char* GetNodeMapArrayName(int index) { return this->GetObjectName( NODE_MAP, index ); }
  int GetNodeMapArrayID( const char* name ) { return this->GetObjectIndex( NODE_MAP, name ); }
  
  int GetNumberOfEdgeMapArrays() { return this->GetNumberOfEdgeMaps(); }
  int GetEdgeMapArrayStatus(int index) { return this->GetObjectStatus( EDGE_MAP, index ); }
  int GetEdgeMapArrayStatus(const char* name) { return this->GetObjectStatus( EDGE_MAP, name ); }
  void SetEdgeMapArrayStatus(int index, int flag) { this->SetObjectStatus( EDGE_MAP, index, flag ); }
  void SetEdgeMapArrayStatus(const char* name, int flag) { this->SetObjectStatus( EDGE_MAP, name, flag ); }
  const char* GetEdgeMapArrayName(int index) { return this->GetObjectName( EDGE_MAP, index ); }
  int GetEdgeMapArrayID( const char* name ) { return this->GetObjectIndex( EDGE_MAP, name ); }
  
  int GetNumberOfFaceMapArrays() { return this->GetNumberOfFaceMaps(); }
  int GetFaceMapArrayStatus(int index) { return this->GetObjectStatus( FACE_MAP, index ); }
  int GetFaceMapArrayStatus(const char* name) { return this->GetObjectStatus( FACE_MAP, name ); }
  void SetFaceMapArrayStatus(int index, int flag) { this->SetObjectStatus( FACE_MAP, index, flag ); }
  void SetFaceMapArrayStatus(const char* name, int flag) { this->SetObjectStatus( FACE_MAP, name, flag ); }
  const char* GetFaceMapArrayName(int index) { return this->GetObjectName( FACE_MAP, index ); }
  int GetFaceMapArrayID( const char* name ) { return this->GetObjectIndex( FACE_MAP, name ); }
  
  int GetNumberOfElementMapArrays() { return this->GetNumberOfElementMaps(); }
  int GetElementMapArrayStatus(int index) { return this->GetObjectStatus( ELEM_MAP, index ); }
  int GetElementMapArrayStatus(const char* name) { return this->GetObjectStatus( ELEM_MAP, name ); }
  void SetElementMapArrayStatus(int index, int flag) { this->SetObjectStatus( ELEM_MAP, index, flag ); }
  void SetElementMapArrayStatus(const char* name, int flag) { this->SetObjectStatus( ELEM_MAP, name, flag ); }
  const char* GetElementMapArrayName(int index) { return this->GetObjectName( ELEM_MAP, index ); }
  int GetElementMapArrayID( const char* name ) { return this->GetObjectIndex( ELEM_MAP, name ); }
  
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
  //   file is read in to a vtkUnstructuredGrid.  Turn this option ON 
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

  void SetAllEdgeBlockArrayStatus( int status )        { this->SetAllArrayStatus( EDGE_BLOCK_CONN, status ); }
  void SetAllFaceBlockArrayStatus( int status )        { this->SetAllArrayStatus( FACE_BLOCK_CONN, status ); }
  void SetAllElementBlockArrayStatus( int status )     { this->SetAllArrayStatus( ELEM_BLOCK_ELEM_CONN, status ); }

  void SetAllNodeSetArrayStatus( int status )          { this->SetAllArrayStatus( NODE_SET_CONN, status ); }
  void SetAllEdgeSetArrayStatus( int status )          { this->SetAllArrayStatus( EDGE_SET_CONN, status ); }
  void SetAllFaceSetArrayStatus( int status )          { this->SetAllArrayStatus( FACE_SET_CONN, status ); }
  void SetAllSideSetArrayStatus( int status )          { this->SetAllArrayStatus( SIDE_SET_CONN, status ); }
  void SetAllElementSetArrayStatus( int status )       { this->SetAllArrayStatus( ELEM_SET_CONN, status ); }

  void SetAllPointResultArrayStatus( int status )      { this->SetAllArrayStatus( NODAL, status ); }
  void SetAllEdgeResultArrayStatus( int status )       { this->SetAllArrayStatus( EDGE_BLOCK, status ); }
  void SetAllFaceResultArrayStatus( int status )       { this->SetAllArrayStatus( FACE_BLOCK, status ); }
  void SetAllElementResultArrayStatus( int status )    { this->SetAllArrayStatus( ELEM_BLOCK, status ); }

  void SetAllNodeSetResultArrayStatus( int status )    { this->SetAllArrayStatus( NODE_SET, status ); }
  void SetAllEdgeSetResultArrayStatus( int status )    { this->SetAllArrayStatus( EDGE_SET, status ); }
  void SetAllFaceSetResultArrayStatus( int status )    { this->SetAllArrayStatus( FACE_SET, status ); }
  void SetAllSideSetResultArrayStatus( int status )    { this->SetAllArrayStatus( SIDE_SET, status ); }
  void SetAllElementSetResultArrayStatus( int status ) { this->SetAllArrayStatus( ELEM_SET, status ); }

  void SetAllNodeMapArrayStatus( int status )          { this->SetAllArrayStatus( NODE_MAP, status ); }
  void SetAllEdgeMapArrayStatus( int status )          { this->SetAllArrayStatus( EDGE_MAP, status ); }
  void SetAllFaceMapArrayStatus( int status )          { this->SetAllArrayStatus( FACE_MAP, status ); }
  void SetAllElementMapArrayStatus( int status )       { this->SetAllArrayStatus( ELEM_MAP, status ); }

  void SetAllHierarchyArrayStatus( int status )        { this->SetAllArrayStatus( HIERARCHY, status ); }
  void SetAllAssemblyArrayStatus( int status )         { this->SetAllArrayStatus( ASSEMBLY, status ); }
  void SetAllMaterialArrayStatus( int status )         { this->SetAllArrayStatus( MATERIAL, status ); }
  void SetAllPartArrayStatus( int status )             { this->SetAllArrayStatus( PART, status ); }

  // Helper functions
  //static int StringsEqual(const char* s1, char* s2);
  //static void StringUppercase(const char* str, char* upperstr);
  //static char *StrDupWithNew(const char *s);

  // time series query functions
  int GetTimeSeriesData( int ID, const char *vName, const char *vType, 
                         vtkFloatArray *result );
  

  //begin USE_EXO_DSP_FILTERS
  int GetNumberOfVariableArrays();
  const char* GetVariableArrayName( int aWhich );
  void EnableDSPFiltering(); 
  void AddFilter( vtkDSPFilterDefinition* aFilter );
  void StartAddingFilter();
  void AddFilterInputVar( char* name );
  void AddFilterOutputVar( char* name );
  void AddFilterNumeratorWeight( double weight );
  void AddFilterForwardNumeratorWeight( double weight );
  void AddFilterDenominatorWeight( double weight );
  void FinishAddingFilter();
  void RemoveFilter( char* outputVariableName );
  void GetDSPOutputArrays( int exoid, int timeStep, vtkUnstructuredGrid* output );

  vtkDSPFilterDefinition* AddingFilter;
  int DSPFilteringIsEnabled;
  vtkDSPFilterGroup** DSPFilters;
  //end USE_EXO_DSP_FILTERS

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

  virtual void SetParser( vtkExodusIIXMLParser* );
  vtkGetObjectMacro(Parser,vtkExodusIIXMLParser);

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
  // Parser that understands the xml part and material file
  vtkExodusIIXMLParser* Parser;

  // Time query function. Called by ExecuteInformation().
  // Fills the TimestepValues array.
  void GetAllTimes(vtkInformationVector*); 

  vtkExodusModel *ExodusModel;
  int PackExodusModelOntoOutput;
  int ExodusModelMetadata;

  int RequestInformation( vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData( vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual void SetExodusModel( vtkExodusModel* em );

private:
  vtkExodusIIReader(const vtkExodusIIReader&); // Not implemented
  void operator=(const vtkExodusIIReader&); // Not implemented

  void AddDisplacements(vtkUnstructuredGrid* output);
};

#endif
