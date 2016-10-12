/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSampleFunction.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkSampleFunction);
vtkCxxSetObjectMacro(vtkSampleFunction,ImplicitFunction,vtkImplicitFunction);

// The heart of the algorithm plus interface to the SMP tools.
template <class T>
class vtkSampleFunctionAlgorithm
{
public:
  vtkImplicitFunction *ImplicitFunction;
  T         *Scalars;
  float     *Normals;
  vtkIdType  Extent[6];
  vtkIdType  Dims[3];
  vtkIdType  SliceSize;
  double     Origin[3];
  double     Spacing[3];
  double     CapValue;

  // Contructor
  vtkSampleFunctionAlgorithm();

  // Interface between VTK and templated functions
  static void SampleAcrossImage(vtkSampleFunction *self, vtkImageData *output,
                                int extent[6], T *scalars, float *normals);

  // Cap the boundaries with the specified cap value (if requested).
  void Cap();

  // Interface implicit function computation to SMP tools.
  template <class TT> class FunctionValueOp
  {
    public:
      FunctionValueOp(vtkSampleFunctionAlgorithm<TT> *algo)
        { this->Algo = algo;}
      vtkSampleFunctionAlgorithm *Algo;

      void  operator() (vtkIdType k, vtkIdType end)
      {
        double x[3];
        vtkIdType *extent=this->Algo->Extent;
        vtkIdType i, j, jOffset, kOffset;
        for ( ; k < end; ++k)
        {
          x[2] = this->Algo->Origin[2] + k*this->Algo->Spacing[2];
          kOffset = (k-extent[4]) * this->Algo->SliceSize;
          for (j=extent[2]; j<=extent[3]; ++j)
          {
            x[1] = this->Algo->Origin[1] + j*this->Algo->Spacing[1];
            jOffset = (j-extent[2])*this->Algo->Dims[0];
            for (i=extent[0]; i<=extent[1]; ++i)
            {
              x[0] = this->Algo->Origin[0] + i*this->Algo->Spacing[0];
              this->Algo->Scalars[(i-extent[0])+jOffset+kOffset] =
                static_cast<TT>(this->Algo->ImplicitFunction->FunctionValue(x));
            }
          }
        }
      }
  };

  // Interface implicit function graadient computation to SMP tools.
  template <class TT> class FunctionGradientOp
  {
    public:
      FunctionGradientOp(vtkSampleFunctionAlgorithm<TT> *algo)
        { this->Algo = algo;}
      vtkSampleFunctionAlgorithm *Algo;

      void  operator() (vtkIdType k, vtkIdType end)
      {
        double x[3], n[3];
        float *nPtr;
        vtkIdType *extent=this->Algo->Extent;
        vtkIdType i, j, jOffset, kOffset;
        for ( ; k < end; ++k)
        {
          x[2] = this->Algo->Origin[2] + k*this->Algo->Spacing[2];
          kOffset = (k-extent[4]) * this->Algo->SliceSize;
          for (j=extent[2]; j<=extent[3]; ++j)
          {
            x[1] = this->Algo->Origin[1] + j*this->Algo->Spacing[1];
            jOffset = (j-extent[2]) * this->Algo->Dims[0];
            for (i=extent[0]; i<=extent[1]; ++i)
            {
              x[0] = this->Algo->Origin[0] + i*this->Algo->Spacing[0];
              this->Algo->ImplicitFunction->FunctionGradient(x,n);
              vtkMath::Normalize(n);
              nPtr = this->Algo->Normals + 3*((i-extent[0])+jOffset+kOffset);
              nPtr[0] = static_cast<TT>(-n[0]);
              nPtr[1] = static_cast<TT>(-n[1]);
              nPtr[2] = static_cast<TT>(-n[2]);
            }//i
          }//j
        }//k
      }
  };
};

//----------------------------------------------------------------------------
// Initialized mainly to eliminate compiler warnings.
template <class T> vtkSampleFunctionAlgorithm<T>::
vtkSampleFunctionAlgorithm():Scalars(NULL),Normals(NULL)
{
  for (int i=0; i<3; ++i)
  {
    this->Extent[2*i] = this->Extent[2*i+1] = 0;
    this->Dims[i] = 0;
    this->Origin[i] = this->Spacing[i] = 0.0;
  }
  this->SliceSize = 0;
  this->CapValue = 0.0;
}

//----------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
template <class T> void vtkSampleFunctionAlgorithm<T>::
SampleAcrossImage(vtkSampleFunction *self, vtkImageData *output,
                  int extent[6], T *scalars, float *normals)
{
  // Populate data into local storage
  vtkSampleFunctionAlgorithm<T> algo;
  algo.ImplicitFunction = self->GetImplicitFunction();
  algo.Scalars = scalars;
  algo.Normals = normals;
  for (int i=0; i<3; ++i)
  {
    algo.Extent[2*i] = extent[2*i];
    algo.Extent[2*i+1] = extent[2*i+1];
    algo.Dims[i] = extent[2*i+1] - extent[2*i] + 1;
  }
  algo.SliceSize = algo.Dims[0]*algo.Dims[1];
  output->GetOrigin(algo.Origin);
  output->GetSpacing(algo.Spacing);
  algo.CapValue = self->GetCapValue();

  // Okay now generate samples using SMP tools
  FunctionValueOp<T> values(&algo);
  vtkSMPTools::For(extent[4],extent[5]+1, values);

  // If requested, generate normals
  if ( algo.Normals )
  {
    FunctionGradientOp<T> gradient(&algo);
    vtkSMPTools::For(extent[4],extent[5]+1, gradient);
  }

  // If requested, cap boundaries
  if ( self->GetCapping() )
  {
    algo.Cap();
  }
}

//----------------------------------------------------------------------------
// Cap the boundaries of the volume if requested.
template <class T> void vtkSampleFunctionAlgorithm<T>::Cap()
{
  vtkIdType i,j,k;
  vtkIdType idx;

  // i-j planes
  //k = this->Extent[4];
  for (j=this->Extent[2]; j<=this->Extent[3]; j++)
  {
    for (i=this->Extent[0]; i<=this->Extent[1]; i++)
    {
      this->Scalars[i+j*this->Dims[0]] = this->CapValue;
    }
  }

  idx = this->Extent[5]*this->SliceSize;
  for (j=this->Extent[2]; j<=this->Extent[3]; j++)
  {
    for (i=this->Extent[0]; i<=this->Extent[1]; i++)
    {
      this->Scalars[idx+i+j*this->Dims[0]] = this->CapValue;
    }
  }

  // j-k planes
  //i = this->Extent[0];
  for (k=this->Extent[4]; k<=this->Extent[5]; k++)
  {
    for (j=this->Extent[2]; j<=this->Extent[3]; j++)
    {
      this->Scalars[j*this->Dims[0]+k*this->SliceSize] =
        this->CapValue;
    }
  }

  i = this->Extent[1];
  for (k=this->Extent[4]; k<=this->Extent[5]; k++)
  {
    for (j=this->Extent[2]; j<=this->Extent[3]; j++)
    {
      this->Scalars[i+j*this->Dims[0]+k*this->SliceSize] =
        this->CapValue;
    }
  }

  // i-k planes
  //j = this->Extent[2];
  for (k=this->Extent[4]; k<=this->Extent[5]; k++)
  {
    for (i=this->Extent[0]; i<=this->Extent[1]; i++)
    {
      this->Scalars[i+k*this->SliceSize] = this->CapValue;
    }
  }

  j = this->Extent[3];
  idx = j*this->Dims[0];
  for (k=this->Extent[4]; k<=this->Extent[5]; k++)
  {
    for (i=this->Extent[0]; i<=this->Extent[1]; i++)
    {
      this->Scalars[idx+i+k*this->SliceSize] = this->CapValue;
    }
  }
}

//----------------------------------------------------------------------------
// Okay define the VTK class proper
vtkSampleFunction::vtkSampleFunction()
{
  this->ModelBounds[0] = -1.0;
  this->ModelBounds[1] = 1.0;
  this->ModelBounds[2] = -1.0;
  this->ModelBounds[3] = 1.0;
  this->ModelBounds[4] = -1.0;
  this->ModelBounds[5] = 1.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Capping = 0;
  this->CapValue = VTK_DOUBLE_MAX;

  this->ImplicitFunction = NULL;

  this->ComputeNormals = 1;
  this->OutputScalarType = VTK_DOUBLE;

  this->ScalarArrayName=0;
  this->SetScalarArrayName("scalars");

  this->NormalArrayName=0;
  this->SetNormalArrayName("normals");

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkSampleFunction::~vtkSampleFunction()
{
  this->SetImplicitFunction(NULL);
  this->SetScalarArrayName(NULL);
  this->SetNormalArrayName(NULL);
}

//----------------------------------------------------------------------------
// Specify the dimensions of the data on which to sample.
void vtkSampleFunction::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
// Specify the dimensions of the data on which to sample.
void vtkSampleFunction::SetSampleDimensions(int dim[3])
{
  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] ||
       dim[1] != this->SampleDimensions[1] ||
       dim[2] != this->SampleDimensions[2] )
  {
    for ( int i=0; i<3; i++)
    {
      this->SampleDimensions[i] = (dim[i] > 0 ? dim[i] : 1);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Set the bounds of the model.
void vtkSampleFunction::SetModelBounds(const double bounds[6])
{
  this->SetModelBounds(bounds[0], bounds[1],
                       bounds[2], bounds[3],
                       bounds[4], bounds[5]);

}

//----------------------------------------------------------------------------
void vtkSampleFunction::SetModelBounds(double xMin, double xMax,
                                       double yMin, double yMax,
                                       double zMin, double zMax)
{
  vtkDebugMacro(<< " setting ModelBounds to ("
                << "(" << xMin << "," << xMax << "), "
                << "(" << yMin << "," << yMax << "), "
                << "(" << zMin << "," << zMax << "), ");
  if ((xMin > xMax) ||
      (yMin > yMax) ||
      (zMin > zMax))
  {
    vtkErrorMacro("Invalid bounds: "
                  << "(" << xMin << "," << xMax << "), "
                  << "(" << yMin << "," << yMax << "), "
                  << "(" << zMin << "," << zMax << ")"
                  << " Bound mins cannot be larger that bound maxs");
    return;
  }
  if (xMin != this->ModelBounds[0] ||
      xMax != this->ModelBounds[1] ||
      yMin != this->ModelBounds[2] ||
      yMax != this->ModelBounds[3] ||
      zMin != this->ModelBounds[4] ||
      zMax != this->ModelBounds[5])
  {
    this->ModelBounds[0] = xMin;
    this->ModelBounds[1] = xMax;
    this->ModelBounds[2] = yMin;
    this->ModelBounds[3] = yMax;
    this->ModelBounds[4] = zMin;
    this->ModelBounds[5] = zMax;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkSampleFunction::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  int wExt[6];
  wExt[0] = 0; wExt[2] = 0; wExt[4] = 0;
  wExt[1] = this->SampleDimensions[0]-1;
  wExt[3] = this->SampleDimensions[1]-1;
  wExt[5] = this->SampleDimensions[2]-1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);

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
    SetPointDataActiveScalarInfo(outInfo,this->OutputScalarType,1);

  outInfo->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
// Produce the data.
void vtkSampleFunction::
ExecuteDataWithInformation(vtkDataObject *outp, vtkInformation *outInfo)
{
  vtkFloatArray *newNormals=NULL;
  float *normals=NULL;

  vtkImageData *output=this->GetOutput();
  int* extent =
    this->GetExecutive()->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  output->SetExtent(extent);
  output = this->AllocateOutputData(outp, outInfo);
  vtkDataArray *newScalars =output->GetPointData()->GetScalars();
  vtkIdType numPts = newScalars->GetNumberOfTuples();

  vtkDebugMacro(<< "Sampling implicit function");

  // Initialize self; create output objects
  //
  if ( !this->ImplicitFunction )
  {
    vtkErrorMacro(<<"No implicit function specified");
    return;
  }

  if ( this->ComputeNormals )
  {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(numPts);
    normals = newNormals->WritePointer(0,numPts);
  }

  void *ptr = output->GetArrayPointerForExtent(newScalars, extent);
  switch (newScalars->GetDataType())
  {
    vtkTemplateMacro(vtkSampleFunctionAlgorithm<VTK_TT>::
                     SampleAcrossImage(this, output, extent, (VTK_TT *)ptr,
                                       normals));
  }

  newScalars->SetName(this->ScalarArrayName);

  // Update self
  //
  if (newNormals)
  {
    // For an unknown reason yet, if the following line is not commented out,
    // it will make ImplicitSum, TestBoxFunction and TestDiscreteMarchingCubes
    // to fail.
    newNormals->SetName(this->NormalArrayName);
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSampleFunction::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType impFuncMTime;

  if ( this->ImplicitFunction != NULL )
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkSampleFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0]
     << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2]
     << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4]
     << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";

  if ( this->ImplicitFunction )
  {
    os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";
  }
  else
  {
    os << indent << "No Implicit function defined\n";
  }

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");

  os << indent << "ScalarArrayName: ";
  if(this->ScalarArrayName!=0)
  {
    os  << this->ScalarArrayName << endl;
  }
  else
  {
    os  << "(none)" << endl;
  }

  os << indent << "NormalArrayName: ";
  if(this->NormalArrayName!=0)
  {
    os  << this->NormalArrayName << endl;
  }
  else
  {
    os  << "(none)" << endl;
  }
}

//----------------------------------------------------------------------------
void vtkSampleFunction::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ImplicitFunction,
                            "ImplicitFunction");
}
