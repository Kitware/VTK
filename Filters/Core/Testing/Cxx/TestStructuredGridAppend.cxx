/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredGridAppend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include <vtkStructuredGridAppend.h>
#include <vtkCellData.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkStructuredGrid.h>
#include <vtkSmartPointer.h>

namespace
{
  const char arrayName[] = "coordinates";

//////////////////////////////////////////////////////////////////////////////
// Create a dataset for testing
//////////////////////////////////////////////////////////////////////////////
  void CreateDataset(vtkStructuredGrid* dataset, int extent[6])
  {
    dataset->SetExtent(extent);
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    // create a point data array
    vtkNew<vtkDoubleArray> pointArray;
    pointArray->SetName(arrayName);
    dataset->GetPointData()->AddArray(pointArray.GetPointer());
    for(int k=extent[4];k<=extent[5];k++)
      {
      for(int j=extent[2];j<=extent[3];j++)
        {
        for(int i=extent[0];i<=extent[1];i++)
          {
          points->InsertNextPoint((double)i, (double)j, (double)k);
          pointArray->InsertNextValue((double)i);
          }
        }
      }
    dataset->SetPoints(points);

    // create a cell data array
    vtkNew<vtkIntArray> cellArray;
    cellArray->SetName(arrayName);
    cellArray->SetNumberOfComponents(3);
    dataset->GetCellData()->AddArray(cellArray.GetPointer());
    int ijk[3];
    for(int k=extent[4];k<extent[5];k++)
      {
      ijk[2] = k;
      for(int j=extent[2];j<extent[3];j++)
        {
        ijk[1] = j;
        for(int i=extent[0];i<extent[1];i++)
          {
          ijk[0] = i;
          cellArray->InsertNextTupleValue(ijk);
          }
        }
      }
  }

//////////////////////////////////////////////////////////////////////////////
// Returns 1 on success, 0 otherwise
//////////////////////////////////////////////////////////////////////////////
  int AppendDatasetsAndCheck(
    const std::vector<vtkSmartPointer<vtkStructuredGrid> > inputs,
    int outputExtent[6])
  {
    vtkNew<vtkStructuredGridAppend> append;
    for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
      {
      append->AddInputData( inputs[inputIndex] );
      }
    append->Update();

    vtkStructuredGrid* output = append->GetOutput();
    int extent[6];
    output->GetExtent(extent);
    for(int i=0;i<6;i++)
      {
      if(extent[i] != outputExtent[i])
        {
        vtkGenericWarningMacro("ERROR: Extent is wrong.");
        return 0;
        }
      }

    if(vtkDoubleArray* pointArray =
       vtkDoubleArray::SafeDownCast(output->GetPointData()->GetArray(arrayName)))
      {
      vtkIdType counter = 0;
      for(int k=extent[4];k<=extent[5];k++)
        {
        for(int j=extent[2];j<=extent[3];j++)
          {
          for(int i=extent[0];i<=extent[1];i++)
            {
            double value = pointArray->GetValue(counter);
            if(value != (double)i)
              {
              vtkGenericWarningMacro("ERROR: Bad point array value " << value
                                     << " which should be " << (double)i);
              return 0;
              }
            counter++;
            }
          }
        }
      }
    else
      {
      vtkGenericWarningMacro("ERROR: Could not find point data array.");
      return 0;
      }

    if(vtkIntArray* cellArray =
       vtkIntArray::SafeDownCast(output->GetCellData()->GetArray(arrayName)))
      {
      vtkIdType counter = 0;
      for(int k=extent[4];k<extent[5];k++)
        {
        for(int j=extent[2];j<extent[3];j++)
          {
          for(int i=extent[0];i<extent[1];i++)
            {
            int values[3];
            cellArray->GetTupleValue(counter, values);
            if(values[0] != i || values[1] != j || values[2] != k)
              {
              vtkGenericWarningMacro("ERROR: Bad cell array tuple value ["
                                     << values[0] << ", " << values[1] << ", " << values[2]
                                     << "] which should be [" << i << ", " << j << ", " << k <<"]");
              return 0;
              }
            counter++;
            }
          }
        }
      }
    else
      {
      vtkGenericWarningMacro("ERROR: Could not find cell data array.");
      return 0;
      }

    return 1;
  }

} // end anonymous namespace

//////////////////////////////////////////////////////////////////////////////
int TestStructuredGridAppend( int, char* [])
{
  std::vector<vtkSmartPointer<vtkStructuredGrid> > inputs;
  int outputExtent[6] = {-1, 19, 0, 4, 0, 5};
  for(int i=0;i<3;i++)
    {
    int extent[6] = {i*6-1, (i+1)*6+1, 0, 4, 0, 5};
    vtkSmartPointer<vtkStructuredGrid> dataset =
      vtkSmartPointer<vtkStructuredGrid>::New();
    CreateDataset(dataset, extent);
    inputs.push_back(dataset);
    }

  if (!AppendDatasetsAndCheck(inputs, outputExtent))
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
