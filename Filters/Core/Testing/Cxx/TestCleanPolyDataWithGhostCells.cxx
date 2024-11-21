// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkAppendDataSets.h>
#include <vtkAppendFilter.h>
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPPolyDataReader.h>

bool CheckOutput(vtkDataSet* output)
{
  for (int i = 0; i < output->GetPointData()->GetArray("RTData")->GetNumberOfTuples(); i++)
  {
    if (output->GetPointData()->GetArray("RTData")->GetTuple1(i) > 1000)
    {
      cerr << output->GetClassName() << " ";
      cerr << __LINE__ << ": "
           << "Ghost value is used instead of master value !" << endl;
      return false;
    }
  }

  if (output->GetNumberOfPoints() != 98)
  {
    cerr << output->GetClassName() << " ";
    cerr << __LINE__ << ": "
         << "Invalid number of points. Expected 98 but got " << output->GetNumberOfPoints() << endl;
    return false;
  }

  if (output->GetPointData()->GetArray("RTData")->GetNumberOfTuples() != 98)
  {
    cerr << output->GetClassName() << " ";
    cerr << __LINE__ << ": "
         << "Invalid point data array size. Expected 98 but got "
         << output->GetPointData()->GetArray("RTData")->GetNumberOfTuples() << endl;
    return false;
  }

  return true;
}

int TestCleanPolyDataWithGhostCells(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ghostBrokenScalars.pvtp");
  vtkNew<vtkXMLPPolyDataReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  vtkNew<vtkCleanPolyData> cleanPolyData;
  cleanPolyData->SetInputData(reader->GetOutput());
  cleanPolyData->SetPointMerging(false);
  cleanPolyData->Update();

  vtkDataSet* output = reader->GetOutput();
  if (output->GetNumberOfPoints() != 212)
  {
    cerr << output->GetClassName() << " ";
    cerr << __LINE__ << ": "
         << "Invalid number of points. Expected 212 but got " << output->GetNumberOfPoints()
         << endl;
    return false;
  }

  if (output->GetPointData()->GetArray("RTData")->GetNumberOfTuples() != 212)
  {
    cerr << output->GetClassName() << " ";
    cerr << __LINE__ << ": "
         << "Invalid point data array size. Expected 212 but got "
         << output->GetPointData()->GetArray("RTData")->GetNumberOfTuples() << endl;
    return false;
  }

  cleanPolyData->SetPointMerging(true);
  cleanPolyData->Update();

  if (!CheckOutput(vtkPolyData::SafeDownCast(cleanPolyData->GetOutput())))
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkAppendFilter> appendFilter;
  appendFilter->SetInputData(reader->GetOutput());
  appendFilter->SetMergePoints(true);
  appendFilter->Update();

  if (!CheckOutput(vtkUnstructuredGrid::SafeDownCast(appendFilter->GetOutput())))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
