/*=========================================================================

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkProjectedTexture - assign texture coordinates for a projected texture
// .SECTION Description
// vtkProjectedTexture assigns texture coordinates to a data set as if
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

class VTK_EXPORT vtkProjectedTexture : public vtkDataSetToDataSetFilter 
{
public:
  vtkProjectedTexture();
  static vtkProjectedTexture *New() {return new vtkProjectedTexture;};
  const char *GetClassName() {return "vtkProjectedTexture";};
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
  void Execute();
  void ComputeNormal();

  float Position[3];
  float Orientation[3];
  float FocalPoint[3];
  float Up[3];
  float AspectRatio[3];
  float SRange[2];
  float TRange[2];
};

#endif

