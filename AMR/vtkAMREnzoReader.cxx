/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMREnzoReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMREnzoReader.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkDataArraySelection.h"
#include "vtkDataArray.h"
#include "vtkPolyData.h"
#include "vtkAMRUtilities.h"
#include "vtkIndent.h"
#include "vtksys/SystemTools.hxx"

#define H5_USE_16_API
#include <hdf5.h>

#include <vtkstd/vector>
#include <vtkstd/string>
#include <cassert>

//==============================================================================


/*****************************************************************************
*
* Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file was adapted from the VisIt Enzo reader (avtEnzoFileFormat). For
* details, see https://visit.llnl.gov/.  The full copyright notice is contained
* in the file COPYRIGHT located at the root of the VisIt distribution or at
* http://www.llnl.gov/visit/copyright.html.
*
*****************************************************************************/

#define     ENZO_READER_SLASH_CHAR    '\\'
#define     ENZO_READER_SLASH_STRING  "\\"
const  int  ENZO_READER_BUFFER_SIZE = 4096;
static char ENZO_READER_STRING[ ENZO_READER_BUFFER_SIZE ];


// ----------------------------------------------------------------------------
//                       Functions for Parsing File Names
// ----------------------------------------------------------------------------


static const char * GetEnzoMajorFileName( const char * path, int & start )
{
  start = 0;

  if (path == 0)
    {
    strcpy( ENZO_READER_STRING, "." );
    return  ENZO_READER_STRING;
    }
  else
  if ( *path == '\0' )
    {
    strcpy( ENZO_READER_STRING, "." );
    return  ENZO_READER_STRING;
    }
  else
    {
    // find end of path string
    int  n = 0;
    while (  ( path[n] != '\0' ) && ( n < ENZO_READER_BUFFER_SIZE )  )
      {
      n ++;
      }

    // deal with string too large
    if ( n == ENZO_READER_BUFFER_SIZE )
      {
      strcpy( ENZO_READER_STRING, "." );
      return  ENZO_READER_STRING;
      }

    // backup, skipping over all trailing ENZO_READER_SLASH_CHAR chars
    int  j = n-1;
    while (  ( j >= 0 ) && ( path[j] == ENZO_READER_SLASH_CHAR )  )
      {
      j --;
      }

    // deal with string consisting of all ENZO_READER_SLASH_CHAR chars
    if ( j == -1 )
      {
      start = -1;
      strcpy( ENZO_READER_STRING, ENZO_READER_SLASH_STRING );
      return  ENZO_READER_STRING;
      }

    // backup to just after next ENZO_READER_SLASH_CHAR char
    int  i = j-1;
    while (  ( i >= 0 ) && ( path[i] != ENZO_READER_SLASH_CHAR )  )
      {
      i --;
      }

    i ++;
    start = i;

    // build the return string
    int   k;
    for ( k = 0; k < j - i + 1; k ++ )
      {
      ENZO_READER_STRING[k] = path[ i + k ];
      }
    ENZO_READER_STRING[k] = '\0';

    return ENZO_READER_STRING;
    }
}

const char * GetEnzoMajorFileName( const char * path )
{
//  int     dummy1;
//  return  GetEnzoMajorFileName( path, dummy1 );
  std::vector< std::string > vpath;
  vtksys::SystemTools::SplitPath( path,vpath );
  assert( vpath.size() >= 1);
  return( vpath[ vpath.size()-1 ].c_str() );
}

const char * GetEnzoDirectory( const char * path )
{
  int start;
  GetEnzoMajorFileName( path, start );
  std::string mydir = vtksys::SystemTools::GetFilenamePath( std::string(path) );
  return mydir.c_str( );
}


// ----------------------------------------------------------------------------
//                       Class vtkEnzoReaderBlock (begin)
// ----------------------------------------------------------------------------


class vtkEnzoReaderBlock
{
public:
  vtkEnzoReaderBlock()  { this->Init(); }
 ~vtkEnzoReaderBlock()  { this->Init(); }

  int                   Index;
  int                   Level;
  int                   ParentId;
  vtkstd::vector< int > ChildrenIds;

  int                   MinParentWiseIds[3];
  int                   MaxParentWiseIds[3];
  int                   MinLevelBasedIds[3];
  int                   MaxLevelBasedIds[3];

  int                   NumberOfParticles;
  int                   NumberOfDimensions;
  int                   BlockCellDimensions[3];
  int                   BlockNodeDimensions[3];

  double                MinBounds[3];
  double                MaxBounds[3];
  double                SubdivisionRatio[3];

  vtkstd::string        BlockFileName;
  vtkstd::string        ParticleFileName;

  void   Init()
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

  void GetParentWiseIds(  vtkstd::vector< vtkEnzoReaderBlock > & blocks  );
  void GetLevelBasedIds(  vtkstd::vector< vtkEnzoReaderBlock > & blocks  );
};

// ----------------------------------------------------------------------------
// get the bounding (cell) Ids of this block in terms of its parent block's
// sub-division resolution (indexing is limited to the scope of the parent)
void vtkEnzoReaderBlock::GetParentWiseIds
  (  vtkstd::vector< vtkEnzoReaderBlock > & blocks  )
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

// ----------------------------------------------------------------------------
// determine the bounding (cell) Ids of this block in terms of the sub-division
// resolution of the level at which its parent block lies (indexing includes
// all siblings of this block --- those sibling blocks beyond the scope of the
// parent of this block)
void vtkEnzoReaderBlock::GetLevelBasedIds
  (  vtkstd::vector< vtkEnzoReaderBlock > & blocks  )
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


// ----------------------------------------------------------------------------
//                       Class vtkEnzoReaderBlock ( end )
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//                     Class  vtkEnzoReaderInternal (begin)
// ----------------------------------------------------------------------------


class vtkEnzoReaderInternal
{
public:
  vtkEnzoReaderInternal( vtkAMREnzoReader * reader )
                          { this->Init(); this->TheReader = reader; }
  vtkEnzoReaderInternal() { this->Init(); }
 ~vtkEnzoReaderInternal() { this->ReleaseDataArray();
                            this->Init();
                          }

  // number of all vtkDataSet (vtkImageData / vtkRectilinearGrid / vtkPolyData)
  // objects that have been SUCCESSFULLY extracted and inserted to the output
  // vtkMultiBlockDataSet (including rectilinear blocks and particle sets)
  int             NumberOfMultiBlocks;

  int             NumberOfDimensions;
  int             NumberOfLevels;
  int             NumberOfBlocks;
  int             ReferenceBlock;
  int             CycleIndex;
  char          * FileName;
  double          DataTime;
  vtkDataArray  * DataArray;
  vtkAMREnzoReader * TheReader;

  vtkstd::string                       DirectoryName;
  vtkstd::string                       MajorFileName;
  vtkstd::string                       BoundaryFileName;
  vtkstd::string                       HierarchyFileName;
  vtkstd::vector< vtkstd::string >     BlockAttributeNames;
  vtkstd::vector< vtkstd::string >     ParticleAttributeNames;
  vtkstd::vector< vtkstd::string >     TracerParticleAttributeNames;
  vtkstd::vector< vtkEnzoReaderBlock > Blocks;

  void   Init()
     {
       this->DataTime   = 0.0;
       this->FileName   = NULL;
       this->TheReader  = NULL;
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

  void   ReleaseDataArray()
     {
     if ( this->DataArray )
       {
       this->DataArray->Delete();
       this->DataArray = NULL;
       }
     }

  void   SetFileName( char * fileName ) { this->FileName = fileName; }
  void   ReadMetaData();
  void   GetAttributeNames();
  void   CheckAttributeNames();
  void   ReadBlockStructures();
  void   ReadGeneralParameters();
  void   DetermineRootBoundingBox();
};

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
  vtkstd::string   theStr = "";

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
      vtkstd::string    szName;
      while ( theStr != "BaryonFileName" )
        {
        stream >> theStr;
        }
      stream >> theStr; // '='
      stream >> szName;

      tmpBlk.BlockFileName = this->DirectoryName + "/" +
                             GetEnzoMajorFileName( szName.c_str() );

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
        tmpBlk.ParticleFileName = this->DirectoryName + "/" +
                                  GetEnzoMajorFileName( szName.c_str() );
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
      while (  ( tmpChr = stream.get() )  !=  '['  );
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

  vtkstd::string tmpStr( "" );
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
  vtkstd::string   blckFile = this->Blocks[ blkIndex ].BlockFileName;
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

//  TODO: Add particle support here (gzagaris)
  vtkPolyData * polyData = vtkPolyData::New();
//  this->TheReader->GetParticles( this->ReferenceBlock - 1, polyData, 0, 0 );

  int           numbPnts = polyData->GetNumberOfPoints();
  polyData->Delete();
  polyData = NULL;

  // block attributes to be removed and / or exported
  vtkstd::vector < vtkstd::string > toRemove;
  vtkstd::vector < vtkstd::string > toExport;
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
    if( this->TheReader->GetCellArrayStatus(
          this->BlockAttributeNames[i].c_str() ) )
//    if(this->TheReader->GetCellArrayStatus(
//            this->BlockAttributeNames[i].c_str() ) &&
//       this->TheReader->LoadAttribute(
//            this->BlockAttributeNames[i].c_str(),this->ReferenceBlock - 1)  )
      {
      numTupls = this->DataArray->GetNumberOfTuples();
      this->ReleaseDataArray();
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
    for ( vtkstd::vector < vtkstd::string >::iterator
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
    for ( vtkstd::vector < vtkstd::string >::iterator
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


// ----------------------------------------------------------------------------
//                     Class  vtkEnzoReaderInternal ( end )
// ----------------------------------------------------------------------------


//==============================================================================
vtkStandardNewMacro(vtkAMREnzoReader);

//-----------------------------------------------------------------------------
vtkAMREnzoReader::vtkAMREnzoReader()
{
  this->Internal = new vtkEnzoReaderInternal( this );
  this->Initialize();
}

//-----------------------------------------------------------------------------
vtkAMREnzoReader::~vtkAMREnzoReader()
{
  delete this->Internal;
  this->Internal=NULL;
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::SetFileName( const char* fileName )
{
  int isValid=0;

  if( fileName && strcmp( fileName, "" ) &&
    ( (this->FileName==NULL) || (strcmp(fileName,this->FileName ) ) ) )
    {
        vtkstd::string  tempName( fileName );
        vtkstd::string  bExtName( ".boundary" );
        vtkstd::string  hExtName( ".hierarchy" );

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

        isValid = 1;
        this->Internal->DirectoryName =
            GetEnzoDirectory(this->Internal->MajorFileName.c_str());
    }

  if( isValid )
    {
      this->BlockMap.clear();
      this->Internal->Blocks.clear();
      this->Internal->NumberOfBlocks = 0;

      if ( this->FileName )
      {
        delete [] this->FileName;
        this->FileName = NULL;
        this->Internal->SetFileName( NULL );
      }
      this->FileName = new char[  strlen( fileName ) + 1  ];
      strcpy( this->FileName, fileName );
      this->FileName[ strlen( fileName ) ] = '\0';
      this->Internal->SetFileName( this->FileName );
    }

  this->Internal->ReadMetaData();
  this->SetUpDataArraySelections( );
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::ReadMetaData()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::GenerateBlockMap()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );

  this->BlockMap.clear();
  this->Internal->ReadMetaData();

  for( int i=0; i < this->Internal->NumberOfBlocks; ++i )
    {

      if( this->GetBlockLevel( i ) <= this->MaxLevel )
        {
          this->BlockMap.push_back( i );
        }

    } // END for all blocks
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetBlockLevel( const int blockIdx )
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );

  this->Internal->ReadMetaData();

  if( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
      vtkErrorMacro( "Block Index (" << blockIdx << ") is out-of-bounds!" );
      return( -1 );
    }
  return( this->Internal->Blocks[ blockIdx+1 ].Level );
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetNumberOfBlocks()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  return( this->Internal->NumberOfBlocks );
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetNumberOfLevels()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  return( this->Internal->NumberOfLevels );
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::GetBlock(
    int index, vtkHierarchicalBoxDataSet *hbds,
    vtkstd::vector< int > &idxcounter)
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  assert( "pre: Output AMR dataset is NULL" && (hbds != NULL)  );

  this->Internal->ReadMetaData();
  int blockIdx                 = this->BlockMap[ index ];
  vtkEnzoReaderBlock &theBlock = this->Internal->Blocks[ blockIdx+1 ];
  int level                    = theBlock.Level;

  double blockMin[3];
  double blockMax[3];
  double spacings[3];

  for( int i=0; i < 3; ++i )
    {
      blockMin[i] = theBlock.MinBounds[i];
      blockMax[i] = theBlock.MaxBounds[i];
      spacings[i] = ( theBlock.BlockNodeDimensions[i] > 1 )?
       (blockMax[i]-blockMin[i])/(theBlock.BlockNodeDimensions[i]-1.0) : 1.0;
    }

  vtkUniformGrid *ug = vtkUniformGrid::New();
  ug->SetDimensions( theBlock.BlockNodeDimensions );
  ug->SetOrigin( blockMin[0],blockMin[1],blockMin[2] );
  ug->SetSpacing( spacings[0],spacings[1],spacings[2] );

  // TODO: load data to the grid

  hbds->SetDataSet(level,idxcounter[level],ug);
  ug->Delete();
  idxcounter[ level ]++;
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::SetUpDataArraySelections()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  this->Internal->GetAttributeNames();

  int numAttrs = static_cast< int >(
      this->Internal->BlockAttributeNames.size() );
  for( int i=0; i < numAttrs; i++ )
    {
      this->CellDataArraySelection->AddArray(
          this->Internal->BlockAttributeNames[i].c_str() );
    } // END for all attributes

}
