/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkCutter.h"
#include "vtkRTAnalyticSource.h"
#include "vtkPolyData.h"
#include "vtkPlane.h"
#include "vtkPolygonBuilder.h"
#include "vtkPoints.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkPointDataToCellData.h"
#include "vtkImageDataToPointSet.h"
#include <assert.h>

bool TestStructured(int type)
{
  vtkSmartPointer<vtkRTAnalyticSource> imageSource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2,2,-2,2,-2,2);

  vtkSmartPointer<vtkAlgorithm>  filter;
  if(type==0)
    {
    filter = imageSource;
    }
  else
    {
    filter = vtkSmartPointer<vtkImageDataToPointSet>::New();
    filter->SetInputConnection(imageSource->GetOutputPort());
    }

  vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
  vtkSmartPointer<vtkPlane> p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(-1.5,-1.5,-1.5);
  p3d->SetNormal(1,1,1);

  cutter->SetCutFunction(p3d);
  cutter->SetInputConnection(0, filter->GetOutputPort());

  cutter->SetGenerateTriangles(0);
  cutter->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if(output->GetNumberOfCells()!=4 || output->CheckAttributes())
    {
    return false;
    }

  cutter->SetGenerateTriangles(1);
  cutter->Update();
  output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if(output->GetNumberOfCells()!=7 || output->CheckAttributes())
    {
    return false;
    }
  return true;
}

bool TestUnstructured()
{
  vtkSmartPointer<vtkRTAnalyticSource> imageSource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2,2,-2,2,-2,2);

  vtkSmartPointer<vtkPointDataToCellData> dataFilter = vtkSmartPointer<vtkPointDataToCellData>::New();
  dataFilter->SetInputConnection(imageSource->GetOutputPort());

  vtkSmartPointer<vtkDataSetTriangleFilter> tetraFilter = vtkSmartPointer<vtkDataSetTriangleFilter>::New();
  tetraFilter->SetInputConnection(dataFilter->GetOutputPort());

  vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
  vtkSmartPointer<vtkPlane> p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(-1.5,-1.5,-1.5);
  p3d->SetNormal(1,1,1);

  cutter->SetCutFunction(p3d);
  cutter->SetInputConnection(0, tetraFilter->GetOutputPort());

  cutter->SetGenerateTriangles(0);
  cutter->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if(output->GetNumberOfCells()!=7)
    {
    return false;
    }

  cutter->SetGenerateTriangles(1);
  cutter->Update();
  output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if(output->GetNumberOfCells()!=10)
    {
    return false;
    }
  return true;
}

int TestCutter(int, char *[])
{
  for(int type=0; type<2; type++)
    {
    if(!TestStructured(type))
      {
      cerr<<"Cutting Structured failed"<<endl;
      return EXIT_FAILURE;
      }
    }

  if(!TestUnstructured())
    {
    cerr<<"Cutting Unstructured failed"<<endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
