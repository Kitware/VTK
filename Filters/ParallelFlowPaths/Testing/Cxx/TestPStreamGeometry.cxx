/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStreamGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestVectorFieldSource.h"
#include <vtkDoubleArray.h>
#include <vtkPStreamTracer.h>
#include <vtkMPIController.h>
#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkCellArray.h>

#include <cstdlib>

#define PRINT(x) cout<<"("<<myRank<<")"<<x<<endl;

namespace Vec3
{
  void copy(double *src,double *dst)
  {
    src[0] = dst[0];
    src[1] = dst[1];
    src[2] = dst[2];
  }
};

inline double ComputeLength(vtkIdList* poly, vtkPoints* pts)
{
  int n = poly->GetNumberOfIds();
  if(n==0) return 0;

  double s(0);
  double p[3];
  pts->GetPoint(poly->GetId(0),p);
  for(int j=1; j<n;j++)
  {
    int pIndex = poly->GetId(j);
    double q[3];
    pts->GetPoint(pIndex,q);
    s+= sqrt( vtkMath::Distance2BetweenPoints(p,q));
    Vec3::copy(p,q);
  }
  return s;
}



int TestPStreamGeometry( int argc, char* argv[] )
{
  vtkNew<vtkMPIController> c;
  vtkMultiProcessController::SetGlobalController(c.GetPointer());
  c->Initialize(&argc,&argv);
  int numProcs = c->GetNumberOfProcesses();
  int myRank = c->GetLocalProcessId();
  if(numProcs!=4) return EXIT_SUCCESS;

  int size(5);
  vtkNew<TestVectorFieldSource> imageSource;
  imageSource->SetExtent(0,size-1,0,1,0,size-1);
  imageSource->SetBoundingBox(-1,1,-1,1,-1,1);

  double stepSize(0.01);
  double radius = 0.8;
  double scale = 1;
  double maximumPropagation = radius*scale*2*vtkMath::Pi();
  double angle = vtkMath::Pi()/20;
  int numTraces=1;

  vtkNew<vtkPStreamTracer> tracer;
  tracer->SetInputConnection(0,imageSource->GetOutputPort());
  tracer->SetIntegrationDirectionToForward();
  tracer->SetIntegratorTypeToRungeKutta4();
  tracer->SetMaximumNumberOfSteps(4*maximumPropagation/stepSize); //shouldn't have to do this fix in stream tracer somewhere!
  tracer->SetMinimumIntegrationStep(stepSize*.1);
  tracer->SetMaximumIntegrationStep(stepSize);
  tracer->SetInitialIntegrationStep(stepSize);
  double start[2] = {radius*cos(angle),radius*sin(angle)};
  vtkNew<vtkPolyData> seeds;
  {
    vtkNew<vtkPoints> seedPoints;
    double dt =  numTraces==1? 0 :1.8/(numTraces-1);
    for(int i=0; i<numTraces;i++)
      seedPoints->InsertNextPoint(start[0],numTraces==1? 0: -0.9+dt*i,start[1]);
    seedPoints->InsertNextPoint(-2,-2,-2); //out of bound point
    seeds->SetPoints(seedPoints.GetPointer());
  }
  tracer->SetInputData(1,seeds.GetPointer());
  tracer->SetMaximumPropagation(maximumPropagation);

  vtkSmartPointer<vtkPolyDataMapper> traceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  traceMapper->SetInputConnection(tracer->GetOutputPort());
  traceMapper->SetPiece(myRank);
  traceMapper->SetNumberOfPieces(numProcs);
  traceMapper->Update();

  vtkPolyData* out = tracer->GetOutput();
  vtkNew<vtkIdList> polyLine;

  vtkCellArray* lines = out->GetLines();
  double totalLength(0);
  lines->InitTraversal();
  while(lines->GetNextCell(polyLine.GetPointer()))
  {
    double d = ComputeLength(polyLine.GetPointer(),out->GetPoints());
    totalLength+=d;
  }

  double totalLengthAll(0);
  c->Reduce(&totalLength,&totalLengthAll,1,vtkCommunicator::SUM_OP,0);


  bool res(true);
  if(myRank==0)
  {
    double err = fabs(totalLengthAll - maximumPropagation)/maximumPropagation  ;
    PRINT("Error in length is: "<<err)
    res = err<0.02;
  }

  // Test IntegrationTime
  tracer->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Velocity");
  vtkNew<vtkPolyData> singleSeed;
  {
    vtkNew<vtkPoints> seedPoints;
    seedPoints->InsertNextPoint(.1, .1, 0);
    singleSeed->SetPoints(seedPoints.GetPointer());
  }
  tracer->SetInputData(1,singleSeed.GetPointer());
  tracer->SetIntegrationDirectionToBoth();
  traceMapper->Update();
  out = tracer->GetOutput();
  vtkDoubleArray* integrationTime =
    vtkArrayDownCast<vtkDoubleArray>(out->GetPointData()->GetArray("IntegrationTime"));
  for(vtkIdType i=0;i<out->GetNumberOfPoints();i++)
  {
    double coord[3];
    out->GetPoint(i, coord);
    double diff = std::abs(coord[2] - integrationTime->GetValue(i));
    if(diff != 0 && diff > std::abs(coord[2])*.0001)
    {
      PRINT("Bad integration time at z-coord "<< coord[2] << " " << integrationTime->GetValue(i))
      res = false;
    }
  }

  c->Finalize();

  return res? EXIT_SUCCESS: EXIT_FAILURE;

}
