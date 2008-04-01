#ifndef __vtkExodusIIReaderPrivate_h
#define __vtkExodusIIReaderPrivate_h

// Do not include this file directly. It is only for use
// from inside the ExodusII reader and its descendants.

#include "vtkToolkits.h" // make sure VTK_USE_PARALLEL is properly set
#include "vtkExodusIICache.h"
#ifdef VTK_USE_PARALLEL
#  include "vtkMultiProcessController.h"
#else // VTK_USE_PARALLEL
class vtkMultiProcessController;
#endif // VTK_USE_PARALLEL

#include "vtksys/RegularExpression.hxx"

#include <vtkstd/map>
#include <vtkstd/vector>

#include "exodusII.h"

class vtkExodusIIXMLParser;

/** This class holds metadata for an Exodus file.
  *
  */
class vtkExodusIIReaderPrivate : public vtkObject
{
public:
  static vtkExodusIIReaderPrivate* New();
  void PrintData( ostream& os, vtkIndent indent );
  vtkTypeRevisionMacro(vtkExodusIIReaderPrivate,vtkObject);
  //virtual void Modified();

  /// Open an ExodusII file for reading. Returns 0 on success.
  int OpenFile( const char* filename );

  /// Close any ExodusII file currently open for reading. Returns 0 on success.
  int CloseFile();

  /// Get metadata for an open file with handle \a exoid.
  int RequestInformation();

  /// Send metadata to other processes in a parallel job.
  void Broadcast( vtkMultiProcessController* controller );

  /// Receive metadata from the rank 0 process in a parallel job.
  void Receive( vtkMultiProcessController* controller );

  /// Read requested data and store in unstructured grid.
  int RequestData( vtkIdType timeStep, vtkMultiBlockDataSet* output );

  // Description:
  // Prepare a data set with the proper structure and arrays but no cells.
  // This is used by the parallel reader when a process has no files assigned to it.
  int SetUpEmptyGrid( vtkMultiBlockDataSet* output );

  /** Reset the class so that another file may be read.
    * This does not change any user-specified parameters, such as
    * which <b>generated</b> arrays should be present, whether there are
    * mode shapes or time steps, etc.
    * Note that which arrays should be loaded is a more delicate
    * issue; if you set these after RequestInformation has been called,
    * these will not be saved.
    * Any settings you make <b>before</b> RequestInformation is called
    * will be saved because they are stored in InitialArrayInfo and 
    * InitialObjectInfo.
    */
  void Reset();

  /** Return user-specified variables to their default values.
    * Calling ResetSettings() and then Reset() will return the class
    * to a state just like it was after New() was called.
    */
  void ResetSettings();

  /// Clears out any data in the cache and restores it to its initial state.
  void ResetCache();

  /** Return the number of time steps in the open file.
    * You must have called RequestInformation() before 
    * invoking this member function.
    */
  int GetNumberOfTimeSteps() { return (int) this->Times.size(); }

  /// Return the current time step
  vtkGetMacro(TimeStep,int);

  /// Set the current time step for subsequent calls to RequestData().
  vtkSetMacro(TimeStep,int);

  /// Return whether subsequent RequestData() calls will produce the minimal 
  /// point set required to represent the output.
  vtkGetMacro(SqueezePoints,int);

  /// Set whether subsequent RequestData() calls will produce the minimal 
  /// point set required to represent the output.
  void SetSqueezePoints( int sp );

  /// Convenience routines that for producing (or not) the minimal point set 
  /// required to represent the output.
  vtkBooleanMacro(SqueezePoints,int);

  /// Return the number of nodes in the output (depends on SqueezePoints)
  int GetNumberOfNodes();

  /** Returns the number of objects of a given type (e.g., EX_ELEM_BLOCK, 
    * EX_NODE_SET, ...). You must have called RequestInformation before 
    * invoking this member function.
    */
  int GetNumberOfObjectsOfType( int otype );

  /** Returns the number of arrays defined over objects of a given type 
    * (e.g., EX_ELEM_BLOCK, EX_NODE_SET, ...).
    * You must have called RequestInformation before invoking this 
    * member function.
    *
    * N.B.: This method will eventually disappear. Really, what we should be 
    * providing is an interface to query the arrays defined on a particular 
    * object, not a class of objects. However, until the reader
    * outputs multiblock datasets, we can't be that specific.
    */
  int GetNumberOfObjectArraysOfType( int otype );

  /** For a given object type, returns the name of the i-th object. 
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  const char* GetObjectName( int otype, int i );

  /** For a given object type, return the user-assigned ID of the i-th object.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  int GetObjectId( int otype, int i );

  /** For a given object type, return the size of the i-th object.
    * The size is the number of entries.
    * As an example, for an element block, it is the number of elements.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  int GetObjectSize( int otype, int i );

  /** For a given object type, returns the status of the i-th object.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  int GetObjectStatus( int otype, int i );

  /** For a given object type, returns the status of the i-th object, where i is
    * an index into the unsorted object array.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  int GetUnsortedObjectStatus( int otype, int i );

  /** For a given object type, sets the status of the i-th object.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  void SetObjectStatus( int otype, int i, int stat );

  /** For a given object type, sets the status of the i-th object,
    * where i is an index into the *unsorted* object array.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  void SetUnsortedObjectStatus( int otype, int i, int stat );

  /** For a given object type, returns the name of the i-th array. 
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  const char* GetObjectArrayName( int otype, int i );

  /** For a given object type, returns the number of components of the i-th 
    * array. You must have called RequestInformation before invoking this 
    * member function.
    */
  int GetNumberOfObjectArrayComponents( int otype, int i );

  /** For a given object type, returns the status of the i-th array.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  int GetObjectArrayStatus( int otype, int i );

  /** For a given object type, sets the status of the i-th array.
    * You must have called RequestInformation before invoking this 
    * member function.
    */
  void SetObjectArrayStatus( int otype, int i, int stat );

  /** Unlike object arrays, attributes are only defined over blocks (not sets)
    * and are defined on a per-block (not a per-block-type) basis.
    * In other words, there is no truth table for attributes.
    * This means the interface is different because each block can have a 
    * different number of attributes with different names.
    */
  int GetNumberOfObjectAttributes( int objectType, int objectIndex );
  const char* GetObjectAttributeName( int objectType, 
                                      int objectIndex, 
                                      int attributeIndex );
  int GetObjectAttributeIndex( int objectType, 
                               int objectIndex, 
                               const char* attribName );
  int GetObjectAttributeStatus( int objectType, 
                                int objectIndex, 
                                int attribIndex );
  void SetObjectAttributeStatus( int objectType, 
                                 int objectIndex, 
                                 int attribIndex, int status );

  /// Generate an array containing the block or set ID associated with each cell.
  vtkGetMacro(GenerateObjectIdArray,int);
  vtkSetMacro(GenerateObjectIdArray,int);
  static const char* GetObjectIdArrayName() { return "ObjectId"; }

  vtkSetMacro(GenerateGlobalElementIdArray,int);
  vtkGetMacro(GenerateGlobalElementIdArray,int);
  static const char* GetGlobalElementIdArrayName() { return "GlobalElementId"; }  

  vtkSetMacro(GenerateGlobalNodeIdArray,int);
  vtkGetMacro(GenerateGlobalNodeIdArray,int);
  static const char* GetGlobalNodeIdArrayName() { return "GlobalNodeId"; }  

  /** Should we generate an array defined over all cells
    * (whether they are members of blocks or sets) indicating the source file?
    */
  vtkSetMacro(GenerateFileIdArray,int);
  vtkGetMacro(GenerateFileIdArray,int);
  static const char* GetFileIdArrayName() { return "FileId"; }  

  /// Set/get the number that identifies this file in a series of files (defaults to 0).
  vtkSetMacro(FileId,int);
  vtkGetMacro(FileId,int);

  static const char *GetGlobalVariableValuesArrayName() 
    { return "GlobalVariableValues"; }  
  static const char *GetGlobalVariableNamesArrayName() 
    { return "GlobalVariableNames"; }  

  virtual void SetApplyDisplacements( int d );
  vtkGetMacro(ApplyDisplacements,int);

  virtual void SetDisplacementMagnitude( double s );
  vtkGetMacro(DisplacementMagnitude,double);

  vtkSetMacro(HasModeShapes,int);
  vtkGetMacro(HasModeShapes,int);

  vtkSetMacro(ModeShapeTime,double);
  vtkGetMacro(ModeShapeTime,double);

  vtkDataArray* FindDisplacementVectors( int timeStep );

  vtkSetMacro(EdgeFieldDecorations,int);
  vtkGetMacro(EdgeFieldDecorations,int);

  vtkSetMacro(FaceFieldDecorations,int);
  vtkGetMacro(FaceFieldDecorations,int);

  const struct ex_init_params* GetModelParams() const 
    { return &this->ModelParameters; }

  /// A struct to hold information about time-varying arrays
  struct ArrayInfoType {
    /// The name of the array
    vtkStdString Name;
    /// The number of components in the array
    int Components;
    /** The type of "glomming" performed.
     * Glomming is the process of aggregating one or more results variable names
     * from the Exodus files into a single VTK result variable name with one or 
     * more components.
     * One of: scalar, vector(2), vector(3), symtensor(6), integrationpoint.
     */
    int GlomType;
    /// Storage type of array (a type that can be passed to 
    /// vtkDataArray::Create())
    int StorageType;
    /// The source of the array (Result or Attribute)
    int Source;
    /// Whether or not the array should be loaded by RequestData
    int Status;
    /// The name of each component of the array as defined by the Exodus file. 
    /// Empty for generated arrays.
    vtkstd::vector<vtkStdString> OriginalNames;
    /// The index of each component of the array as ordered by the Exodus file. 
    /// Empty for generated arrays.
    vtkstd::vector<int> OriginalIndices;
    /** A map describing which objects the variable is defined on.
     * Each key (a pair<int,int>) is a block/set type and integer
     * offset into the corresponding BlockInfo or SetInfo.
     * Its value is true when the variable is defined on the
     * block/set indicated by the key.
     * Otherwise (if the key is absent from the map or present with a
     * false value), the variable is not defined on that block/set.
     */
    vtkstd::vector<int> ObjectTruth;
    /// Clear all the structure members.
    void Reset();
  };

  /// A struct to hold information about Exodus objects (blocks, sets, maps)
  struct ObjectInfoType {
    /// Number of entries in this block.
    int Size;
    /// Should the reader load this block?
    int Status;
    /// User-assigned identification number
    int Id;
    /// User-assigned name
    vtkStdString Name;
  };

  /// A struct to hold information about Exodus maps
  struct MapInfoType : public ObjectInfoType {
  };

  /// A struct to hold information about Exodus blocks or sets 
  /// (they have some members in common)
  struct BlockSetInfoType : public ObjectInfoType {
    /// Id (1-based) of first entry in file-local list across all blocks in file
    vtkIdType FileOffset;
    /** A map from nodal IDs in an Exodus file to nodal IDs in the output mesh.
      * Should only be used when SqueezePoints is true.
      * Otherwise, just subtract 1 from any Exodus node ID to get the VTK node ID.
      */
    vtkstd::map<vtkIdType,vtkIdType> PointMap;
    /** A map from nodal ids in the output mesh to those in an Exodus file.
      * Should only be used when SqueezePoints is true.
      * Otherwise, just add 1 to any VTK node ID to get the Exodus node ID.
      */
    vtkstd::map<vtkIdType,vtkIdType> ReversePointMap;
    /** The next vtk ID to use for a connectivity entry when point squeezing is on 
      * and no point ID exists.
      */
    vtkIdType NextSqueezePoint;
    /// Cached cell connectivity arrays for mesh
    vtkUnstructuredGrid* CachedConnectivity;

    BlockSetInfoType() { this->CachedConnectivity = 0; }
  };

  /// A struct to hold information about Exodus blocks
  struct BlockInfoType : public BlockSetInfoType {
    vtkStdString TypeName;
    // number of boundaries per entry
    // The index is the dimensionality of the entry. 0=node, 1=edge, 2=face
    int BdsPerEntry[3]; 
    int AttributesPerEntry;
    vtkstd::vector<vtkStdString> AttributeNames;
    vtkstd::vector<int> AttributeStatus;
    // VTK cell type (a function of TypeName and BdsPerEntry...)
    int CellType; 
    // Number of points per cell as used by VTK 
    // -- not what's in the file (i.e., BdsPerEntry[0] >= PointsPerCell)
    int PointsPerCell; 
  };

  /// A struct to hold information about Exodus blocks
  struct PartInfoType : public ObjectInfoType {
    vtkstd::vector<int> BlockIndices;
  };
  struct AssemblyInfoType : public ObjectInfoType {
    vtkstd::vector<int> BlockIndices;
  };
  struct MaterialInfoType : public ObjectInfoType {
    vtkstd::vector<int> BlockIndices;
  };

  /// A struct to hold information about Exodus sets
  struct SetInfoType : public BlockSetInfoType {
    int DistFact;     // Number of distribution factors 
                      // (for the entire block, not per array or entry)
  };

  /// Tags to indicate how single-component Exodus arrays are glommed 
  /// (aggregated) into multi-component VTK arrays.
  enum GlomTypes {
    Scalar=0,          //!< The array is a scalar
    Vector2=1,         //!< The array is a 2-D vector
    Vector3=2,         //!< The array is a 3-D vector
    SymmetricTensor=3, //!< The array is a symmetric tensor 
                       //   (order xx, yy, zz, xy, yz, zx)
    IntegrationPoint=4 //!< The array is a set of integration point values
  };

  /// Tags to indicate the source of values for an array.
  enum ArraySourceTypes {
    Result=0,        //!< The array is composed of results variables 
                     //   (that vary over time)
    Attribute=1,     //!< The array is composed of attributes 
                     //   (constants over time)
    Map=2,           //!< The array has a corresponding entry in MapInfo
    Generated=3      //!< The array is procedurally generated (e.g., BlockId)
  };

  /// Time stamp from last time we were in RequestInformation
  vtkTimeStamp InformationTimeStamp;
 
  friend class vtkExodusIIReader;

  virtual void SetParser( vtkExodusIIXMLParser* );
  vtkGetObjectMacro(Parser,vtkExodusIIXMLParser);

  // Because Parts, Materials, and assemblies are not stored as arrays,
  // but rather as maps to the element blocks they make up,  
  // we cannot use the Get|SetObject__() methods directly.

  int GetNumberOfParts();
  const char* GetPartName(int idx);
  const char* GetPartBlockInfo(int idx);
  int GetPartStatus(int idx);
  int GetPartStatus(vtkStdString name);
  void SetPartStatus(int idx, int on);
  void SetPartStatus(vtkStdString name, int flag);
    
  int GetNumberOfMaterials();
  const char* GetMaterialName(int idx);
  int GetMaterialStatus(int idx);
  int GetMaterialStatus(vtkStdString name);
  void SetMaterialStatus(int idx, int on);
  void SetMaterialStatus(vtkStdString name, int flag);

  int GetNumberOfAssemblies();
  const char* GetAssemblyName(int idx);
  int GetAssemblyStatus(int idx);
  int GetAssemblyStatus(vtkStdString name);
  void SetAssemblyStatus(int idx, int on);
  void SetAssemblyStatus(vtkStdString name, int flag);

  void SetFastPathObjectType(vtkExodusIIReader::ObjectType type)
    {this->FastPathObjectType = type;};
  void SetFastPathObjectId(vtkIdType id){this->FastPathObjectId = id;};
  vtkSetStringMacro(FastPathIdType);

  bool IsXMLMetadataValid();

  /** For a given object type, looks for an object in the collection
    * of initial objects of the same name, or if the name is empty, then of 
    * the same id as "info". If found, info's Status is set to the status of
    * the found object. 
    * You DO NOT need to have called RequestInformation before invoking this 
    * member function.
    */
  void GetInitialObjectStatus( int otype, ObjectInfoType *info );

  /** For a given array type, looks for an object in the collection
    * of initial objects of the same name, or if the name is empty, then of 
    * the same id as "info". If found, info's Status is set to the status of
    * the found object. 
    * You DO NOT need to have called RequestInformation before invoking this 
    * member function.
    */
  void GetInitialObjectArrayStatus( int otype, ArrayInfoType *info );

  /** For a given object type, creates and stores an ObjectInfoType
    * object using the given name and status. If the name contains a "ID: %d" 
    * substring, then it is used to initialize the ObjectInfoType.Id value.
    * You DO NOT need to have called RequestInformation before invoking this 
    * member function.
    */
  void SetInitialObjectStatus( int otype, const char *name, int stat );

  /** For a given array type, creates and stores an ArrayInfoType
    * object using the given name and status.
    * You DO NOT need to have called RequestInformation before invoking this 
    * member function.
    */
  void SetInitialObjectArrayStatus( int otype, const char *name, int stat );

  int UpdateTimeInformation();

protected:
  vtkExodusIIReaderPrivate();
  ~vtkExodusIIReaderPrivate();

  /// Returns true when order and text of names are consistent with integration 
  /// points. Called from GlomArrayNames().
  int VerifyIntegrationPointGlom( int nn, 
                                  char** np, 
                                  vtksys::RegularExpression& re, 
                                  vtkStdString& field, 
                                  vtkStdString& ele );

  /// Aggregate Exodus array names into VTK arrays with multiple components
  void GlomArrayNames( int i, 
                       int num_obj, 
                       int num_vars, 
                       char** var_names, 
                       int* truth_tab );

  /// Add generated array information to array info lists.
  void PrepareGeneratedArrayInfo();

  /** Read connectivity information and populate an unstructured grid with cells corresponding to a single block or set.
    *
    * If the connectivity hasn't changed since the last time RequestData was 
    * called, this copies a cache to the output.
    * 
    * Otherwise, this routine iterates over all block and set types.
    * For each type, it iterates over all objects of that type.
    * For each object whose status is 1, it reads that object's connectivity 
    * entries from cache or disk and inserts cells into CachedConnectivity.
    * If SqueezePoints is on, then connectivity entries are translated as 
    * required and PointMap is populated.
    * Finally, CachedConnectivity is shallow-copied to the output.
    * 
    * AssembleOutputConnectivity returns 1 if cache was used, 0 otherwise.
    */
  int AssembleOutputConnectivity( vtkIdType timeStep,
    int otyp, int oidx, int conntypidx, BlockSetInfoType* bsinfop,
    vtkUnstructuredGrid* output );
  /** Fill the output grid's point coordinates array.
    * Returns 1 on success, 0 on failure.
    * Failure occurs when the Exodus library is unable to read the point
    * coordindates array. This can be caused when there is not enough memory
    * or there is a file I/O problem.
    */
  int AssembleOutputPoints( vtkIdType timeStep,
    BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );
  /** Add the requested arrays to the output grid's point data.
    * This adds time-varying results arrays to the grid's vtkPointData object.
    */
  int AssembleOutputPointArrays( vtkIdType timeStep,
    BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );
  /** Add the requested arrays to the output grid's cell data.
    * This adds time-varying results arrays to the grid's vtkCellData object.
    */
  int AssembleOutputCellArrays( vtkIdType timeStep,
    int otyp, int oidx, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );
  /** Add procedurally generated arrays to an output mesh.
    * Currently, the only array that is procedurally generated is the object id 
    * array. Others may be added in the future.
    */
  int AssembleOutputProceduralArrays( vtkIdType timeStep, 
    int otyp, int oidx, vtkUnstructuredGrid* output );
  /// Add mesh-global field data such as QA records to the output mesh.
  int AssembleOutputGlobalArrays( vtkIdType timeStep,
    int otyp, int oidx, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );
  /** Add maps to an output mesh.
    * Maps are special integer arrays that may serve as GlobalId fields in 
    * vtkDataSetAttributes objects.
    * Maps may be procedurally generated if no map is contained in a file.
    * Maps are not time-varying.
    */
  int AssembleOutputPointMaps( vtkIdType timeStep,
    BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );
  int AssembleOutputCellMaps( vtkIdType timeStep,
    int otyp, int oidx, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );
  /** Add fast-path time-varying data to field data of an output block or set.
    */
  int AssembleArraysOverTime(
    int otyp, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );

  // Generate the decorations for edge fields.
  void AssembleOutputEdgeDecorations();

  // Generate the decorations for face fields.
  void AssembleOutputFaceDecorations();

  /// Insert cells from a specified block into a mesh
  void InsertBlockCells(
    int otyp, int obj, int conn_type, int timeStep, BlockInfoType* binfop );

  /// Insert cells from a specified set into a mesh
  void InsertSetCells(
    int otyp, int obj, int conn_type, int timeStep, SetInfoType* sinfop );

  /// Add a point array to an output grid's point data, squeezing if necessary
  void AddPointArray(
    vtkDataArray* src, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output );

  /// Insert cells referenced by a node set.
  void InsertSetNodeCopies(
    vtkIntArray* refs, int otyp, int obj, SetInfoType* sinfo );

  /// Insert cells referenced by an edge, face, or element set.
  void InsertSetCellCopies(
    vtkIntArray* refs, int otyp, int obj, SetInfoType* sinfo );

  /// Insert cells referenced by a side set.
  void InsertSetSides(
    vtkIntArray* refs, int otyp, int obj, SetInfoType* sinfo );

  /** Return an array for the specified cache key. If the array was not cached, 
    * read it from the file.
    * This function can still return 0 if you are foolish enough to request an 
    * array not present in the file, grasshopper.
    */
  vtkDataArray* GetCacheOrRead( vtkExodusIICacheKey );

  /** Return the index of an object type (in a private list of all object types).
    * This returns a 0-based index if the object type was found and -1 if it 
    * was not.
    */
  int GetConnTypeIndexFromConnType( int ctyp );

  /** Return the index of an object type (in a private list of all object types).
    * This returns a 0-based index if the object type was found and -1 if it 
    * was not.
    */
  int GetObjectTypeIndexFromObjectType( int otyp );

  /** Return the number of objects of the given type.
    * The integer typeIndex is not the type of the object (e.g., EX_ELEM_BLOCK), 
    * but is rather the index into the list of all object types 
    * (see obj_types in vtkExodusIIReader.cxx).
    */
  int GetNumberOfObjectsAtTypeIndex( int typeIndex );

  /** Return a pointer to the ObjectInfo of the specified type and index.
    * The integer typeIndex is not the type of the object (e.g., EX_ELEM_BLOCK), 
    * but is rather the index into the list of all object types (see obj_types 
    * in vtkExodusIIReader.cxx). The integer objectIndex is not the ID of the 
    * object (i.e., the ID stored in the Exodus file), but is rather the index 
    * into the list of all objects of the given type.
    */
  ObjectInfoType* GetObjectInfo( int typeIndex, int objectIndex );

  /** Return a pointer to the ObjectInfo of the specified type and index, but 
    * using indices sorted by object ID. This is the same as GetObjectInfo() 
    * except that it uses the SortedObjectIndices member to permute the 
    * requested \a objectIndex and it takes an object type (e.g., EX_ELEM_BLOCK) 
    * rather than an object type index.
    */
  ObjectInfoType* GetSortedObjectInfo( int objectType, int objectIndex );

  /** Return a pointer to the ObjectInfo of the specified type and index, but 
    * using indices sorted by object ID. This is the same as 
    * GetSortedObjectInfo() except that \a objectIndex directly indexes the 
    * object info array rather SortedObjectIndices, and it takes an object 
    * type (e.g., EX_ELEM_BLOCK) rather than an object type index.
    */
  ObjectInfoType* GetUnsortedObjectInfo( int objectType, int objectIndex );

  /** Get the index of the block containing the entity referenced by the 
    * specified file-global ID.
    * In this case, an entity is an edge, face, or element.
    */
  int GetBlockIndexFromFileGlobalId( int otyp, int refId );

  /** Get the block containing the entity referenced by the specified 
    * file-global ID.
    * In this case, an entity is an edge, face, or element.
    */
  BlockInfoType* GetBlockFromFileGlobalId( int otyp, int refId );

  /** Find or create a new SqueezePoint ID (unique sequential list of points 
    * referenced by cells in blocks/sets with Status == 1)
    */
  vtkIdType GetSqueezePointId( BlockSetInfoType* bsinfop, int i );

  /// Determine the VTK cell type for a given edge/face/element block
  void DetermineVtkCellType( BlockInfoType& binfo );

  /** Find an ArrayInfo object for a specific object type using the name 
    *  as a key.
    */
  ArrayInfoType* FindArrayInfoByName( int otyp, const char* name );

  /** Does the specified object type match? Avoid using these... they aren't 
    * robust against new types being implemented.
    */
  int IsObjectTypeBlock( int otyp );
  int IsObjectTypeSet( int otyp );
  int IsObjectTypeMap( int otyp );

  /** Given a map type (NODE_MAP, EDGE_MAP, ...) return the associated object 
    * type (NODAL, EDGE_BLOCK, ...) or vice-versa.
    */
  int GetObjectTypeFromMapType( int mtyp );
  int GetMapTypeFromObjectType( int otyp );
  int GetTemporalTypeFromObjectType( int otyp );

  /** Given a set connectivity type (NODE_SET_CONN, ...), return the associated 
    * object type (NODE_SET, ...) or vice-versa.
    */
  int GetSetTypeFromSetConnType( int sctyp );

  /** Given a block type (EDGE_BLOCK, ...), return the associated block 
    * connectivity type (EDGE_BLOCK_CONN, ...) or vice-versa.
    */
  int GetBlockConnTypeFromBlockType( int btyp );

  /** Function to trim space from names retrieved with ex_get_var_names.
   * This was added because some meshes had displacement arrays named 
   * "DISPX ", "DISPY ", "DISPZ " (note trailing spaces),
   * which prevented glomming and use of the vector field for displacements.
   */
  void RemoveBeginningAndTrailingSpaces( int len, char **names );

  /// Delete any cached connectivity information (for all blocks and sets)
  void ClearConnectivityCaches();

  /** Maps a block type (EX_ELEM_BLOCK, EX_FACE_BLOCK, ...) to a list of blocks 
    * of that type.
    */
  vtkstd::map<int,vtkstd::vector<BlockInfoType> > BlockInfo;
  /** Maps a set type (EX_ELEM_SET, ..., EX_NODE_SET) to a list of sets of 
    *  that type.
    */
  vtkstd::map<int,vtkstd::vector<SetInfoType> > SetInfo;
  /** Maps a map type (EX_ELEM_MAP, ..., EX_NODE_MAP) to a list of maps of that 
    * type. In old-style files, the only entries will be a single node and a 
    * single element map which have no specified ID number or name. In that 
    * case, an ID of 0 and a name of "Default" will be given to both.
    */
  vtkstd::map<int,vtkstd::vector<MapInfoType> > MapInfo;

  vtkstd::vector<PartInfoType> PartInfo;
  vtkstd::vector<MaterialInfoType> MaterialInfo;
  vtkstd::vector<AssemblyInfoType> AssemblyInfo;

  /** Maps an object type to vector of indices that reorder objects of that 
    * type by their IDs. This is used by the user interface to access blocks, 
    * sets, and maps in ascending order. It is not used internally.
    */
  vtkstd::map<int,vtkstd::vector<int> > SortedObjectIndices;
  /// Maps an object type (EX_ELEM_BLOCK, EX_NODE_SET, ...) to a list of arrays 
  //  defined on that type.
  vtkstd::map<int,vtkstd::vector<ArrayInfoType> > ArrayInfo;

  /** Maps an object type (EX_ELEM_BLOCK, EX_NODE_SET, ...) to a list of arrays
    * defined on that type. Used to store initial status of arrays before 
    * RequestInformation can be called.
    */
  vtkstd::map<int,vtkstd::vector<ArrayInfoType> > InitialArrayInfo;

  /** Maps an object type (EX_ELEM_BLOCK, EX_NODE_SET, ...) to a list of objects 
    * defined on that type. Used to store initial status of objects before 
    * RequestInformation can be called.
    */
  vtkstd::map<int,vtkstd::vector<ObjectInfoType> > InitialObjectInfo;

  /// These aren't the variables you're looking for.
  int AppWordSize;
  int DiskWordSize;

  /** The version of Exodus that wrote the currently open file (or a negative 
    * number otherwise).
    */
  float ExodusVersion;

  /// The handle of the currently open file.
  int Exoid;

  /// Parameters describing the currently open Exodus file.
  struct ex_init_params ModelParameters;

  /// A list of time steps for which results variables are stored.
  vtkstd::vector<double> Times;

  /// The current time step
  int TimeStep;

  /** The time value. This is used internally when HasModeShapes is true and 
    * ignored otherwise.
    */
  double ModeShapeTime;

  int GenerateObjectIdArray;
  int GenerateGlobalIdArray;
  int GenerateFileIdArray;
  int GenerateGlobalElementIdArray;
  int GenerateGlobalNodeIdArray;

  /** Defaults to 0. Set by vtkPExodusIIReader on each entry of ReaderList.
    * Used to generate the file ID array over all output cells.
    */
  int FileId;

  /// A least-recently-used cache to hold raw arrays.
  vtkExodusIICache* Cache;

  int ApplyDisplacements;
  float DisplacementMagnitude;
  int HasModeShapes;

  // Specify how to decorate edge and face variables.
  int EdgeFieldDecorations;
  int FaceFieldDecorations;

  // Meshes to support edge and face glyph decorations.
  vtkPolyData* EdgeDecorationMesh;
  vtkPolyData* FaceDecorationMesh;

  /** Should the reader output only points used by elements in the output mesh, 
    * or all the points. Outputting all the points is much faster since the 
    * point array can be read straight from disk and the mesh connectivity need 
    * not be altered. Squeezing the points down to the minimum set needed to 
    * produce the output mesh is useful for glyphing and other point-based 
    * operations. On large parallel datasets, loading all the points implies 
    * loading all the points on all processes and performing subsequent 
    * filtering on a much larger set.
    *
    * By default, SqueezePoints is true for backwards compatability.
    */
  int SqueezePoints;

  /** Pointer to owning reader... this is not registered in order to avoid 
    * circular references.
    */
  vtkExodusIIReader* Parent;

  vtkExodusIIXMLParser* Parser;

  vtkExodusIIReader::ObjectType FastPathObjectType;
  vtkIdType FastPathObjectId;
  char* FastPathIdType;

private:
  vtkExodusIIReaderPrivate( const vtkExodusIIReaderPrivate& ); // Not implemented.
  void operator = ( const vtkExodusIIReaderPrivate& ); // Not implemented.
};

#endif // __vtkExodusIIReaderPrivate_h
