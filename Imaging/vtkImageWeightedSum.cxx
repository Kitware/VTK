/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWeightedSum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageWeightedSum.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkImageWeightedSum, "1.3");
vtkStandardNewMacro(vtkImageWeightedSum);

vtkCxxSetObjectMacro(vtkImageWeightedSum,Weights,vtkDoubleArray);
//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageWeightedSum::vtkImageWeightedSum()
{
  this->SetNumberOfInputPorts(1);

  // array of weights: need as many weights as inputs
  this->Weights = vtkDoubleArray::New();
  // By default normalize
  this->NormalizeByWeight = 1;
}


//----------------------------------------------------------------------------
vtkImageWeightedSum::~vtkImageWeightedSum()
{
  this->Weights->Delete();
}

//----------------------------------------------------------------------------
void vtkImageWeightedSum::SetWeight(vtkIdType id, double weight)
{
  // Reallocate if needed:
  this->Weights->SetNumberOfTuples(id+1);
  // Pass the new weight
  this->Weights->SetValue(id, weight);
}

//----------------------------------------------------------------------------
double vtkImageWeightedSum::CalculateTotalWeight() 
{
  double totalWeight = 0.0;

  for(int i = 0; i < this->Weights->GetNumberOfTuples(); ++i)
    {
    totalWeight += this->Weights->GetValue(i);
    }
  return totalWeight;
}

//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageWeightedSumExecute(vtkImageWeightedSum *self,
                          vtkImageData **inDatas, int numInputs, vtkImageData *outData,
                          int outExt[6], int id, T*)
{
  const int fastpath = 256;
  vtkImageIterator<T> inItsFast[fastpath];
  T* inSIFast[fastpath];
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);

  double *weights = ((vtkDoubleArray *)self->GetWeights())->GetPointer(0);
  double totalWeight = self->CalculateTotalWeight();
  int normalize = self->GetNormalizeByWeight();
  vtkImageIterator<T> *inIts;
  T* *inSI;
  if( numInputs < fastpath )
    {
    inIts = inItsFast;
    inSI = inSIFast;
    }
  else
    {
    inIts = new vtkImageIterator<T>[numInputs];
    inSI = new T*[numInputs];
    }

  // Loop through all input ImageData to initialize iterators
  for(int i=0; i < numInputs; ++i)
    {
    inIts[i].Initialize(inDatas[i], outExt);
    }
  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    for(int j=0; j < numInputs; ++j)
      {
      inSI[j] = inIts[j].BeginSpan();
      }
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    // Pixel operation
    while (outSI != outSIEnd)
      {
      double sum = 0.;
      for(int k=0; k < numInputs; ++k)
        {
        sum += weights[k] * *inSI[k];
        }
      // Divide only if needed and different from 0
      if (normalize && totalWeight != 0.0)
        {
        sum /= totalWeight;
        }
      *outSI = static_cast<T>(sum); // do the cast only at the end
      outSI++;
      for(int l=0; l < numInputs; ++l)
        {
        inSI[l]++;
        }
      }
    for(int j=0; j < numInputs; ++j)
      {
      inIts[j].NextSpan();
      }
    outIt.NextSpan();
    }

  if( numInputs >= fastpath )
    {
    delete[] inIts;
    delete[] inSI;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageWeightedSum::ThreadedRequestData (
  vtkInformation * vtkNotUsed( request ), 
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData, 
  vtkImageData **outData,
  int outExt[6], int id)
{
  if (inData[0][0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " 
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType " 
                  << outData[0]->GetScalarType());
    return;
    }
  
  int numInputs = this->GetNumberOfInputConnections(0);
  int numWeights = this->Weights->GetNumberOfTuples();
  if( numWeights != numInputs )
    {
    vtkErrorMacro( << "Execute: There are " << numInputs << " vtkImageData provided"
      << " but only " << numWeights << " number of weights provided" );
    return;
    }

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageWeightedSumExecute(this , inData[0], numInputs,
        outData[0], outExt, id, static_cast<VTK_TT *>(0))
      );
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
int vtkImageWeightedSum::FillInputPortInformation(int i, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i,info);
}

//----------------------------------------------------------------------------
void vtkImageWeightedSum::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  // objects
  os << indent << "NormalizeByWeight: " <<
    (this->NormalizeByWeight ? "On" : "Off" ) << "\n";
  os << indent << "Weights: " << this->Weights << "\n";
  this->Weights->PrintSelf(os,indent.GetNextIndent());
}

