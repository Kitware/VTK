/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRDataTransferPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
//  A simple utility to demonstrate & test the parallel AMR functionality
//  and inter-block data transfer.

#include <mpi.h>

#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkAssertUtils.hpp"

// Global Variables
vtkMultiProcessController *Controller;

// Function Prototypes
vtkHierarchicalBoxDataSet* GetAMRDataSet( );
vtkUniformGrid* GetGrid( vtkAMRBox &gridBox );

int main( int argc, char **argv )
{
  MPI_Init( &argc, &argv );

  Controller = vtkMultiProcessController::GetGlobalController();
  vtkAssertUtils::assertNotNull( Controller,__FILE__,__LINE__ );

  vtkHierarchicalBoxDataSet *amrData = GetAMRDataSet();
  vtkAssertUtils::assertNotNull( amrData, __FILE__, __LINE__ );

  MPI_Finalize();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

vtkHierarchicalBoxDataSet* GetAMRDataSet( )
{
  vtkAssertUtils::assertNotNull( Controller, __FILE__, __LINE__ );

  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New( );
  vtkAssertUtils::assertNotNull( data, __FILE__, __LINE__ );

  switch( Controller->GetLocalProcessId() )
    {
    case 0:
      // TODO: implement this
      break;
    case 1:
      // TODO: implement this
      break;
    default:
      /* Note: Code should not reach here! */
      return NULL;
    }

  return( data );

}

//----------------------------------------------------------------------------
vtkUniformGrid* GetGrid( vtkAMRBox &gridBox )
{
  // TODO: implement this
  return NULL;
}
