/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkImageToStructuredPoints.h"
#include "vtkScalars.h"
#include "vtkStructuredPoints.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageToStructuredPoints* vtkImageToStructuredPoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageToStructuredPoints");
  if(ret)
    {
    return (vtkImageToStructuredPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageToStructuredPoints;
}




//----------------------------------------------------------------------------
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  this->Translate[0] = this->Translate[1] = this->Translate[2] = 0;
  this->SetNthOutput(0,vtkStructuredPoints::New());
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
vtkStructuredPoints *vtkImageToStructuredPoints::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToStructuredPoints::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetVectorInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToStructuredPoints::GetVectorInput()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[1]);
}




//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Execute()
{
  int uExtent[6];
  int *wExtent;

  int idxX, idxY, idxZ, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ, rowLength;
  unsigned char *inPtr1, *inPtr, *outPtr;
  
  vtkStructuredPoints *output = this->GetOutput();
  vtkImageData *data = this->GetInput();
  vtkImageData *vData = this->GetVectorInput();
  
  if (!data && !vData)
    {
    vtkErrorMacro("Unable to generate data!");
    return;
    }

  output->GetUpdateExtent(uExtent);
  output->SetExtent(uExtent);

  uExtent[0] += this->Translate[0];
  uExtent[1] += this->Translate[0];
  uExtent[2] += this->Translate[1];
  uExtent[3] += this->Translate[1];
  uExtent[4] += this->Translate[2];
  uExtent[5] += this->Translate[2];
  
  // if the data extent matches the update extent then just pass the data
  // otherwise we must reformat and copy the data
  if (data)
    {
    wExtent = data->GetExtent();
    if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] &&
	wExtent[2] == uExtent[2] && wExtent[3] == uExtent[3] &&
	wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
      {
      output->GetPointData()->PassData(data->GetPointData());
      }
    else
      {
      inPtr = (unsigned char *) data->GetScalarPointerForExtent(uExtent);
      outPtr = (unsigned char *) output->GetScalarPointer();
      
      // Get increments to march through data 
      data->GetIncrements(inIncX, inIncY, inIncZ);
      
      // find the region to loop over
      rowLength = (uExtent[1] - uExtent[0]+1)*inIncX*data->GetScalarSize();
      maxX = uExtent[1] - uExtent[0]; 
      maxY = uExtent[3] - uExtent[2]; 
      maxZ = uExtent[5] - uExtent[4];
      inIncY *= data->GetScalarSize();
      inIncZ *= data->GetScalarSize();
      
      // Loop through output pixels
      for (idxZ = 0; idxZ <= maxZ; idxZ++)
	{
	inPtr1 = inPtr + idxZ*inIncZ;
	for (idxY = 0; idxY <= maxY; idxY++)
	  {
	  memcpy(outPtr,inPtr1,rowLength);
	  inPtr1 += inIncY;
	  outPtr += rowLength;
	  }
	}
      }
    }
    
  if (vData)
    {
    // if the data extent matches the update extent then just pass the data
    // otherwise we must reformat and copy the data
    wExtent = vData->GetExtent();
    if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] &&
	wExtent[2] == uExtent[2] && wExtent[3] == uExtent[3] &&
	wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
      {
      vtkVectors *fv = vtkVectors::New(vData->GetScalarType());
      output->GetPointData()->SetVectors(fv);
      fv->SetData(vData->GetPointData()->GetScalars()->GetData());
      fv->Delete();
      }
    else
      {
      vtkVectors *fv = vtkVectors::New(vData->GetScalarType());
      output->GetPointData()->SetVectors(fv);
      float *inPtr2 = (float *)(vData->GetScalarPointerForExtent(uExtent));
      
      fv->SetNumberOfVectors((maxZ+1)*(maxY+1)*(maxX+1));
      vData->GetContinuousIncrements(uExtent, inIncX, inIncY, inIncZ);
      int numComp = vData->GetNumberOfScalarComponents();
      int idx = 0;
      
      // Loop through ouput pixels
      for (idxZ = 0; idxZ <= maxZ; idxZ++)
	{
	for (idxY = 0; idxY <= maxY; idxY++)
	  {
	  for (idxX = 0; idxX <= maxX; idxX++)
	    {
	    fv->SetVector(idx,inPtr2);
	    inPtr2 += numComp;
	    idx++;
	    }
	  inPtr2 += inIncY;
	  }
	inPtr2 += inIncZ;
	}
      fv->Delete();
      }
    }
}

//----------------------------------------------------------------------------
// Copy WholeExtent, Spacing and Origin.
void vtkImageToStructuredPoints::ExecuteInformation()
{
  vtkImageData *input = this->GetInput();
  vtkImageData *vInput = this->GetVectorInput();
  vtkStructuredPoints *output = this->GetOutput();
  int whole[6], *tmp;
  float *spacing, origin[3];
  
  if (output == NULL)
    {
    return;
    }
  
  if (input)
    {
    output->SetScalarType(input->GetScalarType());
    output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());
    input->GetWholeExtent(whole);    
    spacing = input->GetSpacing();
    input->GetOrigin(origin);
    }
  else
    {
    whole[0] = whole[2] = whole[4] = -VTK_LARGE_INTEGER;
    whole[1] = whole[3] = whole[5] = VTK_LARGE_INTEGER;
    spacing = vInput->GetSpacing();
    vInput->GetOrigin(origin);
    }
  
  // intersections for whole extent
  if (vInput)
    {
    tmp = vInput->GetWholeExtent();
    if (tmp[0] > whole[0]) {whole[0] = tmp[0];}
    if (tmp[2] > whole[2]) {whole[2] = tmp[2];}
    if (tmp[4] > whole[4]) {whole[4] = tmp[4];}
    if (tmp[1] < whole[1]) {whole[1] = tmp[1];}
    if (tmp[3] < whole[1]) {whole[3] = tmp[3];}
    if (tmp[5] < whole[1]) {whole[5] = tmp[5];}
    }
    
  // slide min extent to 0,0,0 (I Hate this !!!!)
  this->Translate[0] = whole[0];
  this->Translate[1] = whole[2];
  this->Translate[2] = whole[4];
  
  origin[0] += spacing[0] * whole[0];
  origin[1] += spacing[1] * whole[2];
  origin[2] += spacing[2] * whole[4];
  whole[1] -= whole[0];
  whole[3] -= whole[2];
  whole[5] -= whole[4];
  whole[0] = whole[2] = whole[4] = 0;
  
  output->SetWholeExtent(whole);
  // Now should Origin and Spacing really be part of information?
  // How about xyx arrays in RectilinearGrid of Points in StructuredGrid?
  output->SetOrigin(origin);
  output->SetSpacing(spacing);
}

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::ComputeInputUpdateExtents(vtkDataObject *data)
{
  vtkStructuredPoints *output = (vtkStructuredPoints*)data;
  int ext[6];
  vtkImageData *input;
  
  output->GetUpdateExtent(ext);
  ext[0] += this->Translate[0];
  ext[1] += this->Translate[0];
  ext[2] += this->Translate[1];
  ext[3] += this->Translate[1];
  ext[4] += this->Translate[2];
  ext[5] += this->Translate[2];
  
  input = this->GetInput();
  if (input)
    {
    input->SetUpdateExtent(ext);
    }

  input = this->GetVectorInput();
  if (input)
    {
    input->SetUpdateExtent(ext);
    }
}




