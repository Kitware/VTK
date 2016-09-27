/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSignedDistance.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSignedDistance.h"

#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

vtkStandardNewMacro(vtkSignedDistance);
vtkCxxSetObjectMacro(vtkSignedDistance,Locator,vtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

// The threaded core of the algorithm
template <typename T>
struct SignedDistance
{
  T *Pts;
  float *Normals;
  vtkIdType Dims[3];
  double Origin[3];
  double Spacing[3];
  double Radius;
  vtkAbstractPointLocator *Locator;
  float *Scalars;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  SignedDistance(T *pts, float *normals, int dims[3], double origin[3], double spacing[3],
                 double radius, vtkAbstractPointLocator *loc, float *scalars) :
    Pts(pts), Normals(normals), Radius(radius), Locator(loc), Scalars(scalars)
  {
      for (int i=0; i < 3; ++i)
      {
        this->Dims[i] = static_cast<vtkIdType>(dims[i]);
        this->Origin[i] = origin[i];
        this->Spacing[i] = spacing[i];
      }
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
  }

  // Threaded interpolation method
  void operator() (vtkIdType slice, vtkIdType sliceEnd)
  {
      T *p;
      float *n;
      double x[3], dist;
      vtkIdType numPts;
      double *origin=this->Origin;
      double *spacing=this->Spacing;
      int ii;
      vtkIdType *dims=this->Dims;
      vtkIdType ptId, jOffset, kOffset, sliceSize=dims[0]*dims[1];
      vtkIdList*& pIds = this->PIds.Local();

      for ( ; slice < sliceEnd; ++slice)
      {
        x[2] = origin[2] + slice*spacing[2];
        kOffset = slice*sliceSize;

        for ( vtkIdType j=0;  j < dims[1]; ++j)
        {
          x[1] = origin[1] + j*spacing[1];
          jOffset = j*dims[0];

          for ( vtkIdType i=0; i < dims[0]; ++i)
          {
            x[0] = origin[0] + i*spacing[0];
            ptId = i + jOffset + kOffset;

            // Compute signed distance from surrounding points
            this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
            numPts = pIds->GetNumberOfIds();
            if ( numPts > 0 )
            {
              for (dist=0.0, ii=0; ii < numPts; ++ii)
              {
                p = this->Pts + 3*pIds->GetId(ii);
                n = this->Normals + 3*pIds->GetId(ii);
                dist += n[0]*(x[0]-p[0]) + n[1]*(x[1]-p[1]) + n[2]*(x[2]-p[2]);
              }
              this->Scalars[ptId] = dist / static_cast<double>(numPts);
            }//if nearby points
          }//over i
        }//over j
      }//over slices
  }

  void Reduce()
  {
  }

  static void Execute(vtkSignedDistance *self, T *pts, float *normals, int dims[3],
                      double origin[3], double spacing[3], float *scalars)
  {
      SignedDistance dist(pts, normals, dims, origin, spacing, self->GetRadius(),
                          self->GetLocator(), scalars);
      vtkSMPTools::For(0, dims[2], dist);
  }

}; //SignedDistance

} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
// Construct with sample dimensions=(256,256,256), and so that model bounds are
// automatically computed from the input.
vtkSignedDistance::vtkSignedDistance()
{
  this->Dimensions[0] = 256;
  this->Dimensions[1] = 256;
  this->Dimensions[2] = 256;

  this->Bounds[0] = 0.0;
  this->Bounds[1] = 0.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = 0.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = 0.0;

  this->Radius = 0.1;

  this->Locator = vtkStaticPointLocator::New();

  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkSignedDistance::~vtkSignedDistance()
{
  this->SetLocator(NULL);
}


//----------------------------------------------------------------------------
// Initialize the filter for appending data. You must invoke the
// StartAppend() method before doing successive Appends(). It's also a
// good idea to manually specify the model bounds; otherwise the input
// bounds for the data will be used.
void vtkSignedDistance::StartAppend()
{
  vtkInformation* outInfo = this->GetOutputInformation(0);
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    vtkStreamingDemandDrivenPipeline::GetWholeExtent(outInfo),
    6);

  vtkDebugMacro(<< "Initializing data");
  this->AllocateOutputData(this->GetOutput(), this->GetOutputInformation(0));
  vtkIdType numPts = static_cast<vtkIdType>(this->Dimensions[0]) *
    static_cast<vtkIdType>(this->Dimensions[1]) *
    static_cast<vtkIdType>(this->Dimensions[2]);

  // initialize output to initial unseen value at each location
  float *newScalars = static_cast<float*>(
    this->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0));
  std::fill_n(newScalars, numPts, this->Radius);

  // Compute the initial bounds
  double bounds[6];
  vtkImageData *output=this->GetOutput();
  double tempd[3];
  int i;

  // compute model bounds if not set previously
  if ( this->Bounds[0] >= this->Bounds[1] ||
       this->Bounds[2] >= this->Bounds[3] ||
       this->Bounds[4] >= this->Bounds[5] )
  {
    vtkPolyData *input = vtkPolyData::SafeDownCast(this->GetInput());
    input->GetBounds(bounds);
    for (i=0; i<3; i++)
    {
      this->Bounds[2*i] = bounds[2*i];
      this->Bounds[2*i+1] = bounds[2*i+1];
    }
  }

  // Set volume origin and data spacing
  output->SetOrigin(this->Bounds[0], this->Bounds[2], this->Bounds[4]);

  for (i=0; i<3; i++)
  {
    tempd[i] = (this->Bounds[2*i+1] - this->Bounds[2*i]) /
      (this->Dimensions[i] - 1);
  }
  output->SetSpacing(tempd);

  outInfo->Set(vtkDataObject::ORIGIN(),this->Bounds[0],
               this->Bounds[2], this->Bounds[4]);
  outInfo->Set(vtkDataObject::SPACING(),tempd,3);

  this->Initialized = 1;
}


//----------------------------------------------------------------------------
// Append a data set to the existing output. To use this function,
// you'll have to invoke the StartAppend() method before doing
// successive appends. It's also a good idea to specify the model
// bounds; otherwise the input model bounds is used. When you've
// finished appending, use the EndAppend() method.
void vtkSignedDistance::Append(vtkPolyData *input)
{
  vtkDebugMacro(<< "Appending data");

  // There better be data
  if ( !input || input->GetNumberOfPoints() < 1 )
  {
    return;
  }

  if ( !this->Initialized )
  {
    this->StartAppend();
  }

  // Make sure that there are normals and output scalars
  vtkPoints *pts = input->GetPoints();
  float *scalars = static_cast<float*>(
    this->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0));
  vtkDataArray *normalArray = input->GetPointData()->GetNormals();
  if ( ! normalArray || normalArray->GetDataType() != VTK_FLOAT )
  {
    vtkErrorMacro(<< "Float normals required!");
    return;
  }
  float *normals = static_cast<float*>(normalArray->GetVoidPointer(0));

  // Build the locator
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Finally: compute the signed distance function
  vtkImageData *output=this->GetOutput();
  void *inPtr = pts->GetVoidPointer(0);
  switch (pts->GetDataType())
  {
    vtkTemplateMacro(SignedDistance<VTK_TT>::Execute(this, (VTK_TT *)inPtr, normals,
             this->Dimensions, output->GetOrigin(), output->GetSpacing(), scalars));
  }

}

//----------------------------------------------------------------------------
// Method completes the append process (does the capping if requested).
void vtkSignedDistance::EndAppend()
{
  vtkDataArray *newScalars;
  vtkDebugMacro(<< "End append");

  if (!(newScalars = this->GetOutput()->GetPointData()->GetScalars()))
  {
    vtkErrorMacro("No output produced.");
    return;
  }
}

//----------------------------------------------------------------------------
int vtkSignedDistance::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->Dimensions[0]-1,
               0, this->Dimensions[1]-1,
               0, this->Dimensions[2]-1);

  for (i=0; i < 3; i++)
  {
    origin[i] = this->Bounds[2*i];
    if ( this->Dimensions[i] <= 1 )
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] = (this->Bounds[2*i+1] - this->Bounds[2*i])
              / (this->Dimensions[i] - 1);
    }
  }
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),ar,3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkSignedDistance::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  // get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing space carver");

  if (input == NULL)
  {
    // we do not want to release the data because user might
    // have called Append ...
    return 0;
  }

  this->StartAppend();
  this->Append(input);
  this->EndAppend();

  return 1;
}

//----------------------------------------------------------------------------
// Set the i-j-k dimensions on which to sample the distance function.
void vtkSignedDistance::SetDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetDimensions(dim);
}

//----------------------------------------------------------------------------
void vtkSignedDistance::SetDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting Dimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->Dimensions[0] ||
       dim[1] != this->Dimensions[1] ||
       dim[2] != this->Dimensions[2] )
  {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
    {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
    }

    for (dataDim=0, i=0; i<3 ; i++)
    {
      if (dim[i] > 1)
      {
        dataDim++;
      }
    }

    if ( dataDim  < 3 )
    {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
    }

    for ( i=0; i<3; i++)
    {
      this->Dimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkSignedDistance::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSignedDistance::ProcessRequest(vtkInformation* request,
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* outputVector)
{
  // If we have no input then we will not generate the output because
  // the user already called StartAppend/Append/EndAppend.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
  {
    if(inputVector[0]->GetNumberOfInformationObjects() == 0)
    {
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(vtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
    }
    return 1;
  }
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    if(inputVector[0]->GetNumberOfInformationObjects() == 0)
    {
      return 1;
    }
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkSignedDistance::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
               << this->Dimensions[1] << ", "
               << this->Dimensions[2] << ")\n";
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", "
     << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", "
     << this->Bounds[5] << ")\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
