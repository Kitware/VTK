/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeometryFilterCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test that the proper amount of tuples exist in the
// point and cell data arrays after the vtkGeometryFilter
// is used.

#include <iostream>

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGeometryFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

int TestGeometryFilter( vtkUnstructuredGrid* ug );
int CheckDataSet( vtkDataSet* d );
int CheckFieldData( vtkIdType numGridEntities, vtkFieldData* fd );

// Creates a vtkUnstructuredGrid
class GridFactory
{
public:
  GridFactory();

  void AddVertexCells();
  void AddLineCells();
  void AddTriangleCells();
  void AddTetraCells();
  vtkUnstructuredGrid* Get();

private:
  vtkSmartPointer<vtkUnstructuredGrid> Grid;
};

GridFactory::GridFactory() :
  Grid ( vtkSmartPointer<vtkUnstructuredGrid>::New() )
{
  // the points
  static float x[8][3]={{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0},
                        {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}};

  std::cout << "Defining 8 points\n";
  // Create the points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints( 8 );
  for ( int i = 0; i < 8; ++i )
  {
    points->SetPoint( i, x[i] );
  }
  this->Grid->SetPoints( points );
}

// Create 2 tetras
void GridFactory::AddTetraCells()
{
  if ( ! this->Grid )
    return;

  std::cout << "Adding 2 tetra cells\n";
  vtkIdType pts[4];
  pts[0] = 0; pts[1] = 1; pts[2] = 2; pts[3] = 3;
  this->Grid->InsertNextCell( VTK_TETRA, 4, pts );
  pts[0] = 2; pts[1] = 3; pts[2] = 4; pts[3] = 5;
  this->Grid->InsertNextCell( VTK_TETRA, 4, pts );
}

// Create 2 triangles
void GridFactory::AddTriangleCells()
{
  if ( ! this->Grid )
    return;

  std::cout << "Adding 2 triangle cells\n";

  vtkIdType pts[3];
  pts[0] = 1; pts[1] = 3; pts[2] = 5;
  this->Grid->InsertNextCell( VTK_TRIANGLE, 3, pts );
  pts[0] = 2; pts[1] = 4; pts[2] = 6;
  this->Grid->InsertNextCell( VTK_TRIANGLE, 3, pts );
}

// Create 2 lines
void GridFactory::AddLineCells()
{
  if ( ! this->Grid )
    return;

  std::cout << "Adding 2 line cells\n";

  vtkIdType pts[2];
  pts[0] = 3; pts[1] = 7;
  this->Grid->InsertNextCell( VTK_LINE, 2, pts );
  pts[0] = 0; pts[1] = 4;
  this->Grid->InsertNextCell( VTK_LINE, 2, pts );
}

// Create 2 points
void GridFactory::AddVertexCells()
{
  if ( ! this->Grid )
    return;

  std::cout << "Adding 2 vertex cells\n";

  vtkIdType pts[1];
  pts[0] = 7;
  this->Grid->InsertNextCell( VTK_VERTEX, 1, pts );
  pts[0] = 6;
  this->Grid->InsertNextCell( VTK_VERTEX, 1, pts );
}

// Add cell data and point data for all cells/points, and
// return the unstructured grid.
vtkUnstructuredGrid* GridFactory::Get()
{
  if ( ! this->Grid )
    return NULL;

  // Create a point data array
  const char* name = "foo";
  int num = this->Grid->GetNumberOfPoints();
  std::cout << "Adding point data array '"<<name<<"' with data for "<<num<<" points\n";
  vtkSmartPointer<vtkIdTypeArray> pointDataArray = vtkSmartPointer<vtkIdTypeArray>::New();
  pointDataArray->SetName( name );
  // Creating data for 8 points
  for ( int i = 0; i < num; ++i )
  {
    vtkIdType value = i+100;
    pointDataArray->InsertNextTypedTuple( &value );
  }
  this->Grid->GetPointData()->AddArray( pointDataArray );

  // Create the cell data array
  name = "bar";
  num = this->Grid->GetNumberOfCells();
  std::cout << "Adding cell data array '"<<name<<"' with data for "<<num<<" cells\n";
  vtkSmartPointer<vtkIdTypeArray> cellDataArray = vtkSmartPointer<vtkIdTypeArray>::New();
  cellDataArray->SetName( name );
  cellDataArray->SetNumberOfComponents( 1 );
  for ( int i = 0; i < num; ++i )
  {
    vtkIdType value = i+200;
    cellDataArray->InsertNextTypedTuple( &value );
  }
  this->Grid->GetCellData()->AddArray( cellDataArray );

  return this->Grid;
}

int TestGeometryFilterCellData(int, char*[])
{
  vtkSmartPointer<vtkUnstructuredGrid> ug;

  // Build the unstructured grid
  GridFactory g;
  g.AddTetraCells();
  g.AddTriangleCells();
  g.AddLineCells();
  g.AddVertexCells();
  ug = g.Get();

  // Run it through vtkGeometryFilter
  int retVal = TestGeometryFilter( ug );

  return retVal;
}

// Runs the unstructured grid through the vtkGeometryFilter and prints the
// output
int TestGeometryFilter( vtkUnstructuredGrid* ug )
{
  // Print the input unstructured grid dataset
  std::cout << "\nvtkGeometryFilter input:\n";
  int retVal = CheckDataSet( ug );

  // Do the filtering
  vtkSmartPointer<vtkGeometryFilter> gf = vtkSmartPointer<vtkGeometryFilter>::New();
  gf->SetInputData( ug );
  gf->Update();

  // Print the output poly data
  std::cout << "\nvtkGeometryFilter output:\n";
  vtkPolyData *poly = vtkPolyData::SafeDownCast( gf->GetOutput( ) );
  retVal += CheckDataSet( poly );
  return retVal;
}

int CheckDataSet( vtkDataSet* d )
{
  if ( ! d )
  {
    std::cout << "No dataset\n";
    return 1;
  }

  const char* name;
  if ( vtkUnstructuredGrid::SafeDownCast( d ) )
    name = "vtkUnstructuredGrid";
  else if ( vtkPolyData::SafeDownCast( d ) )
    name = "vtkPolyData";
  else
    name = "vtkDataSet";

  std::cout << name
            << " dimensions: #cells="<<d->GetNumberOfCells()
            << " #points=" << d->GetNumberOfPoints()<< "\n";
  int retVal = CheckFieldData( d->GetNumberOfPoints(), d->GetPointData() );
  retVal += CheckFieldData( d->GetNumberOfCells(), d->GetCellData() );
  return retVal;
}

int CheckFieldData( vtkIdType numGridEntities, vtkFieldData* fd )
{
  int retVal = 0;
  if ( ! fd )
  {
    std::cout << "No field data\n";
    return 1;
  }

  const char* name;
  if ( vtkCellData::SafeDownCast( fd ) )
  {
    name = "cell data";
  }
  else if ( vtkPointData::SafeDownCast( fd ) )
  {
    name = "point data";
  }
  else
  {
    name = "field data";
  }

  for ( int i = 0; i < fd->GetNumberOfArrays(); ++i )
  {
    vtkAbstractArray *a = fd->GetArray( i );
    if(a->GetNumberOfTuples() != numGridEntities)
    {
      vtkGenericWarningMacro(<< name << " array '" << a->GetName() << "' has #tuples="<< a->GetNumberOfTuples()
                             << " but should have " << numGridEntities);
      retVal = 1;
    }
  }
  return retVal;
}
