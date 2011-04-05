/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRProbe.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestAMRProbe.cxx -- demonstrates/tests the AMR probing functionality
//
// .SECTION Description
//  This is a simple utility code to demonstrate and test the functionality of
//  the AMR probe filter.

#include "AMRCommon.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAMRBox.h"
#include "vtkPointSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkAMRProbeFilter.h"
#include "vtkMultiBlockDataSet.h"

#include <cassert>

// Description:
// Returns the list of probes.
vtkPointSet *GetProbes();

//
// Main
//
int main( int argc, char **argv )
{
  std::string file = std::string(argv[1]);
  vtkHierarchicalBoxDataSet *amrds = AMRCommon::ReadAMRData( file );
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );

  vtkPointSet *pntSet = GetProbes();
  assert( "pre: probes are NULL" && (pntSet != NULL) );

  vtkAMRProbeFilter *amrProbeFilter = vtkAMRProbeFilter::New();
  assert( "pre: AMR probe filter is NULL" && (amrProbeFilter != NULL) );

  amrProbeFilter->SetAMRDataSet( amrds );
  amrProbeFilter->SetProbePoints( pntSet );
  amrProbeFilter->Update();

  vtkMultiBlockDataSet *mbds = amrProbeFilter->GetOutput();
  assert( "pre: Multi-block dataset output is NULL" && (mbds != NULL) );

  AMRCommon::WriteMultiBlockData( mbds, "ProbedBlocks" );

  // De-allocate
//  mbds->Delete();
  amrds->Delete();
  pntSet->Delete();
  amrProbeFilter->Delete();
  return 0;
}

//------------------------------------------------------------------------------
vtkPointSet *GetProbes()
{
  vtkPoints *probes       = vtkPoints::New();
  probes->SetNumberOfPoints( 2 );

  probes->SetPoint(0,-1.0, -1.0, 0.0 );
  probes->SetPoint(1, 2.0,  1.0, 0.0 );
  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();
  ug->SetPoints( probes );
  return( ug );
}
