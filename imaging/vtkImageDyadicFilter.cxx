/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDyadicFilter.cxx
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
#include "vtkImageDyadicFilter.hh"
#include "vtkImageCache.hh"


//----------------------------------------------------------------------------
vtkImageDyadicFilter::vtkImageDyadicFilter()
{
  this->Input1 = NULL;
  this->Input2 = NULL;
  this->UseExecuteMethodOn();
}

//----------------------------------------------------------------------------
void vtkImageDyadicFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCachedSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  This works, but is not elegant.
// An Executor would probably be the best solution if this is a problem.
// (The pipeline could vote before it starts processing, but one object
// has to initiate the voting.)
unsigned long int vtkImageDyadicFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageCachedSource::GetPipelineMTime();
  if ( ! this->Input1 )
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input1 not set.");
    time2 = time1;
    }
  else
    {
    time2 = this->Input1->GetPipelineMTime();
    }
    
  // Keep the larger of the two 
  if (time2 > time1)
    time1 = time2;

  if ( ! this->Input2 )
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input2 not set.");
    return time1;
    }
  time2 = this->Input2->GetPipelineMTime();

  // Keep the larger of the two 
  if (time2 > time1)
    time1 = time2;

  return time1;
}


//----------------------------------------------------------------------------
// Description:
// Set the Input1 of this filter. If a DataType has not been set,
// then the DataType of the input is used.
void vtkImageDyadicFilter::SetInput1(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput1: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Input1)
    {
    return;
    }
  
  this->Input1 = input;
  this->Modified();

  // Should we use the data type from the input?
  this->CheckCache();      // make sure a cache exists
  if (this->Output->GetDataType() == VTK_IMAGE_VOID)
    {
    this->Output->SetDataType(input->GetDataType());
    if (this->Output->GetDataType() == VTK_IMAGE_VOID)
      {
      vtkErrorMacro(<< "SetInput1: Cannot determine DataType of input.");
      }
    }
}



//----------------------------------------------------------------------------
// Description:
// Set the Input2 of this filter. If a DataType has not been set,
// then the DataType of the input is used.
void vtkImageDyadicFilter::SetInput2(vtkImageSource *input)
{
  vtkDebugMacro(<< "SetInput2: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Input2)
    {
    return;
    }
  
  this->Input2 = input;
  this->Modified();

  // Should we use the data type from the input?
  this->CheckCache();      // make sure a cache exists
  if (this->Output->GetDataType() == VTK_IMAGE_VOID)
    {
    this->Output->SetDataType(input->GetDataType());
    if (this->Output->GetDataType() == VTK_IMAGE_VOID)
      {
      vtkErrorMacro(<< "SetInput2: Cannot determine DataType of input.");
      }
    }
}



//----------------------------------------------------------------------------
// Description:
// This method gets the input regions from the inputs, uses the cache to
// allocate the output region, and calls the execute method to fill the 
// out region.  If either of the input requests fail, the whole update
// fails.  If dynamic splitting turns out to be important, it
// could be imp[lemented for this class, but would be more complex
// that the vtkImageFilter class.
void vtkImageDyadicFilter::UpdateRegion(vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion1, *inRegion2;
  
  if (this->Debug)
    {
    int *b = outRegion->GetBounds4d();
    cerr << "Debug: In " __FILE__ << ", line " << __LINE__ << "\n" 
	 << this->GetClassName() << " (" << this << "): "
	 << "GenerateRegion: " << b[0] << "," << b[1] << ", "
	 << b[2] << "," << b[3] << ", " 
	 << b[4] << "," << b[5] << ", "
	 << b[6] << "," << b[7] << "\n\n";
    }
  
  // To avoid doing this for each execute1d ...
  this->UpdateImageInformation(outRegion);  // probably already has ImageBounds
  
  // If outBBox is empty return imediately.
  if (outRegion->IsEmpty())
    {
    return;
    }
    
  // Make sure the Input has been set.
  if ( ! this->Input1 || ! this->Input2)
    {
    vtkErrorMacro(<< "An input is not set.");
    return;
    }
  
  // Determine whether to use the execute methods or the generate methods.
  if ( ! this->UseExecuteMethod)
    {
    this->UpdateRegion5d(outRegion);
    return;
    }
  
  // Make the input regions that will be used to generate the output region
  inRegion1 = new vtkImageRegion;
  inRegion2 = new vtkImageRegion;
  
  // Fill in image information
  this->Input1->UpdateImageInformation(inRegion1);
  this->Input2->UpdateImageInformation(inRegion2);
  
  // Translate to local coordinate system
  inRegion1->SetAxes(this->Axes);
  inRegion2->SetAxes(this->Axes);
  
  // Compute the required input region bounds.
  // Copy to fill in bounds of extra dimensions.
  inRegion1->SetBounds(outRegion->GetBounds());
  inRegion2->SetBounds(outRegion->GetBounds());
  this->ComputeRequiredInputRegionBounds(outRegion, inRegion1, inRegion2);

  // Use the input to fill the data of the region.
  this->Input1->UpdateRegion(inRegion1);
  this->Input2->UpdateRegion(inRegion2);
  
  // Make sure the region was not too large 
  if ( ! inRegion1->IsAllocated() || ! inRegion2->IsAllocated())
    {
    // Call alternative slower generate that breaks the task into pieces 
    inRegion1->Delete();
    inRegion2->Delete();
    outRegion->SetSplitFactor(2);
    return;
    }
  
  // Get the output region from the cache (guaranteed to succeed).
  this->Output->AllocateRegion(outRegion);

  // fill the output region 
  this->Execute5d(inRegion1, inRegion2, outRegion);

  // free the input regions
  inRegion1->Delete();
  inRegion2->Delete();
}


//----------------------------------------------------------------------------
// Description:
// This method gets the boundary of the inputs then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageDyadicFilter::UpdateImageInformation(vtkImageRegion *outRegion)
{
  vtkImageRegion *inRegion1, *inRegion2;
  
  // Make sure the Input has been set.
  if ( ! this->Input1 || ! this->Input2)
    {
    vtkErrorMacro(<< "UpdateImageInformation: An input is not set.");
    return;
    }

  inRegion1 = new vtkImageRegion;
  inRegion2 = new vtkImageRegion;
  
  this->Input1->UpdateImageInformation(inRegion1);
  this->Input2->UpdateImageInformation(inRegion2);
  this->ComputeOutputImageInformation(inRegion1, inRegion2, outRegion);

  inRegion1->Delete();
  inRegion2->Delete();
}



//----------------------------------------------------------------------------
// Description:
// This method is passed an inRegion that holds the image information
// (image bounds ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void 
vtkImageDyadicFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion1,
						    vtkImageRegion *inRegion2,
						    vtkImageRegion *outRegion)

{
  // Default: Image information does not change (do nothing).
  // Avoid warnings
  inRegion1 = inRegion1;
  inRegion2 = inRegion2;
  outRegion = outRegion;
}



//----------------------------------------------------------------------------
// Description:
// This method computes the bounds of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// bounds of the output region.  After this method finishes, "region" should 
// have the bounds of the required input region.  The default method assumes
// the required input bounds are the same as the output bounds.
// Note: The splitting methods call this method with outRegion = inRegion.
void vtkImageDyadicFilter::ComputeRequiredInputRegionBounds(
					       vtkImageRegion *outRegion,
					       vtkImageRegion *inRegion1,
					       vtkImageRegion *inRegion2)
{
  inRegion1->SetBounds(outRegion->GetBounds());
  inRegion2->SetBounds(outRegion->GetBounds());
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a 5d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute5d
// method breaks the 5d regions into 4d "images".  The regions have been
// converted to this filters coordinates before this method is called.
void vtkImageDyadicFilter::Execute5d(vtkImageRegion *inRegion1,
				     vtkImageRegion *inRegion2,
				     vtkImageRegion *outRegion)
{
  int coordinate4, min4, max4;
  int *inBounds1, *inBounds2, *outBounds;
  
  // Get the bounds of the forth dimension to be eliminated.
  inBounds1 = inRegion1->GetBounds5d();
  inBounds2 = inRegion2->GetBounds5d();
  outBounds = outRegion->GetBounds5d();

  // This method assumes that the fifth axis of in and out have same bounds.
  min4 = outBounds[8];
  max4 = outBounds[9];
  if (min4 != inBounds1[8] || max4 != inBounds1[9] ||
      min4 != inBounds2[8] || max4 != inBounds2[9]) 
    {
    vtkErrorMacro(<< "Execute5d: Cannot break 5d images into 4d images.");
    return;
    }
  
  // loop over 4d volumes
  for (coordinate4 = min4; coordinate4 <= max4; ++coordinate4)
    {
    // set up the 4d regions.
    inRegion1->SetDefaultCoordinate4(coordinate4);
    inRegion1->SetDefaultCoordinate4(coordinate4);
    outRegion->SetDefaultCoordinate4(coordinate4);
    this->Execute4d(inRegion1, inRegion2, outRegion);
    }
}
  
  

//----------------------------------------------------------------------------
// Description:
// This method is passed a 4d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute4d
// method breaks the 4d regions into 3d volumes.  The regions have been
// converted to this filters coordinates before this method is called.
void vtkImageDyadicFilter::Execute4d(vtkImageRegion *inRegion1, 
				     vtkImageRegion *inRegion2, 
				     vtkImageRegion *outRegion)
{
  int coordinate3, min3, max3;
  int *inBounds1, *inBounds2, *outBounds;
  
  // Get the bounds of the third dimension to be eliminated.
  inBounds1 = inRegion1->GetBounds4d();
  inBounds2 = inRegion2->GetBounds4d();
  outBounds = outRegion->GetBounds4d();

  // This method assumes that the third axis of in and out have same bounds.
  min3 = outBounds[6];
  max3 = outBounds[7];
  if (min3 != inBounds1[6] || max3 != inBounds1[7] ||
      min3 != inBounds2[6] || max3 != inBounds2[7])
    {
    vtkErrorMacro(<< "Execute4d: Cannot break 4d images into volumes.");
    return;
    }
  
  // loop over 3d volumes
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    // set up the 3d regions.
    inRegion1->SetDefaultCoordinate3(coordinate3);
    inRegion1->SetDefaultCoordinate3(coordinate3);
    outRegion->SetDefaultCoordinate3(coordinate3);
    this->Execute3d(inRegion1, inRegion2, outRegion);
    }
}
  
  

//----------------------------------------------------------------------------
// Description:
// This method is passed a 3d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute3d
// method breaks the volumes into images.  The regions have been converted to
// this filters coordinates before this method is called.
void vtkImageDyadicFilter::Execute3d(vtkImageRegion *inRegion1, 
				     vtkImageRegion *inRegion2, 
				     vtkImageRegion *outRegion)
{
  int coordinate2, min2, max2;
  int *inBounds1, *inBounds2, *outBounds;
  
  // Get the bounds of the third dimension to be eliminated.
  inBounds1 = inRegion1->GetBounds3d();
  inBounds2 = inRegion2->GetBounds3d();
  outBounds = outRegion->GetBounds3d();

  // This method assumes that the third axis of in and out have same bounds.
  min2 = outBounds[4];
  max2 = outBounds[5];
  if (min2 != inBounds1[4] || max2 != inBounds1[5] ||
      min2 != inBounds2[4] || max2 != inBounds2[5]) 
    {
    vtkErrorMacro(<< "Execute3d: Cannot break volumes into images.");
    return;
    }
  
  // loop over 2d images
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    // set up the 2d regions.
    inRegion1->SetDefaultCoordinate2(coordinate2);
    inRegion2->SetDefaultCoordinate2(coordinate2);
    outRegion->SetDefaultCoordinate2(coordinate2);
    this->Execute2d(inRegion1, inRegion2, outRegion);
    }
}
  
  

//----------------------------------------------------------------------------
// Description:
// This method is passed a 2d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute2d
// method breaks the images into lines.  The regions have been converted to
// this filters coordinates before this method is called.
void vtkImageDyadicFilter::Execute2d(vtkImageRegion *inRegion1, 
				     vtkImageRegion *inRegion2, 
				     vtkImageRegion *outRegion)
{
  int coordinate1, min1, max1;
  int *inBounds1, *inBounds2, *outBounds;
  
  // Get the bounds of the third dimension to be eliminated.
  inBounds1 = inRegion1->GetBounds2d();
  inBounds2 = inRegion2->GetBounds2d();
  outBounds = outRegion->GetBounds2d();

  // This method assumes that the second axis of in and out have same bounds.
  min1 = outBounds[2];
  max1 = outBounds[3];
  if (min1 != inBounds1[2] || max1 != inBounds1[3] ||
      min1 != inBounds2[2] || max1 != inBounds2[3]) 
    {
    vtkErrorMacro(<< "Execute2d: Cannot break images into lines.");
    return;
    }
  
  // loop over 1d lines
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    // set up the 1d regions.
    inRegion1->SetDefaultCoordinate1(coordinate1);
    inRegion2->SetDefaultCoordinate1(coordinate1);
    outRegion->SetDefaultCoordinate1(coordinate1);
    this->Execute1d(inRegion1, inRegion2, outRegion);
    }
}
  
  
//----------------------------------------------------------------------------
// Description:
// This method is passed a 2d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute2d
// method breaks the images into lines.  The regions have been converted to
// this filters coordinates before this method is called.
void vtkImageDyadicFilter::Execute1d(vtkImageRegion *inRegion1, 
				     vtkImageRegion *inRegion2, 
				     vtkImageRegion *outRegion)
{
  inRegion1 = inRegion1;
  inRegion2 = inRegion2;
  outRegion = outRegion;
  
  vtkErrorMacro(<< "Execute1d: Filter does not specify an execute method.");
}



//============================================================================
// Stuff for filters that do not use the execute methods..
//============================================================================

//----------------------------------------------------------------------------
vtkImageRegion *vtkImageDyadicFilter::GetInput1Region(int *bounds, int dim)
{
  int idx;
  int *imageBounds;
  vtkImageRegion *region;
  
  if ( ! this->Input1)
    {
    vtkErrorMacro(<< "Input1 is not set.");
    return NULL;
    }

  region = new vtkImageRegion;

  // This step is just error checking, and may be wastefull.  The Image
  // Information is automatically computed when UpdateRegion is called.
  this->Input1->UpdateImageInformation(region);
  region->SetAxes(this->GetAxes());
  imageBounds = region->GetImageBounds();
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (imageBounds[idx*2] > 0 || imageBounds[idx*2 + 1] < 0)
      {
      vtkErrorMacro(<< "GetInputRegion1: dim = " << dim 
                    << ", unspecified dimensions do not include 0.");
      region->Delete();
      return NULL;
      }
    }
  
  // Note: This automatical sets the unspecified dimension bounds to [0,0]
  region->SetBounds(bounds, dim);
  this->Input1->UpdateRegion(region);
  
  return region;
}



//----------------------------------------------------------------------------
vtkImageRegion *vtkImageDyadicFilter::GetInput2Region(int *bounds, int dim)
{
  int idx;
  int *imageBounds;
  vtkImageRegion *region;
  
  if ( ! this->Input2)
    {
    vtkErrorMacro(<< "Input2 is not set.");
    return NULL;
    }

  region = new vtkImageRegion;

  // This step is just error checking, and may be wastefull.  The Image
  // Information is automatically computed when UpdateRegion is called.
  this->Input2->UpdateImageInformation(region);
  region->SetAxes(this->GetAxes());
  imageBounds = region->GetImageBounds();
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (imageBounds[idx*2] > 0 || imageBounds[idx*2 + 1] < 0)
      {
      vtkErrorMacro(<< "GetInputRegion2: dim = " << dim 
                    << ", unspecified dimensions do not include 0.");
      region->Delete();
      return NULL;
      }
    }
  
  // Note: This automatical sets the unspecified dimension bounds to [0,0]
  region->SetBounds(bounds, dim);
  this->Input2->UpdateRegion(region);
  
  return region;
}














