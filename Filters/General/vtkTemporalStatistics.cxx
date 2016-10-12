// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkTemporalStatistics.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkSmartPointer.h"

#include <algorithm>

//=============================================================================
vtkStandardNewMacro(vtkTemporalStatistics);

//=============================================================================
const char * const AVERAGE_SUFFIX = "average";
const char * const MINIMUM_SUFFIX = "minimum";
const char * const MAXIMUM_SUFFIX = "maximum";
const char * const STANDARD_DEVIATION_SUFFIX = "stddev";

inline vtkStdString vtkTemporalStatisticsMangleName(const char *originalName,
                                                    const char *suffix)
{
  if (!originalName) return suffix;
  return vtkStdString(originalName) + "_" + vtkStdString(suffix);
}

//-----------------------------------------------------------------------------
// The interim stddev array keeps a sum of squares.
template<class T>
inline void vtkTemporalStatisticsInitializeStdDev(T *outArray,
                                                  vtkIdType arraySize)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    outArray[i] = 0;
  }
}

//-----------------------------------------------------------------------------
template<class T>
inline void vtkTemporalStatisticsAccumulateAverage(const T *inArray,
                                                   T *outArray,
                                                   vtkIdType arraySize)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    outArray[i] += inArray[i];
  }
}

template<class T>
inline void vtkTemporalStatisticsAccumulateMinimum(const T *inArray,
                                                   T *outArray,
                                                   vtkIdType arraySize)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    if (outArray[i] > inArray[i]) outArray[i] = inArray[i];
  }
}

template<class T>
inline void vtkTemporalStatisticsAccumulateMaximum(const T *inArray,
                                                   T *outArray,
                                                   vtkIdType arraySize)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    if (outArray[i] < inArray[i]) outArray[i] = inArray[i];
  }
}

// standard deviation one-pass algorithm from
// http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
// this is numerically stable!
template<class T>
inline void vtkTemporalStatisticsAccumulateStdDev(
  const T *inArray, T *outArray, const T *previousAverage,
  vtkIdType arraySize, int pass)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    double temp = inArray[i]-previousAverage[i]/static_cast<double>(pass);
    outArray[i] = outArray[i] + static_cast<T>(
      pass*temp*temp/static_cast<double>(pass+1) );
  }
}

//-----------------------------------------------------------------------------
template<class T>
inline void vtkTemporalStatisticsFinishAverage(T *outArray, vtkIdType arraySize,
                                               int sumSize)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    outArray[i] /= sumSize;
  }
}

template<class T>
inline void vtkTemporalStatisticsFinishStdDev(T *outArray,
                                              vtkIdType arraySize, int sumSize)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    outArray[i] =
      static_cast<T>(sqrt(static_cast<double>(outArray[i])/sumSize));
  }
}

//=============================================================================
vtkTemporalStatistics::vtkTemporalStatistics()
{
  this->ComputeAverage = 1;
  this->ComputeMinimum = 1;
  this->ComputeMaximum = 1;
  this->ComputeStandardDeviation = 1;

  this->CurrentTimeIndex = 0;
  this->GeneratedChangingTopologyWarning = false;
}

vtkTemporalStatistics::~vtkTemporalStatistics()
{
}

void vtkTemporalStatistics::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ComputeAverage: " << this->ComputeAverage << endl;
  os << indent << "ComputeMinimum: " << this->ComputeMinimum << endl;
  os << indent << "ComputeMaximum: " << this->ComputeMaximum << endl;
  os << indent << "ComputeStandardDeviation: " <<
    this->ComputeStandardDeviation << endl;
}

//-----------------------------------------------------------------------------
int vtkTemporalStatistics::FillInputPortInformation(int vtkNotUsed(port),
                                                    vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkTemporalStatistics::RequestInformation(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **vtkNotUsed(inputVector),
                                 vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // The output data of this filter has no time associated with it.  It is the
  // result of computations that happen over all time.
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//-----------------------------------------------------------------------------
int vtkTemporalStatistics::RequestDataObject(
                                            vtkInformation *vtkNotUsed(request),
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject *input = vtkDataObject::GetData(inInfo);
  vtkDataObject *output = vtkDataObject::GetData(outInfo);

  if (!input)
  {
    return 0;
  }

  vtkSmartPointer<vtkDataObject> newOutput;

  if (!output || !output->IsA(input->GetClassName()))
  {
    newOutput.TakeReference(input->NewInstance());
  }


  if (newOutput)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkTemporalStatistics::RequestUpdateExtent(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // The RequestData method will tell the pipeline executive to iterate the
  // upstream pipeline to get each time step in order.  The executive in turn
  // will call this method to get the extent request for each iteration (in this
  // case the time step).
  double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkTemporalStatistics::RequestData(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject *input = vtkDataObject::GetData(inInfo);
  vtkDataObject *output = vtkDataObject::GetData(outInfo);

  if (this->CurrentTimeIndex == 0)
  {
    // First execution, initialize arrays.
    this->InitializeStatistics(input, output);
  }
  else
  {
    // Subsequent execution, accumulate new data.
    this->AccumulateStatistics(input, output);
  }

  this->CurrentTimeIndex++;

  if (  this->CurrentTimeIndex
      < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    // There is still more to do.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    // We are done.  Finish up.
    this->PostExecute(input, output);
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::InitializeStatistics(vtkDataObject *input,
                                                 vtkDataObject *output)
{
  if (input->IsA("vtkDataSet"))
  {
    this->InitializeStatistics(vtkDataSet::SafeDownCast(input),
                               vtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkGraph"))
  {
    this->InitializeStatistics(vtkGraph::SafeDownCast(input),
                               vtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    this->InitializeStatistics(vtkCompositeDataSet::SafeDownCast(input),
                               vtkCompositeDataSet::SafeDownCast(output));
    return;
  }

  vtkWarningMacro(<< "Unsupported input type: " << input->GetClassName());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::InitializeStatistics(vtkDataSet *input,
                                                 vtkDataSet *output)
{
  output->CopyStructure(input);
  this->InitializeArrays(input->GetFieldData(), output->GetFieldData());
  this->InitializeArrays(input->GetPointData(), output->GetPointData());
  this->InitializeArrays(input->GetCellData(), output->GetCellData());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::InitializeStatistics(vtkGraph *input,
                                                 vtkGraph *output)
{
  output->CopyStructure(input);
  this->InitializeArrays(input->GetFieldData(), output->GetFieldData());
  this->InitializeArrays(input->GetVertexData(), output->GetVertexData());
  this->InitializeArrays(input->GetEdgeData(), output->GetEdgeData());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::InitializeStatistics(vtkCompositeDataSet *input,
                                                 vtkCompositeDataSet *output)
{
  output->CopyStructure(input);

  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal();  !inputItr->IsDoneWithTraversal();
       inputItr->GoToNextItem())
  {
    vtkDataObject *inputObj = inputItr->GetCurrentDataObject();

    vtkSmartPointer<vtkDataObject> outputObj;
    outputObj.TakeReference(inputObj->NewInstance());

    this->InitializeStatistics(inputObj, outputObj);
    output->SetDataSet(inputItr, outputObj);
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::InitializeArrays(vtkFieldData *inFd,
                                             vtkFieldData *outFd)
{
  // Because we need to do mathematical operations, we require all arrays we
  // process to be numeric data (i.e. a vtkDataArray).  We also handle global
  // ids and petigree ids special (we just pass them).  Ideally would just let
  // vtkFieldData or vtkDataSetAttributes handle this for us, but no such method
  // that fits our needs here.  Thus, we pass data a bit differently then other
  // filters.  If I miss something important, it should be added here.

  outFd->Initialize();

  vtkDataSetAttributes *inDsa = vtkDataSetAttributes::SafeDownCast(inFd);
  vtkDataSetAttributes *outDsa = vtkDataSetAttributes::SafeDownCast(outFd);
  if (inDsa)
  {
    vtkDataArray *globalIds = inDsa->GetGlobalIds();
    vtkAbstractArray *pedigreeIds = inDsa->GetPedigreeIds();
    if (globalIds) outDsa->SetGlobalIds(globalIds);
    if (pedigreeIds) outDsa->SetPedigreeIds(pedigreeIds);
  }

  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    vtkDataArray *array = inFd->GetArray(i);
    if (!array) continue;                               // Array not numeric.
    if (outFd->HasArray(array->GetName())) continue;    // Must be Ids.

    this->InitializeArray(array, outFd);
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::InitializeArray(vtkDataArray *array,
                                            vtkFieldData *outFd)
{
  if (this->ComputeAverage || this->ComputeStandardDeviation)
  {
    vtkSmartPointer<vtkDataArray> newArray;
    newArray.TakeReference(vtkArrayDownCast<vtkDataArray>(
                          vtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->DeepCopy(array);
    newArray->SetName(vtkTemporalStatisticsMangleName(array->GetName(),
                                                      AVERAGE_SUFFIX));
    if (outFd->HasArray(newArray->GetName()))
    {
      vtkWarningMacro(<< "Input has two arrays named " << array->GetName()
                      << ".  Output statistics will probably be wrong.");
      return;
    }
    outFd->AddArray(newArray);
  }

  if (this->ComputeMinimum)
  {
    vtkSmartPointer<vtkDataArray> newArray;
    newArray.TakeReference(vtkArrayDownCast<vtkDataArray>(
                          vtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->DeepCopy(array);
    newArray->SetName(vtkTemporalStatisticsMangleName(array->GetName(),
                                                      MINIMUM_SUFFIX));
    outFd->AddArray(newArray);
  }

  if (this->ComputeMaximum)
  {
    vtkSmartPointer<vtkDataArray> newArray;
    newArray.TakeReference(vtkArrayDownCast<vtkDataArray>(
                          vtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->DeepCopy(array);
    newArray->SetName(vtkTemporalStatisticsMangleName(array->GetName(),
                                                      MAXIMUM_SUFFIX));
    outFd->AddArray(newArray);
  }

  if (this->ComputeStandardDeviation)
  {
    vtkSmartPointer<vtkDataArray> newArray;
    newArray.TakeReference(vtkArrayDownCast<vtkDataArray>(
                          vtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->SetName(vtkTemporalStatisticsMangleName(array->GetName(),
                                                    STANDARD_DEVIATION_SUFFIX));

    newArray->SetNumberOfComponents(array->GetNumberOfComponents());
    newArray->CopyComponentNames( array );

    newArray->SetNumberOfTuples(array->GetNumberOfTuples());
    switch (array->GetDataType())
    {
      vtkTemplateMacro(vtkTemporalStatisticsInitializeStdDev(
                           static_cast<VTK_TT*>(newArray->GetVoidPointer(0)),
                           array->GetNumberOfComponents()
                           *array->GetNumberOfTuples()));
    }
    outFd->AddArray(newArray);
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::AccumulateStatistics(vtkDataObject *input,
                                                 vtkDataObject *output)
{
  if (input->IsA("vtkDataSet"))
  {
    this->AccumulateStatistics(vtkDataSet::SafeDownCast(input),
                               vtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkGraph"))
  {
    this->AccumulateStatistics(vtkGraph::SafeDownCast(input),
                               vtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    this->AccumulateStatistics(vtkCompositeDataSet::SafeDownCast(input),
                               vtkCompositeDataSet::SafeDownCast(output));
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::AccumulateStatistics(vtkDataSet *input,
                                                 vtkDataSet *output)
{
  this->AccumulateArrays(input->GetFieldData(), output->GetFieldData());
  this->AccumulateArrays(input->GetPointData(), output->GetPointData());
  this->AccumulateArrays(input->GetCellData(), output->GetCellData());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::AccumulateStatistics(vtkGraph *input,
                                                 vtkGraph *output)
{
  this->AccumulateArrays(input->GetFieldData(), output->GetFieldData());
  this->AccumulateArrays(input->GetVertexData(), output->GetVertexData());
  this->AccumulateArrays(input->GetEdgeData(), output->GetEdgeData());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::AccumulateStatistics(vtkCompositeDataSet *input,
                                                 vtkCompositeDataSet *output)
{
  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal();  !inputItr->IsDoneWithTraversal();
       inputItr->GoToNextItem())
  {
    vtkDataObject *inputObj = inputItr->GetCurrentDataObject();
    vtkDataObject *outputObj = output->GetDataSet(inputItr);

    this->AccumulateStatistics(inputObj, outputObj);
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::AccumulateArrays(vtkFieldData *inFd,
                                             vtkFieldData *outFd)
{
  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    vtkDataArray *inArray = inFd->GetArray(i);
    vtkDataArray *outArray;
    if (!inArray) continue;

    outArray = this->GetArray(outFd, inArray, AVERAGE_SUFFIX);
    if (outArray)
    {


      vtkDataArray* stdevOutArray =
        this->GetArray(outFd, inArray, STANDARD_DEVIATION_SUFFIX);
      if (stdevOutArray)
      {
      switch (inArray->GetDataType())
      {
        // standard deviation must be called before average since the one-pass
        // algorithm uses the average up to the previous time step
        vtkTemplateMacro(vtkTemporalStatisticsAccumulateStdDev(
                           static_cast<const VTK_TT*>(inArray->GetVoidPointer(0)),
                           static_cast<VTK_TT*>(stdevOutArray->GetVoidPointer(0)),
                           static_cast<const VTK_TT*>(outArray->GetVoidPointer(0)),
                           inArray->GetNumberOfComponents()*inArray->GetNumberOfTuples(),
                           this->CurrentTimeIndex));
      }
      // Alert change in data.
      stdevOutArray->DataChanged();
      }




      switch (inArray->GetDataType())
      {
        vtkTemplateMacro(vtkTemporalStatisticsAccumulateAverage(
                        static_cast<const VTK_TT*>(inArray->GetVoidPointer(0)),
                        static_cast<VTK_TT*>(outArray->GetVoidPointer(0)),
                        inArray->GetNumberOfComponents()
                        *inArray->GetNumberOfTuples()));
      }
      // Alert change in data.
      outArray->DataChanged();
    }

    outArray = this->GetArray(outFd, inArray, MINIMUM_SUFFIX);
    if (outArray)
    {
      switch (inArray->GetDataType())
      {
        vtkTemplateMacro(vtkTemporalStatisticsAccumulateMinimum(
                        static_cast<const VTK_TT*>(inArray->GetVoidPointer(0)),
                        static_cast<VTK_TT*>(outArray->GetVoidPointer(0)),
                        inArray->GetNumberOfComponents()
                           *inArray->GetNumberOfTuples()));
      }
      // Alert change in data.
      outArray->DataChanged();
    }

    outArray = this->GetArray(outFd, inArray, MAXIMUM_SUFFIX);
    if (outArray)
    {
      switch (inArray->GetDataType())
      {
        vtkTemplateMacro(vtkTemporalStatisticsAccumulateMaximum(
                        static_cast<const VTK_TT*>(inArray->GetVoidPointer(0)),
                        static_cast<VTK_TT*>(outArray->GetVoidPointer(0)),
                        inArray->GetNumberOfComponents()
                         *inArray->GetNumberOfTuples()));
      }
      // Alert change in data.
      outArray->DataChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::PostExecute(vtkDataObject *input,
                                        vtkDataObject *output)
{
  if (input->IsA("vtkDataSet"))
  {
    this->PostExecute(vtkDataSet::SafeDownCast(input),
                      vtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkGraph"))
  {
    this->PostExecute(vtkGraph::SafeDownCast(input),
                      vtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    this->PostExecute(vtkCompositeDataSet::SafeDownCast(input),
                      vtkCompositeDataSet::SafeDownCast(output));
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::PostExecute(vtkDataSet *input, vtkDataSet *output)
{
  this->FinishArrays(input->GetFieldData(), output->GetFieldData());
  this->FinishArrays(input->GetPointData(), output->GetPointData());
  this->FinishArrays(input->GetCellData(), output->GetCellData());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::PostExecute(vtkGraph *input, vtkGraph *output)
{
  this->FinishArrays(input->GetFieldData(), output->GetFieldData());
  this->FinishArrays(input->GetVertexData(), output->GetVertexData());
  this->FinishArrays(input->GetEdgeData(), output->GetEdgeData());
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::PostExecute(vtkCompositeDataSet *input,
                                        vtkCompositeDataSet *output)
{
  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal();  !inputItr->IsDoneWithTraversal();
       inputItr->GoToNextItem())
  {
    vtkDataObject *inputObj = inputItr->GetCurrentDataObject();
    vtkDataObject *outputObj = output->GetDataSet(inputItr);

    this->PostExecute(inputObj, outputObj);
  }
}

//-----------------------------------------------------------------------------
void vtkTemporalStatistics::FinishArrays(vtkFieldData *inFd,
                                             vtkFieldData *outFd)
{
  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    vtkDataArray *inArray = inFd->GetArray(i);
    vtkDataArray *outArray;
    if (!inArray) continue;

    outArray = this->GetArray(outFd, inArray, AVERAGE_SUFFIX);
    if (outArray)
    {
      switch (inArray->GetDataType())
      {
        vtkTemplateMacro(vtkTemporalStatisticsFinishAverage(
                            static_cast<VTK_TT*>(outArray->GetVoidPointer(0)),
                            inArray->GetNumberOfComponents()
                             *inArray->GetNumberOfTuples(),
                            this->CurrentTimeIndex));
      }
    }
    vtkDataArray *avgArray = outArray;

    // No post processing on minimum.
    // No post processing on maximum.

    outArray = this->GetArray(outFd, inArray, STANDARD_DEVIATION_SUFFIX);
    if (outArray)
    {
      if (!avgArray)
      {
        vtkWarningMacro(<< "Average not computed for " << inArray->GetName()
                        << ", standard deviation skipped.");
        outFd->RemoveArray(outArray->GetName());
      }
      else
      {
        switch (inArray->GetDataType())
        {
          vtkTemplateMacro(vtkTemporalStatisticsFinishStdDev(
                     static_cast<VTK_TT*>(outArray->GetVoidPointer(0)),
                     inArray->GetNumberOfComponents()
                      *inArray->GetNumberOfTuples(),
                     this->CurrentTimeIndex));
        }
        if (!this->ComputeAverage)
        {
          outFd->RemoveArray(avgArray->GetName());
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
vtkDataArray *vtkTemporalStatistics::GetArray(vtkFieldData *fieldData,
                                              vtkDataArray *inArray,
                                              const char *nameSuffix)
{
  vtkStdString outArrayName
    = vtkTemporalStatisticsMangleName(inArray->GetName(), nameSuffix);
  vtkDataArray *outArray = fieldData->GetArray(outArrayName.c_str());
  if (!outArray) return NULL;

  if (   (inArray->GetNumberOfComponents() != outArray->GetNumberOfComponents())
      || (inArray->GetNumberOfTuples() != outArray->GetNumberOfTuples()) )
  {
    if(!this->GeneratedChangingTopologyWarning)
    {
      std::string fieldType = vtkCellData::SafeDownCast(fieldData) == NULL ?
        "points" : "cells";
      vtkWarningMacro("The number of " << fieldType << " has changed between time "
                      << "steps. No arrays of this type will be output since this "
                      << "filter can not handle grids that change over time.");
      this->GeneratedChangingTopologyWarning = true;
    }
    fieldData->RemoveArray(outArray->GetName());
    return NULL;
  }

  return outArray;
}
