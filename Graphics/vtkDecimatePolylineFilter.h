/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePolylineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDecimatePolylineFilter - reduce the number of lines in a polyline
// .SECTION Description
// vtkDecimatePolylineFilter is a filter to reduce the number of lines in a
// polyline. The algorithm functions by evaluating an error metric for each
// vertex (i.e., the distance of the vertex to a line defined from the two
// vertices on either side of the vertex). Then, these vertices are placed
// into a priority queue, and those with larger errors are deleted first.
// The decimation continues until the target reduction is reached.
//
// .SECTION Caveats
// This algorithm is a very simple implementation that overlooks some
// potential complexities. First, if a vertex is multiply connected,
// meaning that it is used by multiple polylines, then the extra
// topological constraints are ignored. Second, the error is not updated
// as vertices are deleted (similar to iteratively computing a quadric
// error metric). Thus, once calculated, the error is used to determine
// which vertices are removed. This can produce less than optimal results.
//
// .SECTION See Also
// vtkDecimate vtkDecimateProp vtkQuadricClustering vtkQuadricDecimation


#ifndef __vtkDecimatePolylineFilter_h
#define __vtkDecimatePolylineFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkPriorityQueue;

class VTK_GRAPHICS_EXPORT vtkDecimatePolylineFilter : public vtkPolyDataAlgorithm
{
public:
  // Description:
  // Standard methods for type information and printing.
  vtkTypeMacro(vtkDecimatePolylineFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate this object with a target reduction of 0.90.
  static vtkDecimatePolylineFilter *New();

  // Description:
  // Specify the desired reduction in the total number of polygons (e.g., if
  // TargetReduction is set to 0.9, this filter will try to reduce the data set
  // to 10% of its original size). 
  vtkSetClampMacro(TargetReduction,double,0.0,1.0);
  vtkGetMacro(TargetReduction,double);

protected:
  vtkDecimatePolylineFilter();
  ~vtkDecimatePolylineFilter();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double TargetReduction;

private:
  vtkDecimatePolylineFilter(const vtkDecimatePolylineFilter&);  // Not implemented.
  void operator=(const vtkDecimatePolylineFilter&);  // Not implemented.
};

#endif


