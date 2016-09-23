/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageLogic.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkImageLogic);

//----------------------------------------------------------------------------
vtkImageLogic::vtkImageLogic()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->Operation = VTK_AND;
  this->OutputTrueValue = 255;
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
void vtkImageLogicExecute1(vtkImageLogic *self, vtkImageData *inData,
                           vtkImageData *outData, int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  T trueValue = static_cast<T>(self->GetOutputTrueValue());
  int op = self->GetOperation();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    // Pixel operation
    switch (op)
    {
      case VTK_NOT:
        while (outSI != outSIEnd)
        {
          if ( ! *inSI)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI++;
        }
        break;
      case VTK_NOP:
        while (outSI != outSIEnd)
        {
          if (*inSI)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI++;
        }
        break;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void vtkImageLogicExecute2(vtkImageLogic *self, vtkImageData *in1Data,
                           vtkImageData *in2Data, vtkImageData *outData,
                           int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt1(in1Data, outExt);
  vtkImageIterator<T> inIt2(in2Data, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  T trueValue = static_cast<T>(self->GetOutputTrueValue());
  int op = self->GetOperation();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI1 = inIt1.BeginSpan();
    T* inSI2 = inIt2.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    // Pixel operation
    switch (op)
    {
      case VTK_AND:
        while (outSI != outSIEnd)
        {
          if (*inSI1 && *inSI2)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case VTK_OR:
        while (outSI != outSIEnd)
        {
          if (*inSI1 || *inSI2)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case VTK_XOR:
        while (outSI != outSIEnd)
        {
          if (( ! *inSI1 && *inSI2) || (*inSI1 && ! *inSI2))
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case VTK_NAND:
        while (outSI != outSIEnd)
        {
          if ( ! (*inSI1 && *inSI2))
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case VTK_NOR:
        while (outSI != outSIEnd)
        {
          if ( ! (*inSI1 || *inSI2))
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
    }
    inIt1.NextSpan();
    inIt2.NextSpan();
    outIt.NextSpan();
  }
}



//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageLogic::ThreadedRequestData (
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

  if (this->Operation == VTK_NOT || this->Operation == VTK_NOP)
  {
    switch (inData[0][0]->GetScalarType())
    {
      vtkTemplateMacro(
        vtkImageLogicExecute1(this, inData[0][0],
                              outData[0], outExt, id,
                              static_cast<VTK_TT *>(0)));
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
    }
  }
  else
  {
    if (inData[1][0] == NULL)
    {
      vtkErrorMacro(<< "Input " << 1 << " must be specified.");
      return;
    }

    // this filter expects that inputs that have the same type:
    if (inData[0][0]->GetScalarType() != inData[1][0]->GetScalarType())
    {
      vtkErrorMacro(<< "Execute: input1 ScalarType, "
                    << inData[0][0]->GetScalarType()
                    << ", must match input2 ScalarType "
                    << inData[1][0]->GetScalarType());
      return;
    }

    // this filter expects that inputs that have the same number of components
    if (inData[0][0]->GetNumberOfScalarComponents() !=
        inData[1][0]->GetNumberOfScalarComponents())
    {
      vtkErrorMacro(<< "Execute: input1 NumberOfScalarComponents, "
                    << inData[0][0]->GetNumberOfScalarComponents()
                    << ", must match out input2 NumberOfScalarComponents "
                    << inData[1][0]->GetNumberOfScalarComponents());
      return;
    }

    switch (inData[0][0]->GetScalarType())
    {
      vtkTemplateMacro(
        vtkImageLogicExecute2( this, inData[0][0],
                               inData[1][0], outData[0], outExt, id,
                               static_cast<VTK_TT *>(0)));
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
    }
  }
}

//----------------------------------------------------------------------------
int vtkImageLogic::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "OutputTrueValue: " << this->OutputTrueValue << "\n";
}

