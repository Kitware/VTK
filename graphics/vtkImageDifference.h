/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageDifference - select piece (e.g., volume of interest) and/or subsample structured points dataset

// .SECTION Description
// vtkImageDifference is a filter that selects a portion of an input structured points 
// dataset, or subsamples an input dataset. (The selected portion of interested is
// referred toas the Volume Of Interest, or VOI.) The output of this filter is a 
// structured points dataset. The filter treats input data of any topological
// dimension (i.e., point, line, image, or volume) and can generate output data 
// of any topological dimension.
//
// To use this filter set the VOI ivar which are i-j-k min/max indices that specify
// a rectangular region in the data. (Note that these are 0-offset.) You can also
// specify a sampling rate to subsample the data.
//
// Typical applications of this filter are to extract a slice from a volume for 
// image processing, subsampling large volumes to reduce data size, or extracting
// regions of a volume with interesting data.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGeometry vtkExtractGrid

#ifndef __vtkImageDifference_h
#define __vtkImageDifference_h

#include "vtkStructuredPointsToStructuredPointsFilter.hh"

class vtkImageDifference : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  vtkImageDifference();
  char *GetClassName() {return "vtkImageDifference";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Update();

  // Description:
  // Specify the Image to compare the input to.
  vtkSetObjectMacro(Image,vtkStructuredPoints);
  vtkGetObjectMacro(Image,vtkStructuredPoints);

  // Description:
  // Return the error in comparing the two images.
  vtkGetMacro(Error,float);
  
  // Description:
  // Return the thresholded error in comparing the two images.
  vtkGetMacro(ThresholdedError,float);
  
protected:
  void Execute();
  vtkStructuredPoints *Image;
  float Error;
  float ThresholdedError;
};

#endif


