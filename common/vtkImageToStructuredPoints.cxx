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
}



//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  if (this->Input)
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
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
  
  if ( ! this->Input)
    {
    vtkErrorMacro("Update: Input Not Set!");
    return;
    }
  
  sInputMTime = this->Input->GetPipelineMTime();
  if ((sInputMTime > this->ExecuteTime) || 
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

}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::Execute()
{
  int extent[6];
  vtkScalars *scalars = NULL;
  float spacing[3];
  float origin[3];
  int dim[3];
  vtkStructuredPoints *output = (vtkStructuredPoints *)(this->Output);
  vtkImageData *data;
  
  // Get the extent with z axis.
  this->GetExtent(3,extent);

  // Fix the size of the cache (for streaming)
  this->Input->UpdateImageInformation();
  this->Input->SetUpdateExtent(extent);

  // we ignore memory limitations since we are going to structured point
  data = this->Input->UpdateAndReturnData();
  
  if (!data)
    {
    vtkErrorMacro("Unable to generate data!");
    return;
    }
  
  // setup the structured points
  output->SetDimensions(data->GetDimensions());
  output->SetSpacing(data->GetSpacing());
  output->SetOrigin(data->GetOrigin());

  output->GetPointData()->PassData(data->GetPointData());
}



