/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTexture.h
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
// .NAME vtkProjectedTexture - assign texture coordinates for a projected texture
// .SECTION Description
// vtkProjectedTexture assigns texture coordinates to a dataset as if
// the texture was projected from a slide projected located somewhere in the
// scene.  Methods are provided to position the projector and aim it at a 
// location, to set the width of the projector's frustum, and to set the
// range of texture coordinates assigned to the dataset.  
//
// Objects in the scene that appear behind the projector are also assigned
// texture coordinates; the projected image is left-right and top-bottom 
// flipped, much as a lens' focus flips the rays of light that pass through
// it.  A warning is issued if a point in the dataset falls at the focus
// of the projector.

#ifndef __vtkProjectedTexture_h
#define __vtkProjectedTexture_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkProjectedTexture : public vtkDataSetToDataSetFilter 
{
public:
  static vtkProjectedTexture *New();
  vtkTypeRevisionMacro(vtkProjectedTexture,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the position of the focus of the projector.
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);

  // Description:
  // Set/Get the focal point of the projector (a point that lies along
  // the center axis of the projector's frustum).
  void SetFocalPoint(float focalPoint[3]);
  void SetFocalPoint(float x, float y, float z);
  vtkGetVectorMacro(FocalPoint,float,3);

  // Description:
  // Get the normalized orientation vector of the projector.
  vtkGetVectorMacro(Orientation,float,3);
  
  // Set/Get the up vector of the projector.
  vtkSetVector3Macro(Up,float);
  vtkGetVectorMacro(Up,float,3);

  // Set/Get the aspect ratio of a perpendicular cross-section of the
  // the projector's frustum.  The aspect ratio consists of three 
  // numbers:  (x, y, z), where x is the width of the 
  // frustum, y is the height, and z is the perpendicular
  // distance from the focus of the projector.
  vtkSetVector3Macro(AspectRatio,float);
  vtkGetVectorMacro(AspectRatio,float,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);
  
protected:
  vtkProjectedTexture();
  ~vtkProjectedTexture() {};

  void Execute();
  void ComputeNormal();

  float Position[3];
  float Orientation[3];
  float FocalPoint[3];
  float Up[3];
  float AspectRatio[3];
  float SRange[2];
  float TRange[2];
private:
  vtkProjectedTexture(const vtkProjectedTexture&);  // Not implemented.
  void operator=(const vtkProjectedTexture&);  // Not implemented.
};

#endif

