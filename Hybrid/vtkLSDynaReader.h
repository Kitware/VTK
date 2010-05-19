/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaReader.h

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

// .NAME vtkLSDynaReader - Read LS-Dyna databases (d3plot)
// .SECTION Description
// This filter reads LS-Dyna databases.
//
// The Set/GetFileName() routines are actually wrappers around the
// Set/GetDatabaseDirectory() members; the actual filename you choose is
// irrelevant -- only the directory name is used.  This is done in order to
// accommodate ParaView.
//
// Note that this reader produces 7 output meshes.
// These meshes are required as several attributes are defined on subsets
// of the mesh.  Below is a list of meshes in the order they are output and
// an explanation of which attributes are unique to each mesh:
// - solid (3D) elements: number of integration points are different than 2D
// - thick shell elements: number of integration points are different than 
// planar 2D
// - shell (2D) elements: number of integration points are different than 3D
// - rigid surfaces: can't have deflection, only velocity, accel, etc.
// - road surfaces: have only a "segment ID" (serves as material ID) and a 
// velocity.
// - beam elements: have Frenet (TNB) frame and cross-section attributes 
// (shape and size)
// - spherical particle hydrodynamics (SPH) elements: have a radius of 
// influence, internal energy, etc.
// Because each mesh has its own cell attributes, the vtkLSDynaReader has a
// rather large API.  Instead of a single set of routines to query and set
// cell array names and status, one exists for each possible output mesh.
// Also, GetNumberOfCells() will return the sum of all the cells in all 7
// meshes.  If you want the number of cells in a specific mesh, there are
// separate routines for each mesh type.
//
// .SECTION "Developer Notes"

// LSDyna files contain 3 different types of sections: control, data, and
// state.  Control sections contain constants that describe the type of
// simulation data in a file or group of files.  Data sections contain
// simulation information that is invariant across individual time steps
// (but can vary when a mesh adaptation occurs).  This information includes
// material, connectivity, and undeformed geometry.  Finally, state data is
// information that varies with each time step.  Unless a mesh adaptation
// occurs, there will be a single control and data section, and they will
// be located at the start of the database (the first file).
// 
// In their infinite wisdom, LSDyna developers decided to split simulation
// data into multiple files, each no larger than some predetermined limit.
// Each file can contain one section, a partial section (if it would not
// fit into a single file), or multiple sections. Files are padded with
// zeros so that their lengths will be multiples of 512*512.  The size of
// each section is determined by constants in the control and data
// sections, which means that these must be parsed carefully in order to
// correctly locate desired information.  Unfortunately, the constants are
// not terribly well-documented and in some cases the documentation is in
// error.
//
// .SECTION "Open Issues"
// The LS-Dyna file format document leaves a good bit open to
// interpretation.  In addition to the "documentation vs. files in the
// wild" issues there are also implementation problems.
//
// - Where exactly may breaks to a new file occur in the pre-state 
// information? At each section?
// - Will state data sections (node/cell data, element deletion, sph data, 
// rigid body motion) be moved to  the beginning of a new file if their data 
// will be too large for a given file, or are all the sections
// counted together as a single state (makes more sense for keeping time 
// word at start of every file).
//  The questions above arise because the docs (p. 3) state "There are 3
// sections in this database." but then call many smaller pieces of data
// "sections". Should they be subsections? The docs are quiet about whether
// the second section (of 3) is ever split across multiple files and, if
// so, whether it is done at (sub)section boundaries when possible or just
// wherever it needs to occur.
// - How many components does Eddy Viscosity have? It's shown as 7 bits in 
// NCFDV1 which makes no sense at all.
// - Why is NARBS larger than 10+NUMNP+NEL8+NEL2+NEL4+NELT (which is the 
// value specified by the documentation)?
// Obviously, NARBS is definitive, but what are the extra numbers at the end?
// - Is there a difference between rigid body elements NUMRBE and rigid road 
// surfaces? It appears that the nodes and connectivity of the road surface 
// are given separately (p.13) while on p.7 the Material
//   Type Data subsection says that shells in a rigid body will just have a 
// certain material ID but be  interspersed among deformable shell elements.
// - Word 37 of the control section serves two possible purposes... it can 
// mean NMSPH or EDLOPT.
//   I assume that different versions of the code use that word differently. 
// How do we know the difference?
// - It's unclear how much state isn't stored when a shell element is marked 
// as rigid. Specifically, is element deletion data stored for rigid shells? 
// Page 21 of the spec is mute on this.
// - The loop to read cell User IDs won't work if Rigid Body and Shell 
// elements are interleaved (which I now believe they are).
//
// On the VTK side of things:
// - Berk has nudged me towards multiblock outputs but hasn't committed to 
// exactly how things can be made efficient for a parallel version of the 
// reader.
// - This reader will eventually need to respond to a second output port for 
// "small spatial, large temporal" queries.
// - The reader doesn't handle crack files (d3crck)
// - The reader doesn't handle interface force files (no default name)
// - The reader doesn't handle time history (abbreviated output) files (d3thdt)
// - The reader doesn't handle dynamic relaxation files (d3drfl)
// - The reader doesn't handle reduced parts (state for a subset of parts) files (d3part)
// - The reader doesn't handle mode shape files (d3eigv)
// - The reader doesn't handle equilibrium iteration files (d3iter)
// - The reader doesn't handle extra time data files (d3xtf)
// - The reader doesn't handle printer files (d3hsp)
// - The reader doesn't handle modal neutral files (d3mnf)
// - The reader doesn't handle packed connectivity.
// - The reader doesn't handle adapted element parent lists (but the 2002 specification says LSDyna doesn't implement it).
// - All the sample datasets have MATTYP = 0. Need something to test MATTYP = 1.
// - I have no test datasets with rigid body and/or road surfaces, so the 
// implementation is half-baked.
// - It's unclear how some of the data should be presented. Although blindly 
// tacking the numbers into a large chuck of cell data is better than nothing,
// some attributes (e.g., forces & moments) lend themselves to more elaborate
// presentation. Also, shell and thick shell elements have stresses that 
// belong to a particular side of an element or have a finite thickness that 
// could be rendered.
//   Finally, beam elements have cross sections that could be rendered. 
// Some of these operations require numerical processing of the results and 
// so we shouldn't eliminate the ability to get at the raw simulation data. 
// Perhaps a filter could be applied to "fancify" the geometry.
//

#ifndef __vtkLSDynaReader_h
#define __vtkLSDynaReader_h

#include <vtkMultiBlockDataSetAlgorithm.h>

class vtkLSDynaReaderPrivate;
class vtkPoints;
class vtkDataArray;
class vtkUnstructuredGrid;

class VTK_HYBRID_EXPORT vtkLSDynaReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkLSDynaReader,vtkMultiBlockDataSetAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkLSDynaReader *New();

  //BTX
  /** LS-Dyna cell types.
   * These may be used as values for the \a cellType argument in member functions.
   * One dataset is created for each cell type so that cells can have different
   * attributes (temperature, pressure, etc.) defined over them.
   * Note that \a NUM_CELL_TYPES is not a cell type, but an enumerant that
   * specifies the total number of cell types. It is used to size arrays.
   */
  enum {
    PARTICLE = 0,
    BEAM = 1,
    SHELL = 2,
    THICK_SHELL = 3,
    SOLID = 4,
    RIGID_BODY = 5,
    ROAD_SURFACE = 6,
    NUM_CELL_TYPES
  };
  //ETX

  // Description:
  // Print out more complete information about the dataset
  // (and less complete information about the VTK hierarchy) than PrintSelf.
  void Dump( ostream &os );

  // Description:
  // A routine to call Dump() from within a lame debugger that won't
  // properly pass a C++ iostream object like cout.
  void DebugDump();

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile( const char* fname );

  // Description:
  // Get/Set the directory containing the LS-Dyna database and determine
  // whether it is valid.
  virtual void SetDatabaseDirectory( const char* );
  const char* GetDatabaseDirectory();
  int IsDatabaseValid();

  // Description:
  // Get/Set the filename. The Set/GetFileName() routines are actually
  // wrappers around the Set/GetDatabaseDirectory() members; the actual
  // filename you choose is irrelevant -- only the directory name is used.
  // This is done in order to accommodate ParaView.
  virtual void SetFileName( const char* );
  const char* GetFileName();

  // Description:
  // The title of the database is a 40 or 80 character text description
  // stored at the front of a d3plot file.  Do not call this function
  // before setting the database directory and calling UpdateInformation().
  char* GetTitle();

  // Description:
  // Retrieve the dimension of points in the database. This should return 2
  // or 3.  Do not call this function before setting the database directory
  // and calling UpdateInformation().
  int GetDimensionality();

  // Description:
  // Retrieve the number of points in the database.  Do not call this
  // function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfNodes();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  //
  // Note that GetNumberOfCells() returns the sum of
  // GetNumberOfContinuumCells() and GetNumberOfParticleCells().
  vtkIdType GetNumberOfCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  //
  // Note that GetNumberOfContinuumCells() returns the sum of
  // GetNumberOfSolidCells(), GetNumberOfThickShellCells(),
  // GetNumberOfShellCells(), GetNumberOfRigidBodyCells(),
  // GetNumberOfRoadSurfaceCells(), and GetNumberOfBeamCells().
  vtkIdType GetNumberOfContinuumCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfSolidCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfThickShellCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfShellCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfRigidBodyCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfRoadSurfaceCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.
  // Do not call this function before setting the database directory and calling UpdateInformation().
  vtkIdType GetNumberOfBeamCells();

  // Description:
  // Retrieve the number of cells of a given type in the database.  Do not
  // call this function before setting the database directory and calling
  // UpdateInformation().
  vtkIdType GetNumberOfParticleCells();

  // Description:
  // Retrieve information about the time extents of the LS-Dyna database.
  // Do not call these functions before setting the database directory and
  // calling UpdateInformation().
  vtkIdType GetNumberOfTimeSteps();
  virtual void SetTimeStep( vtkIdType );
  vtkIdType GetTimeStep();
  double GetTimeValue( vtkIdType );
  vtkGetVector2Macro(TimeStepRange,int);
  vtkSetVector2Macro(TimeStepRange,int);

  // Description:
  // These methods allow you to load only selected subsets of the nodal
  // variables defined over the mesh.
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int);
  virtual void SetPointArrayStatus( int arr, int status );
  virtual void SetPointArrayStatus( const char* arrName, int status );
  int GetPointArrayStatus( int arr );
  int GetPointArrayStatus( const char* arrName );
  int GetNumberOfComponentsInPointArray( int arr );
  int GetNumberOfComponentsInPointArray( const char* arrName );

  // Description:
  // Routines that allow the status of a cell variable to be adjusted or
  // queried independent of the output mesh.  The \a cellType parameter
  // should be one of: LS_POINT, LS_BEAM, LS_SHELL, LS_THICK_SHELL,
  // LS_SOLID, LS_RIGID_BODY, or LS_ROAD_SURFACE
  int GetNumberOfCellArrays( int cellType );
  const char* GetCellArrayName( int cellType, int arr );
  virtual void SetCellArrayStatus( int cellType, int arr, int status );
  virtual void SetCellArrayStatus( int cellType, const char* arrName, int status );
  int GetCellArrayStatus( int cellType, int arr );
  int GetCellArrayStatus( int cellType, const char* arrName );
  int GetNumberOfComponentsInCellArray( int cellType, int arr );
  int GetNumberOfComponentsInCellArray( int cellType, const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfSolidArrays();
  const char* GetSolidArrayName(int);
  virtual void SetSolidArrayStatus( int arr, int status );
  virtual void SetSolidArrayStatus( const char* arrName, int status );
  int GetSolidArrayStatus( int arr );
  int GetSolidArrayStatus( const char* arrName );

  int GetNumberOfComponentsInSolidArray( int a );
  int GetNumberOfComponentsInSolidArray( const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfThickShellArrays();
  const char* GetThickShellArrayName(int);
  virtual void SetThickShellArrayStatus( int arr, int status );
  virtual void SetThickShellArrayStatus( const char* arrName, int status );
  int GetThickShellArrayStatus( int arr );
  int GetThickShellArrayStatus( const char* arrName );

  int GetNumberOfComponentsInThickShellArray( int a );
  int GetNumberOfComponentsInThickShellArray( const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfShellArrays();
  const char* GetShellArrayName(int);
  virtual void SetShellArrayStatus( int arr, int status );
  virtual void SetShellArrayStatus( const char* arrName, int status );
  int GetShellArrayStatus( int arr );
  int GetShellArrayStatus( const char* arrName );

  int GetNumberOfComponentsInShellArray( int a );
  int GetNumberOfComponentsInShellArray( const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfRigidBodyArrays();
  const char* GetRigidBodyArrayName(int);
  virtual void SetRigidBodyArrayStatus( int arr, int status );
  virtual void SetRigidBodyArrayStatus( const char* arrName, int status );
  int GetRigidBodyArrayStatus( int arr );
  int GetRigidBodyArrayStatus( const char* arrName );

  int GetNumberOfComponentsInRigidBodyArray( int a );
  int GetNumberOfComponentsInRigidBodyArray( const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfRoadSurfaceArrays();
  const char* GetRoadSurfaceArrayName(int);
  virtual void SetRoadSurfaceArrayStatus( int arr, int status );
  virtual void SetRoadSurfaceArrayStatus( const char* arrName, int status );
  int GetRoadSurfaceArrayStatus( int arr );
  int GetRoadSurfaceArrayStatus( const char* arrName );

  int GetNumberOfComponentsInRoadSurfaceArray( int a );
  int GetNumberOfComponentsInRoadSurfaceArray( const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfBeamArrays();
  const char* GetBeamArrayName(int);
  virtual void SetBeamArrayStatus( int arr, int status );
  virtual void SetBeamArrayStatus( const char* arrName, int status );
  int GetBeamArrayStatus( int arr );
  int GetBeamArrayStatus( const char* arrName );

  int GetNumberOfComponentsInBeamArray( int a );
  int GetNumberOfComponentsInBeamArray( const char* arrName );

  // Description:
  // These methods allow you to load only selected subsets of the cell
  // variables defined over the mesh.
  int GetNumberOfParticleArrays();
  const char* GetParticleArrayName(int);
  virtual void SetParticleArrayStatus( int arr, int status );
  virtual void SetParticleArrayStatus( const char* arrName, int status );
  int GetParticleArrayStatus( int arr );
  int GetParticleArrayStatus( const char* arrName );

  int GetNumberOfComponentsInParticleArray( int a );
  int GetNumberOfComponentsInParticleArray( const char* arrName );

  // Description:
  // Should deflected coordinates be used, or should the mesh remain
  // undeflected?  By default, this is true but its value is ignored if the
  // nodal "Deflection" array is not set to be loaded.
  vtkSetMacro(DeformedMesh,int);
  vtkGetMacro(DeformedMesh,int);
  vtkBooleanMacro(DeformedMesh,int);

  // Description:
  // Should dead cells be removed from the mesh?  Cells are marked dead by
  // setting the corresponding entry in the <b>cell</b> array "Death" to 0.
  // Cells that are not dead have the corresponding entry in the cell array
  // "Death" set to their material ID.  By default, this is true but its
  // value is ignored if the cell "Death" array is not set to be loaded.
  // It is also ignored if the database's element deletion option is set to
  // denote <b>points</b> (not cells) as deleted; in that case, "Death"
  // will appear to be a point array.
  vtkSetMacro(RemoveDeletedCells,int);
  vtkGetMacro(RemoveDeletedCells,int);
  vtkBooleanMacro(RemoveDeletedCells,int);

  // Description:
  // Split each part into submeshes based on material ID.
  // By default, this is false and all cells of a given
  // type (solid, thick shell, shell, ...) are in a single mesh.
  vtkSetMacro(SplitByMaterialId,int);
  vtkGetMacro(SplitByMaterialId,int);
  vtkBooleanMacro(SplitByMaterialId,int);

  // Description:
  // The name of the input deck corresponding to the current database.
  // This is used to determine the part names associated with each material ID.
  // This file may be in two formats: a valid LSDyna input deck or a 
  // short XML summary.
  // If the file begins with "<?xml" then the summary format is used.
  // Otherwise, the keyword format is used and a summary file will be
  // created if write permissions exist in the directory containing
  // the keyword file. The newly created summary will have ".k" or ".key"
  // stripped from the end of the keyword filename and ".lsdyna" appended.
  vtkSetStringMacro(InputDeck);
  vtkGetStringMacro(InputDeck);

  // Description:
  // These methods allow you to load only selected parts of the input.
  // If InputDeck points to a valid keyword file (or summary), then part
  // names will be taken from that file.
  // Otherwise, when arbitrary material numbering is used, parts will be named
  // "PartXXX (MatlYYY)" where XXX is an increasing sequential number and YYY
  // is the respective material ID. If no input deck is specified and arbitrary
  // arbitrary material numbering is not used, parts will be named
  // "PartXXX" where XXX is a sequential material ID.
  int GetNumberOfPartArrays();
  const char* GetPartArrayName(int);
  virtual void SetPartArrayStatus( int arr, int status );
  virtual void SetPartArrayStatus( const char* partName, int status );
  int GetPartArrayStatus( int arr );
  int GetPartArrayStatus( const char* partName );

protected:
  // All the output grids (one for each possible combination of cell attributes)
  vtkUnstructuredGrid* OutputParticles; // have radius of influence
  vtkUnstructuredGrid* OutputBeams; // have TNB frame
  vtkUnstructuredGrid* OutputShell; // integration points are different than 3D
  vtkUnstructuredGrid* OutputThickShell; // integration points are different than planar 2D
  vtkUnstructuredGrid* OutputSolid; // integration points are different than 2D
  vtkUnstructuredGrid* OutputRigidBody; // can't have deflection, only velocity, accel, ...
  vtkUnstructuredGrid* OutputRoadSurface; // can't have deflection, only velocity, accel, ...

  // Description:
  // Should deflected coordinates be used, or should the mesh remain
  // undeflected?  By default, this is true.
  int DeformedMesh;

  // Description:
  // Should cells marked as deleted be removed from the mesh?
  // By default, this is true.
  int RemoveDeletedCells;

  // Description:
  // Split each mesh into submeshes based on the material ID of each cell.
  int SplitByMaterialId;

  // Description:
  // The range of time steps available within a database.
  // Only valid after UpdateInformation() is called on the reader.
  int TimeStepRange[2];

  // Description:
  // The name of a file containing part names and IDs.
  char* InputDeck;

  vtkLSDynaReader();
  virtual ~vtkLSDynaReader();

  // Description:
  // This function populates the reader's private dictionary with
  // information about the database.  It is called once from
  // RequestInformation() and once any time the adaptation level changes.
  // The adaptation level can change any time the current state(time) is
  // modified.  Upon success, -1 is returned. "Soft" failures return 0 and
  // "hard" failures return 1.
  int ReadHeaderInformation( int currentAdaptLevel );

  // Description:
  // This function scans the list of files in the database and bookmarks the 
  // start of each time step's state information.
  // Before this function is called:
  // - The database directory name must be set,
  // - ReadHeaderInformation must have been called for adaptation level 0, and
  // - The "read head" must be placed at the end of the first adaptation's geometry section.
  // Upon success, -1 is returned. "Soft" failures return 0 and "hard" failures return 1.
  int ScanDatabaseTimeSteps();

  virtual int RequestInformation( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  // Description:
  // These functions read various parts of the database.
  // The functions that take a vtkIdType argument must be passed
  // the current timestep.
  // Functions that do not take a timestep must have the read head
  // positioned to the start of their data sections.
  // These functions should only be called from within RequestData() since
  // they require the various output meshes to exist.
  virtual int ReadNodes();
  virtual int ReadConnectivityAndMaterial();
  virtual int ReadUserIds();
  virtual int ReadState( vtkIdType );
  virtual int ReadDeletion();
  virtual int ReadSPHState( vtkIdType );

  // Description:
  // Called from within ReadHeaderInformation() to read part names
  // associated with material IDs.
  virtual int ReadInputDeck();

  // Description:
  // Called from within ReadHeaderInformation() to read arbitrary material
  // IDs (if present) or manufacture sequential material IDs (if not
  // present).
  virtual int ReadUserMaterialIds();

  // Description:
  // ReadInputDeck determines the type of file (keyword or XML summary) and
  // calls one of these two routines to read the file.
  int ReadInputDeckXML( ifstream& deck );
  int ReadInputDeckKeywords( ifstream& deck );

  // Description:
  // ReadInputDeckKeywords calls this function if it was successful in reading
  // part names for materials.
  int WriteInputDeckSummary( const char* fname );

  void PartFilter( vtkMultiBlockDataSet* mbds, int celltype );
  // Description:
  // Read an array of deletion data.
  // This is used by ReadDeletion to actually read the data from the file
  // (as opposed to attach it to the proper place in the VTK dataset)
  // depending on the value of "MDLOPT".
  // The array passed to this routine is filled with deletion data.
  // The number of tuples must be set on the array previous to calling
  // this routine.
  // The \a anyDeleted argument is set to 0 if no cells in the array are
  // marked deleted, or 1 if any cells are marked for deletion.
  virtual int ReadDeletionArray( vtkDataArray* arr, int& anyDeleted );

private:
  vtkLSDynaReader( const vtkLSDynaReader& ); // Not implemented.
  void operator = ( const vtkLSDynaReader& ); // Not implemented.

  vtkLSDynaReaderPrivate* P;
};

inline void vtkLSDynaReader::SetPointArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfPointArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetPointArrayName(a) ) == 0 )
      {
      this->SetPointArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Point array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetPointArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfPointArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetPointArrayName(a) ) == 0 )
      {
      return this->GetPointArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Point array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInPointArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfPointArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetPointArrayName( a ) ) == 0 )
      {
      return this->GetNumberOfComponentsInPointArray( a );
      }
    }
  //vtkWarningMacro( "Point array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetCellArrayStatus( int cellType, const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfCellArrays( cellType ); ++a )
    {
    if ( strcmp( arrName, this->GetCellArrayName( cellType, a ) ) == 0 )
      {
      this->SetCellArrayStatus( cellType, a, status );
      return;
      }
    }
  vtkWarningMacro( "Cell array \"" << arrName << "\" (type " << cellType << ") does not exist" );
}

inline int vtkLSDynaReader::GetCellArrayStatus( int cellType, const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfCellArrays( cellType ); ++a )
    {
    if ( strcmp( arrName, this->GetCellArrayName( cellType, a ) ) == 0 )
      {
      return this->GetCellArrayStatus( cellType, a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInCellArray( int cellType, const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfCellArrays( cellType ); ++a )
    {
    if ( strcmp( arrName, this->GetCellArrayName( cellType, a ) ) == 0 )
      {
     return this->GetNumberOfComponentsInCellArray( cellType, a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetSolidArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfSolidArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetSolidArrayName(a) ) == 0 )
      {
      this->SetSolidArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Solid array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetSolidArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfSolidArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetSolidArrayName(a) ) == 0 )
      {
      return this->GetSolidArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInSolidArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfSolidArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetSolidArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInSolidArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetThickShellArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfThickShellArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetThickShellArrayName(a) ) == 0 )
      {
      this->SetThickShellArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Thick shell array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetThickShellArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfThickShellArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetThickShellArrayName(a) ) == 0 )
      {
      return this->GetThickShellArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInThickShellArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfThickShellArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetThickShellArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInThickShellArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetShellArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfShellArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetShellArrayName(a) ) == 0 )
      {
      this->SetShellArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Shell array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetShellArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfShellArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetShellArrayName(a) ) == 0 )
      {
      return this->GetShellArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInShellArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfShellArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetShellArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInShellArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetBeamArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfBeamArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetBeamArrayName(a) ) == 0 )
      {
      this->SetBeamArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Beam array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetBeamArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfBeamArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetBeamArrayName(a) ) == 0 )
      {
      return this->GetBeamArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInBeamArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfBeamArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetBeamArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInBeamArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetParticleArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfParticleArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetParticleArrayName(a) ) == 0 )
      {
      this->SetParticleArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Particle array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetParticleArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfParticleArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetParticleArrayName(a) ) == 0 )
      {
      return this->GetParticleArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInParticleArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfParticleArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetParticleArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInParticleArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetRigidBodyArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfRigidBodyArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetRigidBodyArrayName(a) ) == 0 )
      {
      this->SetRigidBodyArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Rigid body array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetRigidBodyArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfRigidBodyArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetRigidBodyArrayName(a) ) == 0 )
      {
      return this->GetRigidBodyArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInRigidBodyArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfRigidBodyArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetRigidBodyArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInRigidBodyArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetRoadSurfaceArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfRoadSurfaceArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetRoadSurfaceArrayName(a) ) == 0 )
      {
      this->SetRoadSurfaceArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Road surface array \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetRoadSurfaceArrayStatus( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfRoadSurfaceArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetRoadSurfaceArrayName(a) ) == 0 )
      {
      return this->GetRoadSurfaceArrayStatus( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline int vtkLSDynaReader::GetNumberOfComponentsInRoadSurfaceArray( const char* arrName )
{
  for ( int a=0; a<this->GetNumberOfRoadSurfaceArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetRoadSurfaceArrayName(a) ) == 0 )
      {
     return this->GetNumberOfComponentsInRoadSurfaceArray( a );
      }
    }
  //vtkWarningMacro( "Cell array \"" << arrName << "\" does not exist" );
  return 0;
}

inline void vtkLSDynaReader::SetPartArrayStatus( const char* arrName, int status )
{
  for ( int a=0; a<this->GetNumberOfPartArrays(); ++a )
    {
    if ( strcmp( arrName, this->GetPartArrayName(a) ) == 0 )
      {
      this->SetPartArrayStatus( a, status );
      return;
      }
    }
  vtkWarningMacro( "Part \"" << arrName << "\" does not exist" );
}

inline int vtkLSDynaReader::GetPartArrayStatus( const char* partName )
{
  for ( int a=0; a<this->GetNumberOfPartArrays(); ++a )
    {
    if ( strcmp( partName, this->GetPartArrayName(a) ) == 0 )
      {
      return this->GetPartArrayStatus( a );
      }
    }
  //vtkWarningMacro( "PartArray \"" << partName << "\" does not exist" );
  return 0;
}

#endif // __vtkLSDynaReader_h
