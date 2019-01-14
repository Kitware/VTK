/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointCloudFilterArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>
#include <vtkRadiusOutlierRemoval.h>

#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>

#include <vtkUnsignedCharArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkShortArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkLongArray.h>

template<typename T> vtkSmartPointer<T> MakeArray (const std::string &name);

int TestPointCloudFilterArrays(int, char*[])
{
  const double pt1[3] = { 0, 0, 0};
  const double pt2[3] = { 1, 0, 0};
  const double pt3[3] = { 2, 0, 0};

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);
  points->InsertNextPoint(pt3);

  // Generate arrays of integral types
  vtkSmartPointer<vtkUnsignedCharArray> uca =
    MakeArray<vtkUnsignedCharArray> ("uca");
  vtkSmartPointer<vtkCharArray> ca =
    MakeArray<vtkCharArray> ("ca");

  vtkSmartPointer<vtkUnsignedShortArray> usa =
    MakeArray<vtkUnsignedShortArray> ("usa");
  vtkSmartPointer<vtkShortArray> sa =
    MakeArray<vtkShortArray> ("sa");

  vtkSmartPointer<vtkUnsignedIntArray> uia =
    MakeArray<vtkUnsignedIntArray> ("uia");
  vtkSmartPointer<vtkIntArray> ia =
    MakeArray<vtkIntArray> ("ia");

  vtkSmartPointer<vtkUnsignedLongArray> ula =
    MakeArray<vtkUnsignedLongArray> ("ula");
  vtkSmartPointer<vtkLongArray> la =
    MakeArray<vtkLongArray> ("la");

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->GetPointData()->AddArray(uca);
  polyData->GetPointData()->AddArray(ca);
  polyData->GetPointData()->AddArray(usa);
  polyData->GetPointData()->AddArray(sa);
  polyData->GetPointData()->AddArray(uia);
  polyData->GetPointData()->AddArray(ia);
  polyData->GetPointData()->AddArray(ula);
  polyData->GetPointData()->AddArray(la);

  vtkSmartPointer<vtkRadiusOutlierRemoval> outlierRemoval =
    vtkSmartPointer<vtkRadiusOutlierRemoval>::New();
  outlierRemoval->SetInputData(polyData);
  outlierRemoval->SetRadius(1.5);
  outlierRemoval->SetNumberOfNeighbors(2);
  outlierRemoval->Update();

  vtkPointData *inPD = polyData->GetPointData();
  vtkPointData *outPD = outlierRemoval->GetOutput()->GetPointData();
  // Number of arrays should match
  if (inPD->GetNumberOfArrays() != outPD->GetNumberOfArrays())
    {
    std::cout << "ERROR: Number of input arrays : " << inPD->GetNumberOfArrays()
              << " != " << outPD->GetNumberOfArrays() << std::endl;
    return EXIT_FAILURE;
    }
  // Types should not change
  int status = 0;
  for (int i = 0; i < outPD->GetNumberOfArrays(); ++i)
  {
    vtkDataArray *outArray = outPD->GetArray(i);
    vtkDataArray *inArray = inPD->GetArray(i);
    if (inArray->GetDataType() != outArray->GetDataType())
    {
     std::cout << "ERROR: Output array: " << outArray->GetName() << ", type: " << outArray->GetDataTypeAsString()
               << " does not match "
               << "Input array: " << inArray->GetName() << ", type: " << inArray->GetDataTypeAsString()
               << std::endl;
       ++status;
    }
  }
  return status == 0 ? EXIT_SUCCESS: EXIT_FAILURE;
}

template<typename T> vtkSmartPointer<T> MakeArray (const std::string &name)
{
  vtkSmartPointer<T> array = vtkSmartPointer<T>::New();
  array->SetName(name.c_str());
  array->SetNumberOfComponents(1);
  array->InsertNextValue(1);
  array->InsertNextValue(2);
  array->InsertNextValue(3);
  return array;
}
