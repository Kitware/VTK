/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToPlane.hh
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
// .NAME vtkTextureMapToPlane - generate texture coordinates by mapping points to plane
// .SECTION Description
// vtkTextureMapToPlane is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a plane. The plane can either be
// user specified or generated automatically. (A least squares method is
// used to generate the plane automatically.)
//
// There are two ways you can specify the plane. The first is to provide a plane 
// normal. In this case the points are projected to a plane, and the points are
// then mapped into the user specified s-t coordinate range. For more control,
// you can specify a plane with three points: an origin and two points defining
// the two axes of the plane. (This is compatible with the vtkPlaneSource.) Using
// the second method, the SRange and TRange vectors are ignored, since the 
// presumption is that the user does not want to scale the texture coordinates;
// and you can adjust the origin and axes points to achieve the texture coordinate
// scaling you need. Note also that using the three point method the axes do not
// have to be orthogonal.

// .SECTION See Also
// vtkTextureMapToBox vtkPlaneSource vtkTextureMapToCylinder vtkTextureMapToSphere
// vtkThresholdTextureCoords

#ifndef __vtkTextureMapToPlane_h
#define __vtkTextureMapToPlane_h

#include "vtkDataSetToDataSetFilter.hh"

class vtkTextureMapToPlane : public vtkDataSetToDataSetFilter 
{
public:
  vtkTextureMapToPlane();
  char *GetClassName() {return "vtkTextureMapToPlane";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify a point defining the origin of the plane. Used in conjunction with
  // the Point1 and Point2 ivars to specify a map plane.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Specify plane normal. An alternative way to specify a map plane. Using this
  // method, the object will scale the resulting texture coordinate between the
  // SRange and TRange specified.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic plane generation.
  vtkSetMacro(AutomaticPlaneGeneration,int);
  vtkGetMacro(AutomaticPlaneGeneration,int);
  vtkBooleanMacro(AutomaticPlaneGeneration,int);

protected:
  void Execute();
  void ComputeNormal();

  float Origin[3];
  float Point1[3];
  float Point2[3];
  float Normal[3];
  float SRange[2];
  float TRange[2];
  int AutomaticPlaneGeneration;

};

#endif


