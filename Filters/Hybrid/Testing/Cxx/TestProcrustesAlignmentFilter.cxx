/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProcrustesAlignmentFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMultiBlockDataSet.h>
#include <vtkPolyData.h>
#include <vtkProcrustesAlignmentFilter.h>
#include <vtkSmartPointer.h>

int TestProcrustesAlignmentFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkPoints> pointsArray[3];

  for(unsigned int i = 0; i < 3; ++i)
    {
    pointsArray[i] = vtkSmartPointer<vtkPoints>::New();
    }

  pointsArray[0]->InsertNextPoint(-1.58614838, -0.66562307, -0.20268087);
  pointsArray[0]->InsertNextPoint(-0.09052952, -1.53144991, 0.80403084);
  pointsArray[0]->InsertNextPoint(-1.17059791, 1.07974386, 0.68106824);
  pointsArray[0]->InsertNextPoint(0.32502091, 0.21391694, 1.68777990);
  pointsArray[0]->InsertNextPoint(-0.32502091, -0.21391694, -1.68777990);
  pointsArray[0]->InsertNextPoint(1.17059791, -1.07974386, -0.68106824);
  pointsArray[0]->InsertNextPoint(0.09052952, 1.53144991, -0.80403084);
  pointsArray[0]->InsertNextPoint(1.58614838, 0.66562307, 0.20268087);

  pointsArray[1]->InsertNextPoint(-1.58614838, -0.66562307, -0.20268087);
  pointsArray[1]->InsertNextPoint(-0.09052952, -1.53144991, 0.80403084);
  pointsArray[1]->InsertNextPoint(-1.17059791, 1.07974386, 0.68106824);
  pointsArray[1]->InsertNextPoint(0.32502091, 0.21391694, 1.68777990);
  pointsArray[1]->InsertNextPoint(-0.32502091, -0.21391694, -1.68777990);
  pointsArray[1]->InsertNextPoint(1.17059791, -1.07974386, -0.68106824);
  pointsArray[1]->InsertNextPoint(0.09052952, 1.53144991, -0.80403084);
  pointsArray[1]->InsertNextPoint(1.58614838, 0.66562307, 0.20268087);

  pointsArray[2]->InsertNextPoint(-1.58614838, -0.66562307, -0.20268087);
  pointsArray[2]->InsertNextPoint(-0.09052952, -1.53144991, 0.80403084);
  pointsArray[2]->InsertNextPoint(-1.17059791, 1.07974386, 0.68106824);
  pointsArray[2]->InsertNextPoint(0.32502091, 0.21391694, 1.68777990);
  pointsArray[2]->InsertNextPoint(-0.32502091, -0.21391694, -1.68777990);
  pointsArray[2]->InsertNextPoint(1.17059791, -1.07974386, -0.68106824);
  pointsArray[2]->InsertNextPoint(0.09052952, 1.53144991, -0.80403084);
  pointsArray[2]->InsertNextPoint(1.58614838, 0.66562307, 0.20268087);

  vtkSmartPointer<vtkMultiBlockDataSet> inputMultiBlockDataSet = vtkSmartPointer<vtkMultiBlockDataSet>::New();

  vtkSmartPointer<vtkProcrustesAlignmentFilter> procrustesAlignmentFilter = vtkSmartPointer<vtkProcrustesAlignmentFilter>::New();
  procrustesAlignmentFilter->SetInputData(inputMultiBlockDataSet);
  procrustesAlignmentFilter->StartFromCentroidOff();

  procrustesAlignmentFilter->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
    inputPoints->SetDataType(VTK_FLOAT);
    inputPoints->DeepCopy(pointsArray[i]);

    vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
    }

  procrustesAlignmentFilter->Update();

  vtkSmartPointer<vtkPoints> meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if(meanPoints->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkMultiBlockDataSet> outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    vtkSmartPointer<vtkPolyData> outputPolyData = vtkPolyData::SafeDownCast(dataObject);
    vtkSmartPointer<vtkPoints> outputPoints = outputPolyData->GetPoints();

    if(outputPoints->GetDataType() != VTK_FLOAT)
      {
      return EXIT_FAILURE;
      }
    }

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
    inputPoints->SetDataType(VTK_DOUBLE);
    inputPoints->DeepCopy(pointsArray[i]);

    vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
    }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if(meanPoints->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    vtkSmartPointer<vtkPolyData> outputPolyData = vtkPolyData::SafeDownCast(dataObject);
    vtkSmartPointer<vtkPoints> outputPoints = outputPolyData->GetPoints();

    if(outputPoints->GetDataType() != VTK_DOUBLE)
      {
      return EXIT_FAILURE;
      }
    }

  procrustesAlignmentFilter->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
    inputPoints->SetDataType(VTK_FLOAT);
    inputPoints->DeepCopy(pointsArray[i]);

    vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
    }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if(meanPoints->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    vtkSmartPointer<vtkPolyData> outputPolyData = vtkPolyData::SafeDownCast(dataObject);
    vtkSmartPointer<vtkPoints> outputPoints = outputPolyData->GetPoints();

    if(outputPoints->GetDataType() != VTK_FLOAT)
      {
      return EXIT_FAILURE;
      }
    }

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
    inputPoints->SetDataType(VTK_DOUBLE);
    inputPoints->DeepCopy(pointsArray[i]);

    vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
    }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if(meanPoints->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    vtkSmartPointer<vtkPolyData> outputPolyData = vtkPolyData::SafeDownCast(dataObject);
    vtkSmartPointer<vtkPoints> outputPoints = outputPolyData->GetPoints();

    if(outputPoints->GetDataType() != VTK_FLOAT)
      {
      return EXIT_FAILURE;
      }
    }

  procrustesAlignmentFilter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
    inputPoints->SetDataType(VTK_FLOAT);
    inputPoints->DeepCopy(pointsArray[i]);

    vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
    }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if(meanPoints->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    vtkSmartPointer<vtkPolyData> outputPolyData = vtkPolyData::SafeDownCast(dataObject);
    vtkSmartPointer<vtkPoints> outputPoints = outputPolyData->GetPoints();

    if(outputPoints->GetDataType() != VTK_DOUBLE)
      {
      return EXIT_FAILURE;
      }
    }

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
    inputPoints->SetDataType(VTK_DOUBLE);
    inputPoints->DeepCopy(pointsArray[i]);

    vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
    }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if(meanPoints->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for(unsigned int i = 0; i < 3; ++i)
    {
    vtkSmartPointer<vtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    vtkSmartPointer<vtkPolyData> outputPolyData = vtkPolyData::SafeDownCast(dataObject);
    vtkSmartPointer<vtkPoints> outputPoints = outputPolyData->GetPoints();

    if(outputPoints->GetDataType() != VTK_DOUBLE)
      {
      return EXIT_FAILURE;
      }
    }

  return EXIT_SUCCESS;
}
