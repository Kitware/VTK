// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageMathematics.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageMathematics);

//------------------------------------------------------------------------------
vtkImageMathematics::vtkImageMathematics()
{
  this->Operation = VTK_ADD;
  this->ConstantK = 1.0;
  this->ConstantC = 0.0;
  this->DivideByZeroToC = 0;
}

//------------------------------------------------------------------------------
void vtkImageMathematics::ReplaceNthInputConnection(int idx, vtkAlgorithmOutput* input)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(0))
  {
    vtkErrorMacro("Attempt to replace connection idx "
      << idx << " of input port " << 0 << ", which has only "
      << this->GetNumberOfInputConnections(0) << " connections.");
    return;
  }

  if (!input || !input->GetProducer())
  {
    vtkErrorMacro("Attempt to replace connection index "
      << idx << " for input port " << 0 << " with "
      << (!input ? "a null input." : "an input with no producer."));
    return;
  }

  this->SetNthInputConnection(0, idx, input);
}

//------------------------------------------------------------------------------
// The default vtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void vtkImageMathematics::SetInputData(int vtkNotUsed(idx), vtkDataObject* input)
{
  this->AddInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
// The default vtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void vtkImageMathematics::SetInputConnection(int idx, vtkAlgorithmOutput* input)
{
  if (idx > 0)
  {
    this->AddInputConnection(0, input);
  }
  else
  {
    this->Superclass::SetInputConnection(idx, input);
  }
}

//------------------------------------------------------------------------------
vtkDataObject* vtkImageMathematics::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
  {
    return nullptr;
  }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//------------------------------------------------------------------------------
// The output extent is the intersection.
int vtkImageMathematics::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo;

  int c, idx;
  int ext[6], unionExt[6];

  // Initialize the union.
  inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), unionExt);

  // two input take intersection
  if (this->Operation == VTK_ADD || this->Operation == VTK_SUBTRACT ||
    this->Operation == VTK_MULTIPLY || this->Operation == VTK_DIVIDE ||
    this->Operation == VTK_MIN || this->Operation == VTK_MAX || this->Operation == VTK_ATAN2)
  {
    for (c = 0; c < this->GetNumberOfInputConnections(0); ++c)
    {
      inInfo = inputVector[0]->GetInformationObject(c);
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      for (idx = 0; idx < 3; ++idx)
      {
        if (unionExt[idx * 2] > ext[idx * 2])
        {
          unionExt[idx * 2] = ext[idx * 2];
        }
        if (unionExt[idx * 2 + 1] < ext[idx * 2 + 1])
        {
          unionExt[idx * 2 + 1] = ext[idx * 2 + 1];
        }
      }
    }
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), unionExt, 6);

  return 1;
}

//------------------------------------------------------------------------------
template <class TValue, class TIvar>
void vtkImageMathematicsClamp(TValue& value, TIvar ivar, vtkImageData* data)
{
  if (ivar < static_cast<TIvar>(data->GetScalarTypeMin()))
  {
    value = static_cast<TValue>(data->GetScalarTypeMin());
  }
  else if (ivar > static_cast<TIvar>(data->GetScalarTypeMax()))
  {
    value = static_cast<TValue>(data->GetScalarTypeMax());
  }
  else
  {
    value = static_cast<TValue>(ivar);
  }
}

//------------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
void vtkImageMathematicsExecute1(vtkImageMathematics* self, vtkImageData* in1Data, T* in1Ptr,
  vtkImageData* outData, T* outPtr, int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0] + 1) * in1Data->GetNumberOfScalarComponents();
  // What a pain.  Maybe I should just make another filter.
  if (op == VTK_CONJUGATE)
  {
    rowLength = (outExt[1] - outExt[0] + 1);
  }
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ + 1) * (maxY + 1) / 50.0);
  target++;

  // Get increments to march through data
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  int divideByZeroToC = self->GetDivideByZeroToC();
  double doubleConstantk = self->GetConstantK();

  // Avoid casts by making constants the same type as input/output
  // Of course they must be clamped to a valid range for the scalar type
  T constantk, constantc;
  vtkImageMathematicsClamp(constantk, self->GetConstantK(), in1Data);
  vtkImageMathematicsClamp(constantc, self->GetConstantC(), in1Data);

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    for (idxY = 0; idxY <= maxY; idxY++)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }
      for (idxR = 0; idxR < rowLength; idxR++)
      {
        // Pixel operation
        switch (op)
        {
          case VTK_INVERT:
            if (*in1Ptr)
            {
              *outPtr = static_cast<T>(1.0 / *in1Ptr);
            }
            else
            {
              if (divideByZeroToC)
              {
                *outPtr = constantc;
              }
              else
              {
                *outPtr = static_cast<T>(outData->GetScalarTypeMax());
              }
            }
            break;
          case VTK_SIN:
            *outPtr = static_cast<T>(sin(static_cast<double>(*in1Ptr)));
            break;
          case VTK_COS:
            *outPtr = static_cast<T>(cos(static_cast<double>(*in1Ptr)));
            break;
          case VTK_EXP:
            *outPtr = static_cast<T>(exp(static_cast<double>(*in1Ptr)));
            break;
          case VTK_LOG:
            *outPtr = static_cast<T>(log(static_cast<double>(*in1Ptr)));
            break;
          case VTK_ABS:
            *outPtr = static_cast<T>(fabs(static_cast<double>(*in1Ptr)));
            break;
          case VTK_SQR:
            *outPtr = static_cast<T>(*in1Ptr * *in1Ptr);
            break;
          case VTK_SQRT:
            *outPtr = static_cast<T>(sqrt(static_cast<double>(*in1Ptr)));
            break;
          case VTK_ATAN:
            *outPtr = static_cast<T>(atan(static_cast<double>(*in1Ptr)));
            break;
          case VTK_MULTIPLYBYK:
            *outPtr = static_cast<T>(doubleConstantk * static_cast<double>(*in1Ptr));
            break;
          case VTK_ADDC:
            *outPtr = constantc + *in1Ptr;
            break;
          case VTK_REPLACECBYK:
            *outPtr = (*in1Ptr == constantc) ? constantk : *in1Ptr;
            break;
          case VTK_CONJUGATE:
            outPtr[0] = in1Ptr[0];
            outPtr[1] = static_cast<T>(-1.0 * static_cast<double>(in1Ptr[1]));
            // Why bother trying to figure out the continuous increments.
            outPtr++;
            in1Ptr++;
            break;
        }
        outPtr++;
        in1Ptr++;
      }
      outPtr += outIncY;
      in1Ptr += inIncY;
    }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
  }
}

//------------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void vtkImageMathematicsExecute2(vtkImageMathematics* self, vtkImageData* inData, T* inPtr,
  vtkImageData* outData, T* outPtr, int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();
  int divideByZeroToC = self->GetDivideByZeroToC();
  double constantc = self->GetConstantC();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0] + 1) * inData->GetNumberOfScalarComponents();
  // What a pain.  Maybe I should just make another filter.
  if (op == VTK_COMPLEX_MULTIPLY)
  {
    rowLength = (outExt[1] - outExt[0] + 1);
  }

  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ + 1) * (maxY + 1) / 50.0);
  target++;

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }
      for (idxR = 0; idxR < rowLength; idxR++)
      {
        // Pixel operation
        switch (op)
        {
          case VTK_ADD:
            *outPtr += *inPtr;
            break;
          case VTK_SUBTRACT:
            *outPtr -= *inPtr;
            break;
          case VTK_MULTIPLY:
            *outPtr *= *inPtr;
            break;
          case VTK_DIVIDE:
            if (*inPtr)
            {
              *outPtr /= *inPtr;
            }
            else
            {
              if (divideByZeroToC)
              {
                *outPtr = static_cast<T>(constantc);
              }
              else
              {
                // *outPtr = (T)(*in1Ptr / 0.00001);
                *outPtr = static_cast<T>(outData->GetScalarTypeMax());
              }
            }
            break;
          case VTK_MIN:
            *outPtr = (*outPtr < *inPtr ? *outPtr : *inPtr);
            break;
          case VTK_MAX:
            *outPtr = (*outPtr > *inPtr ? *outPtr : *inPtr);
            break;
          case VTK_ATAN2:
            if (*outPtr == 0.0 && *inPtr == 0.0)
            {
              *outPtr = 0;
            }
            else
            {
              *outPtr =
                static_cast<T>(atan2(static_cast<double>(*outPtr), static_cast<double>(*inPtr)));
            }
            break;
          case VTK_COMPLEX_MULTIPLY:
            double tmp[2];
            tmp[0] = outPtr[0];
            tmp[1] = outPtr[1];
            outPtr[0] = tmp[0] * inPtr[0] - tmp[1] * inPtr[1];
            outPtr[1] = tmp[1] * inPtr[0] + tmp[0] * inPtr[1];
            // Why bother trying to figure out the continuous increments.
            outPtr++;
            inPtr++;
            break;
        }
        outPtr++;
        inPtr++;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
    outPtr += outIncZ;
    inPtr += inIncZ;
  }
}

//------------------------------------------------------------------------------
template <class T>
void vtkImageMathematicsInitOutput(
  vtkImageData* inData, T* inPtr, vtkImageData* vtkNotUsed(outData), T* outPtr, int ext[6])
{
  int idxY, idxZ;
  int maxY, maxZ;
  vtkIdType outIncY, outIncZ;
  int rowLength;
  int typeSize;
  T *outPtrZ, *outPtrY;
  T *inPtrZ, *inPtrY;

  // This method needs to copy scalars from input to output for the update-extent.
  vtkDataArray* inArray = inData->GetPointData()->GetScalars();
  typeSize = vtkDataArray::GetDataTypeSize(inArray->GetDataType());
  outPtrZ = outPtr;
  inPtrZ = inPtr;
  // Get increments to march through data
  vtkIdType increments[3];
  increments[0] = inArray->GetNumberOfComponents();
  increments[1] = increments[0] * (ext[1] - ext[0] + 1);
  increments[2] = increments[1] * (ext[3] - ext[2] + 1);
  outIncY = increments[1];
  outIncZ = increments[2];

  // Find the region to loop over
  rowLength = (ext[1] - ext[0] + 1) * inArray->GetNumberOfComponents();
  rowLength *= typeSize;
  maxY = ext[3] - ext[2];
  maxZ = ext[5] - ext[4];

  // Loop through input pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    outPtrY = outPtrZ;
    inPtrY = inPtrZ;
    for (idxY = 0; idxY <= maxY; idxY++)
    {
      memcpy(outPtrY, inPtrY, rowLength);
      outPtrY += outIncY;
      inPtrY += outIncY;
    }
    outPtrZ += outIncZ;
    inPtrZ += outIncZ;
  }
}

//------------------------------------------------------------------------------
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageMathematics::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector),
  vtkImageData*** inData, vtkImageData** outData, int outExt[6], int id)
{
  void* inPtr1;
  void* outPtr;

  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  for (int idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
  {
    inPtr1 = inData[0][idx1]->GetScalarPointerForExtent(outExt);
    if (this->Operation == VTK_ADD || this->Operation == VTK_SUBTRACT ||
      this->Operation == VTK_MULTIPLY || this->Operation == VTK_DIVIDE ||
      this->Operation == VTK_MIN || this->Operation == VTK_MAX || this->Operation == VTK_ATAN2 ||
      this->Operation == VTK_COMPLEX_MULTIPLY)
    {
      if (this->Operation == VTK_COMPLEX_MULTIPLY)
      {
        if (inData[0][idx1]->GetNumberOfScalarComponents() != 2 ||
          inData[0][idx1]->GetNumberOfScalarComponents() != 2)
        {
          vtkErrorMacro("Complex inputs must have two components.");
          return;
        }
        // this filter expects that input is the same type as output.
        if (inData[0][idx1]->GetScalarType() != outData[0]->GetScalarType())
        {
          vtkErrorMacro(<< "Execute: input1 ScalarType, " << inData[0][idx1]->GetScalarType()
                        << ", must match output ScalarType " << outData[0]->GetScalarType());
          return;
        }
      }
      if (idx1 == 0)
      {
        switch (inData[0][idx1]->GetScalarType())
        {
          vtkTemplateMacro(vtkImageMathematicsInitOutput(inData[0][idx1],
            static_cast<VTK_TT*>(inPtr1), outData[0], static_cast<VTK_TT*>(outPtr), outExt));
          default:
            vtkErrorMacro(<< "InitOutput: Unknown ScalarType");
            return;
        }
      }
      else
      {
        switch (inData[0][idx1]->GetScalarType())
        {
          vtkTemplateMacro(vtkImageMathematicsExecute2(this, inData[0][idx1],
            static_cast<VTK_TT*>(inPtr1), outData[0], static_cast<VTK_TT*>(outPtr), outExt, id));
          default:
            vtkErrorMacro(<< "Execute: Unknown ScalarType");
            return;
        }
      }
    }
    else
    {
      // this filter expects that input is the same type as output.
      if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
      {
        vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0][0]->GetScalarType()
                      << ", must match out ScalarType " << outData[0]->GetScalarType());
        return;
      }

      if (this->Operation == VTK_CONJUGATE)
      {
        if (inData[0][0]->GetNumberOfScalarComponents() != 2)
        {
          vtkErrorMacro("Complex inputs must have two components.");
          return;
        }
      }

      switch (inData[0][0]->GetScalarType())
      {
        vtkTemplateMacro(vtkImageMathematicsExecute1(this, inData[0][0],
          static_cast<VTK_TT*>(inPtr1), outData[0], static_cast<VTK_TT*>(outPtr), outExt, id));
        default:
          vtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkImageMathematics::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkImageMathematics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "ConstantK: " << this->ConstantK << "\n";
  os << indent << "ConstantC: " << this->ConstantC << "\n";
  os << indent << "DivideByZeroToC: " << (this->DivideByZeroToC ? "On" : "Off") << "\n";
}
VTK_ABI_NAMESPACE_END
