/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkEdgels.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLinkEdgels - links edgels together to form digital curves.
// .SECTION Description
// vtkLinkEdgels links edgels into digital curves which are then stored 
// as polylines. The algorithm works one pixel at a time only looking at
// its immediate neighbors. There is a GradientThreshold that can be set 
// that eliminates any pixels with a smaller gradient value. This can
// be used as the lower threshold of a two value edgel thresholding. 
//
// For the remaining edgels, links are first tried for the four
// connected neighbors.  A successful neighbor will satisfy three
// tests. First both edgels must be above the gradient
// threshold. Second, the difference between the orientation between
// the two edgels (Alpha) and each edgels orientation (Phi) must be
// less than LinkThreshold. Third, the difference between the two
// edgels Phi values must be less than PhiThreshold.
// The most successful link is selected. The measure is simply the 
// sum of the three angle differences (actually stored as the sum of
// the cosines). If none of the four connect neighbors succeeds, then
// the eight connect neighbors are examined using the same method.
//  
// This filter requires gradient information so you will need to use
// a vtkImageGradient at some point prior to this filter.  Typically
// a vtkNonMaximumSuppression filter is also used. vtkThresholdEdgels
// can be used to complete the two value edgel thresholding as used
// in a Canny edge detector. The vtkSubpixelPositionEdgels filter 
// can also be used after this filter to adjust the edgel locations.

// .SECTION see also
// vtkImage vtkImageGradient vtkNonMaximumSuppression

#ifndef __vtkLinkEdgels_h
#define __vtkLinkEdgels_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkLinkEdgels : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkLinkEdgels,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct instance of vtkLinkEdgels with GradientThreshold set to 
  // 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
  static vtkLinkEdgels *New();

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
  vtkLinkEdgels();
  ~vtkLinkEdgels() {};

  void Execute();
  void LinkEdgels(int xdim, int ydim,float *image, vtkDataArray *inVectors,
                  vtkCellArray *newLines, vtkPoints *newPts,
                  vtkFloatArray *outScalars, vtkFloatArray *outVectors,
                  int z);
  float GradientThreshold;
  float PhiThreshold;
  float LinkThreshold;
private:
  vtkLinkEdgels(const vtkLinkEdgels&);  // Not implemented.
  void operator=(const vtkLinkEdgels&);  // Not implemented.
};

#endif
