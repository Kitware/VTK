/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareContourFilter.h
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

class VTK_PATENTED_EXPORT vtkKitwareContourFilter : public vtkContourFilter
{
public:
  vtkTypeRevisionMacro(vtkKitwareContourFilter,vtkContourFilter);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkKitwareContourFilter *New();

protected:
  vtkKitwareContourFilter();
  ~vtkKitwareContourFilter();

  void ComputeInputUpdateExtents(vtkDataObject *data);
  void Execute();
  void ExecuteInformation();

  //special contouring for structured points
  void StructuredPointsContour(int dim); 
  //special contouring for structured grid
  void StructuredGridContour(int dim);
  //default if not structured data
  void DataSetContour();
private:
  vtkKitwareContourFilter(const vtkKitwareContourFilter&);  // Not implemented.
  void operator=(const vtkKitwareContourFilter&);  // Not implemented.
};

#endif


