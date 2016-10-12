/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPParticleTracers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAlgorithm.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkPParticlePathFilter.h"
#include "vtkPParticleTracer.h"
#include "vtkPStreaklineFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <vector>

#define EXPECT(expected,actual,msg,so)                        \
  if(!(expected == actual)) {                               \
  vtkGenericWarningMacro(<<msg<< " Expecting a value of " << expected << \
                         " but getting a value of " << actual << " for static option of " << so); \
  return EXIT_FAILURE;\
  }

using namespace std;
class TestTimeSource : public vtkAlgorithm
{
public:
  static TestTimeSource *New();
  vtkTypeMacro(TestTimeSource,vtkAlgorithm);

  void SetBoundingBox(double x0, double x1, double y0,
                      double y1, double z0, double z1)
  {
    this->BoundingBox[0] = x0;
    this->BoundingBox[1] = x1;
    this->BoundingBox[2] = y0;
    this->BoundingBox[3] = y1;
    this->BoundingBox[4] = z0;
    this->BoundingBox[5] = z1;
  }

  void SetExtent(int xMin, int xMax, int yMin, int yMax,
                    int zMin, int zMax)
  {
    int modified = 0;

    if (this->Extent[0] != xMin)
    {
      modified = 1;
      this->Extent[0] = xMin ;
    }
    if (this->Extent[1] != xMax)
    {
      modified = 1;
      this->Extent[1] = xMax ;
    }
    if (this->Extent[2] != yMin)
    {
      modified = 1;
      this->Extent[2] = yMin ;
    }
    if (this->Extent[3] != yMax)
    {
      modified = 1;
      this->Extent[3] = yMax ;
    }
    if (this->Extent[4] != zMin)
    {
      modified = 1;
      this->Extent[4] = zMin ;
    }
    if (this->Extent[5] != zMax)
    {
      modified = 1;
      this->Extent[5] = zMax ;
    }
    if (modified)
    {
      this->Modified();
    }
  }

  int GetNumberOfTimeSteps()
  {
    return static_cast<int>(this->TimeSteps.size());
  }

protected:
  TestTimeSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    for(int i=0; i<20 ;i++)
    {
      this->TimeSteps.push_back(i);
    }

    this->Extent[0] = 0;
    this->Extent[1] = 1;
    this->Extent[2] = 0;
    this->Extent[3] = 1;
    this->Extent[4] = 0;
    this->Extent[5] = 1;

    this->BoundingBox[0]=0;
    this->BoundingBox[1]=1;
    this->BoundingBox[2]=0;
    this->BoundingBox[3]=1;
    this->BoundingBox[4]=0;
    this->BoundingBox[5]=1;
  }

  void GetSpacing(double dx[3])
  {
    for(int i=0; i<3; i++)
    {
      dx[i] = (this->BoundingBox[2*i+1]- this->BoundingBox[2*i]) / (this->Extent[2*i+1] - this->Extent[2*i]);
    }
  }

  ~TestTimeSource() { }

  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector)
  {
    // generate the data
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
      return this->RequestData(request, inputVector, outputVector);
    }

    // execute information
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      return this->RequestInformation(request, inputVector, outputVector);
    }
    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
  }

  int FillOutputPortInformation(int, vtkInformation *info)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }

  virtual int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *outputInfoVector)
  {
    // get the info objects
    vtkInformation *outInfo = outputInfoVector->GetInformationObject(0);

    double range[2]= {0,9};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 range,2);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &this->TimeSteps[0], static_cast<int>(this->TimeSteps.size()));


    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->Extent,6);

    double spacing[3];
    this->GetSpacing(spacing);

    outInfo->Set(vtkDataObject::SPACING(), spacing[0], spacing[1], spacing[2]);

    double origin[3] = {this->BoundingBox[0],this->BoundingBox[2],this->BoundingBox[4]};
    outInfo->Set(vtkDataObject::ORIGIN(),origin,3);

    outInfo->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);

    return 1;
  }


  int RequestData(
    vtkInformation* ,
    vtkInformationVector** vtkNotUsed( inputVector ),
    vtkInformationVector* outputVector)
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

    double timeStep = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),timeStep);
    // set the extent to be the update extent
    vtkImageData *outImage = vtkImageData::SafeDownCast(output);
    if (outImage)
    {
      int* uExtent = outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
      outImage->SetExtent(uExtent);
      //vtkGenericWarningMacro("time step in producer is " << timeStep);
      vtkFloatArray* outArray1 = vtkFloatArray::New();
      outArray1->SetName("Test");
      outArray1->SetNumberOfComponents(1);
      outArray1->SetNumberOfTuples(outImage->GetNumberOfPoints());
      for(int i=0; i<outImage->GetNumberOfPoints(); i++)
      {
        outArray1->SetTuple(i, &timeStep);
      }
      outImage->GetPointData()->SetScalars(outArray1);
      outArray1->Delete();
    }
    else
    {
      return 0 ;
    }

    vtkFloatArray* outArray = vtkFloatArray::New();
    outArray->SetName("Gradients");
    outArray->SetNumberOfComponents(3);
    outArray->SetNumberOfTuples(outImage->GetNumberOfPoints());
    assert(outArray->GetNumberOfTuples()==outImage->GetNumberOfPoints());
    outImage->GetPointData()->AddArray(outArray);
    outArray->Delete();
    outImage->GetPointData()->SetActiveVectors("Gradients");

    int *extent = outImage->GetExtent();
    vtkIdType stepX,stepY,stepZ;
    outImage->GetContinuousIncrements(extent,stepX,stepY,stepZ);

    float * outPtr = static_cast<float*> (outImage->GetArrayPointerForExtent(outArray,extent));
    int gridSize[3] = {this->Extent[1]-this->Extent[0],
                       this->Extent[3]-this->Extent[2],
                       this->Extent[5]-this->Extent[4]};

    double* origin = outImage->GetOrigin();

    double size[3];
    for(int i=0; i<3; i++)
    {
      size[i] = this->BoundingBox[2*i+1]- this->BoundingBox[2*i];
    }

    double speed = 0.5+0.5/(1.0+ 0.5*timeStep);
    for (int iz = extent[4]; iz<=extent[5]; iz++)
    {
      for (int iy = extent[2]; iy<=extent[3]; iy++)
      {
        for (int ix = extent[0]; ix<=extent[1]; ix++)
        {
          double x = size[0]*((double)ix)/gridSize[0] + origin[0];
          double z = size[2]*((double)iz)/gridSize[2] + origin[2];
          *(outPtr++) = -z *speed;
          *(outPtr++) = 0;
          *(outPtr++) = x *speed;
        }
        outPtr += stepY;
      }
      outPtr += stepZ;
    }

    return 1;
  }
private:
  TestTimeSource(const TestTimeSource&) VTK_DELETE_FUNCTION;
  void operator=(const TestTimeSource&) VTK_DELETE_FUNCTION;

  vector<double> TimeSteps;
  int Extent[6];
  double BoundingBox[6];
  int Spacing;

};

vtkStandardNewMacro(TestTimeSource);

int TestPParticleTracer(vtkMPIController* c, int staticOption)
{
  vtkNew<TestTimeSource> imageSource;
  int size(5);
  imageSource->SetExtent(0,size-1,0,1,0,size-1);
  imageSource->SetBoundingBox(-1,1,-1,1,-1,1);

  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.5,0,0.001);
  // points->InsertNextPoint(0.99,0,0.99);

  vtkNew<vtkPolyData> ps;
  ps->SetPoints(points.GetPointer());

  vtkNew<vtkPParticleTracer> filter;
  filter->SetStaticMesh(staticOption);
  filter->SetStaticSeeds(staticOption);
  filter->SetInputConnection(0,imageSource->GetOutputPort());
  filter->SetInputData(1,ps.GetPointer());
  filter->SetStartTime(0.0);

  vector<double> times;
  times.push_back(0.5);
  times.push_back(1.5);
  times.push_back(2.5);
  times.push_back(3.5);
  times.push_back(4.5);
  times.push_back(5.5);
  times.push_back(6.5);
  times.push_back(7.5);
  times.push_back(8.5);
  times.push_back(9.5);
  times.push_back(13.5);

  int numTraced(0);
  for(unsigned int ti = 0; ti<times.size();ti++)
  {
    filter->SetTerminationTime(times[ti]);

    vtkSmartPointer<vtkPolyDataMapper> traceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    traceMapper->SetInputConnection(filter->GetOutputPort());
    traceMapper->SetPiece(c->GetLocalProcessId());
    traceMapper->SetNumberOfPieces(c->GetNumberOfProcesses());
    traceMapper->Update();

    vtkPolyData* out = filter->GetOutput();
    vtkPoints* pts = out->GetPoints();

    numTraced+= pts->GetNumberOfPoints();
  }

  if(c->GetLocalProcessId()==0)
  {
    EXPECT(5, numTraced, "PParticleTracer: wrong number of points.", staticOption);
  }
  else
  {
    EXPECT(6, numTraced,"PParticleTracer: wrong number of points.", staticOption);
  }
  return EXIT_SUCCESS;
}


int TestPParticlePathFilter(vtkMPIController* c, int staticOption)
{
  vtkNew<TestTimeSource> imageSource;
  int size(5);
  imageSource->SetExtent(0,size-1,0,1,0,size-1);
  imageSource->SetBoundingBox(-1,1,-1,1,-1,1);

  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.5,0,0.001);

  vtkNew<vtkPolyData> ps;
  ps->SetPoints(points.GetPointer());

  vtkNew<vtkParticlePathFilter> filter;
  filter->SetStaticMesh(staticOption);
  filter->SetStaticSeeds(staticOption);
  filter->SetInputConnection(0,imageSource->GetOutputPort());
  filter->SetInputData(1,ps.GetPointer());
//  filter->SetForceReinjectionEveryNSteps(1);

  filter->SetStartTime(0.0);
  filter->SetTerminationTime(11.5);

  vtkSmartPointer<vtkPolyDataMapper> traceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  traceMapper->SetInputConnection(filter->GetOutputPort());
  traceMapper->SetPiece(c->GetLocalProcessId());
  traceMapper->SetNumberOfPieces(c->GetNumberOfProcesses());
  traceMapper->Update();

  vtkPolyData* out = filter->GetOutput();
  vtkPointData* pd = out->GetPointData();
  for(int i=0; i<pd->GetNumberOfArrays();i++)
  {
    assert(pd->GetArray(i)->GetNumberOfTuples()==out->GetPoints()->GetNumberOfPoints());
  }

  vtkCellArray* lines = out->GetLines();
  if(c->GetLocalProcessId() == 1)
  {
    EXPECT(2, lines->GetNumberOfCells(),"PParticlePath: wrong number of cells.", staticOption);
    vtkNew<vtkIdList> trace;
    lines->InitTraversal();
    lines->GetNextCell(trace.GetPointer());
    int tail;
    tail = trace->GetId(trace->GetNumberOfIds()-1);
    EXPECT(4, pd->GetArray("Test")->GetTuple1(tail), "PParticlePath: wrong tuple value.", staticOption);
  }
  else
  {
    EXPECT(1, lines->GetNumberOfCells(), "PParticlePath: wrong number of cells.", staticOption);

    vtkNew<vtkIdList> trace;
    lines->InitTraversal();
    lines->GetNextCell(trace.GetPointer());
    int head, tail;
    head = trace->GetId(0);
    tail = trace->GetId(trace->GetNumberOfIds()-1);
    EXPECT(4, pd->GetArray("Test")->GetTuple1(head), "PParticlePath: head", staticOption);
    EXPECT(9, pd->GetArray("Test")->GetTuple1(tail), "PParticlePath: tail", staticOption);
  }

  return EXIT_SUCCESS;
}

int TestPStreaklineFilter(vtkMPIController* c, int staticOption)
{
  vtkNew<TestTimeSource> imageSource;
  int size(5);
  imageSource->SetExtent(0,size-1,0,1,0,size-1);
  imageSource->SetBoundingBox(-1,1,-1,1,-1,1);

  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.5,0,0.001);
  points->InsertNextPoint(0.4,0,0.001);

  // points->InsertNextPoint(0.99,0,0.99);

  vtkNew<vtkPolyData> ps;
  ps->SetPoints(points.GetPointer());

  vtkNew<vtkPStreaklineFilter> filter;
  filter->SetStaticMesh(staticOption);
  filter->SetStaticSeeds(staticOption);
  filter->SetInputConnection(0,imageSource->GetOutputPort());
  filter->SetInputData(1,ps.GetPointer());

  filter->SetStartTime(0.0);
  filter->SetTerminationTime(11.5);

  vtkSmartPointer<vtkPolyDataMapper> traceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  traceMapper->SetInputConnection(filter->GetOutputPort());
  traceMapper->SetPiece(c->GetLocalProcessId());
  traceMapper->SetNumberOfPieces(c->GetNumberOfProcesses());
  traceMapper->Update();

  vtkPolyData* out = filter->GetOutput();
  vtkCellArray* lines = out->GetLines();

  if(c->GetLocalProcessId() == 0) //all the streaks go to 0 because of implementation
  {
    EXPECT(2, lines->GetNumberOfCells(),"PStreakline: wrong number of cells.", staticOption);
    vtkNew<vtkIdList> trace;
    lines->InitTraversal();
    lines->GetNextCell(trace.GetPointer());
    EXPECT(13, trace->GetNumberOfIds(),"PStreakline: wrong number of points.", staticOption);
    lines->GetNextCell(trace.GetPointer());
    EXPECT(13, trace->GetNumberOfIds(),"PStreakline: wrong number of points.", staticOption);
  }
  else
  {
    EXPECT(0, out->GetNumberOfPoints(),"PStreakline: No other process should have streaks.", staticOption);
  }

  return EXIT_SUCCESS;
}


int TestPParticleTracers(int argc, char* argv[])
{
  vtkSmartPointer<vtkMPIController> c=vtkSmartPointer<vtkMPIController>::New();
  vtkMultiProcessController::SetGlobalController(c);
  c->Initialize(&argc,&argv);

  int retVal = 0;
  retVal += TestPParticleTracer(c, 1);
  retVal += TestPParticleTracer(c, 0);
  c->Barrier();

  retVal += TestPParticlePathFilter(c, 1);
  retVal += TestPParticlePathFilter(c, 0);
  c->Barrier();

  retVal += TestPStreaklineFilter(c, 1);
  retVal += TestPStreaklineFilter(c, 0);

  c->Finalize();
  return retVal;
}
