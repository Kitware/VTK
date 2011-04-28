/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRSlicer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRSlicer.cxx -- Slices a 3-D AMR dataset into a 2-D AMR.
//
// .SECTION Description
//  A simple utility code that demonstrates & tests the functionality of the
//  AMR slicer.

#include <cmath>
#include <sstream>
#include <cassert>
#include <mpi.h>

#include "vtkUniformGrid.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkAMRUtilities.h"
#include "vtkAMRSliceFilter.h"

#include "AMRCommon.h"


int main( int argc, char **argv )
{
  vtkHierarchicalBoxDataSet *myAMR =
    AMRCommon::ReadAMRData( std::string( argv[1] ) );
  assert( "pre: AMR dataset is NULL" && (myAMR != NULL) );

  int normal    = atoi( argv[2] );
  double offset = atof( argv[3] );

  vtkAMRSliceFilter *slicer = vtkAMRSliceFilter::New();
  slicer->SetInput( myAMR );
  slicer->SetNormal( normal );
  slicer->SetOffSetFromOrigin( offset );
  slicer->Update();

  vtkHierarchicalBoxDataSet *sliceAMR = slicer->GetOutput();
  assert( "pre: slice AMR dataset is NULL" && (sliceAMR != NULL) );
  AMRCommon::WriteAMRData( sliceAMR, "SLICE" );

//  myAMR->Delete();
//  slicer->Delete();
//  sliceAMR->Delete();
  return 0;
}
