/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToImage.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include "vtkColorScalars.h"
#include "vtkImageRegion.h"

//----------------------------------------------------------------------------
vtkStructuredPointsToImage::vtkStructuredPointsToImage()
{
  this->Input = NULL;
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
int vtkStructuredPointsToImage::GetScalarType()
{
  int type;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "GetDataType: Input not set");
    return VTK_VOID;
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
  if (strcmp(type, "ColorScalar") == 0 &&
      scalars->GetNumberOfValuesPerScalar () != 1)
    {
    return VTK_FLOAT;
    }

  type = scalars->GetDataType();
  if (strcmp(type, "float") == 0)
    {
    return VTK_FLOAT;
    }
  if (strcmp(type, "int") == 0)
    {
    return VTK_INT;
    }
  if (strcmp(type, "short") == 0)
    {
    return VTK_SHORT;
    }
  if (strcmp(type, "unsigned short") == 0)
    {
    return VTK_UNSIGNED_SHORT;
    }
  if (strcmp(type, "unsigned char") == 0)
    {
    return VTK_UNSIGNED_CHAR;
    }
  
  vtkErrorMacro(<< "GetDataType: Can not handle type " << type);
  return VTK_VOID;
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::Execute(vtkImageRegion *region)
{
  vtkFloatScalars *newScalars = NULL;
  vtkStructuredPoints *input;
  vtkPointData *pointData;
  vtkScalars *scalars;
  char *type;
  int size[3];
  vtkImageData *data;

  // Check to see if requested data is contained in the structured points.
  input = this->Input;
  input->GetDimensions(size);

  // Get scalars as float
  scalars = input->GetPointData()->GetScalars();
  type = scalars->GetDataType();
  if (strcmp(scalars->GetScalarType(), "ColorScalar") == 0 &&
      scalars->GetNumberOfValuesPerScalar () != 1)
    {
    int bpp;
    unsigned char *buffer;
    int axes[5];
    
    // Convert to a float scalar
    newScalars = new vtkFloatScalars;
    int num, idx;
    num = scalars->GetNumberOfScalars();
    bpp = ((vtkColorScalars *)scalars)->GetNumberOfValuesPerScalar();
    num = num*bpp;
    buffer = ((vtkColorScalars *)scalars)->GetPtr(0);
    for (idx = 0; idx < num; ++idx)
      {
      newScalars->InsertNextScalar(buffer[idx]);
      }
    // Create a new data object for the scalars
    data = new vtkImageData;
    // Setting data Axes has not "matured yet" lets see if it works first.
    axes[0] = 4; axes[1] = 0; axes[2] = 1; axes[3] = 2; axes[4] = 3;
    data->SetAxes(axes);
    data->SetExtent(0, bpp-1, 0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
    pointData = data->GetPointData();
    pointData->SetScalars(newScalars);
    newScalars->Delete();  // registered by point data.
    }
  else if ((strcmp(type,"float") == 0) || (strcmp(type,"short") == 0) || 
	   (strcmp(type,"int") == 0) || (strcmp(type,"unsigned short") == 0) ||
	   (strcmp(type, "unsigned char") == 0))
    {
    // Create a new data object for the scalars
    data = new vtkImageData;
    data->SetExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1, 0, 0, 0, 0);
    pointData = data->GetPointData();
    pointData->SetScalars(scalars);
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
  float origin[3];
  vtkScalars *scalars;
  
  input = this->Input;
  input->GetDimensions(size);
  input->GetAspectRatio(aspectRatio);
  input->GetOrigin(origin);
  
  region->SetAspectRatio(3, aspectRatio);
  region->SetOrigin(3, origin);
  if (region->GetScalarType() == VTK_VOID)
    {
    region->SetScalarType(this->ComputeDataType());
    }
  
  region->SetImageExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
  
  // Get scalars to find out if we need to add components
  scalars = input->GetPointData()->GetScalars();
  if (strcmp(scalars->GetScalarType(), "ColorScalar") == 0 &&
      scalars->GetNumberOfValuesPerScalar () != 1)
    {
    int bpp;
    bpp = ((vtkColorScalars *)scalars)->GetNumberOfValuesPerScalar();
    region->SetAxisImageExtent(VTK_IMAGE_COMPONENT_AXIS, 0, bpp-1);
    }
}











