/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageComposite.cxx
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
#include "vtkImageComposite.h"
#include "vtkStructuredPoints.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageComposite* vtkImageComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageComposite");
  if(ret)
    {
    return (vtkImageComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageComposite;
}




//----------------------------------------------------------------------------
vtkImageComposite::vtkImageComposite()
{
}

//----------------------------------------------------------------------------
vtkImageComposite::~vtkImageComposite()
{
}

//----------------------------------------------------------------------------
void vtkImageComposite::AddInput(vtkImageData *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
void vtkImageComposite::RemoveInput(vtkImageData *ds)
{
  this->vtkProcessObject::RemoveInput(ds);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageComposite::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
void vtkImageComposite::Execute()
{
  vtkStructuredPoints *output = this->GetOutput();
  int numPts = output->GetNumberOfPoints();
  vtkStructuredPoints *input;
  int i, j;
  int firstFlag = 1;
  vtkScalars *inPScalars;
  vtkDataArray *inZData;
  vtkFloatArray *outZArray;
  vtkFieldData *outZField;
  float *outZPtr, *inZ, *outZ;
  vtkScalars *outPScalars;
  unsigned char *outPPtr, *inP, *outP;

  // Since this is not an image filter, we need to allocate.
  input = this->GetInput(0);
  numPts = input->GetNumberOfPoints();
  output->SetDimensions(input->GetDimensions());
  output->SetSpacing(input->GetSpacing());

  // allocate the output
  outZArray = vtkFloatArray::New();
  outZArray->Allocate(numPts);
  outZArray->SetNumberOfTuples(numPts);
  outZPtr = outZArray->WritePointer(0, numPts);
  outZField = vtkFieldData::New();
  outZField->SetArray(0, outZArray);
  outZField->SetArrayName(0, "ZBuffer");

  outPScalars = vtkScalars::New();
  outPScalars->SetDataType(VTK_UNSIGNED_CHAR);
  outPScalars->SetNumberOfComponents(3);  
  outPScalars->SetNumberOfScalars(numPts);
  outPPtr = (unsigned char *)(outPScalars->GetVoidPointer(0));

  // composite each input
  for (i = 0; i < this->NumberOfInputs; ++i)
    {
    input = (vtkStructuredPoints*)(this->Inputs[i]);
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
          inPScalars->GetNumberOfComponents() != 3)
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
        if (firstFlag || *inZ < *outZ)
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
          outP += 3;
          inP += 3;
          }
        }
        firstFlag = 0;
      }
    }
  output->GetPointData()->SetScalars(outPScalars);
  output->GetPointData()->SetFieldData(outZField);
  outPScalars->Delete();
  outZField->Delete();
  outZArray->Delete();

}



//----------------------------------------------------------------------------
void vtkImageComposite::ComputeInputUpdateExtents(vtkDataObject *data)
{
  vtkStructuredPoints *output = (vtkStructuredPoints*)data;
  vtkStructuredPoints *input;
  int i;

  for ( i = 0; i < this->NumberOfInputs; ++i)
    {
    input = this->GetInput(i);
    if (input)
      {  
      input->CopyUpdateExtent(output);
      }
    }
}


//----------------------------------------------------------------------------
void vtkImageComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToStructuredPointsFilter::PrintSelf(os,indent);
}

