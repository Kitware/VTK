/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToImage.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include <string.h>
#include "vtkStructuredPointsToImage.h"

//----------------------------------------------------------------------------
vtkStructuredPointsToImage::vtkStructuredPointsToImage()
{
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::UpdateInput()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...");
    return;
    }

  // This will cause an update if the pipeline has been changed.
  this->Input->Update();
  
  // If the input has been released.  Force it to update.
  if ( this->Input->GetDataReleased() )
    {
    this->Input->ForceUpdate();
    }

}


//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::UpdateRegion(vtkImageRegion *region)
{
  // Make sure input is up to date
  this->UpdateInput();
  
  // Make sure image information is update
  this->ComputeImageInformation(region);
  
  // Create the data for the region.
  this->Execute(region);

  // Release the inputs data, if that is what it wants.
  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}


//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::UpdateImageInformation(vtkImageRegion *region)
{
  // Make sure input is up to date
  this->UpdateInput();
  
  // Make sure image information is update
  this->ComputeImageInformation(region);
  
  // Release the inputs data, if that is what it wants.
  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}


//----------------------------------------------------------------------------
unsigned long vtkStructuredPointsToImage::GetPipelineMTime()
{
  unsigned long time, temp;
  
  time = this->GetMTime();

  if ( this->Input )
    {
    // This will cause an update if the pipeline has been changed.
    this->Input->Update();
    temp = this->Input->GetMTime();
    if (temp > time)
      time = temp;
    }
  
  return time;
}




//----------------------------------------------------------------------------
int vtkStructuredPointsToImage::GetDataType()
{
  int type;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "GetDataType: Input not set");
    return VTK_IMAGE_VOID;
    }

  // We have to get the scalars.
  this->UpdateInput();

  type = this->ComputeDataType();
  
  // Release the inputs data, if that is what it wants.
  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();

  return type;
}


//----------------------------------------------------------------------------
int vtkStructuredPointsToImage::ComputeDataType()
{
  char *type;
  vtkScalars *scalars;

  
  scalars = (this->Input->GetPointData())->GetScalars();
  
  type = scalars->GetScalarType();
  if (strcmp(type, "ColorScalar"))
    {
    return VTK_IMAGE_FLOAT;
    }

  type = scalars->GetDataType();
  if (strcmp(type, "float") == 0)
    {
    return VTK_IMAGE_FLOAT;
    }
  if (strcmp(type, "int") == 0)
    {
    return VTK_IMAGE_INT;
    }
  if (strcmp(type, "short") == 0)
    {
    return VTK_IMAGE_SHORT;
    }
  if (strcmp(type, "unsigned short") == 0)
    {
    return VTK_IMAGE_UNSIGNED_SHORT;
    }
  if (strcmp(type, "unsigned char") == 0)
    {
    return VTK_IMAGE_UNSIGNED_CHAR;
    }
  
  vtkErrorMacro(<< "GetDataType: Can not handle type " << type);
  return VTK_IMAGE_VOID;
}






//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::Execute(vtkImageRegion *region)
{
  vtkFloatScalars *newScalars = NULL;
  vtkStructuredPoints *input;
  vtkScalars *scalars;
  char *type;
  int size[3];
  int *bounds;
  vtkImageData *data;

  // Check to see if requested data is contained in the structured points.
  input = this->Input;
  input->GetDimensions(size);
  bounds = region->GetBounds();
  if (bounds[0] < 0 || bounds[2] < 0 || bounds[4] < 0 ||
      bounds[1] >= size[0] || bounds[3] >= size[1] || bounds[5] >= size[2])
    {
    vtkErrorMacro(<< "Execute: Requested region is not in structured points.");
    return;
    }

  // Make sure 4th dimension is empty
  if (bounds[6] != 0 || bounds[7] != 0) 
    {
    vtkErrorMacro(<< "Execute: Structured points are only 3d! ");
    return;
    }

  // Get scalars as float
  scalars = input->GetPointData()->GetScalars();
  type = scalars->GetDataType();
  if (strcmp(scalars->GetScalarType(), "ColorScalar") == 0)
    {
    // Convert to a float scalar
    newScalars = new vtkFloatScalars;
    int num, idx;
    num = scalars->GetNumberOfScalars();
    for (idx = 0; idx < num; ++idx)
      {
      newScalars->InsertNextScalar(scalars->GetScalar(idx));
      }
    // Create a new data object for the scalars
    data = new vtkImageData;
    data->SetBounds(0, size[0]-1, 0, size[1]-1, 0, size[2]-1, 0, 0, 0, 0);
    data->SetScalars(newScalars);
    newScalars->UnRegister(this);
    }
  else if ((strcmp(type,"float") == 0) || (strcmp(type,"short") == 0) || 
	   (strcmp(type,"int") == 0) || (strcmp(type,"unsigned short") == 0) ||
	   (strcmp(type, "unsigned char") == 0))
    {
    // Create a new data object for the scalars
    data = new vtkImageData;
    data->SetBounds(0, size[0]-1, 0, size[1]-1, 0, size[2]-1, 0, 0, 0, 0);
    data->SetScalars(scalars);
    }
  else
    {
    vtkErrorMacro(<< "Execute: Can not handle data type " << type);
    return;
    }

  region->SetData(data);
  data->UnRegister(this);
}


//----------------------------------------------------------------------------
void
vtkStructuredPointsToImage::ComputeImageInformation(vtkImageRegion *region)
{
  vtkStructuredPoints *input;
  int size[3];
  float aspectRatio[3];

  input = this->Input;
  input->GetDimensions(size);
  //input->GetOrigin(origin);
  input->GetAspectRatio(aspectRatio);

  region->SetImageBounds3d(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
  region->SetAspectRatio3d(aspectRatio);
  if (region->GetDataType() == VTK_IMAGE_VOID)
    {
    region->SetDataType(this->ComputeDataType());
    }
}











