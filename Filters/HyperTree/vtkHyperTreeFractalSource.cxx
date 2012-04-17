/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeFractalSource.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeFractalSource.h"

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <assert.h>

vtkStandardNewMacro(vtkHyperTreeFractalSource);

//----------------------------------------------------------------------------
vtkHyperTreeFractalSource::vtkHyperTreeFractalSource()
{
  this->GridSize[0] = 1;
  this->GridSize[1] = 1;
  this->GridSize[2] = 1;
  this->AxisBranchFactor = 2;
  this->MaximumLevel = 1;
  this->Dimension = 3;
  this->Dual = false;
}

//----------------------------------------------------------------------------
vtkHyperTreeFractalSource::~vtkHyperTreeFractalSource()
{
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeFractalSource::NewHyperTreeGrid()
{
  // Instantiate hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::New();

  // Set grid parameters
  output->SetGridSize( this->GridSize );
  output->SetDimension( this->Dimension );
  output->SetAxisBranchFactor( this->AxisBranchFactor );

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

    if ( i == 0 )
      {
      output->SetXCoordinates( coords );
      }
    else if ( i == 1 )
      {
      output->SetYCoordinates( coords );
      }
    else // i must be 2 here
      {
      output->SetZCoordinates( coords );
      }

    // Clean up
    coords->Delete();
    }

  vtkDoubleArray *scalars = vtkDoubleArray::New();
  scalars->SetNumberOfComponents( 1 );

  vtkIdType fact = 1;
  for ( int i = 1; i < this->MaximumLevel; ++ i )
    {
    fact *= this->AxisBranchFactor;
    }

  scalars->Allocate( fact * fact );
  scalars->SetName( "Test" );
  output->GetLeafData()->SetScalars( scalars );
  scalars->UnRegister( this );

  int n[3];
  output->GetGridSize( n );
  for ( int i = 0; i < n[0]; ++ i )
    {
    for ( int j = 0; j < n[1]; ++ j )
      {
      for ( int k = 0; k < n[2]; ++ k )
        {
        int index = ( k * this->GridSize[1] + j ) * this->GridSize[0] + i;

        vtkHyperTreeCursor* cursor = output->NewCellCursor( i, j, k );
        cursor->ToRoot();

        int idx[3];
        idx[0] = idx[1] = idx[2] = 0;
        int offset = output->GetLeafData()->GetScalars()->GetNumberOfTuples();
        this->Subdivide( cursor, 1, output, index, idx, offset );
        cursor->UnRegister( this );
        } // k
      } // j
    } // i

  output->SetDualGridFlag( this->Dual );

  scalars->Squeeze();
  assert("post: dataset_and_data_size_match" && output->CheckAttributes()==0);

  return output;
}

//----------------------------------------------------------------------------
void vtkHyperTreeFractalSource::Subdivide( vtkHyperTreeCursor* cursor,
                                           int level,
                                           vtkHyperTreeGrid* output,
                                           int index,
                                           int idx[3],
                                           int offset )
{
  // Determine whether to subdivide.
  int subdivide = 1;

  if ( idx[0] || idx[1] || idx[2] )
    {
    subdivide = 0;
    }
  if ( ! index && idx[1] == 1 && ! idx[2] )
    {
    subdivide = 1;
    }

  // Check for hard coded minimum and maximum level restrictions.
  if ( level >= this->MaximumLevel )
    {
    subdivide = 0;
    }

  if ( subdivide )
    {
    output->SubdivideLeaf( cursor, index );

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
          cursor->ToChild(childIdx);
          this->Subdivide( cursor,
                           level + 1,
                           output,
                           index,
                           newIdx,
                           offset );
          cursor->ToParent();
          ++ childIdx;
          }
        }
      }
    }
  else
    {
    // Cell value
    float val = idx[0] + idx[1] + idx[2];

    // Offset cell index as needed
    vtkIdType id = offset + cursor->GetLeafId();
    output->GetLeafData()->GetScalars()->InsertTuple1( id, val );
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeFractalSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
