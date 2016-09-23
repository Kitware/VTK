/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleImplicitFunctionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSampleImplicitFunctionFilter.h"

#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImplicitFunction.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkSampleImplicitFunctionFilter);
vtkCxxSetObjectMacro(vtkSampleImplicitFunctionFilter,ImplicitFunction,vtkImplicitFunction);

// Interface between vtkSMPTools and the VTK pipeline
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
vtkSampleImplicitFunctionFilter::vtkSampleImplicitFunctionFilter()
{
  this->ImplicitFunction = NULL;

  this->ComputeGradients = 1;

  this->ScalarArrayName = 0;
  this->SetScalarArrayName("Implicit scalars");

  this->GradientArrayName = 0;
  this->SetGradientArrayName("Implicit gradients");
}

//----------------------------------------------------------------------------
vtkSampleImplicitFunctionFilter::~vtkSampleImplicitFunctionFilter()
{
  this->SetImplicitFunction(NULL);
  this->SetScalarArrayName(NULL);
  this->SetGradientArrayName(NULL);
}

//----------------------------------------------------------------------------
// Produce the output data
int vtkSampleImplicitFunctionFilter::RequestData(
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

  // Pass the output attribute data
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

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
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars(this->ScalarArrayName);
  newScalars->Delete();

  if ( this->ComputeGradients )
  {
    newGradients->SetName(this->GradientArrayName);
    output->GetPointData()->AddArray(newGradients);
    output->GetPointData()->SetActiveVectors(this->GradientArrayName);
    newGradients->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSampleImplicitFunctionFilter::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSampleImplicitFunctionFilter::GetMTime()
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
void vtkSampleImplicitFunctionFilter::
ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ImplicitFunction,
                            "ImplicitFunction");
}

//----------------------------------------------------------------------------
void vtkSampleImplicitFunctionFilter::PrintSelf(ostream& os, vtkIndent indent)
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
