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
#include "vtkGraymap.h"
#include "vtkAGraymap.h"
#include "vtkPixmap.h"
#include "vtkAPixmap.h"
#include "vtkImageCache.h"
#include "vtkImageToStructuredPoints.h"
#include "vtkFloatVectors.h"
#include "vtkFloatScalars.h"


//----------------------------------------------------------------------------
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->Extent[idx*2] = -VTK_LARGE_INTEGER;
    this->Extent[idx*2+1] = VTK_LARGE_INTEGER;
    }

  this->Input = NULL;
  this->VectorInput = NULL;

  this->Output = vtkImageData::New();
  this->Output->SetSource(this);
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);

  if (this->Input)
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  if (this->VectorInput)
    {
    os << indent << "VectorInput: (" << this->VectorInput << ")\n";
    }
  else
    {
    os << indent << "VectorInput: (none)\n";
    }
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] 
     << ", " << this->Extent[4] << ", " << this->Extent[5] << ")\n";
}



//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetExtent(int num, int *extent)
{
  int idx, modified = 0;

  if (num > 3)
    {
    vtkWarningMacro(<< "SetExtent: " << num << "is to large.");
    num = 3;
    }
  for (idx = 0; idx < num*2; ++idx)
    {
    if (this->Extent[idx] != extent[idx])
      {
      this->Extent[idx] = extent[idx];
      modified = 1;
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::GetExtent(int num, int *extent)
{
  int idx;

  if (num > 3)
    {
    vtkWarningMacro(<< "GetExtent: Requesting too large");
    num = 3;
    }
  
  for (idx = 0; idx < num*2; ++idx)
    {
    extent[idx] = this->Extent[idx];
    }
  
}

  

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Update()
{
  unsigned long sInputMTime = 0;
  unsigned long vInputMTime = 0;
  
  if ( ! this->Input)
    {
    vtkErrorMacro("Update: Input Not Set!");
    return;
    }
  
  sInputMTime = this->Input->GetPipelineMTime();
  if (this->VectorInput)
    {
    vInputMTime = this->VectorInput->GetPipelineMTime();
    }
  if ((sInputMTime > this->ExecuteTime) || 
      (vInputMTime > this->ExecuteTime) ||
      this->GetMTime() > this->ExecuteTime)
    {
    vtkDebugMacro(<< "Update: Condition satisfied, executeTime = " 
    << this->ExecuteTime
    << ", modifiedTime = " << this->GetMTime() 
    << ", scalar input MTime = " << sInputMTime
    << ", released = " << this->Output->GetDataReleased());
    
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
  if (this->VectorInput && this->VectorInput->ShouldIReleaseData())
    {
    this->VectorInput->ReleaseData();
    }

}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Execute()
{
  int extent[6];
  float origin[3];
  float spacing[3];
  int *uExtent, *wExtent;
  
  vtkImageData *output = (vtkImageData *)(this->Output);
  vtkImageData *data;
  vtkImageData *vData;
  
  // Get the extent with z axis.
  this->GetExtent(3,extent);

  // Fix the size of the cache (for streaming)
  this->Input->UpdateImageInformation();
  this->Input->SetUpdateExtent(extent);
  if (this->VectorInput)
    {
    this->VectorInput->UpdateImageInformation();
    this->VectorInput->SetUpdateExtent(extent);
    vData = this->VectorInput->UpdateAndReturnData();
    if (!vData)
      {
      vtkErrorMacro("Unable to generate data!");
      return;
      }
    }
  
  // we ignore memory limitations since we are going to structured point
  data = this->Input->UpdateAndReturnData();
  
  if (!data)
    {
    vtkErrorMacro("Unable to generate data!");
    return;
    }
  
  // setup the structured points
  uExtent = this->Input->GetUpdateExtent();
  
  output->SetExtent(uExtent);
  output->SetScalarType(data->GetScalarType());
  output->SetNumberOfScalarComponents(data->GetNumberOfScalarComponents());
  output->SetDimensions(uExtent[1] - uExtent[0] + 1,
			uExtent[3] - uExtent[2] + 1,
			uExtent[5] - uExtent[4] + 1);
  output->SetSpacing(data->GetSpacing());

  data->GetSpacing(spacing);
  data->GetOrigin(origin);

  origin[0] += (float)(uExtent[0]) * spacing[0]; 
  origin[1] += (float)(uExtent[2]) * spacing[1]; 
  origin[2] += (float)(uExtent[4]) * spacing[2];

  output->SetOrigin(origin);
  wExtent = data->GetExtent();
  
  // if the data extent matches the update extent then just pass the data
  // otherwise we must reformat and copy the data
  if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] &&
      wExtent[2] == uExtent[2] && wExtent[3] == uExtent[3] &&
      wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
    {
    output->GetPointData()->PassData(data->GetPointData());
    if (this->VectorInput)
      {
      // make sure the vectors are float
      if (vData->GetScalarType() != VTK_FLOAT)
	{
	vtkWarningMacro(<< "vector data must be of type float!!");
	}
      else
	{
	vtkFloatVectors *fv = vtkFloatVectors::New();
	output->GetPointData()->SetVectors(fv);
	fv->SetV(((vtkFloatScalars *)
		  vData->GetPointData()->GetScalars())->GetS());
	fv->Delete();
	}
      }
    }
  else
    {
    unsigned char *inPtr = (unsigned char *) data->GetScalarPointerForExtent(uExtent);
    
    unsigned char *outPtr = (unsigned char *) output->GetScalarPointer();
    
    int idxX, idxY, idxZ;
    int maxX, maxY, maxZ;
    int inIncX, inIncY, inIncZ;
    int rowLength;
    unsigned char *inPtr1;
    
    // Get increments to march through data 
    data->GetIncrements(inIncX, inIncY, inIncZ);

    // find the region to loop over
    rowLength = (uExtent[1] - uExtent[0]+1)*inIncX*data->GetScalarSize();
    maxX = uExtent[1] - uExtent[0]; 
    maxY = uExtent[3] - uExtent[2]; 
    maxZ = uExtent[5] - uExtent[4];
    inIncY *= data->GetScalarSize();
    inIncZ *= data->GetScalarSize();
    
    // Loop through ouput pixels
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

    if (this->VectorInput)
      {
      // make sure the vectors are float
      if (vData->GetScalarType() != VTK_FLOAT)
	{
	vtkWarningMacro(<< "vector data must be of type float!!");
	}
      else
	{
	vtkFloatVectors *fv = vtkFloatVectors::New();
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
}



