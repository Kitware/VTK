/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareContourFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkKitwareContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkKitwareContourFilter is a filter that takes as input any dataset and 
// generates on output isosurfaces and/or isolines. The exact form 
// of the output depends upon the dimensionality of the input data. 
// Data consisting of 3D cells will generate isosurfaces, data 
// consisting of 2D cells will generate isolines, and data with 1D 
// or 0D cells will generate isopoints. Combinations of output type 
// are possible if the input dimension is mixed.
//
// This filter will identify special dataset types (e.g., structured
// points) and use the appropriate specialized filter to process the
// data. For examples, if the input dataset type is a volume, this
// filter will create an internal vtkSyncronizedTemplates3D instance 
// and use it. This gives much better performance on StructuredPoints 
// and StructuredGrids.
// 
// To use this filter you must specify one or more contour values.
// You can either use the method SetValue() to specify each contour
// value, or use GenerateValues() to generate a series of evenly
// spaced contours. It is also possible to accelerate the operation of
// this filter (at the cost of extra memory) by using a
// vtkScalarTree. A scalar tree is used to quickly locate cells that
// contain a contour surface. This is especially effective if multiple
// contours are being extracted. If you want to use a scalar tree,
// invoke the method UseScalarTreeOn().

// .SECTION Caveats
// For StructuredPoints And StructuredGrids, normals are computed
// by default, but it is an expensive computation.  Processing for 
// other data set types has not been extended to include normal 
// computation.  In the mean time, use vtkPolyDataNormals to compute 
// the surface normals.

// .SECTION See Also
// vtkSynchronizedTemplates3D vtkSynchronizedTemplates2D 
// vtkGridSynchronizedTemplates3D

#ifndef __vtkKitwareContourFilter_h
#define __vtkKitwareContourFilter_h

#include "vtkContourFilter.h"

class VTK_EXPORT vtkKitwareContourFilter : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkKitwareContourFilter,vtkContourFilter);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkKitwareContourFilter *New();

protected:
  vtkKitwareContourFilter();
  ~vtkKitwareContourFilter();
  vtkKitwareContourFilter(const vtkKitwareContourFilter&) {};
  void operator=(const vtkKitwareContourFilter&) {};
  
  void Execute();

  //special contouring for structured points
  void StructuredPointsContour(int dim); 
  //special contouring for structured grid
  void StructuredGridContour(int dim);
  //default if not structured data
  void DataSetContour();
};

#endif


