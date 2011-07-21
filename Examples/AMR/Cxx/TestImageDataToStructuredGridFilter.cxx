/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataToStructuredGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestImageDataToStructuredGridFilter.cxx
//
// A simple utility that demonstrates & tests the functionality of the
// vtkImageDataToStructuredGridFilter.

#include "vtkImageToStructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkAssertUtils.hpp"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkStructuredGridWriter.h"

// Function Prototypes
vtkUniformGrid* GetGrid( double *origin, double *spacing, int *ndim );

int main( int argc, char **argv )
{
  double origin[3]  = { 0.0,0.0,0.0 };
  double spacing[3] = { 0.5,0.2,0.0 };
  int    ndim[3]    = { 10, 10, 1 };

  vtkUniformGrid* myGrid = GetGrid( origin, spacing, ndim );
  vtkAssertUtils::assertNotNull( myGrid, __FILE__, __LINE__ );
  vtkAssertUtils::assertEquals(
      myGrid->GetCellData()->GetNumberOfArrays(),1,__FILE__,__LINE__);
  vtkImageToStructuredGrid* myFilter = vtkImageToStructuredGrid::New( );
  vtkAssertUtils::assertNotNull( myFilter, __FILE__, __LINE__  );

  myFilter->SetInput( myGrid );
  myFilter->Update();
  vtkStructuredGrid* sGrid = myFilter->GetOutput();
  vtkAssertUtils::assertNotNull( sGrid, __FILE__, __LINE__ );

  myGrid->Delete();

  vtkStructuredGridWriter* myWriter = vtkStructuredGridWriter::New( );
  vtkAssertUtils::assertNotNull(myWriter,__FILE__,__LINE__);

  myWriter->SetInput( sGrid );
  myWriter->SetFileName( "myGrid.vtk" );
  myWriter->Update();
  myWriter->Delete();

  return 0;
}

//------------------------------------------------------------------------------
vtkUniformGrid* GetGrid( double *origin, double *spacing, int *ndim )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin( origin );
  grd->SetSpacing( spacing );
  grd->SetDimensions( ndim );

  vtkDoubleArray* pntData = vtkDoubleArray::New();
  pntData->SetName( "XYZ-NODE" );
  pntData->SetNumberOfComponents( 1 );
  pntData->SetNumberOfTuples( grd->GetNumberOfPoints() );

  double node[3];
  for( int pntIdx=0; pntIdx < grd->GetNumberOfPoints(); ++pntIdx )
    {
      grd->GetPoint( pntIdx, node );
      pntData->SetValue(pntIdx, (node[0]+node[1]+node[2]) );
    } // END for all points
  grd->GetPointData()->AddArray( pntData );


  vtkDoubleArray* xyz = vtkDoubleArray::New( );
  xyz->SetName( "XYZ-CELL" );
  xyz->SetNumberOfComponents( 1 );
  xyz->SetNumberOfTuples( grd->GetNumberOfCells() );

  for( int cellIdx=0; cellIdx < grd->GetNumberOfCells(); ++cellIdx )
    {
      vtkCell* myCell = grd->GetCell( cellIdx);
      vtkAssertUtils::assertNotNull( myCell, __FILE__, __LINE__ );

      vtkPoints *cellPoints = myCell->GetPoints();
      vtkAssertUtils::assertNotNull( cellPoints, __FILE__, __LINE__ );

      double xyzCenter[3];
      xyzCenter[0] = 0.0;
      xyzCenter[1] = 0.0;
      xyzCenter[2] = 0.0;

      for( int cp=0; cp < cellPoints->GetNumberOfPoints(); ++cp )
        {
          double pnt[3];
          cellPoints->GetPoint( cp, pnt );
          xyzCenter[0] += pnt[0];
          xyzCenter[1] += pnt[1];
          xyzCenter[2] += pnt[2];
        } // END for all cell points

      xyzCenter[0] = xyzCenter[0] / (cellPoints->GetNumberOfPoints());
      xyzCenter[1] = xyzCenter[1] / (cellPoints->GetNumberOfPoints());
      xyzCenter[2] = xyzCenter[2] / (cellPoints->GetNumberOfPoints());

      double f = xyzCenter[0]*xyzCenter[0] +
          xyzCenter[1]*xyzCenter[1] + xyzCenter[2]*xyzCenter[2];
      xyz->SetTuple1(cellIdx,f);

    } // END for all cells

  grd->GetCellData()->AddArray(xyz);
  return grd;
}
