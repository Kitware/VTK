/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParticleTracers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRTAnalyticSource.h"
#include "vtkNew.h"
#include "vtkImageGradient.h"
#include "vtkImageData.h"
#include "vtkStreamTracer.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkDoubleArray.h"
#include <cassert>

int TestFieldNames(int, char*[])
{
  //create a multiblock data set of two images
  //with touching x extents so stream traces can
  //go from one to the other

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-10,0,-10,10,-10,10);

  vtkNew<vtkImageGradient> gradient;
  gradient->SetDimensionality(3);
  gradient->SetInputConnection(source->GetOutputPort());
  gradient->Update();

  vtkSmartPointer<vtkImageData> image0 = vtkSmartPointer<vtkImageData>::New();
  image0->DeepCopy(vtkImageData::SafeDownCast(gradient->GetOutputDataObject(0)));
  image0->GetPointData()->SetActiveVectors("RTDataGradient");

  source->SetWholeExtent(0,10,-10,10,-10,10);
  gradient->Update();

  vtkSmartPointer<vtkImageData> image1 = vtkSmartPointer<vtkImageData>::New();
  image1->DeepCopy(vtkImageData::SafeDownCast(gradient->GetOutputDataObject(0)));

  vtkIdType numPts =  image0->GetNumberOfPoints();
  vtkSmartPointer<vtkDoubleArray> arr0 =vtkSmartPointer<vtkDoubleArray>::New();
  arr0->Allocate(numPts);
  arr0->SetNumberOfComponents(1);
  arr0->SetNumberOfTuples(image0->GetNumberOfPoints());
  for(vtkIdType idx=0; idx<numPts; idx++)
  {
    arr0->SetTuple1(idx, 1.0);
  }
  arr0->SetName("array 0");
  image0->GetPointData()->AddArray(arr0);

  vtkSmartPointer<vtkDoubleArray> arr1 =vtkSmartPointer<vtkDoubleArray>::New();
  arr1->Allocate(numPts);
  arr1->SetName("array 1");
  for(vtkIdType idx=0; idx<numPts; idx++)
  {
    arr1->SetTuple1(idx, 2.0);
  }
  image1->GetPointData()->AddArray(arr1);

  vtkNew<vtkMultiBlockDataSet> dataSets;
  dataSets->SetNumberOfBlocks( 2 );
  dataSets->SetBlock( 0, image0 );
  dataSets->SetBlock( 1, image1 );

  //create one seed
  vtkNew<vtkPolyData> seeds;
  vtkNew<vtkPoints> seedPoints;
  seedPoints->InsertNextPoint(-4.0,0,0);
  seeds->SetPoints(seedPoints.GetPointer());


  //perform the tracing and watch for warning
  vtkNew<vtkStreamTracer> tracer;
  tracer->SetSourceData(seeds.GetPointer());
  tracer->SetInputData(dataSets.GetPointer());
  tracer->SetMaximumPropagation(20.0);

  //run the tracing
  tracer->Update();

  //verify results
  vtkPolyData* trace = vtkPolyData::SafeDownCast(tracer->GetOutputDataObject(0));
  if(   trace->GetPointData()->GetArray("array 0")!=NULL
     || trace->GetPointData()->GetArray("array 1")!=NULL
     || trace->GetPointData()->GetArray("RTData")==NULL
     || trace->GetNumberOfPoints()==0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestStreamTracer(int n, char* a[])
{
  int numFailures(0);
  numFailures += TestFieldNames(n,a);
  return numFailures;
}
