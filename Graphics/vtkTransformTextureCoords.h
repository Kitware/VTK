/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformTextureCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransformTextureCoords - transform (scale, rotate, translate) texture coordinates
// .SECTION Description
// vtkTransformTextureCoords is a filter that operates on texture
// coordinates. It ingests any type of dataset, and outputs a dataset of the
// same type. The filter lets you scale, translate, and rotate texture
// coordinates. For example, by using the the Scale ivar, you can shift
// texture coordinates that range from (0->1) to range from (0->10) (useful
// for repeated patterns).
// 
// The filter operates on texture coordinates of dimension 1->3. The texture 
// coordinates are referred to as r-s-t. If the texture map is two dimensional,
// the t-coordinate (and operations on the t-coordinate) are ignored.

// .SECTION See Also
// vtkTextureMapToPlane  vtkTextureMapToCylinder 
// vtkTextureMapToSphere vtkThresholdTextureCoords vtkTexture

#ifndef __vtkTransformTextureCoords_h
#define __vtkTransformTextureCoords_h

#include "vtkDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkTransformTextureCoords : public vtkDataSetAlgorithm 
{
public:
  vtkTypeMacro(vtkTransformTextureCoords,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create instance with Origin (0.5,0.5,0.5); Position (0,0,0); and Scale
  // set to (1,1,1). Rotation of the texture coordinates is turned off.
  static vtkTransformTextureCoords *New();

  // Description:
  // Set/Get the position of the texture map. Setting the position translates
  // the texture map by the amount specified. 
  vtkSetVector3Macro(Position,double);
  vtkGetVectorMacro(Position,double,3);

  // Description:
  // Incrementally change the position of the texture map (i.e., does a
  // translate or shift of the texture coordinates).
  void AddPosition(double deltaR, double deltaS, double deltaT);
  void AddPosition(double deltaPosition[3]);
  
  // Description:
  // Set/Get the scale of the texture map. Scaling in performed independently 
  // on the r, s and t axes.
  vtkSetVector3Macro(Scale,double);
  vtkGetVectorMacro(Scale,double,3);

  // Description:
  // Set/Get the origin of the texture map. This is the point about which the
  // texture map is flipped (e.g., rotated). Since a typical texture map ranges
  // from (0,1) in the r-s-t coordinates, the default origin is set at 
  // (0.5,0.5,0.5).
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

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
  vtkTransformTextureCoords();
  ~vtkTransformTextureCoords() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Origin[3]; //point around which map rotates
  double Position[3]; //controls translation of map
  double Scale[3]; //scales the texture map
  int FlipR; //boolean indicates whether to flip texture around r-axis
  int FlipS; //boolean indicates whether to flip texture around s-axis
  int FlipT; //boolean indicates whether to flip texture around t-axis
private:
  vtkTransformTextureCoords(const vtkTransformTextureCoords&);  // Not implemented.
  void operator=(const vtkTransformTextureCoords&);  // Not implemented.
};

#endif
