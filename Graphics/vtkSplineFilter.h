/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineFilter.h
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
// .NAME vtkSplineFilter - generate uniformly subdivided polylines from a set of input polyline using a vtkSpline
// .SECTION Description
// vtkSplineFilter is a filter that generates an output polylines from an
// input set of polylines. The polylines are uniformly subdivided and produced
// with the help of a vtkSpline class that the user can specify (by default a
// vtkCardinalSpline is used). The number of subdivisions of the line can be
// controlled in several ways. The user can either specify the number of
// subdivisions or a length of each subdivision can be provided (and the
// class will figure out how many subdivisions is required over the whole
// polyline). The maximum number of subdivisions can also be set.
//
// The output of this filter is a polyline per input polyline (or line). New
// points and texture coordinates are created. Point data is interpolated and
// cell data passed on. Any polylines with less than two points, or who have
// coincident points, are ignored.

// .SECTION See Also
// vtkRibbonFilter vtkTubeFilter

#ifndef __vtkSplineFilter_h
#define __vtkSplineFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkSpline.h"

#define VTK_SUBDIVIDE_SPECIFIED 0
#define VTK_SUBDIVIDE_LENGTH    1

class VTK_GRAPHICS_EXPORT vtkSplineFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkSplineFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct the class with no limit on the number of subdivisions
  // and using an instance of vtkCardinalSpline to perform interpolation.
  static vtkSplineFilter *New();

  // Description:
  // Set the maximum number of subdivisions that are created for each
  // polyline.
  vtkSetClampMacro(MaximumNumberOfSubdivisions,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumNumberOfSubdivisions,int);

  // Description:
  // Specify how the number of subdivisions is determined.
  vtkSetClampMacro(Subdivide,int,VTK_SUBDIVIDE_SPECIFIED,VTK_SUBDIVIDE_LENGTH);
  vtkGetMacro(Subdivide,int);
  void SetSubdivideToSpecified()
    {this->SetSubdivide(VTK_SUBDIVIDE_SPECIFIED);}
  void SetSubdivideToLength()
    {this->SetSubdivide(VTK_SUBDIVIDE_LENGTH);}
  const char *GetSubdivideAsString();

  // Description:
  // Set the number of subdivisions that are created for the
  // polyline. This method only has effect if Subdivisions is set
  // to SetSubdivisionsToSpecify().
  vtkSetClampMacro(NumberOfSubdivisions,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfSubdivisions,int);

  // Description:
  // Control the number of subdivisions that are created for the
  // polyline based on an absolute length. The length of the spline
  // is divided by this length to determine the number of subdivisions.
  vtkSetClampMacro(Length,float,0.0000001,VTK_LARGE_FLOAT);
  vtkGetMacro(Length,float);

  // Description:
  // Specify an instance of vtkSpline to use to perform the interpolation.
  vtkSetObjectMacro(Spline,vtkSpline);
  vtkGetObjectMacro(Spline,vtkSpline);

protected:
  vtkSplineFilter();
  ~vtkSplineFilter();
  
  // Usual data generation method
  void Execute();

  int       MaximumNumberOfSubdivisions;
  int       Subdivide;
  int       NumberOfSubdivisions;
  float     Length;
  vtkSpline *Spline;
  vtkSpline *XSpline;
  vtkSpline *YSpline;
  vtkSpline *ZSpline;

  //helper methods
  int GeneratePoints(vtkIdType offset, vtkIdType npts, vtkIdType *pts, 
                     vtkPoints *inPts, vtkPoints *newPts, vtkPointData *pd, 
                     vtkPointData *outPD, vtkFloatArray *newTCoords);

  void GenerateLine(vtkIdType offset, vtkIdType numGenPts, vtkIdType inCellId,
                  vtkCellData *cd, vtkCellData *outCD, vtkCellArray *newLines);

  //helper members
  vtkFloatArray *TCoordMap;

private:
  vtkSplineFilter(const vtkSplineFilter&);  // Not implemented.
  void operator=(const vtkSplineFilter&);  // Not implemented.
};

#endif
