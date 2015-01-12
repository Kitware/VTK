/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkSplineFilter_h
#define vtkSplineFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_SUBDIVIDE_SPECIFIED 0
#define VTK_SUBDIVIDE_LENGTH    1

#define VTK_TCOORDS_OFF                    0
#define VTK_TCOORDS_FROM_NORMALIZED_LENGTH 1
#define VTK_TCOORDS_FROM_LENGTH            2
#define VTK_TCOORDS_FROM_SCALARS           3

class vtkCellArray;
class vtkCellData;
class vtkFloatArray;
class vtkPointData;
class vtkPoints;
class vtkSpline;

class VTKFILTERSGENERAL_EXPORT vtkSplineFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSplineFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct the class with no limit on the number of subdivisions
  // and using an instance of vtkCardinalSpline to perform interpolation.
  static vtkSplineFilter *New();

  // Description:
  // Set the maximum number of subdivisions that are created for each
  // polyline.
  vtkSetClampMacro(MaximumNumberOfSubdivisions,int,1,VTK_INT_MAX);
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
  vtkSetClampMacro(NumberOfSubdivisions,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfSubdivisions,int);

  // Description:
  // Control the number of subdivisions that are created for the
  // polyline based on an absolute length. The length of the spline
  // is divided by this length to determine the number of subdivisions.
  vtkSetClampMacro(Length,double,0.0000001,VTK_DOUBLE_MAX);
  vtkGetMacro(Length,double);

  // Description:
  // Specify an instance of vtkSpline to use to perform the interpolation.
  virtual void SetSpline(vtkSpline*);
  vtkGetObjectMacro(Spline,vtkSpline);

  // Description:
  // Control whether and how texture coordinates are produced. This is
  // useful for striping the output polyline. The texture coordinates
  // can be generated in three ways: a normalized (0,1) generation;
  // based on the length (divided by the texture length); and by using
  // the input scalar values.
  vtkSetClampMacro(GenerateTCoords,int,VTK_TCOORDS_OFF,
                   VTK_TCOORDS_FROM_SCALARS);
  vtkGetMacro(GenerateTCoords,int);
  void SetGenerateTCoordsToOff()
    {this->SetGenerateTCoords(VTK_TCOORDS_OFF);}
  void SetGenerateTCoordsToNormalizedLength()
    {this->SetGenerateTCoords(VTK_TCOORDS_FROM_NORMALIZED_LENGTH);}
  void SetGenerateTCoordsToUseLength()
    {this->SetGenerateTCoords(VTK_TCOORDS_FROM_LENGTH);}
  void SetGenerateTCoordsToUseScalars()
    {this->SetGenerateTCoords(VTK_TCOORDS_FROM_SCALARS);}
  const char *GetGenerateTCoordsAsString();

  // Description:
  // Control the conversion of units during the texture coordinates
  // calculation. The TextureLength indicates what length (whether
  // calculated from scalars or length) is mapped to the [0,1)
  // texture space.
  vtkSetClampMacro(TextureLength,double,0.000001,VTK_INT_MAX);
  vtkGetMacro(TextureLength,double);

protected:
  vtkSplineFilter();
  ~vtkSplineFilter();

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int       MaximumNumberOfSubdivisions;
  int       Subdivide;
  int       NumberOfSubdivisions;
  double     Length;
  vtkSpline *Spline;
  vtkSpline *XSpline;
  vtkSpline *YSpline;
  vtkSpline *ZSpline;
  int       GenerateTCoords;
  double     TextureLength; //this length is mapped to [0,1) texture space

  //helper methods
  int GeneratePoints(vtkIdType offset, vtkIdType npts, vtkIdType *pts,
                     vtkPoints *inPts, vtkPoints *newPts, vtkPointData *pd,
                     vtkPointData *outPD, int genTCoords,
                     vtkFloatArray *newTCoords);

  void GenerateLine(vtkIdType offset, vtkIdType numGenPts, vtkIdType inCellId,
                    vtkCellData *cd, vtkCellData *outCD, vtkCellArray *newLines);

  //helper members
  vtkFloatArray *TCoordMap;

private:
  vtkSplineFilter(const vtkSplineFilter&);  // Not implemented.
  void operator=(const vtkSplineFilter&);  // Not implemented.
};

#endif
