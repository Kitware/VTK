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
vtkStructuredPointsToImage::~vtkStructuredPointsToImage()
{
  //if (this->Input)
  //  {
  //  this->Input->UnRegister(this);
  //  }
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ")\n";
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
  if ( ! scalars)
    {
    this->Input->Update();
    scalars = (this->Input->GetPointData())->GetScalars();
    }
  if ( ! scalars)
    {
    vtkErrorMacro("ComputeDataType: Could not get scalars from input");
    return VTK_VOID;
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
  char *type;
  int *size;
  int dataExtent[8];
  vtkImageData *data;
  int dataAxes[5] = {VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS,
		     VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS, VTK_IMAGE_TIME_AXIS};
  vtkScalars *scalars;
  
  if ( ! this->Input)
    {
    vtkErrorMacro("Input not set.");
    return;
    }
  
  scalars = this->Input->GetPointData()->GetScalars();
  // We do not handle bit arrays (yet?)
  if (strcmp(scalars->GetClassName(), "vtkBitScalars") == 0)
    {
    vtkErrorMacro("This class does not handle bit scalars.");
    return;
    }
  
  // Determine the extent of the data
  size = this->Input->GetDimensions();
  dataExtent[0] = dataExtent[2] = dataExtent[4] = dataExtent[6] = 0;
  dataExtent[3] = size[0] - 1;
  dataExtent[5] = size[1] - 1;
  dataExtent[7] = size[2] - 1;
  if (strcmp(scalars->GetScalarType(), "ColorScalar") == 0)
    {
    dataExtent[1]=((vtkColorScalars *)scalars)->GetNumberOfValuesPerScalar()-1;
    }
  else
    {
    dataExtent[1] = 0;
    }

  // Convert the scalars array into vtkImageData
  data = vtkImageData::New();
  data->SetAxes(dataAxes);
  data->SetExtent(4, dataExtent);
  type = scalars->GetDataType();
  if (strcmp(type, "unsigned char") == 0)
    {
    vtkUnsignedCharScalars *dataScalars = vtkUnsignedCharScalars::New();
    if (strcmp(scalars->GetScalarType(), "ColorScalar") == 0)
      {
      dataScalars->SetS(((vtkColorScalars *)scalars)->GetS());
      }
    else
      {
      dataScalars->SetS(((vtkUnsignedCharScalars *)scalars)->GetS());
      }
    data->SetScalarType(VTK_UNSIGNED_CHAR);
    data->SetScalars(dataScalars);
    dataScalars->UnRegister(this);
    }
  else if (strcmp(type, "unsigned short") == 0)
    {
    // Since we know the scalars are not color scalars, just copy scalars.
    data->SetScalarType(VTK_UNSIGNED_SHORT);
    data->SetScalars(scalars);
    }
  else if (strcmp(type, "short") == 0)
    {
    // Since we know the scalars are not color scalars, just copy scalars.
    data->SetScalarType(VTK_SHORT);
    data->SetScalars(scalars);
    }
  else if (strcmp(type, "float") == 0)
    {
    // Since we know the scalars are not color scalars, just copy scalars.
    data->SetScalarType(VTK_FLOAT);
    data->SetScalars(scalars);
    }
  else if (strcmp(type, "int") == 0)
    {
    // Since we know the scalars are not color scalars, just copy scalars.
    data->SetScalarType(VTK_INT);
    data->SetScalars(scalars);
    }
  
  region->SetData(data);
  // Get rid of our ref count.
  data->Delete();
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
  
  if ( ! this->Input)
    {
    vtkErrorMacro("Input not set.");
    return;
    }
  
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











