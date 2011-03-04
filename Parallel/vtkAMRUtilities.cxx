/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRUtilities.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRUtilities.h"
#include "vtkAMRBox.h"
#include "vtkUniformGrid.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkCommunicator.h"

#include <cmath>
#include <limits>
#include <cassert>

//------------------------------------------------------------------------------
void vtkAMRUtilities::PrintSelf( std::ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::ComputeDataSetOrigin(
       double origin[3], vtkHierarchicalBoxDataSet *amrData,
       vtkMultiProcessController *controller )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL) );

  double min[3];
  min[0] = min[1] = min[2] = 100;

  // Note, we only need to check at level 0 since, the grids at
  // level 0 are guaranteed to cover the entire domain. Most datasets
  // will have a single grid at level 0.
  for( int idx=0; idx < amrData->GetNumberOfDataSets(0); ++idx )
    {

      vtkUniformGrid *gridPtr = amrData->GetDataSet( 0, idx );
      if( gridPtr != NULL )
        {
          double *gridBounds = gridPtr->GetBounds();
          assert( "Failed when accessing grid bounds!" && (gridBounds!=NULL) );

          if( gridBounds[0] < min[0] )
            min[0] = gridBounds[0];
          if( gridBounds[2] < min[1] )
            min[1] = gridBounds[2];
          if( gridBounds[4] < min[2] )
            min[2] = gridBounds[4];

        }

    } // END for all data-sets at level 0

  // If data is distributed, get the global min
  if( controller != NULL )
    {
      if( controller->GetNumberOfProcesses() > 1 )
        {
          // TODO: Define a custom operator s.t. only one all-reduce operation
          // is called.
          controller->AllReduce(&min[0],&origin[0],1,vtkCommunicator::MIN_OP);
          controller->AllReduce(&min[1],&origin[1],1,vtkCommunicator::MIN_OP);
          controller->AllReduce(&min[2],&origin[2],1,vtkCommunicator::MIN_OP);
          return;
        }
    }

   // Else this is a single process
   origin[0] = min[0];
   origin[1] = min[1];
   origin[2] = min[2];
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::CollectAMRMetaData(
    vtkHierarchicalBoxDataSet *amrData, vtkMPIController *myController )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL));
  // TODO: implement this
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::ComputeLevelRefinementRatio(
    vtkHierarchicalBoxDataSet *amr )
{
  // sanity check
  assert( "Input AMR Data is NULL" && (amr != NULL)  );

}
