/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformTextureCoords.h
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
// .NAME vtkTransformTexture - transform (scale, rotate, translate) texture coordinates
// .SECTION Description
// vtkTransformTexture is a filter that operates on texture coordinates. It ingests
// any type of dataset, and outputs a dataset of the same type. The filter lets
// you scale, translate, and rotate texture coordinates. For example, by using the
// the Scale ivar, you can shift texture coordinates that range from (0->1) to
// range from (0->10) (useful for repeated patterns).
// 
// The filter operates on texture coordinates of dimension 1->3. The texture 
// coordinates are referred to as r-s-t. If the texture map is two dimensional,
// the t-coordinate (and operations on the t-coordinate) are ignored.

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToBox vtkTextureMapToCylinder 
// vtkTextureMapToSphere vtkThresholdTextureCoords vtkTexture vtkTCoords

#ifndef __vtkTransformTextureCoords_h
#define __vtkTransformTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"

class vtkTransformTextureCoords : public vtkDataSetToDataSetFilter 
{
public:
  vtkTransformTextureCoords();
  char *GetClassName() {return "vtkTransformTextureCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the position of the texture map. Setting the position translates
  // the texture map by the amount specified. 
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);

  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaR, float deltaS, float deltaT);

  // Description:
  // Set/Get the scale of the texture map. Scaling in performed independently 
  // on the r, s and t axes.
  vtkSetVector3Macro(Scale,float);
  vtkGetVectorMacro(Scale,float,3);

  // Description:
  // Set/Get the origin of the texture map. This is the point about which the
  // texture map is flipped (e.g., rotated). Since a typical texture map ranges
  // from (0,1) in the r-s-t coordinates, the default origin is set at 
  // (0.5,0.5,0.5).
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Boolean indicates whether the texture map should be flipped around the 
  // s-axis. Note that the flips occur around the texture origin.
  vtkSetMacro(FlipR,int);
  vtkGetMacro(FlipR,int);
  vtkBooleanMacro(FlipR,int);

  // Description:
  // Boolean indicates whether the texture map should be flipped around the 
  // s-axis. Note that the flips occur around the texture origin.
  vtkSetMacro(FlipS,int);
  vtkGetMacro(FlipS,int);
  vtkBooleanMacro(FlipS,int);

  // Description:
  // Boolean indicates whether the texture map should be flipped around the 
  // t-axis. Note that the flips occur around the texture origin.
  vtkSetMacro(FlipT,int);
  vtkGetMacro(FlipT,int);
  vtkBooleanMacro(FlipT,int);

protected:
  void Execute();

  float Origin[3]; //point around which map rotates
  float Position[3]; //controls translation of map
  float Scale[3]; //scales the texture map
  int FlipR; //boolean indicates whether to flip texture around r-axis
  int FlipS; //boolean indicates whether to flip texture around s-axis
  int FlipT; //boolean indicates whether to flip texture around t-axis
};

#endif


