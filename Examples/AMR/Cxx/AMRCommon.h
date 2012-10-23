/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRCommon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRCommon.h -- Encapsulates common functionality for AMR data.
//
// .SECTION Description
// This header encapsulates some common functionality for AMR data to
// simplify and expedite the development of examples.

#ifndef AMRCOMMON_H_
#define AMRCOMMON_H_

#include <cassert> // For C++ assert
#include <sstream> // For C++ string streams

#include "vtkCell.h"
#include "vtkCompositeDataWriter.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOverlappingAMR.h"
#include "vtkStructuredGridWriter.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGrid.h"
#include "vtkXMLHierarchicalBoxDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkXMLUniformGridAMRWriter.h"

namespace AMRCommon {



//------------------------------------------------------------------------------
// Description:
// Writes a uniform grid as a structure grid
void WriteUniformGrid( vtkUniformGrid *g, std::string prefix )
{
  assert( "pre: Uniform grid (g) is NULL!" && (g != NULL) );

  vtkXMLImageDataWriter *imgWriter = vtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << imgWriter->GetDefaultFileExtension();
  imgWriter->SetFileName( oss.str().c_str() );
  imgWriter->SetInputData( g );
  imgWriter->Write();

  imgWriter->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Writes the given AMR dataset to a *.vth file with the given prefix.
void WriteAMRData( vtkOverlappingAMR *amrData, std::string prefix )
{
  // Sanity check
  assert( "pre: AMR dataset is NULL!" && (amrData != NULL) );

  vtkCompositeDataWriter *writer = vtkCompositeDataWriter::New();

  std::ostringstream oss;
  oss << prefix << ".vthb";
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(amrData);
  writer->Write();
  writer->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Reads AMR data to the given data-structure from the prescribed file.
vtkHierarchicalBoxDataSet* ReadAMRData( std::string file )
{
  // Sanity check
//  assert( "pre: AMR dataset is NULL!" && (amrData != NULL) );

  vtkXMLHierarchicalBoxDataReader *myAMRReader=
   vtkXMLHierarchicalBoxDataReader::New();
  assert( "pre: AMR Reader is NULL!" && (myAMRReader != NULL) );

  std::ostringstream oss;
  oss.str("");
  oss.clear();
  oss << file << ".vthb";

  std::cout << "Reading AMR Data from: " << oss.str() << std::endl;
  std::cout.flush();

  myAMRReader->SetFileName( oss.str().c_str() );
  myAMRReader->Update();

  vtkHierarchicalBoxDataSet *amrData =
   vtkHierarchicalBoxDataSet::SafeDownCast( myAMRReader->GetOutput() );
  assert( "post: AMR data read is NULL!" && (amrData != NULL) );
  return( amrData );
}

//------------------------------------------------------------------------------
// Description:
// Writes the given multi-block data to an XML file with the prescribed prefix
void WriteMultiBlockData( vtkMultiBlockDataSet *mbds, std::string prefix )
{
  // Sanity check
  assert( "pre: Multi-block dataset is NULL" && (mbds != NULL) );
  vtkXMLMultiBlockDataWriter *writer = vtkXMLMultiBlockDataWriter::New();

  std::ostringstream oss;
  oss.str(""); oss.clear();
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str( ).c_str( ) );
  writer->SetInputData( mbds );
  writer->Write();
  writer->Delete();
}

//------------------------------------------------------------------------------
// Constructs a uniform grid instance given the prescribed
// origin, grid spacing and dimensions.
vtkUniformGrid* GetGrid( double* origin,double* h,int* ndim )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin( origin );
  grd->SetSpacing( h );
  grd->SetDimensions( ndim );
  return grd;
}

//------------------------------------------------------------------------------
// Computes the cell center for the cell corresponding to cellIdx w.r.t.
// the given grid. The cell center is stored in the supplied buffer c.
void ComputeCellCenter( vtkUniformGrid *grid, const int cellIdx, double c[3] )
{
  assert( "pre: grid != NULL" && (grid != NULL) );
  assert( "pre: Null cell center buffer" && (c != NULL)  );
  assert( "pre: cellIdx in bounds" &&
          (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells() ) );

  vtkCell *myCell = grid->GetCell( cellIdx );
  assert( "post: cell is NULL" && (myCell != NULL) );

  double pCenter[3];
  double *weights = new double[ myCell->GetNumberOfPoints() ];
  int subId       = myCell->GetParametricCenter( pCenter );
  myCell->EvaluateLocation( subId,pCenter,c,weights );
  delete [] weights;
}


} // END namespace

#endif /* AMRCOMMON_H_ */
