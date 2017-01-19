/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDensityFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointDensityFilter.h"

#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkImageData.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkArrayListTemplate.h" // For processing attribute data


vtkStandardNewMacro(vtkPointDensityFilter);
vtkCxxSetObjectMacro(vtkPointDensityFilter,Locator,vtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Operator() processes slices.
struct ComputePointDensity
{
  int Dims[3];
  double Origin[3];
  double Spacing[3];
  float *Density;
  vtkAbstractPointLocator *Locator;
  double Radius, Volume;
  int Form;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  ComputePointDensity(int dims[3], double origin[3], double spacing[3], float *dens,
                      vtkAbstractPointLocator *loc, double radius, int form) :
    Density(dens), Locator(loc), Radius(radius), Form(form)
  {
      for (int i=0; i < 3; ++i)
      {
        this->Dims[i] = dims[i];
        this->Origin[i] = origin[i];
        this->Spacing[i] = spacing[i];
      }
    this->Volume = (4.0/3.0) * vtkMath::Pi() * radius * radius * radius;
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
  }

  void operator() (vtkIdType slice, vtkIdType sliceEnd)
  {
    vtkIdList*& pIds = this->PIds.Local();
    double *origin=this->Origin;
    double *spacing=this->Spacing;
    int *dims=this->Dims;
    double x[3];
    vtkIdType numPts, sliceSize=dims[0]*dims[1];
    float *dens = this->Density + slice*sliceSize;
    double radius = this->Radius;
    double volume = this->Volume;
    vtkAbstractPointLocator *locator=this->Locator;
    int form = this->Form;

    for ( ; slice < sliceEnd; ++slice )
    {
      x[2] = origin[2] + slice*spacing[2];
      for ( int j=0;  j < dims[1]; ++j)
      {
        x[1] = origin[1] + j*spacing[1];
        for ( int i=0; i < dims[0]; ++i)
        {
          x[0] = origin[0] + i*spacing[0];
          // Retrieve the local neighborhood
          locator->FindPointsWithinRadius(radius, x, pIds);
          numPts = pIds->GetNumberOfIds();

          if ( form == VTK_DENSITY_FORM_NPTS )
          {
            *dens++ = static_cast<float>(numPts);
          }
          else //VTK_DENSITY_FORM_VOLUME_NORM
          {
            *dens++ = static_cast<float>(numPts) / volume;
          }
        }//over i
      }//over j
    }//over slices
  }

  void Reduce()
  {
  }

  static void Execute(vtkPointDensityFilter *self, int dims[3], double origin[3],
                      double spacing[3], float *density, double radius, int form)
  {
    ComputePointDensity compDens(dims, origin, spacing, density,
                                 self->GetLocator(), radius, form);
    vtkSMPTools::For(0, dims[2], compDens);
  }
}; //ComputePointDensity

//----------------------------------------------------------------------------
// The threaded core of the algorithm; processes weighted points.
template <typename T>
struct ComputeWeightedDensity : public ComputePointDensity
{
  const T *Weights;

  ComputeWeightedDensity(T *weights, int dims[3], double origin[3], double spacing[3],
                         float *dens, vtkAbstractPointLocator *loc, double radius, int form) :
    ComputePointDensity(dims,origin,spacing,dens,loc,radius,form), Weights(weights)
  {
  }

  void operator() (vtkIdType slice, vtkIdType sliceEnd)
  {
    vtkIdList*& pIds = this->PIds.Local();
    double *origin=this->Origin;
    double *spacing=this->Spacing;
    int *dims=this->Dims;
    double x[3];
    vtkIdType sample, numPts, sliceSize=dims[0]*dims[1];
    float *dens = this->Density + slice*sliceSize;
    double radius = this->Radius;
    double volume = this->Volume;
    vtkAbstractPointLocator *locator=this->Locator;
    int form = this->Form;
    double d;
    const T *weights = this->Weights;

    for ( ; slice < sliceEnd; ++slice )
    {
      x[2] = origin[2] + slice*spacing[2];
      for ( int j=0;  j < dims[1]; ++j)
      {
        x[1] = origin[1] + j*spacing[1];
        for ( int i=0; i < dims[0]; ++i)
        {
          x[0] = origin[0] + i*spacing[0];
          // Retrieve the local neighborhood
          locator->FindPointsWithinRadius(radius, x, pIds);
          numPts = pIds->GetNumberOfIds();
          for (d=0.0,sample=0; sample<numPts; ++sample)
          {
            d += *(weights + pIds->GetId(sample));
          }

          if ( form == VTK_DENSITY_FORM_NPTS )
          {
            *dens++ = static_cast<float>(d);
          }
          else //VTK_DENSITY_FORM_VOLUME_NORM
          {
            *dens++ = static_cast<float>(d) / volume;
          }
        }//over i
      }//over j
    }//over slices
  }

  void Reduce()
  {
  }

  static void Execute(vtkPointDensityFilter *self, T *weights, int dims[3],
                      double origin[3], double spacing[3], float *density,
                      double radius, int form)
  {
    ComputeWeightedDensity compDens(weights, dims, origin, spacing, density,
                                    self->GetLocator(), radius, form);
    vtkSMPTools::For(0, dims[2], compDens);
  }
}; //ComputeWeightedDensity

//----------------------------------------------------------------------------
// Optional kernel to compute gradient of density function. Also the gradient
// magnitude and function classification is computed.
struct ComputeGradients
{
  int Dims[3];
  double Origin[3];
  double Spacing[3];
  float *Density;
  float *Gradients;
  float *GradientMag;
  unsigned char *FuncClassification;

  ComputeGradients(int dims[3], double origin[3], double spacing[3], float *dens,
                   float *grad, float *mag, unsigned char *fclass) :
    Density(dens), Gradients(grad), GradientMag(mag), FuncClassification(fclass)

  {
    for (int i=0; i < 3; ++i)
    {
      this->Dims[i] = dims[i];
      this->Origin[i] = origin[i];
      this->Spacing[i] = spacing[i];
    }
  }

  void operator() (vtkIdType slice, vtkIdType sliceEnd)
  {
    double *spacing=this->Spacing;
    int *dims=this->Dims;
    vtkIdType sliceSize=dims[0]*dims[1];
    float *d = this->Density + slice*sliceSize;
    float *grad = this->Gradients + 3*slice*sliceSize;
    float *mag = this->GradientMag + slice*sliceSize;
    unsigned char *fclass = this->FuncClassification + slice*sliceSize;
    float f, dp, dm;
    bool nonZeroComp;
    int idx[3], incs[3];
    incs[0] = 1;
    incs[1] = dims[0];
    incs[2] = sliceSize;

    for ( ; slice < sliceEnd; ++slice )
    {
      idx[2] = slice;
      for ( int j=0;  j < dims[1]; ++j)
      {
        idx[1] = j;
        for ( int i=0; i < dims[0]; ++i)
        {
          idx[0] = i;
          nonZeroComp = false;
          for (int ii=0; ii < 3; ++ii)
          {
            if ( idx[ii] == 0 )
            {
              dm = *d;
              dp = *(d + incs[ii]);
              f = 1.0;
            }
            else if ( idx[ii] == dims[ii]-1 )
            {
              dm = *(d - incs[ii]);
              dp = *d;
              f = 1.0;
            }
            else
            {
              dm = *(d - incs[ii]);
              dp = *(d + incs[ii]);
              f = 0.5;
            }
            grad[ii] = f * (dp-dm) / spacing[ii];
            nonZeroComp = (( dp != 0.0 || dm != 0.0) ? true : nonZeroComp );
          }

          // magnitude
          if ( nonZeroComp )
          {
            *mag++ = vtkMath::Norm(grad);
            *fclass++ = vtkPointDensityFilter::NON_ZERO;
          }
          else
          {
            *mag++ = 0.0;
            *fclass++ = vtkPointDensityFilter::ZERO;
          }
          d++;
          grad += 3;

        }//over i
      }//over j
    }//over slices
  }

  void Reduce()
  {
  }

  static void Execute(int dims[3], double origin[3], double spacing[3],
                      float *density, float *grad, float *mag,
                      unsigned char *fclass)
  {
    ComputeGradients compGrad(dims, origin, spacing, density, grad, mag, fclass);
    vtkSMPTools::For(0, dims[2], compGrad);
  }
}; //ComputeGradients

} //anonymous namespace


//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkPointDensityFilter::vtkPointDensityFilter()
{
  this->SampleDimensions[0] = 100;
  this->SampleDimensions[1] = 100;
  this->SampleDimensions[2] = 100;

  // All of these zeros mean automatic computation
  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;
  this->AdjustDistance = 0.10;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;

  this->DensityEstimate = VTK_DENSITY_ESTIMATE_RELATIVE_RADIUS;
  this->DensityForm = VTK_DENSITY_FORM_NPTS;

  this->Radius = 1.0;
  this->RelativeRadius = 1.0;

  this->ScalarWeighting = false;

  this->ComputeGradient = false;

  this->Locator = vtkStaticPointLocator::New();

}

//----------------------------------------------------------------------------
vtkPointDensityFilter::~vtkPointDensityFilter()
{
  this->Locator->UnRegister(this);
  this->Locator = NULL;
}

//----------------------------------------------------------------------------
int vtkPointDensityFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPointDensityFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->SampleDimensions[0]-1,
               0, this->SampleDimensions[1]-1,
               0, this->SampleDimensions[2]-1);

  for (i=0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2*i];
    if ( this->SampleDimensions[i] <= 1 )
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
              / (this->SampleDimensions[i] - 1);
    }
  }
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),ar,3);

  vtkDataObject::
    SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);

  return 1;
}

//----------------------------------------------------------------------------
// Compute the size of the sample bounding box automatically from the
// input data.
void vtkPointDensityFilter::
ComputeModelBounds(vtkDataSet *input, vtkImageData *output,
                   vtkInformation *outInfo)
{
  const double *bounds;
  int i, adjustBounds=0;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
       this->ModelBounds[2] >= this->ModelBounds[3] ||
       this->ModelBounds[4] >= this->ModelBounds[5] )
  {
    adjustBounds = 1;
    bounds = input->GetBounds();
  }
  else
  {
    bounds = this->ModelBounds;
  }

  // Adjust bounds so model fits strictly inside (only if not set previously)
  if ( adjustBounds )
  {
    double c, l;
    for (i=0; i<3; i++)
    {
      l = (1.0+this->AdjustDistance) * (bounds[2*i+1] - bounds[2*i]) / 2.0;
      c = (bounds[2*i+1] + bounds[2*i]) / 2.0;
      this->ModelBounds[2*i] = c - l;
      this->ModelBounds[2*i+1] = c + l;
    }
  }

  // Set volume origin and data spacing
  outInfo->Set(vtkDataObject::ORIGIN(),
               this->ModelBounds[0],this->ModelBounds[2],
               this->ModelBounds[4]);
  memcpy(this->Origin,outInfo->Get(vtkDataObject::ORIGIN()), sizeof(double)*3);
  output->SetOrigin(this->Origin);

  for (i=0; i<3; i++)
  {
    this->Spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    if ( this->Spacing[i] <= 0.0 )
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(vtkDataObject::SPACING(),this->Spacing,3);
  output->SetSpacing(this->Spacing);
}

//----------------------------------------------------------------------------
// Set the dimensions of the sampling volume
void vtkPointDensityFilter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void vtkPointDensityFilter::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << ","
                << dim[1] << "," << dim[2] << ")");

  if (dim[0] != this->SampleDimensions[0] ||
      dim[1] != this->SampleDimensions[1] ||
      dim[2] != this->SampleDimensions[2] )
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
      this->SampleDimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Produce the output data
int vtkPointDensityFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if ( !input || !output )
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if ( numPts < 1 )
  {
    return 1;
  }

  // Configure the output
  output->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);
  int* extent = this->GetExecutive()->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  // Configure the output
  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(input, output, outInfo);

  //  Make sure points are available
  vtkIdType npts = input->GetNumberOfPoints();
  if ( npts == 0 )
  {
    vtkWarningMacro(<<"No POINTS input!!");
    return 1;
  }

  // Algorithm proper
  // Start by building the locator.
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Determine the appropriate radius
  double radius;
  if ( this->DensityEstimate == VTK_DENSITY_ESTIMATE_FIXED_RADIUS )
  {
    radius = this->Radius;
  }
  else //VTK_DENSITY_ESTIMATE_RELATIVE_RADIUS
  {
    radius = this->RelativeRadius * vtkMath::Norm(this->Spacing);
  }

  // If weighting points
  vtkDataArray *weights = this->GetInputArrayToProcess(0, inputVector);
  void *w=NULL;
  if ( weights && this->ScalarWeighting )
  {
    w = weights->GetVoidPointer(0);
  }

  // Grab the density array and process it.
  output->AllocateScalars(outInfo);
  vtkDataArray *density = output->GetPointData()->GetScalars();
  float *d = static_cast<float*>(output->GetArrayPointerForExtent(density, extent));

  int dims[3];
  double origin[3], spacing[3];
  output->GetDimensions(dims);
  output->GetOrigin(origin);
  output->GetSpacing(spacing);
  if ( !w )
  {
    ComputePointDensity::Execute(this, dims, origin, spacing, d, radius,
                                 this->DensityForm);
  }
  else
  {
    switch (weights->GetDataType())
    {
      vtkTemplateMacro(ComputeWeightedDensity<VTK_TT>::Execute(this, (VTK_TT *)w, dims,
                                 origin, spacing, d, radius, this->DensityForm));
    }
  }

  // If the gradient is requested, compute the vector gradient and magnitude.
  // Also compute the classification of the gradient value.
  if ( this->ComputeGradient )
  {
    //Allocate space
    vtkIdType num = density->GetNumberOfTuples();

    vtkFloatArray *gradients = vtkFloatArray::New();
    gradients->SetNumberOfComponents(3);
    gradients->SetNumberOfTuples(num);
    gradients->SetName("Gradient");
    output->GetPointData()->AddArray(gradients);
    float *grad = static_cast<float*>(gradients->GetVoidPointer(0));
    gradients->Delete();

    vtkFloatArray *magnitude = vtkFloatArray::New();
    magnitude->SetNumberOfComponents(1);
    magnitude->SetNumberOfTuples(num);
    magnitude->SetName("Gradient Magnitude");
    output->GetPointData()->AddArray(magnitude);
    float *mag = static_cast<float*>(magnitude->GetVoidPointer(0));
    magnitude->Delete();

    vtkUnsignedCharArray *fclassification =
      vtkUnsignedCharArray::New();
    fclassification->SetNumberOfComponents(1);
    fclassification->SetNumberOfTuples(num);
    fclassification->SetName("Classification");
    output->GetPointData()->AddArray(fclassification);
    unsigned char *fclass =
      static_cast<unsigned char*>(fclassification->GetVoidPointer(0));
    fclassification->Delete();

    //Thread the computation over slices
    ComputeGradients::Execute(dims,origin,spacing,d,grad,mag,fclass);
  }

  return 1;
}

//----------------------------------------------------------------------------
const char *vtkPointDensityFilter::GetDensityEstimateAsString()
{
  if ( this->DensityEstimate == VTK_DENSITY_ESTIMATE_FIXED_RADIUS )
  {
    return "Fixed Radius";
  }
  else //if ( this->DensityEstimate == VTK_DENSITY_ESTIMATE_RELATIVE_RADIUS )
  {
    return "Relative Radius";
  }
}

//----------------------------------------------------------------------------
const char *vtkPointDensityFilter::GetDensityFormAsString()
{
  if ( this->DensityForm == VTK_DENSITY_FORM_VOLUME_NORM )
  {
    return "Volume Norm";
  }
  else //if ( this->DensityForm == VTK_DENSITY_FORM_NPTS )
  {
    return "Number of Points";
  }
}

//----------------------------------------------------------------------------
void vtkPointDensityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: ("
               << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0]
     << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2]
     << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4]
     << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "AdjustDistance: " << this->AdjustDistance << "\n";

  os << indent << "Density Estimate: "
     << this->GetDensityEstimateAsString() << "\n";

  os << indent << "Density Form: "
     << this->GetDensityFormAsString() << "\n";

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Relative Radius: " << this->RelativeRadius << "\n";

  os << indent << "Scalar Weighting: "
     << (this->ScalarWeighting ? "On\n" : "Off\n");

  os << indent << "Compute Gradient: "
     << (this->ComputeGradient ? "On\n" : "Off\n");

  os << indent << "Locator: " << this->Locator << "\n";
}
