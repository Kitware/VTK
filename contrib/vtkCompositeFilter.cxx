/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkCompositeFilter.h"

//----------------------------------------------------------------------------
vtkCompositeFilter::vtkCompositeFilter()
{
}

//----------------------------------------------------------------------------
vtkCompositeFilter::~vtkCompositeFilter()
{
}

//----------------------------------------------------------------------------
void vtkCompositeFilter::AddInput(vtkStructuredPoints *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
void vtkCompositeFilter::RemoveInput(vtkStructuredPoints *ds)
{
  this->vtkProcessObject::RemoveInput(ds);
}

//----------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single unstructured grid
void vtkCompositeFilter::Execute()
{
  vtkStructuredPoints *output = this->GetOutput();
  int numPts output->GetNumberOfPoints()
  vtkStructuredPoints *input;
  int i, j;
  vtkScalars *inPScalars;
  vtkDataArray *inZData;
  vtkFloatArray *outZArray;
  vtkFieldData *outZField;
  float *outZPtr, *inZ, *outZ;
  vtkUnsignedCharArray *outPArray;
  vtkScalars *outPScalars;
  unsigned char *outPPtr, *inP, *outP;

  // allocate the output
  outZArray = vtkFloatArray::New();
  outZArray->Allocate(numPts);
  outZArray->SetNumberOfTuples(numPts);
  outZPtr = outZArray->WritePointer(0, numPts);
  outZField = vtkFieldData::New();
  outZField->SetArray(0, zArray);
  outZField->SetArrayName(0, "ZBuffer");

  outPArray = vtkUnsignedCharArray::New();
  outPArray->SetNumberOfComponents(4);
  outPArray->Allocate(numPts);
  outPPtr = outPArray->WritePointer(0, numPts);
  outPScalars = vtkScalars::New();
  outPScalars->SetData(outPArray);

  // initialze the zbuffer to 0.0
  memset(outZPtr, 0, numPts*sizeof(float));

  // composite each input
  for (i = 0; i < this->NumberOfInputs; ++i)
    {
    input = (vtkStructuredPoints*)(this->Inputs[0]);
    if (input && input->GetPointData()->GetScalars() && 
        input->GetPointData()->GetFieldData()) 
      {
      if (input->GetNumberOfPoints() != numPts)
        {
        vtkErrorMacro("PointMismatch.");
        continue;
        }
      inPScalars = input->GetPointData()->GetScalars();
      if (inPScalars->GetDataType() != VTK_UNSIGNED_CHAR ||
          inPScalars->GetNumberOfComponents() != 4)
        {
        vtkErrorMacro("Bap Pixel data format.");
        continue;
        }
      inZData = input->GetPointData()->GetFieldData()->GetArray("ZBuffer");
      if (inZData == NULL || inZData->GetDataType() != VTK_FLOAT)
        {
        vtkErrorMacro("Bad z data format");
        continue;
        }

      outZ = outZPtr;
      inZ = ((vtkFloatArray*)inZData)->GetPointer(0);
      outP = outPPtr;
      inP = (unsigned char *)(inPScalars->GetVoidPointer(0));
      
      for (j = 0; j < numPts; ++j)
        {
        if (*inZ > *outZ)
          {
          *outZ++ = *inZ++;
          *outP++ = *inP++;
          *outP++ = *inP++;
          *outP++ = *inP++;
          }
        else
          {
          ++outZ;
          ++inZ;
          outP += 4;
          inP += 4;
          }
        }
      }
    }
  output->GetPointData()->SetScalars(outPScalars);
  output->GetPointData()->SetFieldData(outPField);
  outPScalars->Delete();
  outPArray->Delete();
  outZField->Delete();
  outZArray->Delete();

}





//----------------------------------------------------------------------------
void vtkCompositeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToStructuredPointsFilter::PrintSelf(os,indent);
}

