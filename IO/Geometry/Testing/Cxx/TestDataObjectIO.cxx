/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataObjectIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellData.h"
#include "vtkCubeSource.h"
#include "vtkDataObjectWriter.h"
#include "vtkDelaunay3D.h"
#include "vtkEdgeListIterator.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVariant.h"
#include "vtkMultiPieceDataSet.h"

void InitializeData(vtkPolyData* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(vtkPolyData* Output, vtkPolyData* Input)
{
  if(Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if(Input->GetNumberOfPolys() != Output->GetNumberOfPolys())
    return false;

  return true;
}

void InitializeData(vtkRectilinearGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
}

bool CompareData(vtkRectilinearGrid* Output, vtkRectilinearGrid* Input)
{
  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(vtkStructuredGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
}

bool CompareData(vtkStructuredGrid* Output, vtkStructuredGrid* Input)
{
  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(vtkTable* Data)
{
  vtkIntArray* const column1 = vtkIntArray::New();
  Data->AddColumn(column1);
  column1->Delete();
  column1->SetName("column1");

  vtkIntArray* const column2 = vtkIntArray::New();
  Data->AddColumn(column2);
  column2->Delete();
  column2->SetName("column2");

  Data->InsertNextBlankRow();
  Data->InsertNextBlankRow();
  Data->InsertNextBlankRow();

  Data->SetValue(0, 0, 1);
  Data->SetValue(0, 1, 2);
  Data->SetValue(1, 0, 3);
  Data->SetValue(1, 1, 4);
  Data->SetValue(2, 0, 5);
  Data->SetValue(2, 1, 6);
}

bool CompareData(vtkTable* Output, vtkTable* Input)
{
  if(Input->GetNumberOfColumns() != Output->GetNumberOfColumns())
    return false;
  if(Input->GetNumberOfRows() != Output->GetNumberOfRows())
    return false;

  for(int column = 0; column != Input->GetNumberOfColumns(); ++column)
    {
    for(int row = 0; row != Input->GetNumberOfRows(); ++row)
      {
      if(Input->GetValue(row, column).ToDouble() != Output->GetValue(row, column).ToDouble())
        {
        return false;
        }
      }
    }

  return true;
}

void InitializeData(vtkUnstructuredGrid* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  vtkDelaunay3D* const delaunay = vtkDelaunay3D::New();
  delaunay->AddInputConnection(source->GetOutputPort());
  delaunay->Update();

  Data->ShallowCopy(delaunay->GetOutput());

  delaunay->Delete();
  source->Delete();
}

bool CompareData(vtkUnstructuredGrid* Output, vtkUnstructuredGrid* Input)
{
  if(Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if(Input->GetNumberOfCells() != Output->GetNumberOfCells())
    return false;

  return true;
}

template<typename DataT>
bool TestDataObjectSerialization()
{
  DataT* const output_data = DataT::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  vtkGenericDataObjectWriter* const writer = vtkGenericDataObjectWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  vtkGenericDataObjectReader* const reader = vtkGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  vtkDataObject *obj = reader->GetOutput();
  DataT* const input_data = DataT::SafeDownCast(obj);
  if(!input_data)
    {
    reader->Delete();
    output_data->Delete();
    return false;
    }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}

int TestDataObjectIO(int /*argc*/, char* /*argv*/[])
{
  int result = 0;

  if(!TestDataObjectSerialization<vtkPolyData>())
    {
    cerr << "Error: failure serializing vtkPolyData" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkRectilinearGrid>())
    {
    cerr << "Error: failure serializing vtkRectilinearGrid" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkStructuredGrid>())
    {
    cerr << "Error: failure serializing vtkStructuredGrid" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkTable>())
    {
    cerr << "Error: failure serializing vtkTable" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkUnstructuredGrid>())
    {
    cerr << "Error: failure serializaing vtkUnstructuredGrid" << endl;
    result = 1;
    }

  return result;
}
