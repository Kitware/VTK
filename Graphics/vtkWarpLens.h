/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.h
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
// .NAME vtkWarpLens - deform geometry by applying lens distortion
// .SECTION Description
// vtkWarpLens is a filter that modifies point coordinates by moving
// in accord with a lens distortion model.

#ifndef __vtkWarpLens_h
#define __vtkWarpLens_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkWarpLens : public vtkPointSetToPointSetFilter
{
public:
  static vtkWarpLens *New();
  vtkTypeRevisionMacro(vtkWarpLens,vtkPointSetToPointSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify second order symmetric radial lens distortion parameter.
  // This is obsoleted by newer instance variables.
  void SetKappa(float kappa);
  float GetKappa();

  // Description:
  // Specify the center of radial distortion in pixels.
  // This is obsoleted by newer instance variables.
  void SetCenter(float centerX, float centerY);
  float *GetCenter();

  // Description:
  // Specify the calibrated principal point of the camera/lens
  vtkSetVector2Macro(PrincipalPoint,float);
  vtkGetVectorMacro(PrincipalPoint,float,2);

  // Description:
  // Specify the symmetric radial distortion parameters for the lens
  vtkSetMacro(K1,float);
  vtkGetMacro(K1,float);
  vtkSetMacro(K2,float);
  vtkGetMacro(K2,float);

  // Description:
  // Specify the decentering distortion parameters for the lens
  vtkSetMacro(P1,float);
  vtkGetMacro(P1,float);
  vtkSetMacro(P2,float);
  vtkGetMacro(P2,float);

  // Description:
  // Specify the imager format width / height in mm
  vtkSetMacro(FormatWidth,float);
  vtkGetMacro(FormatWidth,float);
  vtkSetMacro(FormatHeight,float);
  vtkGetMacro(FormatHeight,float);

  // Description:
  // Specify the image width / height in pixels
  vtkSetMacro(ImageWidth,int);
  vtkGetMacro(ImageWidth,int);
  vtkSetMacro(ImageHeight,int);
  vtkGetMacro(ImageHeight,int);


protected:
  vtkWarpLens();
  ~vtkWarpLens() {};

  void Execute();

  float PrincipalPoint[2];      // The calibrated principal point of camera/lens in mm
  float K1;                     // Symmetric radial distortion parameters
  float K2;
  float P1;                     // Decentering distortion parameters
  float P2;
  float FormatWidth;            // imager format width in mm
  float FormatHeight;           // imager format height in mm
  int ImageWidth;               // image width in pixels
  int ImageHeight;              // image height in pixels
private:
  vtkWarpLens(const vtkWarpLens&);  // Not implemented.
  void operator=(const vtkWarpLens&);  // Not implemented.
};

#endif
