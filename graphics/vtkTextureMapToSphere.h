/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToSphere.h
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
// .NAME vtkTextureMapToSphere - generate texture coordinates by mapping points to sphere
// .SECTION Description
// vtkTextureMapToSphere is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a sphere. The sphere can either be
// user specified or generated automatically. (The sphere is generated 
// automatically by computing the center (i.e., averaged coordinates) of the sphere.)
// Note that the generated texture coordinates range beween (0,1). The s-coordinate
// lies in the angular direction around the z-axis, measured counter-clockwise from the
// x-axis. The t-coordinate lies in the angular direction measured down from the north
// pole towards the south pole.
//
// A special ivar controls how the s-coordinate is generated. If PreventSeam
// is set to true, the s-texture varies from 0->1 and then 1->0 (corresponding
// to angles of 0->180 and 180->360).

// .Section Caveats
// The resulting texture coordinates will lie between (0,1), and the texture coordinates
// are determined with respect to the modeller's x-y-z coordinate system. Use the class 
// vtkTransformTextureCoords to linearly scale and shift the origin of the texture
// coordinates (if necessary).

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToCylinder vtkTextureMapToBox
// vtkTransformTexture vtkThresholdTextureCoords

#ifndef __vtkTextureMapToSphere_h
#define __vtkTextureMapToSphere_h

#include "vtkDataSetToDataSetFilter.h"

class vtkTextureMapToSphere : public vtkDataSetToDataSetFilter 
{
public:
  vtkTextureMapToSphere();
  char *GetClassName() {return "vtkTextureMapToSphere";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify a point defining the center of the sphere.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Turn on/off automatic sphere generation. This means it automatically finds 
  // the sphere center.
  vtkSetMacro(AutomaticSphereGeneration,int);
  vtkGetMacro(AutomaticSphereGeneration,int);
  vtkBooleanMacro(AutomaticSphereGeneration,int);

  // Description:
  // Control how the texture coordinates are generated. If PreventSeam is set, the
  // s-coordinate ranges from 0->1 and 1->0 corresponding to the theta angle variation
  // between 0->180 and 180->0 degrees. Otherwise, the s-ccordinate ranges from 0->1 
  // between 0->360 degrees.
  vtkSetMacro(PreventSeam,int);
  vtkGetMacro(PreventSeam,int);
  vtkBooleanMacro(PreventSeam,int);

protected:
  void Execute();

  float Center[3];
  int AutomaticSphereGeneration;
  int PreventSeam;

};

#endif


