/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkSurfels.h
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
// .NAME vtkLinkSurfels - links edgels together to form digital curves.
// .SECTION Description
// vtkLinkSurfels links edgels into surfaces which are then stored 
// as triangles. The algorithm works one pixel at a time only looking at
// its immediate neighbors. There is a GradientThreshold that can be set 
// that eliminates any pixels with a smaller gradient value. This can
// be used as the lower threshold of a two value edgel thresholding. 
//
// For the remaining edgels, links are first tried for the four
// connected neighbors.  A succesful neighbor will satisfy three
// tests. First both edgels must be above the gradient
// threshold. Second, the difference between the orientation between
// the two edgels (Alpha) and each edgels orientation (Phi) must be
// less than LinkThreshold. Third, the difference between the two
// edgels Phi values must be less than PhiThreshold.
// The most successful link is selected. The meaure is simply the 
// sum of the three angle differences (actually stored as the sum of
// the cosines). If none of the four connect neighbors succeds, then
// the eight connect neighbors are examined using the same method.
//  
// This filter requires gradient information so you will need to use
// a vtkImageGradient at some point prior to this filter.  Typically
// a vtkNonMaximumSuppression filter is also used. vtkThresholdEdgels
// can be used to complete the two value edgel thresholding as used
// in a Canny edge detector. The vtkSubpixelPositionEdgels filter 
// can also be used after this filter to adjust the edgel locations.

// .SECTION see also
// vtkImage vtkImageGradient vtkLinkEdgels vtkNonMaximumSuppression

#ifndef __vtkLinkSurfels_h
#define __vtkLinkSurfels_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_EXPORT vtkLinkSurfels : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkLinkSurfels();
  vtkLinkSurfels *New() {return new vtkLinkSurfels;};
  char *GetClassName() {return "vtkLinkSurfels";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the threshold for Phi vs. Alpha link thresholding.
  vtkSetMacro(LinkThreshold,float);
  vtkGetMacro(LinkThreshold,float);

  // Description:
  // Set/get the threshold for Phi vs. Phi link thresholding.
  vtkSetMacro(PhiThreshold,float);
  vtkGetMacro(PhiThreshold,float);

  // Description:
  // Set/Get the threshold for image gradient thresholding.
  vtkSetMacro(GradientThreshold,float);
  vtkGetMacro(GradientThreshold,float);

protected:
  void Execute();
  void LinkSurfels(int xdim, int ydim, int zdim,
		   float *image, vtkVectors *inVectors,
		   vtkCellArray *newLines, vtkFloatPoints *newPts,
		   vtkFloatScalars *outScalars, vtkFloatVectors *outVectors,
		   float *aspect, float *origin);
  float GradientThreshold;
  float PhiThreshold;
  float LinkThreshold;
};

#endif

