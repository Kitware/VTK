/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMREnzoParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMREnzoParticlesReader.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkPointData.h"
#include "vtkIntArray.h"
#include "vtkDataArray.h"

#include <cassert>
#include <vector>

#include "vtksys/SystemTools.hxx"

#define H5_USE_16_API
#include "vtk_hdf5.h"      // for the HDF data loading engine

#include "vtkAMREnzoReaderInternal.h"

//------------------------------------------------------------------------------
//            HDF5 Utility Routines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Description:
// Finds the block index (blockIndx) within the HDF5 file associated with
// the given file index.
static bool FindBlockIndex( hid_t fileIndx, const int blockIdx, hid_t &rootIndx )
{
  // retrieve the contents of the root directory to look for a group
  // corresponding to the target block, if available, open that group
  hsize_t numbObjs;
  rootIndx = H5Gopen( fileIndx, "/" );
  if( rootIndx < 0 )
    {
    vtkGenericWarningMacro( "Failed to open root node of particles file" );
    return false;
    }

  bool found = false;
  H5Gget_num_objs( rootIndx, &numbObjs );
  for( int objIndex=0; objIndex < static_cast < int >(numbObjs); objIndex++  )
    {
    if( H5Gget_objtype_by_idx( rootIndx, objIndex ) == H5G_GROUP  )
      {
      int    blckIndx;
      char   blckName[65];
      H5Gget_objname_by_idx( rootIndx, objIndex, blckName, 64 );

      // Is this the target block?
      if( (sscanf( blckName, "Grid%d", &blckIndx )==1) &&
          (blckIndx  ==  blockIdx) )
        {
        // located the target block
        rootIndx = H5Gopen( rootIndx, blckName );
        if( rootIndx < 0 )
          {
          vtkGenericWarningMacro( "Could not locate target block!\n" );
          }
        found = true;
        break;
        }
      } // END if group
    } // END for all objects
  return( found );
}

//------------------------------------------------------------------------------
// Description:
// Returns the double array
static void GetDoubleArrayByName(
    const hid_t rootIdx, const char* name, std::vector<double> &array)
{
  // turn off warnings
  void       * pContext = NULL;
  H5E_auto_t   erorFunc;
  H5Eget_auto( &erorFunc, &pContext );
  H5Eset_auto( NULL, NULL );

  hid_t arrayIdx = H5Dopen( rootIdx, name );
  if( arrayIdx < 0 )
    {
      vtkGenericWarningMacro( "Cannot open array: " << name << "\n");
      return;
    }

  // turn warnings back on
  H5Eset_auto( erorFunc, pContext );
  pContext = NULL;

  // get the number of particles
  hsize_t      dimValus[3];
  hid_t        spaceIdx = H5Dget_space( arrayIdx );
  H5Sget_simple_extent_dims( spaceIdx, dimValus, NULL );
  int          numbPnts = dimValus[0];

  array.resize( numbPnts );
  H5Dread( arrayIdx,H5T_NATIVE_DOUBLE,H5S_ALL,H5S_ALL,H5P_DEFAULT,&array[0] );

//  H5Dclose( spaceIdx );
//  H5Dclose( arrayIdx );
}

//------------------------------------------------------------------------------
//          END of HDF5 Utility Routine definitions
//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkAMREnzoParticlesReader);

//------------------------------------------------------------------------------
vtkAMREnzoParticlesReader::vtkAMREnzoParticlesReader()
{
  this->Internal     = new vtkEnzoReaderInternal();
  this->ParticleType = -1; /* undefined particle type */
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMREnzoParticlesReader::~vtkAMREnzoParticlesReader()
{
  if( this->Internal != NULL )
    {
    delete this->Internal;
    }
  this->Internal = NULL;
}

//------------------------------------------------------------------------------
void vtkAMREnzoParticlesReader::PrintSelf(
    std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMREnzoParticlesReader::ReadMetaData()
{
  if( this->Initialized )
    {
    return;
    }

  if( !this->FileName )
    {
    vtkErrorMacro("No FileName set!");
    return;
    }

  this->Internal->SetFileName( this->FileName );
  std::string  tempName( this->FileName );
  std::string  bExtName( ".boundary" );
  std::string  hExtName( ".hierarchy" );

  if( tempName.length() > hExtName.length() &&
       tempName.substr(tempName.length()-hExtName.length() )== hExtName )
     {
     this->Internal->MajorFileName =
         tempName.substr( 0, tempName.length() - hExtName.length() );
     this->Internal->HierarchyFileName = tempName;
     this->Internal->BoundaryFileName  =
         this->Internal->MajorFileName + bExtName;
     }
  else if( tempName.length() > bExtName.length() &&
      tempName.substr( tempName.length() - bExtName.length() )==bExtName )
    {
    this->Internal->MajorFileName =
       tempName.substr( 0, tempName.length() - bExtName.length() );
    this->Internal->BoundaryFileName  = tempName;
    this->Internal->HierarchyFileName =
       this->Internal->MajorFileName + hExtName;
    }
  else
   {
   vtkErrorMacro( "Enzo file has invalid extension!");
   return;
   }

   this->Internal->DirectoryName =
       GetEnzoDirectory(this->Internal->MajorFileName.c_str());

  this->Internal->ReadMetaData();
  this->Internal->CheckAttributeNames();

  this->NumberOfBlocks = this->Internal->NumberOfBlocks;
  this->Initialized    = true;

  this->SetupParticleDataSelections();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMREnzoParticlesReader::GetParticlesTypeArray(
    const int blockIdx )
{

  vtkIntArray *array = vtkIntArray::New();
  if( this->ParticleDataArraySelection->ArrayExists( "particle_type" ) )
    {
    this->Internal->LoadAttribute( "particle_type", blockIdx );
    array->DeepCopy( this->Internal->DataArray );
    }
  return( array );
}

//------------------------------------------------------------------------------
bool vtkAMREnzoParticlesReader::CheckParticleType(
    const int idx, vtkIntArray *ptypes )
{
  assert( "pre: particles type array should not be NULL" && (ptypes != NULL) );

  if( ptypes->GetNumberOfTuples() > 0 &&
      this->ParticleDataArraySelection->ArrayExists( "particle_type" ) )
    {
    int ptype = ptypes->GetValue( idx );
    if( (this->ParticleType==0) || (ptype==this->ParticleType) )
      {
      return true;
      }
    else
      {
      return false;
      }
    }
  else
    {
    return true;
    }
}

//------------------------------------------------------------------------------
vtkPolyData* vtkAMREnzoParticlesReader::GetParticles(
    const char* file, const int blockIdx )
{
  vtkPolyData *particles = vtkPolyData::New();
  vtkPoints   *positions = vtkPoints::New();
  positions->SetDataTypeToDouble();
  vtkPointData *pdata    = particles->GetPointData();

  hid_t fileIndx = H5Fopen( file, H5F_ACC_RDONLY, H5P_DEFAULT );
  if( fileIndx < 0 )
    {
    vtkErrorMacro( "Failed opening particles file!" );
    return NULL;
    }

  hid_t rootIndx;
  if( ! FindBlockIndex( fileIndx, blockIdx+1,rootIndx ) )
    {
    vtkErrorMacro( "Could not locate target block!" );
    return NULL;
    }

  //
  // Load the particles position arrays by name.
  // In Enzo the following arrays are available:
  //  ( 1 ) particle_position_i
  //  ( 2 ) tracer_particle_position_i
  //
  // where i \in {x,y,z}.
  std::vector< double > xcoords;
  std::vector< double > ycoords;
  std::vector< double > zcoords;

  // TODO: should we handle 2-D particle datasets?
  GetDoubleArrayByName( rootIndx, "particle_position_x", xcoords );
  GetDoubleArrayByName( rootIndx, "particle_position_y", ycoords );
  GetDoubleArrayByName( rootIndx, "particle_position_z", zcoords );

  vtkIntArray *particleTypes = vtkIntArray::SafeDownCast(
      this->GetParticlesTypeArray( blockIdx ) );

  assert( "Coordinate arrays must have the same size: " &&
           (xcoords.size()==ycoords.size() ) );
  assert( "Coordinate arrays must have the same size: " &&
           (ycoords.size()==zcoords.size() ) );

  int TotalNumberOfParticles = static_cast<int>(xcoords.size());
  positions->SetNumberOfPoints( TotalNumberOfParticles );

  vtkIdList *ids = vtkIdList::New();
  ids->SetNumberOfIds( TotalNumberOfParticles );

  vtkIdType NumberOfParticlesLoaded = 0;
  for( int i=0; i < TotalNumberOfParticles; ++i )
    {
    if( (i%this->Frequency) == 0  )
      {
      if( this->CheckLocation( xcoords[i], ycoords[i],zcoords[i] ) &&
          this->CheckParticleType( i, particleTypes) )
        {
        int pidx = NumberOfParticlesLoaded;
        ids->InsertId( pidx, i );
        positions->SetPoint( pidx, xcoords[i],ycoords[i],zcoords[i] );
        ++NumberOfParticlesLoaded;
        } // END if within requested region
      } // END if within requested interval
    } // END for all particles
  H5Gclose( rootIndx );
  H5Fclose( fileIndx );

  ids->SetNumberOfIds( NumberOfParticlesLoaded );
  ids->Squeeze();

  positions->SetNumberOfPoints( NumberOfParticlesLoaded );
  positions->Squeeze();

  particles->SetPoints( positions );
  positions->Delete();

  // Create CellArray consisting of a single polyvertex cell
  vtkCellArray *polyVertex     = vtkCellArray::New();

  polyVertex->InsertNextCell( NumberOfParticlesLoaded  );
  for( vtkIdType idx=0; idx < NumberOfParticlesLoaded; ++idx )
    polyVertex->InsertCellPoint( idx );

  particles->SetVerts( polyVertex );
  polyVertex->Delete();

  // Release the particle types array
  particleTypes->Delete();

  int numArrays = this->ParticleDataArraySelection->GetNumberOfArrays();
  for( int i=0; i < numArrays; ++i )
    {
    const char* name = this->ParticleDataArraySelection->GetArrayName( i );
    if( this->ParticleDataArraySelection->ArrayIsEnabled( name ) )
      {
      // Note: 0-based indexing is used for loading particles
      this->Internal->LoadAttribute( name, blockIdx );
      assert( "pre: particle attribute size mismatch" &&
         (this->Internal->DataArray->GetNumberOfTuples()==
          TotalNumberOfParticles) );

      vtkDataArray *array = this->Internal->DataArray->NewInstance();
      array->SetName(this->Internal->DataArray->GetName( )  );
      array->SetNumberOfTuples( NumberOfParticlesLoaded );
      array->SetNumberOfComponents(
          this->Internal->DataArray->GetNumberOfComponents() );

      vtkIdType numIds = ids->GetNumberOfIds();
      for( vtkIdType pidx=0; pidx < numIds; ++pidx )
        {
          vtkIdType particleIdx = ids->GetId( pidx );
          int numComponents = array->GetNumberOfComponents();
          for( int k=0; k < numComponents; ++k )
            {
              array->SetComponent( pidx, k,
                  this->Internal->DataArray->GetComponent(
                      particleIdx,k ) );
            } // END for all components
        } // END for all ids of loaded particles
      pdata->AddArray( array );
      array->Delete();
      } // END if the array is supposed to be loaded
    } // END for all particle arrays

  ids->Delete();
  return( particles );
}

//------------------------------------------------------------------------------
void vtkAMREnzoParticlesReader::SetupParticleDataSelections()
{
  assert( "pre: Intenal reader is NULL" && (this->Internal != NULL) );

  unsigned int N =
    static_cast<unsigned int>( this->Internal->ParticleAttributeNames.size() );
  for( unsigned int i=0; i < N; ++i )
    {
    bool isParticleAttribute =
     vtksys::SystemTools::StringStartsWith(
       this->Internal->ParticleAttributeNames[ i ].c_str( ),
        "particle_" );

    if( isParticleAttribute )
      {
      this->ParticleDataArraySelection->AddArray(
          this->Internal->ParticleAttributeNames[ i ].c_str() );
      }
    } // END for all particles attributes
  this->InitializeParticleDataSelections();
}

//------------------------------------------------------------------------------
int vtkAMREnzoParticlesReader::GetTotalNumberOfParticles( )
{
  assert( "Internal reader is null" && (this->Internal!=NULL) );
  int numParticles = 0;
  for( int blockIdx=0; blockIdx < this->NumberOfBlocks; ++blockIdx )
    {
    numParticles += this->Internal->Blocks[ blockIdx ].NumberOfParticles;
    }
  return( numParticles );
}

//------------------------------------------------------------------------------
vtkPolyData* vtkAMREnzoParticlesReader::ReadParticles(const int blkidx)
{
  // this->Internal->Blocks includes a pseudo block -- the roo as block #0
  int iBlockIdx    = blkidx+1;
  int NumParticles = this->Internal->Blocks[ iBlockIdx ].NumberOfParticles;

  if( NumParticles <= 0 )
    {
    vtkPolyData* emptyParticles = vtkPolyData::New();
    assert( "Cannot create particles dataset" && ( emptyParticles != NULL ) );
    return( emptyParticles );
    }

  std::string pfile = this->Internal->Blocks[iBlockIdx].ParticleFileName;
  if( pfile == "" )
    {
    vtkErrorMacro( "No particles file found, string is empty!" );
    return NULL;
    }

  vtkPolyData* particles = this->GetParticles(pfile.c_str(), blkidx  );
  return( particles );
}
