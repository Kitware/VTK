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
#include "vtkImageSimpleCache.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"

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
void vtkStructuredPointsToImage::InternalUpdate(vtkImageData *data)
{
  // Make sure input is up to date
  this->UpdateInput();
  
  // Create the data for the region.
  this->Execute(data);

  // Release the inputs data, if that is what it wants.
  if ( this->Input->ShouldIReleaseData() )
    {
    this->Input->ReleaseData();
    }
}


//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::UpdateImageInformation()
{
  int size[3];
  vtkScalars *scalars;

  if ( ! this->Input)
    {
    vtkErrorMacro("Input not set.");
    return;
    }
  
  // Make sure input is up to date
  this->UpdateInput();
  this->Input->GetDimensions(size);

  this->CheckCache();
  this->Output->SetSpacing(this->Input->GetSpacing());
  this->Output->SetOrigin(this->Input->GetOrigin());
  // If the Scalar type has not been set previously, compute it.
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(this->ComputeDataType());
    }
  
  // Get scalars to find out if we need to add components
  scalars = this->Input->GetPointData()->GetScalars();
  this->Output->SetWholeExtent(0, size[0]-1, 
			       0, size[1]-1, 0, size[2]-1);
  this->Output->SetNumberOfScalarComponents(scalars->GetNumberOfComponents());
  
  // Releasing data here would be inefficient because cache calls
  // UpdateImageInformation and then Update.
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
      {
      time = temp;
      }
    }
  
  return time;
}




//----------------------------------------------------------------------------
int vtkStructuredPointsToImage::ComputeDataType()
{
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
    
  return scalars->GetDataType();
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToImage::Execute(vtkImageData *data)
{
  vtkScalars *scalars;
  
  if ( ! this->Input)
    {
    vtkErrorMacro("Input not set.");
    return;
    }
  
  scalars = this->Input->GetPointData()->GetScalars();
  // We do not handle bit arrays (yet?)
  if ( scalars->GetDataType() == VTK_BIT )
    {
    vtkErrorMacro("This class does not handle bit scalars.");
    return;
    }
  
  data->SetExtent(this->Output->GetWholeExtent());

  // just pass the data 
  data->GetPointData()->PassData(this->Input->GetPointData());
}








