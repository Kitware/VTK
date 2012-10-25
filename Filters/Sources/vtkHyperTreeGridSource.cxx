/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridSource.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridSource.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeCursor.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <vtksys/ios/sstream>

#include <assert.h>

vtkStandardNewMacro(vtkHyperTreeGridSource);

//----------------------------------------------------------------------------
vtkHyperTreeGridSource::vtkHyperTreeGridSource()
{
  // This a source: no input ports
  this->SetNumberOfInputPorts( 0 );

  // Grid parameters
  this->AxisBranchFactor = 2;
  this->MinimumLevel = 1;
  this->MaximumLevel = 1;

  // Grid topology
  this->Dimension = 3;
  this->GridSize[0] = 1;
  this->GridSize[1] = 1;
  this->GridSize[2] = 1;

  // Grid geometry
  this->XCoordinates = vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples( 2 );
  this->XCoordinates->SetComponent( 0, 0, 0. );
  this->XCoordinates->SetComponent( 1, 0, 1. );
  this->YCoordinates = vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples( 2 );
  this->YCoordinates->SetComponent( 0, 0, 0. );
  this->YCoordinates->SetComponent( 1, 0, 1. );
  this->ZCoordinates = vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples( 2 );
  this->ZCoordinates->SetComponent( 0, 0, 0. );
  this->ZCoordinates->SetComponent( 1, 0, 1. );

  // Grid description
  this->Descriptor = ".";

  // By default expose the primal grid API
  this->Dual = false;
}

//----------------------------------------------------------------------------
vtkHyperTreeGridSource::~vtkHyperTreeGridSource()
{
  if ( this->XCoordinates )
    {
    this->XCoordinates->UnRegister( this );
    this->XCoordinates = NULL;
    }

  if ( this->YCoordinates )
    {
    this->YCoordinates->UnRegister( this );
    this->YCoordinates = NULL;
    }

  if ( this->ZCoordinates )
    {
    this->ZCoordinates->UnRegister( this );
    this->ZCoordinates = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "GridSize: "
     << this->GridSize[0] <<","
     << this->GridSize[1] <<","
     << this->GridSize[2] << endl;

  os << indent << "MaximumLevel: " << this->MaximumLevel << endl;
  os << indent << "MinimumLevel: " << this->MinimumLevel << endl;
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "AxisBranchFactor: " <<this->AxisBranchFactor << endl;
  os << indent << "BlockSize: " <<this->BlockSize << endl;

  if ( this->XCoordinates )
    {
    this->XCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->YCoordinates )
    {
    this->YCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->ZCoordinates )
    {
    this->ZCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }

  os << indent << "Descriptor: " << this->Descriptor << endl;
  os << indent << "Dual: " << this->Dual << endl;

  os << indent << "Output: " << endl;
  this->Output->PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetDescriptor( const vtkStdString& string )
{
  this->Descriptor = string;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkStdString vtkHyperTreeGridSource::GetDescriptor()
{
  return this->Descriptor;
}

//----------------------------------------------------------------------------
// Description:
// Return the maximum number of levels of the hyperoctree.
// \post positive_result: result>=1
unsigned int vtkHyperTreeGridSource::GetMaximumLevel()
{
  assert( "post: positive_result" && this->MaximumLevel >= 1 );
  return this->MaximumLevel;
}

//----------------------------------------------------------------------------
// Description:
// Set the maximum number of levels of the hypertrees. If
// GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
// \pre positive_levels: levels>=1
// \post is_set: this->GetLevels()==levels
// \post min_is_valid: this->GetMinLevels()<this->GetLevels()
void vtkHyperTreeGridSource::SetMaximumLevel( unsigned int levels )
{
  if ( levels < 1 )
    {
    levels = 1;
    }

  if ( this->MaximumLevel == levels )
    {
    return;
    }

  this->MaximumLevel = levels;

  // Update minimum level as well if needed
  if( this->MinimumLevel > levels )
    {
    this->MinimumLevel = levels;
    }
  this->Modified();

  assert( "post: is_set" && this->GetMaximumLevel() == levels );
  assert( "post: min_is_valid" && this->GetMinimumLevel() <= this->GetMaximumLevel() );
}


//----------------------------------------------------------------------------
// Description:
// Return the minimal number of levels of systematic subdivision.
// \post positive_result: result>=0
unsigned int vtkHyperTreeGridSource::GetMinimumLevel()
{
  assert( "post: positive_result" );
  return this->MinimumLevel;
}

//----------------------------------------------------------------------------
// Description:
// Set the minimal number of levels of systematic subdivision.
// \pre positive_minLevels: minLevels>=0 && minLevels<this->GetLevels()
// \post is_set: this->GetMinLevels()==minLevels
void vtkHyperTreeGridSource::SetMinimumLevel( unsigned int minLevels )
{
  if ( minLevels < 1 )
    {
    minLevels = 1;
    }

  if ( this->MinimumLevel == minLevels )
    {
    return;
    }

  this->Modified();
  this->MinimumLevel = minLevels;
  assert( "post: is_set" && this->GetMinimumLevel() == minLevels );
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::RequestInformation( vtkInformation*,
                                                vtkInformationVector**,
                                                vtkInformationVector* outputVector )
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // We cannot give the exact number of levels of the hypertrees
  // because it is not generated yet and this process depends on the recursion formula.
  // Just send an upper limit instead.
  outInfo->Set( vtkHyperTreeGrid::LEVELS(), this->MaximumLevel );
  outInfo->Set( vtkHyperTreeGrid::DIMENSION(), this->Dimension );

  double origin[3];
  origin[0] = this->XCoordinates->GetTuple1( 0 );
  origin[1] = this->YCoordinates->GetTuple1( 0 );
  origin[2] = this->ZCoordinates->GetTuple1( 0 );
  outInfo->Set( vtkDataObject::ORIGIN(), origin, 3 );

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::RequestData( vtkInformation*,
                                         vtkInformationVector**,
                                         vtkInformationVector* outputVector )
{
  // Retrieve the output
  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );
  this->Output = vtkHyperTreeGrid::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
  if ( ! this->Output )
    {
    return 0;
    }

  // Initialize descriptor parsing
  if ( ! this->Initialize() )
    {
    return 0;
    }

  // Set grid parameters
  this->Output->SetGridSize( this->GridSize );
  this->Output->SetDimension( this->Dimension );
  this->Output->SetAxisBranchFactor( this->AxisBranchFactor );
  this->Output->SetDualGridFlag( this->Dual );

  // Per-axis scaling
  double scale[3];
  scale[0] = 1.5;
  scale[1] = 1.;
  scale[2] = .7;

  // Create geometry
  for ( int i = 0; i < 3; ++ i )
    {
    vtkDoubleArray *coords = vtkDoubleArray::New();
    int n = this->GridSize[i] + 1;
    coords->SetNumberOfValues( n );
    for ( int j = 0; j < n; ++ j )
      {
      coords->SetValue( j, scale[i] * static_cast<double>( j ) );
      }

    switch ( i )
      {
      case 0:
        this->Output->SetXCoordinates( coords );
        break;
      case 1:
        this->Output->SetYCoordinates( coords );
        break;
      case 2:
        this->Output->SetZCoordinates( coords );
        break;
      }

    // Clean up
    coords->Delete();
    }

  // Prepare array of doubles for cell values
  vtkDoubleArray* scalars = vtkDoubleArray::New();
  scalars->SetName( "Cell Value" );
  scalars->SetNumberOfComponents( 1 );
  vtkIdType fact = 1;
  for ( unsigned int i = 1; i < this->MaximumLevel; ++ i )
    {
    fact *= this->AxisBranchFactor;
    }
  scalars->Allocate( fact * fact );

  // Set leaf (cell) data and clean up
  this->Output->GetLeafData()->SetScalars( scalars );
  scalars->UnRegister( this );

  // Iterate over grid of trees
  int n[3];
  this->Output->GetGridSize( n );
  for ( int k = 0; k < n[2]; ++ k )
    {
    for ( int j = 0; j < n[1]; ++ j )
      {
      for ( int i = 0; i < n[0]; ++ i )
        {
        // Calculate global index
        int index_g = ( k * this->GridSize[1] + j ) * this->GridSize[0] + i;

        // Initialize cursor
        vtkHyperTreeCursor* cursor = this->Output->NewCellCursor( i, j, k );
        cursor->ToRoot();

        // Initialize local cell index
        int idx[3];
        idx[0] = idx[1] = idx[2] = 0;

        // Retrieve offset into array of scalars and recurse
        this->Subdivide( cursor,
                         0,
                         index_g,
                         0,
                         idx,
                         this->Output->GetLeafData()->GetScalars()->GetNumberOfTuples(),
                         0 );

        // Clean up
        cursor->UnRegister( this );
        } // i
      } // j
    } // k

  assert( "post: dataset_and_data_size_match" && this->Output->CheckAttributes() == 0 );

  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridSource::Initialize()
{
  // Calculate refined block size
  this->BlockSize = this->AxisBranchFactor;
  for ( int i = 1; i < this->Dimension; ++ i )
    {
    this->BlockSize *= this->AxisBranchFactor;
    }

  // Calculate total level 0 grid size
  unsigned int nTotal = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];

  // Parse string descriptor
  unsigned int nRefined = 0;
  unsigned int nLeaves = 0;
  unsigned int nNextLevel = 0;
  bool rootLevel = true;
  vtksys_ios::ostringstream stream;
  for ( vtkStdString::iterator it = this->Descriptor.begin(); it != this->Descriptor.end(); ++ it )
    {
    char c = *it;
    switch ( c )
      {
      case ' ':
        // Space is allowed as benign separator
        continue;
      case '|':
        //  A level is complete
        this->LevelDescriptors.push_back( stream.str().c_str() );

        // Check whether cursor is still at rool level
        if ( rootLevel )
          {
          rootLevel = false;

          // Verify that total number of root cells is consistent with descriptor
          if ( nRefined + nLeaves != nTotal )
            {
            vtkErrorMacro(<<"String "
                          << this->Descriptor
                          << " describes "
                          << nRefined + nLeaves
                          << " root cells != "
                          << nTotal);
            return 0;
            }
          } // if ( rootLevel )
        else
          {
          // Verify that level descriptor cardinality matches expected value
          if (  stream.str().size() != nNextLevel )
            {
            vtkErrorMacro(<<"String level descriptor "
                          << stream.str().c_str()
                          << " has cardinality "
                          << stream.str().size()
                          << " which is not expected value of "
                          << nNextLevel);

            return 0;
            }
          } // else

        // Predict next level descriptor cardinality
        nNextLevel = nRefined * this->BlockSize;

        // Reset per level values
        stream.str( "" );
        nRefined = 0;
        nLeaves = 0;

        break;
      case 'R':
        // Refined cell, update branch counter
        ++ nRefined;

        // Append character to per level string
        stream << c;

        break;
      case '.':
        // Leaf cell, update leaf counter
        ++ nLeaves;

        // Append character to per level string
        stream << c;

        break;
      default:
        vtkErrorMacro(<< "Unrecognized character: "
                      << c
                      << " in string "
                      << this->Descriptor);

        return 0;
      } // switch( c )
    } // i

  // Verify and append last level string
  if (  stream.str().size() != nNextLevel )
    {
    vtkErrorMacro(<<"String level descriptor "
                  << stream.str().c_str()
                  << " has cardinality "
                  << stream.str().size()
                  << " which is not expected value of "
                  << nNextLevel);

    return 0;
    }
  this->LevelDescriptors.push_back( stream.str().c_str() );

  // Reset maximum depth if fewer levels are described
  unsigned int nLevels = static_cast<unsigned int>( this->LevelDescriptors.size() );
  if ( nLevels < this->MaximumLevel )
    {
    this->MaximumLevel = nLevels;
    }

  // Create vector of counters as long as tree depth
  for ( unsigned int i = 0; i < nLevels; ++ i )
    {
    this->LevelCounters.push_back( 0 );
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::Subdivide( vtkHyperTreeCursor* cursor,
                                        unsigned int level,
                                        int index_g,
                                        int index_l,
                                        int idx[3],
                                        int cellIdOffset,
                                        int parentPos )
{
  // Calculate pointer into level descriptor string
  int pointer = level ? index_l + parentPos * this->BlockSize : index_g;

  // Determine whether to subdivide or not
  bool subdivide = this->LevelDescriptors.at( level ).at( pointer ) == 'R' ? true : false;

  // Check for hard coded minimum and maximum level restrictions
  if ( level + 1 >= this->MaximumLevel )
    {
    subdivide = 0;
    }

  if ( subdivide )
    {
    // Subdivide hyper tree grid leaf
    this->Output->SubdivideLeaf( cursor, index_g );

    // Now traverse to children.
    int xDim, yDim, zDim;
    xDim = yDim = zDim = 1;
    if ( this->Dimension == 1 )
      {
      xDim = this->AxisBranchFactor;
      }
    else if ( this->Dimension == 2 )
      {
      xDim = yDim = this->AxisBranchFactor;
      }
    else if ( this->Dimension == 3 )
      {
      xDim = yDim = zDim = this->AxisBranchFactor;
      }
    int childIdx = 0;
    int newIdx[3];
    for ( int z = 0; z < zDim; ++ z )
      {
      newIdx[2] = idx[2] * zDim + z;
      for ( int y = 0; y < yDim; ++ y )
        {
        newIdx[1] = idx[1] * yDim + y;
        for ( int x = 0; x < xDim; ++ x )
          {
          newIdx[0] = idx[0] * xDim + x;

          // Set cursor to child
          cursor->ToChild( childIdx );

          // Calculate local index
          index_l = x + this->AxisBranchFactor
            * ( y + this->AxisBranchFactor * z );

          // Recurse
          this->Subdivide( cursor,
                           level + 1,
                           index_g,
                           index_l,
                           newIdx,
                           cellIdOffset,
                           this->LevelCounters.at( level ) );

          // Reset cursor to parent
          cursor->ToParent();

          // Increment child index
          ++ childIdx;
          }
        }
      }

    // Increment current level counter
    ++ this->LevelCounters.at( level );
    } // if ( subdivide )
  else
    {
    // Retrieve cartesian coordinates w.r.t. global grid
    int gs[3];
    this->Output->GetGridSize( gs );
    div_t q1 = div( index_g, gs[0] );
    double x[3];
    x[0] = q1.rem;
    x[1] = 0.;
    x[2] = 0.;
    if ( gs[1] )
      {
      div_t q2 = div( q1.quot, gs[1] );
      x[1] = q2.rem;
      x[2] = q2.quot;
      }

    // Center coordinates w.r.t. global grid center
    for ( int i = 0; i < 3; ++ i )
      {
      x[i] -= .5 * gs[i];
      }

    // Cell value is depth level
    double val = level;

    // Offset cell index as needed
    vtkIdType id = cellIdOffset + cursor->GetLeafId();
    this->Output->GetLeafData()->GetScalars()->InsertTuple1( id, val );
    } // else
}

//-----------------------------------------------------------------------------
