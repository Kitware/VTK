/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAppendComponents.h"

#include "vtkAlgorithmOutput.h"
#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageAppendComponents);

//----------------------------------------------------------------------------
void vtkImageAppendComponents::ReplaceNthInputConnection(int idx,
                                                vtkAlgorithmOutput *input)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(0))
    {
    vtkErrorMacro("Attempt to replace connection idx " << idx
                  << " of input port " << 0 << ", which has only "
                  << this->GetNumberOfInputConnections(0)
                  << " connections.");
    return;
    }

  if (!input || !input->GetProducer())
    {
    vtkErrorMacro("Attempt to replace connection index " << idx
                  << " for input port " << 0 << " with " <<
                  (!input ? "a null input." : "an input with no producer."));
    return;
    }

  this->SetNthInputConnection(0, idx, input);
}

//----------------------------------------------------------------------------
// The default vtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void vtkImageAppendComponents::SetInputData(int idx, vtkDataObject *input)
{
  this->SetInputDataInternal(idx, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkImageAppendComponents::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// This method tells the output it will have more components
int vtkImageAppendComponents::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inScalarInfo;

  int idx1, num;
  num = 0;
  for (idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
    {
    inScalarInfo = vtkDataObject::GetActiveFieldInformation(
      inputVector[0]->GetInformationObject(idx1),
      vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (inScalarInfo &&
      inScalarInfo->Has( vtkDataObject::FIELD_NUMBER_OF_COMPONENTS() ) )
      {
      num += inScalarInfo->Get( vtkDataObject::FIELD_NUMBER_OF_COMPONENTS() );
      }
    }

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, -1, num);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageAppendComponentsExecute(vtkImageAppendComponents *self,
                                     vtkImageData *inData,
                                     vtkImageData *outData,
                                     int outComp,
                                     int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int numIn = inData->GetNumberOfScalarComponents();
  int numSkip = outData->GetNumberOfScalarComponents() - numIn;
  int i;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
    {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan() + outComp;
    T* outSIEnd = outIt.EndSpan();
    while (outSI < outSIEnd)
      {
      // now process the components
      for (i = 0; i < numIn; ++i)
        {
        *outSI = *inSI;
        ++outSI;
        ++inSI;
        }
      outSI = outSI + numSkip;
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}

//----------------------------------------------------------------------------
int vtkImageAppendComponents::FillInputPortInformation(int i,
                                                       vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i,info);
}

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppendComponents::ThreadedRequestData (
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int idx1, outComp;

  outComp = 0;
  for (idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
    {
    if (inData[0][idx1] != NULL)
      {
      // this filter expects that input is the same type as output.
      if (inData[0][idx1]->GetScalarType() != outData[0]->GetScalarType())
        {
        vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType ("
                      << inData[0][idx1]->GetScalarType()
                      << "), must match output ScalarType ("
                      << outData[0]->GetScalarType() << ")");
        return;
        }
      switch (inData[0][idx1]->GetScalarType())
        {
        vtkTemplateMacro(
          vtkImageAppendComponentsExecute ( this,
                                            inData[0][idx1], outData[0],
                                            outComp, outExt, id,
                                            static_cast<VTK_TT *>(0)) );
       default:
         vtkErrorMacro(<< "Execute: Unknown ScalarType");
         return;
       }
      outComp += inData[0][idx1]->GetNumberOfScalarComponents();
      }
    }
}

