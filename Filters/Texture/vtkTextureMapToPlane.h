/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextureMapToPlane - generate texture coordinates by mapping points to plane
// .SECTION Description
// vtkTextureMapToPlane is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a plane. The plane can either be
// user specified or generated automatically. (A least squares method is
// used to generate the plane automatically.)
//
// There are two ways you can specify the plane. The first is to provide a
// plane normal. In this case the points are projected to a plane, and the
// points are then mapped into the user specified s-t coordinate range. For
// more control, you can specify a plane with three points: an origin and two
// points defining the two axes of the plane. (This is compatible with the
// vtkPlaneSource.) Using the second method, the SRange and TRange vectors
// are ignored, since the presumption is that the user does not want to scale
// the texture coordinates; and you can adjust the origin and axes points to
// achieve the texture coordinate scaling you need. Note also that using the
// three point method the axes do not have to be orthogonal.

// .SECTION See Also
//  vtkPlaneSource vtkTextureMapToCylinder
// vtkTextureMapToSphere vtkThresholdTextureCoords

#ifndef vtkTextureMapToPlane_h
#define vtkTextureMapToPlane_h

#include "vtkFiltersTextureModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSTEXTURE_EXPORT vtkTextureMapToPlane : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkTextureMapToPlane,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with s,t range=(0,1) and automatic plane generation turned on.
  static vtkTextureMapToPlane *New();

  // Description:
  // Specify a point defining the origin of the plane. Used in conjunction with
  // the Point1 and Point2 ivars to specify a map plane.
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);

  // Description:
  // Specify plane normal. An alternative way to specify a map plane. Using
  // this method, the object will scale the resulting texture coordinate
  // between the SRange and TRange specified.
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,double);
  vtkGetVectorMacro(SRange,double,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,double);
  vtkGetVectorMacro(TRange,double,2);

  // Description:
  // Turn on/off automatic plane generation.
  vtkSetMacro(AutomaticPlaneGeneration,int);
  vtkGetMacro(AutomaticPlaneGeneration,int);
  vtkBooleanMacro(AutomaticPlaneGeneration,int);

protected:
  vtkTextureMapToPlane();
  ~vtkTextureMapToPlane() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void ComputeNormal(vtkDataSet *output);

  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Normal[3];
  double SRange[2];
  double TRange[2];
  int AutomaticPlaneGeneration;

private:
  vtkTextureMapToPlane(const vtkTextureMapToPlane&);  // Not implemented.
  void operator=(const vtkTextureMapToPlane&);  // Not implemented.
};

#endif
