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
#include "vtkTemporalInterpolator.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalDataSet.h"

#include "vtkstd/algorithm"
#include "vtkstd/vector"

vtkStandardNewMacro(vtkTemporalInterpolator);

//----------------------------------------------------------------------------
vtkTemporalInterpolator::vtkTemporalInterpolator()
{
  this->DiscreteTimeStepInterval = 0.0; // non value
  this->ResampleFactor           = 0;   // non value
  this->Ratio  = 0.0;
  this->DeltaT = 0.0;
  this->Tfrac  = 0.0;
}

//----------------------------------------------------------------------------
vtkTemporalInterpolator::~vtkTemporalInterpolator()
{
}

//----------------------------------------------------------------------------
void vtkTemporalInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ResampleFactor: "
     << this->ResampleFactor << "\n";
  os << indent << "DiscreteTimeStepInterval: "
     << this->DiscreteTimeStepInterval << "\n";
}
/*
//----------------------------------------------------------------------------
int vtkTemporalInterpolator::FillInputPortInformation(
  int port, 
  vtkInformation* info)
{
  if (port == 0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTemporalDataSet");
  }
  return 1;
}
//----------------------------------------------------------------------------
int vtkTemporalInterpolator::RequestDataObject(
  vtkInformation* vtkNotUsed(reqInfo), 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
    
      if (!output || !output->IsA(input->GetClassName())) 
{
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(outInfo);
        newOutput->Delete();
        }
      }
  return 1;
}
  return 0;
}
*/
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

  //
  // find time on input
  //
  int     numTimes = 0;
  double *inTimes  = NULL;
  double  outRange[2];

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    outRange[0] = inTimes[0];
    outRange[1] = inTimes[numTimes-1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 outRange,2);
    }

  // Can we continue
  if (numTimes<2) 
    {
    vtkErrorMacro(<<"Not enough input time steps for interpolation");
    return 0;
    }

  //
  // Now compute the interpolated output times
  //
  if (this->DiscreteTimeStepInterval>0.0)
    {
    //
    // We know the input has got N time steps, 
    // how many output steps are we producing, and what are they.
    //
    int NumberOfOutputTimeSteps = 1 + 
      static_cast<int>(0.5+((outRange[1]-outRange[0])/this->DiscreteTimeStepInterval));

    // Generate list of new output time step values
    vtkstd::vector<double> OutputTimeValues;
    for (int i=0; i<NumberOfOutputTimeSteps; i++) 
      {
      OutputTimeValues.push_back(
        (double)(i)*this->DiscreteTimeStepInterval + outRange[0]);
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
      &OutputTimeValues[0], NumberOfOutputTimeSteps);
    }
  else if (this->ResampleFactor>0)
    {
    // Generate list of new output time step values
    vtkstd::vector<double> OutputTimeValues;
    OutputTimeValues.reserve(numTimes*this->ResampleFactor);
    for (int i=1; i<numTimes; i++) 
      {
      double t0 = inTimes[i-1];
      double t1 = inTimes[i];
      double step = (t1-t0)/(double)this->ResampleFactor;
      for (int j=0; j<this->ResampleFactor; j++) 
        {
        double newT = t0 + j*step;
        OutputTimeValues.push_back(newT);
        }
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
      &OutputTimeValues[0], static_cast<int>(OutputTimeValues.size()));
    }
  else 
    {
    // unset the time steps if they are set
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
      {
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      }
    }
  return 1;
}


//----------------------------------------------------------------------------
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
    vtkDataObject *out1;
    if (upTimes[upIdx] <= inTimes[0])
      {
      // pass the lowest data
      vtkDebugMacro(<<"Interpolation time below/== range : " << inTimes[0]);
      vtkDataObject *in1 = inData->GetTimeStep(0);
      out1 = in1->NewInstance();
      out1->ShallowCopy(in1);
      outData->SetTimeStep(upIdx, out1);
      if (in1->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
        {
        out1->GetInformation()->Set(vtkDataObject::DATA_GEOMETRY_UNMODIFIED(),1);
        }
      out1->Delete();
      }
    // above the range?
    else if (upTimes[upIdx] >= inTimes[numInTimes-1])
      {
      // pass the highest data
      vtkDebugMacro(<<"Interpolation time above/== range : " << inTimes[numInTimes-1] << " of " << numInTimes);
      vtkDataObject *in1 = inData->GetTimeStep(numInTimes-1);
      out1 = in1->NewInstance();
      out1->ShallowCopy(in1);
      outData->SetTimeStep(upIdx, out1);
        if (in1->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
          {
          out1->GetInformation()->Set(vtkDataObject::DATA_GEOMETRY_UNMODIFIED(),1);
          }
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
        vtkDebugMacro(<<"Interpolation time " << inTimes[i]);
        vtkDataObject *in1 = inData->GetTimeStep(i);
        out1 = in1->NewInstance();
        out1->ShallowCopy(in1);
        outData->SetTimeStep(upIdx, out1);
        if (in1->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
          {
          out1->GetInformation()->Set(vtkDataObject::DATA_GEOMETRY_UNMODIFIED(),1);
          }
        out1->Delete();
        }
      else
        {
        // interpolate i-1 and i
        vtkDataObject *in1 = inData->GetTimeStep(i-1);
        vtkDataObject *in2 = inData->GetTimeStep(i);
        this->Ratio  = (upTimes[upIdx]-inTimes[i-1])/(inTimes[i] - inTimes[i-1]);
        this->Tfrac  = (upTimes[upIdx]-inTimes[i-1]);
        this->DeltaT = (inTimes[i] - inTimes[i-1]);
        vtkDebugMacro(<<"Interpolation times " << inTimes[i-1] << "->" << inTimes[i] 
          << " : " << upTimes[upIdx] << " Interpolation ratio " << this->Ratio );
        out1 = this->InterpolateDataObject(in1,in2,this->Ratio);
        // stamp this new dataset with a time key
        out1->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), 
                                 upTimes, numUpTimes);
        outData->SetTimeStep(upIdx, out1);
        out1->Delete();
        }
      outData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), 
                                 &upTimes[upIdx], 1);
      }
    }

  // @TODO remove this when we move to new time framework
  // stamp the new temporal dataset with a time key (old style of management)
  outData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), 
                                 upTimes, numUpTimes);

  vtkSmartPointer<vtkDoubleArray> originalTimes = vtkSmartPointer<vtkDoubleArray>::New();
  originalTimes->SetName("OriginalTimeSteps");
  originalTimes->SetNumberOfTuples(numInTimes);
  for (int i=0; i<numInTimes; i++) {
    originalTimes->SetValue(i, inTimes[i]);
  }
  outData->GetFieldData()->AddArray(originalTimes);

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

  // find the required input time steps and request them
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
      vtkDebugMacro(<<"Requesting " << numInUpTimes << " times ");
      
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
  else if (vtkCompositeDataSet::SafeDownCast(in1)) 
    {
    vtkCompositeDataSet*mgds[2];
    mgds[0] = vtkCompositeDataSet::SafeDownCast(in1);
    mgds[1] = vtkCompositeDataSet::SafeDownCast(in2);

    // It is essential that mgds[0] an mgds[1] has the same structure.
    //
    // We need to loop over blocks etc and build up a new dataset
    //
    vtkCompositeDataSet *output = mgds[0]->NewInstance();
    output->CopyStructure(mgds[0]);
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(mgds[0]->NewIterator());

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataObject* dataobj1 = iter->GetCurrentDataObject();
      vtkDataObject* dataobj2 = mgds[1]->GetDataSet(iter);
      if (!dataobj1 || !dataobj2) 
        {
        vtkWarningMacro(
          "The composite datasets were not identical in structure.");
        continue;
        }
      vtkDataObject *result = 
        this->InterpolateDataObject(dataobj1, dataobj2, ratio);
      if (result)
        {
        output->SetDataSet(iter, result);
        result->Delete();
        }
      else
        {
        vtkErrorMacro(<<"Unexpected error during interpolation");
        // need to clear up memory we may have allocated and lost :(
        return NULL;
        }
      }
    if (in1->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()) &&
        in2->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
      {
      output->GetInformation()->Set(vtkDataObject::DATA_GEOMETRY_UNMODIFIED(),1);
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
  vtkPointSet *inPointSet1 = vtkPointSet::SafeDownCast(input[0]);
  vtkPointSet *inPointSet2 = vtkPointSet::SafeDownCast(input[1]);
  vtkPointSet *outPointSet = vtkPointSet::SafeDownCast(output);
  if (inPointSet1 && inPointSet2) 
    {
    vtkDataArray *outarray = NULL;
    vtkPoints *outpoints;

    if (inPointSet1->GetNumberOfPoints()>0 && inPointSet2->GetNumberOfPoints()>0)
      {
      vtkDataArray *arrays[2];
      arrays[0] = inPointSet1->GetPoints()->GetData();
      arrays[1] = inPointSet2->GetPoints()->GetData();

      // allocate double for output if input is double - otherwise float
      // do a quick check to see if all arrays have the same number of tuples
      if (!this->VerifyArrays(arrays, 2)) 
        {
        vtkWarningMacro
          ("Interpolation aborted for points because the number of "
           "tuples/components in each time step are different");
        }
      outarray = this->InterpolateDataArray(
          ratio, arrays,arrays[0]->GetNumberOfTuples());
      // Do not shallow copy points from either input, because otherwise when
      // we set the actual point coordinate data we overwrite the original
      // we must instantiate a new points object 
      // (ie we override the copystrucure above)
      vtkPoints *inpoints = inPointSet1->GetPoints();
      outpoints = inpoints->NewInstance();
      outPointSet->SetPoints(outpoints);
      }
    else
      {
      // not much we can do really
      outpoints = vtkPoints::New();
      outPointSet->SetPoints(outpoints);
      }

    if (vtkDoubleArray::SafeDownCast(outarray)) 
      {
      outpoints->SetDataTypeToDouble();
      }
    else 
      {
      outpoints->SetDataTypeToFloat();
      }
    outpoints->SetNumberOfPoints(inPointSet1->GetNumberOfPoints());
    outpoints->SetData(outarray);
    outpoints->Delete();
    if (outarray)
      {
      outarray->Delete();
      }
    }
  //
  // Interpolate pointdata if present
  //
  output->GetPointData()->ShallowCopy(input[0]->GetPointData());
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
  output->GetCellData()->ShallowCopy(input[0]->GetCellData());
  for (int s=0; s<input[0]->GetCellData()->GetNumberOfArrays(); ++s) 
    {
    // copy the structure
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
  if (in1->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()) &&
      in2->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
    {
    output->GetInformation()->Set(vtkDataObject::DATA_GEOMETRY_UNMODIFIED(),1);
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
  T *outData = static_cast<T*>(output->GetVoidPointer(0));
  T *inData0 = static_cast<T*>(arrays[0]->GetVoidPointer(0));
  T *inData1 = static_cast<T*>(arrays[1]->GetVoidPointer(0));

  double oneMinusRatio = 1.0 - ratio;

  unsigned long idx;
  for (idx = 0; idx < static_cast<unsigned long>(numTuple*numComp); ++idx)
    {
    *outData = static_cast<T>((*inData0)*oneMinusRatio + (*inData1)*ratio);
    outData++;
    inData0++;
    inData1++;
  }
}


//----------------------------------------------------------------------------
vtkDataArray *vtkTemporalInterpolator
::InterpolateDataArray(double ratio, vtkDataArray **arrays, vtkIdType N)
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

