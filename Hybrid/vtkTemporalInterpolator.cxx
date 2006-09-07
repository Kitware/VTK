/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*================================================
  Created by Ken Martin, 
  leveraging code written by John Biddiscomb
  ================================================*/

#include "vtkTemporalInterpolator.h"

#include "vtkTemporalDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkstd/algorithm"
#include "vtkstd/vector"

vtkCxxRevisionMacro(vtkTemporalInterpolator, "1.1");
vtkStandardNewMacro(vtkTemporalInterpolator);

//----------------------------------------------------------------------------
vtkTemporalInterpolator::vtkTemporalInterpolator()
{
}

//----------------------------------------------------------------------------
vtkTemporalInterpolator::~vtkTemporalInterpolator()
{
}

//----------------------------------------------------------------------------
void vtkTemporalInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Change the information
int vtkTemporalInterpolator::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // we throw out the discrete entries and our output is considered to be
  // continuous
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    double outRange[2];
    outRange[0] = inTimes[0];
    outRange[1] = inTimes[numTimes-1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 outRange,2);
    // unset the time steps if they are set
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
      {
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      }
    }

  return 1;
}


//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkTemporalInterpolator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  vtkTemporalDataSet *inData = vtkTemporalDataSet::SafeDownCast
    (inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTemporalDataSet *outData = vtkTemporalDataSet::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!inData || !outData)
    {
    return 1;
    }

  // get the input times
  double *inTimes = inData->GetInformation()
    ->Get(vtkDataObject::DATA_TIME_STEPS());
  int numInTimes = inData->GetInformation()
    ->Length(vtkDataObject::DATA_TIME_STEPS());

  // get the requested update times
  double *upTimes =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
  int numUpTimes = 
    outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

  // for each targeted output time
  int upIdx;
  for (upIdx = 0; upIdx < numUpTimes; ++upIdx)
    {
    // below the range
    if (upTimes[upIdx] <= inTimes[0])
      {
      // pass the lowest data
      vtkDataObject *in1 = inData->GetDataSet(0,0);
      vtkDataObject *out1 = in1->NewInstance();
      out1->ShallowCopy(in1);
      outData->SetDataSet(upIdx,0,out1);
      out1->Delete();
      }
    // above the range?
    else if (upTimes[upIdx] >= inTimes[numInTimes-1])
      {
      // pass the highest data
      vtkDataObject *in1 = inData->GetDataSet(numInTimes-1,0);
      vtkDataObject *out1 = in1->NewInstance();
      out1->ShallowCopy(in1);
      outData->SetDataSet(upIdx,0,out1);
      out1->Delete();
      }
    // in the middle, interpolate
    else
      {
      int i = 0;
      while (upTimes[upIdx] > inTimes[i])
        {
        ++i;
        }
      // was there an exact time match? If so shallow copy
      if (upTimes[upIdx] == inTimes[i])
        {
        // pass the match
        vtkDataObject *in1 = inData->GetDataSet(i,0);
        vtkDataObject *out1 = in1->NewInstance();
        out1->ShallowCopy(in1);
        outData->SetDataSet(upIdx,0,out1);
        out1->Delete();
        }
      else
        {
        // interpolate i-1 and i
        vtkDataObject *in1 = inData->GetDataSet(i-1,0);
        vtkDataObject *in2 = inData->GetDataSet(i,0);
        vtkDataObject *out1 = 
          this->InterpolateDataObject
          (in1,in2,(upTimes[upIdx]-inTimes[i-1])/(inTimes[i] - inTimes[i-1]));
        outData->SetDataSet(upIdx,0,out1);
        out1->Delete();
        }
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalInterpolator::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // find the required input time steps and request the,
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    // get the update times
    double *upTimes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numUpTimes = 
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

    // get the available input times
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numInTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    // only if the input is not continuous should we do anything
    if (inTimes)
      {
      bool *inTimesToUse;
      inTimesToUse = new bool [numInTimes];
      int i;
      for (i = 0; i < numInTimes; ++i)
        {
        inTimesToUse[i] = false;
        }
      
      // for each requested time mark the required input times
      int u;
      i = 0;
      for (u = 0; u < numUpTimes; ++u)
        {
        // below the range
        if (upTimes[u] <= inTimes[0])
          {
          inTimesToUse[0] = true;
          }
        // above the range?
        else if (upTimes[u] >= inTimes[numInTimes-1])
          {
          inTimesToUse[numInTimes-1] = true;
          }
        // in the middle
        else
          {
          while (upTimes[u] > inTimes[i])
            {
            ++i;
            }
          inTimesToUse[i] = true;
          inTimesToUse[i-1] = true;
          }
        }
      
      // how many input times do we need?
      int numInUpTimes = 0;
      for (i = 0; i < numInTimes; ++i)
        {
        if (inTimesToUse[i])
          {
          numInUpTimes++;
          }
        }
      
      double *inUpTimes = new double [numInUpTimes];
      u = 0;
      for (i = 0; i < numInTimes; ++i)
        {
        if (inTimesToUse[i])
          {
          inUpTimes[u] = inTimes[i];
          u++;
          }
        }
      
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                  inUpTimes,numInUpTimes);
      
      delete [] inUpTimes;
      delete [] inTimesToUse;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkTemporalInterpolator
::VerifyArrays(vtkDataArray **arrays, int N)
{
  vtkIdType Nt = arrays[0]->GetNumberOfTuples();
  vtkIdType Nc = arrays[0]->GetNumberOfComponents();
  for (int i=1; i<N; ++i) 
    {
    if (arrays[i]->GetNumberOfTuples()!=Nt) 
      {
      return false;
      }
    if (arrays[i]->GetNumberOfComponents()!=Nc) 
      {
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
vtkDataObject *vtkTemporalInterpolator
::InterpolateDataObject( vtkDataObject *in1, vtkDataObject *in2, double ratio)
{
  if (vtkDataSet::SafeDownCast(in1)) 
    {
    //
    // if we have reached the Leaf/DataSet level, we can interpolate directly
    //
    vtkDataSet *inds1 = vtkDataSet::SafeDownCast(in1);
    vtkDataSet *inds2 = vtkDataSet::SafeDownCast(in2);
    return this->InterpolateDataSet(inds1, inds2, ratio);
    }
  else if (vtkMultiGroupDataSet::SafeDownCast(in1)) 
    {
    vtkMultiGroupDataSet *mgds[2];
    mgds[0] = vtkMultiGroupDataSet::SafeDownCast(in1);
    mgds[1] = vtkMultiGroupDataSet::SafeDownCast(in2);

    //
    // We need to loop over blocks etc and build up a new dataset
    //
    vtkMultiGroupDataSet *output = mgds[0]->NewInstance();
    int numGroups = mgds[0]->GetNumberOfGroups();
    output->SetNumberOfGroups(numGroups);
    //
    for (int g=0; g<numGroups; ++g) 
      {
      int numDataSets = mgds[0]->GetNumberOfDataSets(g);
      for (int d=0; d<numDataSets; ++d) 
        {
        // These multigroup dataset can have null data, it's bad, but
        // we'll just skip the rest of that bundle
        vtkDataObject *dataobj1 = mgds[0]->GetDataSet(g,d);
        vtkDataObject *dataobj2 = mgds[1]->GetDataSet(g,d);
        if (!dataobj1 || !dataobj2) 
          {
          vtkWarningMacro
            (
             "The MultiGroup datasets were not identical in structure : Group " 
             << g << " Dataset " << d << " was skipped");
          continue;
          }
        vtkDataObject *result = 
          this->InterpolateDataObject(dataobj1, dataobj2, ratio);
        if (result) 
          {
          output->SetDataSet(g, d, result); 
          }
        else 
          {
          vtkErrorMacro(<<"Unexpected error during interpolation");
          // need to clear up memory we may have allocated and lost :(
          return NULL;
          }
        }
      }
    return output;
    }
  else 
    {
    vtkErrorMacro("We cannot yet interpolate this type of dataset");
    return NULL;
    }
}

//----------------------------------------------------------------------------
vtkDataSet *vtkTemporalInterpolator
::InterpolateDataSet(vtkDataSet *in1, vtkDataSet *in2, double ratio)
{
  vtkDataSet *input[2];
  input[0] = in1;
  input[1] = in2;

  //
  vtkDataSet *output = input[0]->NewInstance();
  output->CopyStructure(input[0]);
  //
  // Interpolate points if the dataset is a vtkPointSet
  //
  if (vtkPointSet::SafeDownCast(input[0])) 
    {
    vtkstd::vector<vtkDataArray*> arrays;
    arrays.push_back
      (vtkPointSet::SafeDownCast(input[0])->GetPoints()->GetData());
    arrays.push_back
      (vtkPointSet::SafeDownCast(input[1])->GetPoints()->GetData());

    // allocate double for output if input is double - otherwise float
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], 2)) 
      {
      vtkWarningMacro
        ("Interpolation aborted for points because the number of "
         "tuples/components in each time step are different");
      }
    vtkDataArray *outarray = 
      this->InterpolateDataArray(ratio, &arrays[0],
                                 arrays[0]->GetNumberOfTuples());
    vtkPoints *outpoints = vtkPointSet::SafeDownCast(output)->GetPoints();
    if (vtkDoubleArray::SafeDownCast(outarray)) 
      {
      outpoints->SetDataTypeToDouble();
      }
    else 
      {
      outpoints->SetDataTypeToFloat();
      }
    outpoints->SetNumberOfPoints(arrays[0]->GetNumberOfTuples());
    outpoints->SetData(outarray);
    outarray->Delete();
    }
  //
  // Interpolate pointdata if present
  //
  for (int s=0; s < input[0]->GetPointData()->GetNumberOfArrays(); ++s) 
    {
    vtkstd::vector<vtkDataArray*> arrays;
    char *scalarname = NULL;
    for (int i=0; i<2; ++i) 
      {
      //
      // On some data, the scalar arrays are consistent but ordered
      // differently on each time step, so we will fetch them by name if
      // possible.
      //
      if (i==0 || (scalarname==NULL)) 
        {
        vtkDataArray *dataarray = input[i]->GetPointData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
        }
      else 
        {
        vtkDataArray *dataarray = 
          input[i]->GetPointData()->GetArray(scalarname);
        arrays.push_back(dataarray);
        }
      }
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], 2)) 
      {
      vtkWarningMacro(<<"Interpolation aborted for array " 
        << (scalarname ? scalarname : "(unnamed array)") 
        << " because the number of tuples/components"
        << " in each time step are different");
      }
    // allocate double for output if input is double - otherwise float
    vtkDataArray *outarray = 
      this->InterpolateDataArray(ratio, &arrays[0],
                                 arrays[0]->GetNumberOfTuples());
    output->GetPointData()->AddArray(outarray);
    outarray->Delete();
    }
  //
  // Interpolate celldata if present
  //
  for (int s=0; s<input[0]->GetCellData()->GetNumberOfArrays(); ++s) 
    {
    vtkstd::vector<vtkDataArray*> arrays;
    char *scalarname = NULL;
    for (int i=0; i<2; ++i) 
      {
      //
      // On some data, the scalar arrays are consistent but ordered
      // differently on each time step, so we will fetch them by name if
      // possible.
      //
      if (i==0 || (scalarname==NULL)) 
        {
        vtkDataArray *dataarray = input[i]->GetCellData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
        }
      else 
        {
        vtkDataArray *dataarray = 
          input[i]->GetCellData()->GetArray(scalarname);
        arrays.push_back(dataarray);
        }
      }
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], 2)) 
      {
      vtkWarningMacro(<<"Interpolation aborted for array " 
                      << (scalarname ? scalarname : "(unnamed array)") 
                      << " because the number of tuples/components"
                      << " in each time step are different");
      }
    // allocate double for output if input is double - otherwise float
    vtkDataArray *outarray = 
      this->InterpolateDataArray(ratio, &arrays[0],
                                 arrays[0]->GetNumberOfTuples());
    output->GetCellData()->AddArray(outarray);
    outarray->Delete();
    }
  return output;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkTemporalInterpolatorExecute(vtkTemporalInterpolator *,
                                    double ratio,
                                    vtkDataArray *output,
                                    vtkDataArray **arrays,
                                    int numComp,
                                    int numTuple,
                                    T *)
{
  output->Allocate(numTuple);
  T *outData = static_cast<T*>(output->GetVoidPointer(0));
  T *inData1 = static_cast<T*>(arrays[0]->GetVoidPointer(0));
  T *inData2 = static_cast<T*>(arrays[1]->GetVoidPointer(0));

  double oneMinusRatio = 1.0 - ratio;

  unsigned long idx;
  for (idx = 0; idx < static_cast<unsigned long>(numTuple*numComp); ++idx)
    {
    *outData = static_cast<T>(*inData1*oneMinusRatio + *inData2*ratio);
    outData++;
    inData1++;
    inData2++;
    }
}


//----------------------------------------------------------------------------
vtkDataArray *vtkTemporalInterpolator
::InterpolateDataArray(double ratio, vtkDataArray **arrays, 
                       vtkIdType N)
{
  //
  // Create the output
  //
  vtkAbstractArray *aa = arrays[0]->CreateArray(arrays[0]->GetDataType());
  vtkDataArray *output = vtkDataArray::SafeDownCast(aa);
  
  int Nc = arrays[0]->GetNumberOfComponents();

  //
  // initialize the output
  //
  output->SetNumberOfComponents(Nc);
  output->SetNumberOfTuples(N);
  output->SetName(arrays[0]->GetName());

  // now do the interpolation
  switch (arrays[0]->GetDataType())
    {
    vtkTemplateMacro(vtkTemporalInterpolatorExecute
                     (this, ratio, output, arrays, Nc, N,
                      static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }

  return output;
}

