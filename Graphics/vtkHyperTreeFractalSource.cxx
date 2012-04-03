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

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeCursor.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include <assert.h>
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkHyperTreeFractalSource);

//----------------------------------------------------------------------------
vtkHyperTreeFractalSource::vtkHyperTreeFractalSource()
{
  this->NumberOfRootCells[0] = 1;
  this->NumberOfRootCells[1] = 1;
  this->NumberOfRootCells[2] = 1;
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

  output->SetNumberOfRootCells( this->NumberOfRootCells );
  output->SetDimension( this->Dimension );
  output->SetAxisBranchFactor( this->AxisBranchFactor );
  for ( int i = 0; i < 3; ++i )
    {
    this->Size[i] = 1.0;
    this->Origin[i] = 0.0;
    }
  if (this->Dimension == 2)
    {
    this->Size[2] = 0.0;
    }
  output->SetSize(this->Size);
  output->SetOrigin(this->Origin);

  // Create geometry
  for ( int i = 0; i < 3; ++ i )
    {
    vtkDoubleArray *coords = vtkDoubleArray::New();
    int n = this->NumberOfRootCells[i] + 1;
    coords->SetNumberOfValues( n );
    for ( int j = 0; j < n; ++ j ) 
      {
      coords->SetValue( j, static_cast<double>( j ) );
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
  vtkIdType maxNumberOfCells=fact*fact*fact;

  scalars->Allocate(maxNumberOfCells/fact);
  scalars->SetName("Test");
  output->GetLeafData()->SetScalars(scalars);
  scalars->UnRegister(this);

  vtkHyperTreeCursor* cursor = output->NewCellCursor( 0 );
  cursor->ToRoot();

  int idx[3];
  idx[0] = idx[1] = idx[2] = 0;
  this->Subdivide( cursor, 1, output, this->Origin, this->Size, idx );

  cursor->UnRegister( this );

  output->SetDualGridFlag( this->Dual );

  scalars->Squeeze();
  assert("post: dataset_and_data_size_match" && output->CheckAttributes()==0);

  return output;
}

//----------------------------------------------------------------------------
void vtkHyperTreeFractalSource::Subdivide( vtkHyperTreeCursor* cursor,
                                           int level, 
                                           vtkHyperTreeGrid*
                                           output,
                                           double* origin, 
                                           double* size,
                                           int idx[3] )
{
  // Determine whether to subdivide.
  int subdivide = 1;

  if ( idx[0] || idx[1] || idx[2] )
    {
    subdivide = 0;
    }

  // Check for hard coded minimum and maximum level restrictions.
  if ( level >= this->MaximumLevel )
    {
    subdivide = 0;
    }

  if (subdivide)
    {
    output->SubdivideLeaf( cursor, 0 );
    double newOrigin[3];
    double newSize[3];
    newSize[0] = size[0] / this->AxisBranchFactor;
    newSize[1] = size[1] / this->AxisBranchFactor;
    newSize[2] = size[2] / this->AxisBranchFactor;

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
    for (int z = 0; z < zDim; ++z)
      {
      newIdx[2] = idx[2] * zDim + z;
      for (int y = 0; y < yDim; ++y)
        {
        newIdx[1] = idx[1] * yDim + y;
        for (int x = 0; x < xDim; ++x)
          {
          newIdx[0] = idx[0] * xDim + x;
          newOrigin[0] = origin[0] + static_cast<double>( x ) * newSize[0];
          newOrigin[1] = origin[1] + static_cast<double>( y ) * newSize[1];
          newOrigin[2] = origin[2] + static_cast<double>( z ) * newSize[2];
          cursor->ToChild(childIdx);
          this->Subdivide( cursor,
                           level + 1,
                           output,
                           newOrigin,
                           newSize,
                           newIdx);
          cursor->ToParent();
          ++ childIdx;
          }
        }
      }
    }
  else
    {
    float val = idx[0] + idx[1] + idx[2];
    // Weight cell values for smoother iso surface.
    vtkIdType id = cursor->GetLeafId();
    output->GetLeafData()->GetScalars()->InsertTuple1( id, val );
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeFractalSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
