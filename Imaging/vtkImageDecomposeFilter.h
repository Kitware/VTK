/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposeFilter.h
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
// .NAME vtkImageDecomposeFilter - Filters that execute axes in series.
// .SECTION Description
// This superclass molds the vtkImageIterateFilter superclass so
// it iterates over the axes.  The filter uses dimensionality to 
// determine how many axes to execute (starting from x).  
// The filter also provides convenience methods for permuting information
// retrieved from input, output and vtkImageData.

#ifndef __vtkImageDecomposeFilter_h
#define __vtkImageDecomposeFilter_h


#include "vtkImageIterateFilter.h"

class VTK_IMAGING_EXPORT vtkImageDecomposeFilter : public vtkImageIterateFilter
{
public:
  // Description:
  // Construct an instance of vtkImageDecomposeFilter filter with default
  // dimensionality 3.
  vtkTypeRevisionMacro(vtkImageDecomposeFilter,vtkImageIterateFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Dimensionality is the number of axes which are considered during
  // execution. To process images dimensionality would be set to 2.
  void SetDimensionality(int dim);
  vtkGetMacro(Dimensionality,int);

#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // Obsolete legacy methods.
  void SetFilteredAxes(int axis0);
  void SetFilteredAxes(int axis0, int axis2);
  void SetFilteredAxes(int axis0, int axis2, int axis3);
#endif

  // Description:
  // Private methods kept public for template execute functions.
  void PermuteIncrements(int *increments, int &inc0, int &inc1, int &inc2);
  void PermuteExtent(int *extent, int &min0, int &max0, int &min1, int &max1,
                     int &min2, int &max2);
  
protected:
  vtkImageDecomposeFilter();
  ~vtkImageDecomposeFilter() {};

  int Dimensionality;


private:
  vtkImageDecomposeFilter(const vtkImageDecomposeFilter&);  // Not implemented.
  void operator=(const vtkImageDecomposeFilter&);  // Not implemented.
};

#endif










