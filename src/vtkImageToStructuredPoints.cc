/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkImageToStructuredPoints.hh"

// Decription:
// Constructor.
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  this->Input = NULL;
  this->WholeImageFlag = 1;
  this->FlipYFlag = 1;
}





// Description:
// Update input to this filter and the filter itself.
void vtkImageToStructuredPoints::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  if (this->Input->GetPipelineMTime() > this->ExecuteTime ||
      this->GetMTime() > this->ExecuteTime)
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}






// Not connected to the Image pipeline yet. Just uses the Input variable.
void vtkImageToStructuredPoints::Execute()
{
  vtkImageRegion *region;
  vtkGraymap *graymap;
  int dim[3];
  float aspectRatio[3] = {1.0, 1.0, 1.0};
  float origin[3];
  vtkStructuredPoints *output = this->GetOutput();
  
  // error checking
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Execute:Please specify an input!");
    return;
    }

  // get the input region
  if (this->WholeImageFlag)
    this->Input->GetBoundary(this->Offset, this->Size);
  region = this->Input->RequestRegion(this->Offset, this->Size);
  if ( ! region)
    {
    vtkErrorMacro(<< "Execute: Could not get region.");
    return;
    }
  
  // make the output scalars
  graymap = new vtkGraymap;
  
  // Copy the data from input region to output scalars
  this->Generate(region, graymap);
    
  // setup the structured points with the scalars
  region->GetOffset(dim);
  origin[0] = (float)(dim[0]);   origin[1] = (float)(dim[1]);   origin[2] = (float)(dim[2]); 
  region->GetSize(dim);
  
  output->SetDimensions(dim);
  output->SetAspectRatio(aspectRatio);
  output->SetOrigin(origin);
  output->GetPointData()->SetScalars(graymap);

  // delete the temporary structures
  graymap->Delete();
  region->Delete();
}


// Copy the region data to scalar data
void vtkImageToStructuredPoints::Generate(vtkImageRegion *region, 
					  vtkGraymap *scalars)
{
  float max, min;
  int idx0, idx1, idx2;
  int size0, size1, size2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  float *inPtr0, *inPtr1, *inPtr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  
  
  
  // set up variables to march through data
  inPtr2 = region->GetPointer(region->GetOffset());
  region->GetSize(size0, size1, size2);
  region->GetInc(inInc0, inInc1, inInc2);
  vtkDebugMacro(<< "Generate: size = (" 
                << size0 << ", " << size1 << ", " << size2 << ")");
  // output scalar data stuff
  outPtr2 = scalars->WritePtr(0,size0*size1*size2);
  outInc0 = 1;
  outInc1 = size0;
  outInc2 = size0 * size1;
  // move the initial position to the lower left corner of the image.
  if (this->FlipYFlag)
    {
    outPtr2 = outPtr2 + size0*(size1 - 1);
    outInc1 = -size0;
    }
  max = min = *inPtr2;
  for (idx2 = 0; idx2 < size2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = 0; idx1 < size1; ++idx1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;
      for (idx0 = 0; idx0 < size0; ++idx0)
	{
	
	// Compute the Max and Min for debugging purposes
	if (*inPtr0 > max)
	  max = *inPtr0;
	if (*inPtr0 < min)
	  min = *inPtr0;

	// Copy the pixel (dont wrap)
	if (*inPtr0 < 0.0)
	  *outPtr0 = 0;
	else if (*inPtr0 < 256.0)
	  *outPtr0 = (unsigned char)(*inPtr0);
	else
	  *outPtr0 = 255;

	
	inPtr0 += inInc0;
	outPtr0 += outInc0;
	}
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
  
  vtkDebugMacro(<< "Generate: range of input was (" << min << ", " << max << ").");
}

