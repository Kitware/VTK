#include "TestVectorFieldSource.h"
#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkDataObject.h>
#include <vtkImageData.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(TestVectorFieldSource);

void TestVectorFieldSource::SetBoundingBox(double x0, double x1, double y0,
                    double y1, double z0, double z1)
{
  this->BoundingBox[0] = x0;
  this->BoundingBox[1] = x1;
  this->BoundingBox[2] = y0;
  this->BoundingBox[3] = y1;
  this->BoundingBox[4] = z0;
  this->BoundingBox[5] = z1;

}
void TestVectorFieldSource::SetExtent(int xMin, int xMax, int yMin, int yMax,
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

TestVectorFieldSource::TestVectorFieldSource()
{
  Extent[0] = 0;
  Extent[1] = 1;
  Extent[2] = 0;
  Extent[3] = 1;
  Extent[4] = 0;
  Extent[5] = 1;

  this->SetNumberOfInputPorts(0);

  BoundingBox[0]=0;
  BoundingBox[1]=1;
  BoundingBox[2]=0;
  BoundingBox[3]=1;
  BoundingBox[4]=0;
  BoundingBox[5]=1;
}

// Description:
// Destructor.
TestVectorFieldSource::~TestVectorFieldSource()
{

}
int TestVectorFieldSource::RequestInformation(vtkInformation *,
                               vtkInformationVector **,
                               vtkInformationVector *outputInfoVector)
{
  // get the info objects
  vtkInformation *outInfo = outputInfoVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->Extent,6);

  double spacing[3];
  this->GetSpacing(spacing);
  outInfo->Set(vtkDataObject::SPACING(), spacing[0], spacing[1], spacing[2]);

  double origin[3] = {this->BoundingBox[0],this->BoundingBox[2],this->BoundingBox[4]};
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  return 1;
}

void TestVectorFieldSource::GetSpacing(double dx[3])
{
  for(int i=0; i<3; i++)
    {
    dx[i] = (this->BoundingBox[2*i+1]- this->BoundingBox[2*i]) / (this->Extent[2*i+1] - this->Extent[2*i]);
    }
}

void TestVectorFieldSource::GetSize(double dx[3])
{
  for(int i=0; i<3; i++)
    {
    dx[i] = this->BoundingBox[2*i+1]- this->BoundingBox[2*i];
    }
}
void TestVectorFieldSource::ExecuteDataWithInformation(vtkDataObject *outData,vtkInformation *outInfo)
{
  vtkImageData* outImage = this->AllocateOutputData(outData,outInfo);
  if (outImage->GetNumberOfPoints() <= 0)
    {
    return;
    }


  vtkDataArray* outArray = vtkDataArray::SafeDownCast(vtkAbstractArray::CreateArray(VTK_FLOAT));
  outArray->SetName("Gradients");
  outArray->SetNumberOfComponents(3);
  outArray->SetNumberOfTuples(outImage->GetNumberOfPoints());
  outImage->GetPointData()->AddArray(outArray);
  outImage->GetPointData()->SetActiveVectors("Gradients");

  int *extent = outImage->GetExtent(); //old style
//    PRINT("Extent: "<<extent[0]<<" "<<extent[1]<<" "<<extent[2]<<" "<<extent[3]<<" "<<extent[4]<<" "<<extent[5])
//    PRINT("Bound: "<<bb[0]<<" "<<bb[1]<<" "<<bb[2]<<" "<<bb[3]<<" "<<bb[4]<<" "<<bb[5])

  vtkIdType stepX,stepY,stepZ;
  outImage->GetContinuousIncrements(extent,stepX,stepY,stepZ);

  float * outPtr = static_cast<float*> (outImage->GetArrayPointerForExtent(outArray,extent));
//   PRINT(stepY<<" "<<stepZ)

  int gridSize[3] = {Extent[1]-Extent[0],Extent[3]-Extent[2],Extent[5]-Extent[4]};

  double* origin = outImage->GetOrigin();

  double size[3]; GetSize(size);

  for (int iz = extent[4]; iz<=extent[5]; iz++)
   {
   for (int iy = extent[2]; iy<=extent[3]; iy++)
     {
     for (int ix = extent[0]; ix<=extent[1]; ix++)
       {
       double x = size[0]*((double)ix)/gridSize[0] + origin[0];
       double y = size[1]*((double)iy)/gridSize[1] + origin[1];
       double z = size[2]*((double)iz)/gridSize[2] + origin[2];
       *(outPtr++) = -z;
       *(outPtr++) = y*0;
       *(outPtr++) = x;
       }
     outPtr += stepY;
     }
   outPtr += stepZ;
   }
  outArray->Delete();
}
