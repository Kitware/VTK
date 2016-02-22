/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSampleDataSet.h"

#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImplicitFunction.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkSampleDataSet);
vtkCxxSetObjectMacro(vtkSampleDataSet,ImplicitFunction,vtkImplicitFunction);

//Interface between vtkSMPTools and VTK pipeline
namespace {

struct SampleDataSet
{
  vtkDataSet *Input;
  vtkImplicitFunction *Function;
  float *Scalars;

  // Contructor
  SampleDataSet(vtkDataSet *input, vtkImplicitFunction *imp, float *s) :
    Input(input), Function(imp), Scalars(s)
    {
    }

  void  operator() (vtkIdType ptId, vtkIdType endPtId)
    {
      double x[3];
      float *n = this->Scalars + ptId;
      for ( ; ptId < endPtId; ++ptId )
        {
        this->Input->GetPoint(ptId, x);
        *n++ = this->Function->FunctionValue(x);
        }
    }
};

struct SampleDataSetWithGradients
{
  vtkDataSet *Input;
  vtkImplicitFunction *Function;
  float *Scalars;
  float *Gradients;

  // Contructor
  SampleDataSetWithGradients(vtkDataSet *input, vtkImplicitFunction *imp, float *s, float *g) :
    Input(input), Function(imp), Scalars(s), Gradients(g)
    {
    }

  void  operator() (vtkIdType ptId, vtkIdType endPtId)
    {
      double x[3], g[3];
      float *n = this->Scalars + ptId;
      float *v = this->Gradients + 3*ptId;
      for ( ; ptId < endPtId; ++ptId )
        {
        this->Input->GetPoint(ptId, x);
        *n++ = this->Function->FunctionValue(x);
        this->Function->FunctionGradient(x,g);
        *v++ = g[0];
        *v++ = g[1];
        *v++ = g[2];
        }
    }
};

} //anonymous namespace


//----------------------------------------------------------------------------
// Okay define the VTK class proper
vtkSampleDataSet::vtkSampleDataSet()
{
  this->ImplicitFunction = NULL;

  this->ComputeGradients = 1;

  this->ScalarArrayName=0;
  this->SetScalarArrayName("Implicit scalars");

  this->GradientArrayName=0;
  this->SetGradientArrayName("Implicit gradients");
}

//----------------------------------------------------------------------------
vtkSampleDataSet::~vtkSampleDataSet()
{
  this->SetImplicitFunction(NULL);
  this->SetScalarArrayName(NULL);
  this->SetGradientArrayName(NULL);
}

//----------------------------------------------------------------------------
// Produce the output data
int vtkSampleDataSet::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkDebugMacro(<< "Generating implicit data");

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
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

  // Ensure implicit function is specified
  //
  if ( !this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return 1;
    }

  // The output geometic structure is the same as the input
  output->CopyStructure(input);

  // Set up for execution
  vtkFloatArray *newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);
  float *scalars = newScalars->WritePointer(0,numPts);

  vtkFloatArray *newGradients=NULL;
  float *gradients=NULL;
  if ( this->ComputeGradients )
    {
    newGradients = vtkFloatArray::New();
    newGradients->SetNumberOfComponents(3);
    newGradients->SetNumberOfTuples(numPts);
    gradients = newGradients->WritePointer(0,numPts);
    }

  // Threaded execute
  if ( this->ComputeGradients )
    {
    SampleDataSetWithGradients
      sample(input,this->ImplicitFunction,scalars,gradients);
    vtkSMPTools::For(0,numPts, sample);
    }
  else
    {
    SampleDataSet sample(input,this->ImplicitFunction,scalars);
    vtkSMPTools::For(0,numPts, sample);
    }

  // Update self
  newScalars->SetName(this->ScalarArrayName);
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  if ( this->ComputeGradients )
    {
    // For an unknown reason yet, if the following line is not commented out,
    // it will make ImplicitSum, TestBoxFunction and TestDiscreteMarchingCubes
    // to fail.
    newGradients->SetName(this->GradientArrayName);
    output->GetPointData()->SetVectors(newGradients);
    newGradients->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSampleDataSet::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
unsigned long vtkSampleDataSet::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkSampleDataSet::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ImplicitFunction,
                            "ImplicitFunction");
}

//----------------------------------------------------------------------------
void vtkSampleDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->ImplicitFunction )
    {
    os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";
    }
  else
    {
    os << indent << "No Implicit function defined\n";
    }

  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");

  os << indent << "Scalar Array Name: ";
  if(this->ScalarArrayName != 0)
    {
    os  << this->ScalarArrayName << endl;
    }
  else
    {
    os  << "(none)" << endl;
    }

  os << indent << "Gradient Array Name: ";
  if(this->GradientArrayName != 0)
    {
    os  << this->GradientArrayName << endl;
    }
  else
    {
    os  << "(none)" << endl;
    }
}
