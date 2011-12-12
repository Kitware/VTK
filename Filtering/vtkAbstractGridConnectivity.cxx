/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAbstractGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAbstractGridConnectivity.h"


vtkAbstractGridConnectivity::vtkAbstractGridConnectivity()
{
  this->NumberOfGrids       = 0;
  this->NumberOfGhostLayers = 0;
}

//------------------------------------------------------------------------------
vtkAbstractGridConnectivity::~vtkAbstractGridConnectivity()
{
  for( int i=0; i < this->NumberOfGrids; ++i )
    {
    if( this->GridPointData[ i ] != NULL )
      {
      this->GridPointData[ i ]->Delete();
      }
    if( this->GridCellData[ i ] != NULL )
      {
      this->GridCellData[ i ]->Delete();
      }
    } // END for all grids

  this->GridPointData.clear();
  this->GridCellData.clear();
  this->GridPointGhostArrays.clear();
  this->GridCellGhostArrays.clear();

  if( this->NumberOfGhostLayers )
    {
    for( int i=0; i < this->NumberOfGrids; ++i )
      {
      if( this->GhostedGridPointData[i] != NULL )
        {
        this->GhostedGridPointData[i]->Delete();
        }
      if( this->GhostedGridCellData[i] != NULL )
        {
        this->GhostedGridCellData[i]->Delete();
        }
      if( this->GhostedPointGhostArray[i] != NULL )
        {
        this->GhostedPointGhostArray[i]->Delete();
        }
      if( this->GhostedCellGhostArray[i] != NULL )
        {
        this->GhostedCellGhostArray[i]->Delete();
        }
      }
    }
  this->GhostedGridPointData.clear();
  this->GhostedGridCellData.clear();
  this->GhostedPointGhostArray.clear();
  this->GhostedCellGhostArray.clear();

}

//------------------------------------------------------------------------------
void vtkAbstractGridConnectivity::PrintSelf(std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
