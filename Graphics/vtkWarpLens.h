/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkWarpLens - deform geometry by applying lens distortion
// .SECTION Description
// vtkWarpLens is a filter that modifies point coordinates by moving
// in accord with a lens distortion model.

#ifndef __vtkWarpLens_h
#define __vtkWarpLens_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_EXPORT vtkWarpLens : public vtkPointSetToPointSetFilter
{
public:
  static vtkWarpLens *New();
  vtkTypeMacro(vtkWarpLens,vtkPointSetToPointSetFilter);
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
  vtkWarpLens(const vtkWarpLens&) {};
  void operator=(const vtkWarpLens&) {};

  void Execute();

  float PrincipalPoint[2]; 	// The calibrated principal point of camera/lens in mm
  float K1; 			// Symmetric radial distortion parameters
  float K2;
  float P1;			// Decentering distortion parameters
  float P2;
  float FormatWidth;		// imager format width in mm
  float FormatHeight;		// imager format height in mm
  int ImageWidth;		// image width in pixels
  int ImageHeight;		// image height in pixels
};

#endif
