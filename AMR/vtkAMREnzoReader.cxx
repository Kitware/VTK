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

#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkLongLongArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"

#define H5_USE_16_API
#include <hdf5.h>

#include <sstream>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <cassert>

#include "vtkAMREnzoReaderInternal.h"

vtkStandardNewMacro(vtkAMREnzoReader);

//-----------------------------------------------------------------------------
vtkAMREnzoReader::vtkAMREnzoReader()
{
  this->Internal = new vtkEnzoReaderInternal();
  this->Initialize();
}

//-----------------------------------------------------------------------------
vtkAMREnzoReader::~vtkAMREnzoReader()
{
  delete this->Internal;
  this->Internal=NULL;

  this->BlockMap.clear();

  if( this->FileName )
    {
      delete [] this->FileName;
      this->FileName = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetIndexFromArrayName( std::string arrayName )
{
  char stringIdx = arrayName.at( arrayName.size()-2 );
  return( atoi( &stringIdx ) );
}

//-----------------------------------------------------------------------------
double vtkAMREnzoReader::GetConversionFactor( const std::string name )
{
  if( this->label2idx.find( name ) != this->label2idx.end() )
    {
      int idx = this->label2idx[ name ];
      if( this->conversionFactors.find( idx ) != this->conversionFactors.end() )
        return( this->conversionFactors[ idx ] );
      else
        return( 1.0 );
    }
  else
    return( 1.0 );
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::ParseLabel(
    const std::string labelString, int &idx, std::string &label)
{

  std::vector< std::string > strings;

  std::istringstream iss( labelString );
  std::string word;
  while ( iss >> word )
    {
      if( ! vtksys::SystemTools::StringStartsWith( word.c_str(), "=") )
        strings.push_back( word );
    }

  idx   = this->GetIndexFromArrayName( strings[0] );
  label = strings[ strings.size()-1 ];

//
//  for( unsigned int i=0; i < strings.size(); ++i )
//    std::cout << "[ " << strings[ i ] << " ] ";
//  std::cout << "idx: " << idx << std::endl;
//  std::cout.flush();
//
//  this->label2idx[ label ] = idx;
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::ParseCFactor(
    const std::string labelString, int &idx, double &factor )
{
  std::vector< std::string > strings;

  std::istringstream iss( labelString );
  std::string word;
  while( iss >> word )
    {
      if( ! vtksys::SystemTools::StringStartsWith( word.c_str(), "=") )
        strings.push_back( word );
    }

  idx    = this->GetIndexFromArrayName( strings[0] );
  factor = atof( strings[ strings.size()-1 ].c_str() );

//  for( unsigned int i=0; i < strings.size(); ++i )
//    std::cout << "[ " << strings[ i ] << " ] ";
//  std::cout << " idx: "     << idx    << std::endl;
//  std::cout << " factor:  " << factor << std::endl;
//
//  this->conversionFactors[ idx ] = factor;
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::ParseConversionFactors( )
{
  assert( "pre: FileName should not be NULL" && (this->FileName != NULL) );

  // STEP 0: Extract the parameters file from the user-supplied filename
  std::string baseDir =
      vtksys::SystemTools::GetFilenamePath(
          std::string( this->FileName ) );

  std::string paramsFile = baseDir + "/" +
      vtksys::SystemTools::GetFilenameWithoutExtension(
          std::string( this->FileName ) );

  // STEP 1: Open Parameters file
  std::ifstream ifs;
  ifs.open( paramsFile.c_str() );
  assert( "pre: Cannot open parameters file" && ( ifs.is_open() ) );

  // STEP 2: Parsing parameters file
  std::string line;  // temp string to store a line read from the params file
  std::string label; // stores the attribute name
  double cf;         // stores the conversion factor
  int    idx;        // stores the attribute label index
  while( getline(ifs, line) )
    {
      if( vtksys::SystemTools::StringStartsWith(
          line.c_str(), "DataLabel" ) )
        {
          this->ParseLabel( line, idx, label );
          this->label2idx[ label ] = idx;
        }
      else if( vtksys::SystemTools::StringStartsWith(
               line.c_str(), "#DataCGSConversionFactor" ) )
        {
          this->ParseCFactor( line, idx, cf );
          this->conversionFactors[ idx ] = cf;
        }
    }

  // STEP 3: Close parameters file
  ifs.close();
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::SetFileName( const char* fileName )
{
  assert("pre: Internal Enzo AMR Reader is NULL" && (this->Internal != NULL ));

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
      this->LoadedMetaData = false;

      if ( this->FileName )
      {
        delete [] this->FileName;
        this->FileName = NULL;
        this->Internal->SetFileName( NULL );
        this->ParseConversionFactors( );
      }
      this->FileName = new char[  strlen( fileName ) + 1  ];
      strcpy( this->FileName, fileName );
      this->FileName[ strlen( fileName ) ] = '\0';
      this->Internal->SetFileName( this->FileName );
      this->ParseConversionFactors( );
    }

  this->Internal->ReadMetaData();
  this->SetUpDataArraySelections();
  this->InitializeArraySelections();
  this->Modified();
  return;
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
int vtkAMREnzoReader::FillMetaData( )
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  assert( "pre: metadata object is NULL" && (this->metadata != NULL) );

  this->Internal->ReadMetaData();
  std::vector< int > b2level;
  b2level.resize( this->Internal->NumberOfLevels+1, 0 );

  // this->Internal->Blocks includes a pseudo block -- the root as block #0
  for( int i=0; i < this->Internal->NumberOfBlocks; ++i )
    {
      vtkEnzoReaderBlock &theBlock = this->Internal->Blocks[ i+1 ];
      int level                    = theBlock.Level;
      int id                       = b2level[ level ];
      int internalIdx              = i;

      double blockMin[ 3 ];
      double blockMax[ 3 ];
      double spacings[ 3 ];

      for( int j=0; j < 3; ++j )
        {
          blockMin[j] = theBlock.MinBounds[j];
          blockMax[j] = theBlock.MaxBounds[j];
          spacings[j] = (theBlock.BlockNodeDimensions[j] > 1)?
          (blockMax[j]-blockMin[j])/(theBlock.BlockNodeDimensions[j]-1.0):1.0;
        }

      vtkUniformGrid *ug = vtkUniformGrid::New();
      ug->SetDimensions( theBlock.BlockNodeDimensions );
      ug->SetOrigin( blockMin[0], blockMin[1], blockMin[2] );
      ug->SetSpacing( spacings[0], spacings[1], spacings[2] );

      this->metadata->SetDataSet( level, id, ug );
      this->metadata->SetCompositeIndex( level, id, internalIdx );
      ug->Delete();
      b2level[ level ]++;
    } // END for all blocks

  // NOTE: the controller here is null since each process loads its own metadata
  vtkAMRUtilities::GenerateMetaData( this->metadata, NULL );
  return( 1 );
}

//-----------------------------------------------------------------------------
vtkUniformGrid* vtkAMREnzoReader::GetAMRGrid( const int blockIdx )
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );

  this->Internal->ReadMetaData();

  // this->Internal->Blocks includes a pseudo block --- the root as block #0
  vtkEnzoReaderBlock &theBlock = this->Internal->Blocks[ blockIdx+1 ];
  double blockMin[3];
  double blockMax[3];
  double spacings[3];

  for( int i=0; i < 3; ++i )
    {
      blockMin[ i ] = theBlock.MinBounds[ i ];
      blockMax[ i ] = theBlock.MaxBounds[ i ];
      spacings[ i ] = (theBlock.BlockNodeDimensions[i] > 1)?
          (blockMax[i]-blockMin[i])/(theBlock.BlockNodeDimensions[i]-1.0) : 1.0;
    }

  vtkUniformGrid *ug = vtkUniformGrid::New();
  ug->SetDimensions( theBlock.BlockNodeDimensions );
  ug->SetOrigin( blockMin[0], blockMin[1], blockMin[2] );
  ug->SetSpacing( spacings[0], spacings[1], spacings[2] );
  return( ug );
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::GetAMRGridData(
    const int blockIdx, vtkUniformGrid *block, const char *field)
{
  assert( "pre: AMR block is NULL" && (block != NULL));

  this->Internal->GetBlockAttribute( field, blockIdx, block );
  if( this->ConvertToCGS == 1 )
    {
      double conversionFactor = this->GetConversionFactor(field);
      if( conversionFactor != 1.0 )
        {
          vtkDataArray *data = block->GetCellData()->GetArray( field );
          assert( "pre: data array is NULL!" && (data != NULL) );

          vtkIdType numTuples = data->GetNumberOfTuples();
          for( vtkIdType t=0; t < numTuples; ++t )
            {
              int numComp = data->GetNumberOfComponents();
              for( int c=0; c < numComp; ++c )
               {
                 double f = data->GetComponent( t, c );
                 data->SetComponent( t, c, f*conversionFactor );
               } // END for all components
            } // END for all tuples

        } // END if the conversion factor is not 1.0
    } // END if conversion to CGS units is requested
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
