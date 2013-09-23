/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMREnzoReaderInternal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMREnzoReaderInternal.h"

#define H5_USE_16_API
#include "vtk_hdf5.h"        // for the HDF5 library

#include "vtksys/SystemTools.hxx"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"

// ----------------------------------------------------------------------------
//                       Functions for Parsing File Names
// ----------------------------------------------------------------------------


static std::string GetEnzoMajorFileName( const char * path )
{
  return(vtksys::SystemTools::GetFilenameName( std::string( path ) ) );
}

// ----------------------------------------------------------------------------
//                       Class vtkEnzoReaderBlock (begin)
// ----------------------------------------------------------------------------
//------------------------------------------------------------------------------
void vtkEnzoReaderBlock::Init()
{
  this->BlockFileName    = "";
  this->ParticleFileName = "";

  this->Index    = -1;
  this->Level    = -1;
  this->ParentId = -1;
  this->ChildrenIds.clear();
  this->NumberOfParticles  = 0;
  this->NumberOfDimensions = 0;

  this->MinParentWiseIds[0] =
  this->MinParentWiseIds[1] =
  this->MinParentWiseIds[2] =
  this->MaxParentWiseIds[0] =
  this->MaxParentWiseIds[1] =
  this->MaxParentWiseIds[2] = -1;

  this->MinLevelBasedIds[0] =
  this->MinLevelBasedIds[1] =
  this->MinLevelBasedIds[2] =
  this->MaxLevelBasedIds[0] =
  this->MaxLevelBasedIds[1] =
  this->MaxLevelBasedIds[2] = -1;

  this->BlockCellDimensions[0] =
  this->BlockCellDimensions[1] =
  this->BlockCellDimensions[2] =
  this->BlockNodeDimensions[0] =
  this->BlockNodeDimensions[1] =
  this->BlockNodeDimensions[2] = 0;

  this->MinBounds[0] =
  this->MinBounds[1] =
  this->MinBounds[2] = VTK_DOUBLE_MAX;
  this->MaxBounds[0] =
  this->MaxBounds[1] =
  this->MaxBounds[2] =-VTK_DOUBLE_MAX;

  this->SubdivisionRatio[0] =
  this->SubdivisionRatio[1] =
  this->SubdivisionRatio[2] = 1.0;
}

//------------------------------------------------------------------------------
void vtkEnzoReaderBlock::DeepCopy(const vtkEnzoReaderBlock *other)
{
  this->BlockFileName    = other->BlockFileName;
  this->ParticleFileName = other->ParticleFileName;

  this->Index    = other->Index;
  this->Level    = other->Level;
  this->ParentId = other->ParentId;
  this->ChildrenIds = other->ChildrenIds;
  this->NumberOfParticles  = other->NumberOfParticles;
  this->NumberOfDimensions = other->NumberOfDimensions;

  this->MinParentWiseIds[0] = other->MinParentWiseIds[0];
  this->MinParentWiseIds[1] = other->MinParentWiseIds[1];
  this->MinParentWiseIds[2] = other->MinParentWiseIds[2];
  this->MaxParentWiseIds[0] = other->MaxParentWiseIds[0];
  this->MaxParentWiseIds[1] = other->MaxParentWiseIds[1];
  this->MaxParentWiseIds[2] = other->MaxParentWiseIds[2];

  this->MinLevelBasedIds[0] = other->MinLevelBasedIds[0];
  this->MinLevelBasedIds[1] = other->MinLevelBasedIds[1];
  this->MinLevelBasedIds[2] = other->MinLevelBasedIds[2];
  this->MaxLevelBasedIds[0] = other->MaxLevelBasedIds[0];
  this->MaxLevelBasedIds[1] = other->MaxLevelBasedIds[1];
  this->MaxLevelBasedIds[2] = other->MaxLevelBasedIds[2];

  this->BlockCellDimensions[0] = other->BlockCellDimensions[0];
  this->BlockCellDimensions[1] = other->BlockCellDimensions[1];
  this->BlockCellDimensions[2] = other->BlockCellDimensions[2];
  this->BlockNodeDimensions[0] = other->BlockNodeDimensions[0];
  this->BlockNodeDimensions[1] = other->BlockNodeDimensions[1];
  this->BlockNodeDimensions[2] = other->BlockNodeDimensions[2];

  this->MinBounds[0] = other->MinBounds[0];
  this->MinBounds[1] = other->MinBounds[1];
  this->MinBounds[2] = other->MinBounds[2];
  this->MaxBounds[0] = other->MaxBounds[0];
  this->MaxBounds[1] = other->MaxBounds[1];
  this->MaxBounds[2] = other->MaxBounds[2];

  this->SubdivisionRatio[0] = other->SubdivisionRatio[0];
  this->SubdivisionRatio[1] = other->SubdivisionRatio[1];
  this->SubdivisionRatio[2] = other->SubdivisionRatio[2];
}

//------------------------------------------------------------------------------
// get the bounding (cell) Ids of this block in terms of its parent block's
// sub-division resolution (indexing is limited to the scope of the parent)
void vtkEnzoReaderBlock::GetParentWiseIds
  (  std::vector< vtkEnzoReaderBlock > & blocks  )
{
  if ( this->ParentId != 0 )
    {
    // the parent is not the root and then we need to determine the offset
    // (in terms of the number of parent divisions / cells) of the current
    // block's beginning / ending position relative to the parent block's
    // beginning position
    vtkEnzoReaderBlock & parent = blocks[ this->ParentId ];
    this->MinParentWiseIds[0]   = static_cast < int >
          (  0.5 + parent.BlockCellDimensions[0]
                 * ( this->MinBounds[0]  - parent.MinBounds[0] )
                 / ( parent.MaxBounds[0] - parent.MinBounds[0] )  );
    this->MaxParentWiseIds[0]   = static_cast < int >
          (  0.5 +  parent.BlockCellDimensions[0]
                 * ( this->MaxBounds[0]  - parent.MinBounds[0] )
                 / ( parent.MaxBounds[0] - parent.MinBounds[0] )  );

    this->MinParentWiseIds[1]   = static_cast < int >
          (  0.5 + parent.BlockCellDimensions[1]
                 * ( this->MinBounds[1]  - parent.MinBounds[1] )
                 / ( parent.MaxBounds[1] - parent.MinBounds[1] )  );
    this->MaxParentWiseIds[1]   = static_cast < int >
          (  0.5 + parent.BlockCellDimensions[1]
                 * ( this->MaxBounds[1]  - parent.MinBounds[1] )
                 / ( parent.MaxBounds[1] - parent.MinBounds[1] )  );

    if ( this->NumberOfDimensions == 3 )
      {
      this->MinParentWiseIds[2] = static_cast < int >
          (  0.5 + parent.BlockCellDimensions[2]
                 * ( this->MinBounds[2]  - parent.MinBounds[2] )
                 / ( parent.MaxBounds[2] - parent.MinBounds[2] )  );
      this->MaxParentWiseIds[2] = static_cast < int >
          (  0.5 + parent.BlockCellDimensions[2]
                 * ( this->MaxBounds[2]  - parent.MinBounds[2] )
                 / ( parent.MaxBounds[2] - parent.MinBounds[2] )  );
      }
    else
      {
      this->MinParentWiseIds[2] = 0;
      this->MaxParentWiseIds[2] = 0;
      }

    // the ratio for mapping two parent-wise Ids to 0 and
    // this->BlockCellDimension[i],
    // respectively, while the same region is covered
    this->SubdivisionRatio[0] = static_cast < double >
          ( this->BlockCellDimensions[0] ) /
          ( this->MaxParentWiseIds[0] - this->MinParentWiseIds[0] );
    this->SubdivisionRatio[1] = static_cast < double >
          ( this->BlockCellDimensions[1] ) /
          ( this->MaxParentWiseIds[1] - this->MinParentWiseIds[1] );

    if ( this->NumberOfDimensions == 3 )
      {
      this->SubdivisionRatio[2] = static_cast < double >
          ( this->BlockCellDimensions[2] ) /
          ( this->MaxParentWiseIds[2] - this->MinParentWiseIds[2] );
      }
    else
      {
      this->SubdivisionRatio[2] = 1.0;
      }
    }
  else
    {
    // Now that the parent is the root, it can not provide cell-dimensions
    // information (BlockCellDimensions[0 .. 2]) directly, as the above does.
    // Fortunately we can obtain it according to the spatial ratio of the
    // child block (the current one) to the parent (root) and the child block's
    // cell-dimensions information. This derivation is based on the definition
    // of 'level' that all children blocks at the same level (e.g., the current
    // block and its siblings) share the same sub-division ratio relative to
    // their parent (the root herein).
    vtkEnzoReaderBlock & block0 = blocks[0];

    double xRatio = ( this->MaxBounds[0]  - this->MinBounds[0]  ) /
                    ( block0.MaxBounds[0] - block0.MinBounds[0] );
    this->MinParentWiseIds[0] = static_cast < int >
          (  0.5 + ( this->BlockCellDimensions[0] / xRatio ) // parent's dim
                 * ( this->MinBounds[0]  - block0.MinBounds[0] )
                 / ( block0.MaxBounds[0] - block0.MinBounds[0] )
          );
    this->MaxParentWiseIds[0] = static_cast < int >
          (  0.5 + ( this->BlockCellDimensions[0] / xRatio )
                 * ( this->MaxBounds[0]  - block0.MinBounds[0] )
                 / ( block0.MaxBounds[0] - block0.MinBounds[0] )
          );

    double yRatio = ( this->MaxBounds[1]  - this->MinBounds[1]  ) /
                    ( block0.MaxBounds[1] - block0.MinBounds[1] );
    this->MinParentWiseIds[1] = static_cast < int >
          (  0.5 + ( this->BlockCellDimensions[1] / yRatio )
                 * ( this->MinBounds[1]  - block0.MinBounds[1] )
                 / ( block0.MaxBounds[1] - block0.MinBounds[1] )
          );
    this->MaxParentWiseIds[1] = static_cast < int >
          (  0.5 + ( this->BlockCellDimensions[1] / yRatio )
                 * ( this->MaxBounds[1]  - block0.MinBounds[1] )
                 / ( block0.MaxBounds[1] - block0.MinBounds[1] )
          );

    if ( this->NumberOfDimensions == 3 )
      {
      double zRatio = ( this->MaxBounds[2]  - this->MinBounds[2]  ) /
                      ( block0.MaxBounds[2] - block0.MinBounds[2] );
      this->MinParentWiseIds[2] = static_cast < int >
          (  0.5 + ( this->BlockCellDimensions[2] / zRatio )
                 * ( this->MinBounds[2]  - block0.MinBounds[2] )
                 / ( block0.MaxBounds[2] - block0.MinBounds[2] )
          );
      this->MaxParentWiseIds[2] = static_cast < int >
          (  0.5 + ( this->BlockCellDimensions[2] / zRatio )
                 * ( this->MaxBounds[2]  - block0.MinBounds[2] )
                 / ( block0.MaxBounds[2] - block0.MinBounds[2] )
          );
      }
    else
      {
      this->MinParentWiseIds[2] = 0;
      this->MaxParentWiseIds[2] = 0;
      }

    this->SubdivisionRatio[0] = 1.0;
    this->SubdivisionRatio[1] = 1.0;
    this->SubdivisionRatio[2] = 1.0;
    }
}

//------------------------------------------------------------------------------
void vtkEnzoReaderBlock::GetLevelBasedIds
  (  std::vector< vtkEnzoReaderBlock > & blocks  )
{
  // note that this function is invoked from the root in a top-down manner
  // and the parent-wise Ids have been determined in advance

  if ( this->ParentId != 0 )
    {
    // the parent is not the root and therefore we need to exploit the level-
    // based Ids of the parent, of which the shifted verson is multiplied by
    // the refinement ratio

    vtkEnzoReaderBlock & parent = blocks[ this->ParentId ];
    this->MinLevelBasedIds[0] = static_cast < int >
                                (  ( parent.MinLevelBasedIds[0]
                                     + this->MinParentWiseIds[0]
                                   ) * this->SubdivisionRatio[0]
                                );

    this->MinLevelBasedIds[1] = static_cast < int >
                                (  ( parent.MinLevelBasedIds[1]
                                     + this->MinParentWiseIds[1]
                                   ) * this->SubdivisionRatio[1]
                                );
    this->MinLevelBasedIds[2] = static_cast < int >
                                (  ( parent.MinLevelBasedIds[2]
                                     + this->MinParentWiseIds[2]
                                   ) * this->SubdivisionRatio[2]
                                );

    this->MaxLevelBasedIds[0] = static_cast < int >
                                (  ( parent.MinLevelBasedIds[0]
                                     + this->MaxParentWiseIds[0]
                                   ) * this->SubdivisionRatio[0]
                                );
    this->MaxLevelBasedIds[1] = static_cast < int >
                                (  ( parent.MinLevelBasedIds[1]
                                     + this->MaxParentWiseIds[1]
                                   ) * this->SubdivisionRatio[1]
                                );
    this->MaxLevelBasedIds[2] = static_cast < int >
                                (  ( parent.MinLevelBasedIds[2]
                                     + this->MaxParentWiseIds[2]
                                   ) * this->SubdivisionRatio[2]
                                );
    }
  else
    {
    // now that the parent is the root, the parent-wise Ids
    // are just the level-based Ids
    this->MinLevelBasedIds[0] = this->MinParentWiseIds[0];
    this->MinLevelBasedIds[1] = this->MinParentWiseIds[1];
    this->MinLevelBasedIds[2] = this->MinParentWiseIds[2];

    this->MaxLevelBasedIds[0] = this->MaxParentWiseIds[0];
    this->MaxLevelBasedIds[1] = this->MaxParentWiseIds[1];
    this->MaxLevelBasedIds[2] = this->MaxParentWiseIds[2];
    }
}
//------------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                       Class vtkEnzoReaderBlock ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  vtkEnzoReaderInternal (begin)
// ----------------------------------------------------------------------------

vtkEnzoReaderInternal::vtkEnzoReaderInternal()
{
  this->Init();
}

//------------------------------------------------------------------------------
vtkEnzoReaderInternal::~vtkEnzoReaderInternal()
{
  this->ReleaseDataArray();
  this->Init();
  this->FileName = NULL;
}

//------------------------------------------------------------------------------
void vtkEnzoReaderInternal::Init()
{
  this->DataTime   = 0.0;
  this->FileName   = NULL;
  //this->TheReader  = NULL;
  this->DataArray  = NULL;
  this->CycleIndex = 0;

  this->ReferenceBlock = 0;
  this->NumberOfBlocks = 0;
  this->NumberOfLevels = 0;
  this->NumberOfDimensions  = 0;
  this->NumberOfMultiBlocks = 0;

  this->DirectoryName = "";
  this->MajorFileName = "";
  this->BoundaryFileName  = "";
  this->HierarchyFileName = "";

  this->Blocks.clear();
  this->BlockAttributeNames.clear();
  this->ParticleAttributeNames.clear();
  this->TracerParticleAttributeNames.clear();
}

//------------------------------------------------------------------------------
void vtkEnzoReaderInternal::ReleaseDataArray()
{
  if( this->DataArray )
    {
    this->DataArray->Delete();
    this->DataArray = NULL;
    }
}

//------------------------------------------------------------------------------
int vtkEnzoReaderInternal::GetBlockAttribute(
    const char *atribute, int blockIdx, vtkDataSet *pDataSet )
{

  // this function must be called by GetBlock( ... )
  this->ReadMetaData();

  if ( atribute == NULL || blockIdx < 0  ||
       pDataSet == NULL || blockIdx >= this->NumberOfBlocks )
    {
    return 0;
    }

  // try obtaining the attribute and attaching it to the grid as a cell data
  // NOTE: the 'equal' comparison below is MUST because in some cases (such
  // as cosmological datasets) not all rectilinear blocks contain an assumably
  // common block attribute. This is the case where such a block attribute
  // may result from a particles-to-cells interpolation process. Thus those
  // blocks with particles (e.g., the reference one with the fewest cells)
  // contain it as a block attribute, whereas others without particles just
  // do not contain this block attribute.
  int   succeded = 0;
  if (  this->LoadAttribute( atribute, blockIdx ) &&
        ( pDataSet->GetNumberOfCells() ==
          this->DataArray->GetNumberOfTuples() )
     )
    {
    succeded = 1;
    pDataSet->GetCellData()->AddArray( this->DataArray );
    this->ReleaseDataArray();
    }

  return succeded;
}

//------------------------------------------------------------------------------
int vtkEnzoReaderInternal::LoadAttribute( const char *atribute, int blockIdx )
{
  // TODO: implement this
  // called by GetBlockAttribute( ... ) or GetParticlesAttribute()
  this->ReadMetaData();

  if ( atribute == NULL || blockIdx < 0  ||
       blockIdx >= this->NumberOfBlocks )
    {
    return 0;
    }

  // this->Internal->Blocks includes a pseudo block --- the root as block #0
  blockIdx ++;

  // currently only the HDF5 file format is supported
  std::string blckFile = this->Blocks[ blockIdx ].BlockFileName;
  hid_t  fileIndx = H5Fopen( blckFile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT );
  if( fileIndx < 0 )
    {
    return 0;
    }

  // retrieve the contents of the root directory to look for a group
  // corresponding to the target block, if available, open that group

  int     blckIndx;
  char    blckName[65];
  int     objIndex;
  hsize_t numbObjs;
  hid_t   rootIndx = H5Gopen( fileIndx, "/" );
  H5Gget_num_objs( rootIndx, &numbObjs );
  for ( objIndex=0; objIndex < static_cast < int >( numbObjs ); objIndex ++  )
    {
    int type = H5Gget_objtype_by_idx( rootIndx, objIndex );
    if ( type == H5G_GROUP )
      {
      H5Gget_objname_by_idx( rootIndx, objIndex, blckName, 64 );
      if ( (  sscanf( blckName, "Grid%d", &blckIndx )  ==  1  )  &&
           (  (blckIndx  ==  blockIdx) || (blckIndx == blockIdx+1) ) )
        {
        // located the target block
        rootIndx = H5Gopen( rootIndx, blckName );
        break;
        }
      }
    } // END for all objects

  // disable error messages while looking for the attribute (if any) name
  // and enable error messages when it is done
  void       * pContext = NULL;
  H5E_auto_t   erorFunc;
  H5Eget_auto( &erorFunc, &pContext );
  H5Eset_auto( NULL, NULL );

  hid_t        attrIndx = H5Dopen( rootIndx, atribute );

  H5Eset_auto( erorFunc, pContext );
  pContext = NULL;

  // check if the data attribute exists
  if ( attrIndx < 0 )
    {
    vtkGenericWarningMacro(
     "Attribute (" << atribute << ") data does not exist in file "
     << blckFile.c_str() );
    H5Gclose( rootIndx );
    H5Fclose( fileIndx );
    return 0;
    }

  // get cell dimensions and the valid number
  hsize_t cellDims[3];
  hid_t   spaceIdx = H5Dget_space( attrIndx );
  H5Sget_simple_extent_dims( spaceIdx, cellDims, NULL );
  hsize_t numbDims = H5Sget_simple_extent_ndims( spaceIdx );

  // number of attribute tuples = number of cells (or particles)
  int numTupls = 0;
  switch( numbDims )
    {
    case 1:
      numTupls = cellDims[0];
      break;
    case 2:
      numTupls = cellDims[0] * cellDims[1];
      break;
    case 3:
      numTupls = cellDims[0] * cellDims[1] * cellDims[2];
      break;
    default:
      H5Gclose( spaceIdx );
      H5Fclose( attrIndx );
      H5Gclose( rootIndx );
      H5Fclose( fileIndx );
      return 0;
    }

  // determine the data type, load the values, and, if necessary, convert
  // the raw data to double type
  // DOUBLE / FLOAT  / INT  / UINT  / CHAR  / UCHAR
  // SHORT  / USHORT / LONG / ULONG / LLONG / ULLONG / LDOUBLE

  this->ReleaseDataArray(); // data array maintained by internal

  hid_t tRawType = H5Dget_type( attrIndx );
  hid_t dataType = H5Tget_native_type( tRawType, H5T_DIR_ASCEND );
  if (  H5Tequal( dataType, H5T_NATIVE_FLOAT )  )
    {
    this->DataArray = vtkFloatArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    float  * arrayPtr = static_cast < float * >
     (vtkFloatArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_DOUBLE )  )
    {
    this->DataArray = vtkDoubleArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    double  * arrayPtr = static_cast < double * >
     (vtkDoubleArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_INT )  )
    {
    this->DataArray = vtkIntArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    int  * arrayPtr = static_cast < int * >
     (vtkIntArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_UINT )  )
    {
    this->DataArray = vtkUnsignedIntArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    unsigned int  * arrayPtr = static_cast < unsigned int * >
     (vtkUnsignedIntArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_SHORT )  )
    {
    this->DataArray = vtkShortArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    short  * arrayPtr = static_cast < short * >
     (vtkShortArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_USHORT )  )
    {
    this->DataArray = vtkUnsignedShortArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    unsigned short  * arrayPtr = static_cast < unsigned short * >
     (vtkUnsignedShortArray::SafeDownCast( this->DataArray )->GetPointer( 0 ));
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_UCHAR )  )
    {

    this->DataArray = vtkUnsignedCharArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    unsigned char  * arrayPtr = static_cast < unsigned char * >
     (vtkUnsignedCharArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_LONG )  )
    {
    this->DataArray = vtkLongArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    long  * arrayPtr = static_cast < long * >
     (vtkLongArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
  if (  H5Tequal( dataType, H5T_NATIVE_LLONG )  )
    {
    this->DataArray = vtkLongLongArray::New();
    this->DataArray->SetNumberOfTuples( numTupls );
    long long  * arrayPtr = static_cast < long long * >
     ( vtkLongLongArray::SafeDownCast( this->DataArray )->GetPointer( 0 )  );
    H5Dread( attrIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr );
    arrayPtr = NULL;
    }
  else
    {
//        vtkErrorMacro( "Unknown HDF5 data type --- it is not FLOAT, "       <<
//                       "DOUBLE, INT, UNSIGNED INT, SHORT, UNSIGNED SHORT, " <<
//                       "UNSIGNED CHAR, LONG, or LONG LONG." << endl );
    H5Tclose( dataType );
    H5Tclose( tRawType );
    H5Tclose( spaceIdx );
    H5Dclose( attrIndx );
    H5Gclose( rootIndx );
    H5Fclose( fileIndx );
    return 0;
    }


  // do not forget to provide a name for the array
  this->DataArray->SetName( atribute );

// This close statements cause a crash!
//  H5Tclose( dataType );
//  H5Tclose( tRawType );
//  H5Tclose( spaceIdx );
//  H5Dclose( attrIndx );
//  H5Gclose( rootIndx );
//  H5Fclose( fileIndx );
  return 1;
}

//------------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// parse the hierarchy file to create block structures, including the bounding
// box, cell dimensions, grid / node dimensions, number of particles, level Id,
// block file name, and particle file name of each block
void vtkEnzoReaderInternal::ReadBlockStructures()
{
  ifstream stream( this->HierarchyFileName.c_str() );
  if ( !stream )
    {
    vtkGenericWarningMacro( "Invalid hierarchy file name: " <<
                            this->HierarchyFileName.c_str() << endl );
    return;
    }

  // init the root block, addressing only 4 four fields
  vtkEnzoReaderBlock  block0;
  block0.Index    = 0;
  block0.Level    =-1;
  block0.ParentId =-1;
  block0.NumberOfDimensions  = this->NumberOfDimensions;
  this->Blocks.push_back( block0 );

  int     levlId = 0;
  int     parent = 0;
  std::string   theStr = "";

  while ( stream )
    {
    while ( stream &&
            theStr != "Grid" &&
            theStr != "Time" &&
            theStr != "Pointer:"
          )
      {
      stream >> theStr;
      }

    // block information
    if ( theStr == "Grid" )
      {
      vtkEnzoReaderBlock tmpBlk;
      tmpBlk.NumberOfDimensions = this->NumberOfDimensions;

      stream >> theStr; // '='
      stream >> tmpBlk.Index;

      // the starting and ending (cell --- not node) Ids of the block
      int     minIds[3];
      int     maxIds[3];
      while ( theStr != "GridStartIndex" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='

      if ( this->NumberOfDimensions == 3 )
        {
        stream >> minIds[0] >> minIds[1] >> minIds[2];
        }
      else
        {
        stream >> minIds[0] >> minIds[1];
        }

      while ( theStr != "GridEndIndex" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='

      if ( this->NumberOfDimensions == 3 )
        {
        stream >> maxIds[0] >> maxIds[1] >> maxIds[2];
        }
      else
        {
        stream >> maxIds[0] >> maxIds[1];
        }

      // the cell dimensions of the block
      tmpBlk.BlockCellDimensions[0] = maxIds[0] - minIds[0] + 1;
      tmpBlk.BlockCellDimensions[1] = maxIds[1] - minIds[1] + 1;
      if ( this->NumberOfDimensions == 3 )
        {
        tmpBlk.BlockCellDimensions[2] = maxIds[2] - minIds[2] + 1;
        }
      else
        {
        tmpBlk.BlockCellDimensions[2] = 1;
        }

      // the grid (node --- not means the block) dimensions of the block
      tmpBlk.BlockNodeDimensions[0] = tmpBlk.BlockCellDimensions[0] + 1;
      tmpBlk.BlockNodeDimensions[1] = tmpBlk.BlockCellDimensions[1] + 1;
      if ( this->NumberOfDimensions == 3 )
        {
        tmpBlk.BlockNodeDimensions[2] = tmpBlk.BlockCellDimensions[2] + 1;
        }
      else
        {
        tmpBlk.BlockNodeDimensions[2] = 1;
        }

      // the min bounding box of the block
      while ( theStr != "GridLeftEdge" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='

      if ( this->NumberOfDimensions == 3 )
        {
        stream >> tmpBlk.MinBounds[0]
               >> tmpBlk.MinBounds[1] >> tmpBlk.MinBounds[2];
        }
      else
        {
        tmpBlk.MinBounds[2] = 0;
        stream >> tmpBlk.MinBounds[0] >> tmpBlk.MinBounds[1];
        }

      // the max bounding box of the block
      while ( theStr != "GridRightEdge" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='

      if ( this->NumberOfDimensions == 3 )
        {
        stream >> tmpBlk.MaxBounds[0]
               >> tmpBlk.MaxBounds[1] >> tmpBlk.MaxBounds[2];
        }
      else
        {
        tmpBlk.MaxBounds[2] = 0;
        stream >> tmpBlk.MaxBounds[0] >> tmpBlk.MaxBounds[1];
        }

      // obtain the block file name (szName includes the full path)
      std::string    szName;
      while ( theStr != "BaryonFileName" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='
      stream >> szName;

//      std::cout << "szname: " << szName.c_str() << std::endl;
//      std::cout.flush();
      tmpBlk.BlockFileName =
          this->DirectoryName + "/" + GetEnzoMajorFileName(szName.c_str());

      // obtain the particle file name (szName includes the full path)
      while ( theStr != "NumberOfParticles" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='
      stream >> tmpBlk.NumberOfParticles;

      if ( tmpBlk.NumberOfParticles > 0 )
        {
        while ( theStr != "ParticleFileName" )
          {
          stream >> theStr;
          }
        stream >> theStr; // '='
        stream >> szName;
        tmpBlk.ParticleFileName =
            this->DirectoryName + "/" +GetEnzoMajorFileName(szName.c_str());
        }

      tmpBlk.Level    = levlId;
      tmpBlk.ParentId = parent;

      if (  static_cast < int > ( this->Blocks.size() )  !=  tmpBlk.Index  )
        {
        vtkGenericWarningMacro( "The blocks in the hierarchy file " <<
                                this->HierarchyFileName.c_str()     <<
                                " are currently expected to be "    <<
                                " listed in order."                 << endl );
        return;
        }

      this->Blocks.push_back( tmpBlk );
      this->Blocks[parent].ChildrenIds.push_back( tmpBlk.Index );
      this->NumberOfBlocks = static_cast < int > ( this->Blocks.size() ) - 1;
      }
    else
    if ( theStr == "Pointer:" )
      {
      theStr = "";
      int    tmpInt;
      char   tmpChr;
      while (  ( tmpChr = stream.get() )  !=  '['  ) ;
      while (  ( tmpChr = stream.get() )  !=  ']'  ) theStr += tmpChr;

      int    blkIdx = atoi( theStr.c_str() );
      stream.get(); // -
      stream.get(); // >
      stream >> theStr;
      if ( theStr == "NextGridNextLevel" )
        {
        stream >> theStr; // '='
        stream >> tmpInt;
        if ( tmpInt != 0 )
          {
          levlId = this->Blocks[blkIdx].Level + 1;
          this->NumberOfLevels = ( levlId+1 > this->NumberOfLevels )
                               ? ( levlId+1 ) : this->NumberOfLevels;
          parent = blkIdx;
          }
        }
      else // theStr == "NextGridThisLevel"
        {
        stream >> theStr; // '='
        stream >> tmpInt;
        }
      }
    else
    if ( theStr == "Time" )
      {
      stream >> theStr; // '='
      stream >> this->DataTime;
      }

    stream >> theStr;
    }

  stream.close();
}

//------------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// obtain the general information of the dataset (number of dimensions)
void vtkEnzoReaderInternal::ReadGeneralParameters()
{
  ifstream stream( this->MajorFileName.c_str() );
  if ( !stream )
    {
    vtkGenericWarningMacro( "Invalid parameter file " <<
                            this->MajorFileName.c_str() << endl );
    return;
    }

  std::string tmpStr( "" );
  while ( stream )
    {
    stream >> tmpStr;

    if ( tmpStr == "InitialCycleNumber" )
      {
      stream >> tmpStr; // '='
      stream >> this->CycleIndex;
      }
    else
    if ( tmpStr == "InitialTime" )
      {
      stream >> tmpStr; // '='
      stream >> this->DataTime;
      }
    else
    if ( tmpStr == "TopGridRank" )
      {
      stream >> tmpStr; // '='
      stream >> this->NumberOfDimensions;
      }
    }

  stream.close();
}

//------------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// get the bounding box of the root block based on those of its descendants
void vtkEnzoReaderInternal::DetermineRootBoundingBox()
{
  vtkEnzoReaderBlock & block0 = this->Blocks[0];

  // now loop over all level zero grids
  for ( int blkIdx = 1; blkIdx <= this->NumberOfBlocks &&
                        this->Blocks[blkIdx].ParentId == 0; blkIdx ++ )
  for ( int dimIdx = 0; dimIdx <  this->NumberOfDimensions; dimIdx ++ )
    {
    block0.MinBounds[dimIdx] =
    ( this->Blocks[ blkIdx ].MinBounds[ dimIdx ] < block0.MinBounds[ dimIdx ] )
    ? this->Blocks[ blkIdx ].MinBounds[ dimIdx ] : block0.MinBounds[ dimIdx ];

    block0.MaxBounds[dimIdx] =
    ( this->Blocks[ blkIdx ].MaxBounds[ dimIdx ] > block0.MaxBounds[ dimIdx ] )
    ? this->Blocks[ blkIdx ].MaxBounds[ dimIdx ] : block0.MaxBounds[ dimIdx ];
    }
}

//------------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// perform an initial collection of attribute names (for block and particles)
void vtkEnzoReaderInternal::GetAttributeNames()
{
  int   wasFound = 0;       // any block with particles was found?
  int   blkIndex = 0;       // index of the block with fewest cells
                            // (either with or without particles)
  int   numCells = INT_MAX; // number of cells of  a block
  int   numbBlks = static_cast < int > ( this->Blocks.size() );

  for ( int i = 1; i < numbBlks; i ++ )
    {
    vtkEnzoReaderBlock & tmpBlock = this->Blocks[i];
    if (  wasFound && ( tmpBlock.NumberOfParticles <= 0 )  )
      {
      continue;
      }

    int  tempNumb = tmpBlock.BlockCellDimensions[0] *
                    tmpBlock.BlockCellDimensions[1] *
                    tmpBlock.BlockCellDimensions[2];

    if (  (  tempNumb  < numCells  ) ||
          ( !wasFound && tmpBlock.NumberOfParticles > 0 )
       )
      {
      if (  !wasFound ||
           ( wasFound && tmpBlock.NumberOfParticles > 0 )
         )
        {
        numCells = tempNumb;
        blkIndex = tmpBlock.Index;
        wasFound = ( tmpBlock.NumberOfParticles > 0 ) ? 1 : 0;
        }
      }
    }
  this->ReferenceBlock = blkIndex;


  // open the block file
  std::string   blckFile = this->Blocks[ blkIndex ].BlockFileName;
  hid_t   fileIndx = H5Fopen( blckFile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT );

  if ( fileIndx < 0 )
    {
    vtkGenericWarningMacro(
       "Failed to open HDF5 grid file " << blckFile.c_str() );
    return;
    }

  // retrieve the contents of the root directory to look for a group
  // corresponding to the specified block name (the one with the fewest
  // cells --- either with or without particles) and, if available, open
  // that group

  int     objIndex;
  hsize_t numbObjs;
  hid_t   rootIndx = H5Gopen( fileIndx, "/" );
  H5Gget_num_objs( rootIndx, &numbObjs );

  for (  objIndex = 0;  objIndex < static_cast < int > ( numbObjs );
         objIndex ++  )
    {
    if (  H5Gget_objtype_by_idx( rootIndx, objIndex )  ==  H5G_GROUP  )
      {
      int   blckIndx;
      char  blckName[65];
      H5Gget_objname_by_idx( rootIndx, objIndex, blckName, 64 );

      if (  sscanf( blckName, "Grid%d", &blckIndx ) == 1 &&
            ( blckIndx == blkIndex ) // does this block have the fewest cells?
         )
        {
        rootIndx = H5Gopen( rootIndx, blckName ); // located the target block
        break;
        }
      }
    }


  // in case of entering a sub-directory, obtain the number of objects here
  // and proceed with the parsing work (now rootIndx points to the group of
  // of target block)
  H5Gget_num_objs( rootIndx, &numbObjs );

  for (  objIndex = 0;  objIndex < static_cast < int > ( numbObjs );
         objIndex ++  )
    {
    if (  H5Gget_objtype_by_idx( rootIndx, objIndex ) == H5G_DATASET  )
      {
      char  tempName[65];
      H5Gget_objname_by_idx( rootIndx, objIndex, tempName, 64 );

      // NOTE: to do the same diligence as HDF4 here, we should
      // really H5Dopen, H5Dget_space, H5Sget_simple_extent_ndims
      // and make sure it is a 3D (or 2D?) object before assuming
      // it is a mesh variable.  For now, assume away!
      if (   (  strlen( tempName )  >  8  ) &&
             (  strncmp( tempName, "particle", 8 )  ==  0  )
         )
        {
        // it's a particle variable and skip over coordinate arrays
        if (  strncmp( tempName, "particle_position_", 18 ) != 0  )
          {
          this->ParticleAttributeNames.push_back( tempName );
          }
        }
      else
      if (   (  strlen( tempName )  >  16  ) &&
             (  strncmp( tempName, "tracer_particles", 16 )  ==  0  )
         )
        {
        // it's a tracer_particle variable and skip over coordinate arrays
        if (  strncmp( tempName, "tracer_particle_position_", 25 ) != 0  )
          {
          this->TracerParticleAttributeNames.push_back( tempName );
          }
        }
      else
        {
        this->BlockAttributeNames.push_back( tempName );
        }
      }
    }

  H5Gclose( rootIndx );
  H5Fclose( fileIndx );
}

// ----------------------------------------------------------------------------
// This function checks the block attributes, of which some might be actually
// particle attributes since a flexible (not standard) attributes naming scheme
// (such as the one adopted in cosmological datasets) causes this Enzo reader,
// specifically function GetAttributeNames(), to take particle attributes as
// block attributes. This function detects and corrects such problems, if any.
// Invalid block attributes are removed, possibly re-considered particle ones.
void vtkEnzoReaderInternal::CheckAttributeNames()
{
  // number of cells of the reference block
  vtkEnzoReaderBlock &
                theBlock = this->Blocks[ this->ReferenceBlock ];
  int           numCells = theBlock.BlockCellDimensions[0] *
                           theBlock.BlockCellDimensions[1] *
                           theBlock.BlockCellDimensions[2];


  // number of particles of the reference block, if any
//  std::cout << "Reference Block: " << this->ReferenceBlock << std::endl;
//  std::cout << "BlockIdx: " << this->ReferenceBlock - 1 << std::endl;
//  std::cout.flush();

  vtkPolyData * polyData = vtkPolyData::New();
//  this->TheReader->GetParticles( this->ReferenceBlock - 1, polyData, 0, 0 );

  int           numbPnts = polyData->GetNumberOfPoints();
  polyData->Delete();
  polyData = NULL;

  // block attributes to be removed and / or exported
  std::vector < std::string > toRemove;
  std::vector < std::string > toExport;
  toRemove.clear();
  toExport.clear();

  // determine to-be-removed and to-be-exported block attributes
  int   i;
  int   blockAttrs = static_cast < int > ( this->BlockAttributeNames.size() );
  for ( i = 0; i < blockAttrs; i ++ )
    {
    // the actual number of tuples of a block attribute loaded from the
    // file for the reference block
    int   numTupls = 0;
    if( this->DataArray != NULL )
      {
      numTupls = this->DataArray->GetNumberOfTuples();
      this->ReleaseDataArray();
      }
    else
      {
      numTupls = 0;
      }

    // compare the three numbers
    if ( numTupls != numCells )
      {
      if ( numTupls == numbPnts )
        {
        toExport.push_back( this->BlockAttributeNames[i] );
        }
      else
        {
        toRemove.push_back( this->BlockAttributeNames[i] );
        }
      }
    }

  int  nRemoves = static_cast < int > ( toRemove.size() );
  int  nExports = static_cast < int > ( toExport.size() );

  // remove block attributes
  for ( i = 0; i < nRemoves; i ++ )
    {
    for ( std::vector < std::string >::iterator
          stringIt  = this->BlockAttributeNames.begin();
          stringIt != this->BlockAttributeNames.end();
          stringIt ++
        )
      {
      if (  ( *stringIt )  ==  toRemove[i]  )
        {
        this->BlockAttributeNames.erase( stringIt );
        break;
        }
      }
    }

  // export attributes from blocks to particles
  for ( i = 0; i < nExports; i ++ )
    {
    for ( std::vector < std::string >::iterator
          stringIt  = this->BlockAttributeNames.begin();
          stringIt != this->BlockAttributeNames.end();
          stringIt ++
        )
      {
      if (  ( *stringIt )  ==  toExport[i]  )
        {
        this->ParticleAttributeNames.push_back( *stringIt );
        this->BlockAttributeNames.erase( stringIt );
        break;
        }
      }
    }

  toRemove.clear();
  toExport.clear();
}

// ----------------------------------------------------------------------------
// get the meta data
void vtkEnzoReaderInternal::ReadMetaData()
{
  // Check to see if we have read it
  if ( this->NumberOfBlocks > 0 )
    {
    return;
    }

  // get the general parameters (number of dimensions)
  this->ReadGeneralParameters();

  // obtain the block structures
  this->ReadBlockStructures();

  // determine the bounding box of the root block
  this->DetermineRootBoundingBox();

  // get the parent-wise and level-based bounding Ids of each block in a
  // top-down manner
  int   blocks = static_cast < int > ( this->Blocks.size() );
  for ( int i = 1; i < blocks; i ++ )
    {
    this->Blocks[i].GetParentWiseIds( this->Blocks );
    this->Blocks[i].GetLevelBasedIds( this->Blocks );
    }

  // locate the block that contains the fewest cells (either with or without
  // particles) and collect the attribute names

  this->GetAttributeNames();

  // verify the initial set of attribute names
  this->CheckAttributeNames();

}
