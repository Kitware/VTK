/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTextureCoords.h
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
// .NAME vtkThresholdTextureCoords - compute 1D, 2D, or 3D texture coordinates based on scalar threshold
// .SECTION Description
// vtkThresholdTextureCoords is a filter that generates texture coordinates for
// any input dataset type given a threshold criterion. The criterion can take 
// three forms: 1) greater than a particular value (ThresholdByUpper()); 
// 2) less than a particular value (ThresholdByLower(); or 3) between two 
// values (ThresholdBetween(). If the threshold criterion is satisfied, 
// the "in" texture coordinate will be set (this can be specified by the
// user). If the threshold criterion is not satisfied the "out" is set.
// .SECTION Caveats
// There is a texture map - texThres.vtk - that can be used in conjunction
// with this filter. This map defines a "transparent" region for texture 
// coordinates 0<=r<0.5, and an opaque full intensity map for texture 
// coordinates 0.5<r<=1.0. There is a small transition region for r=0.5.
// .SECTION See Also
// vtkThreshold vtkThresholdPoints vtkTextureMapToPlane vtkTextureMapToSphere
// vtkTextureMapToCylinder vtkTextureMapToBox

#ifndef __vtkThresholdTextureCoords_h
#define __vtkThresholdTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkThresholdTextureCoords : public vtkDataSetToDataSetFilter
{
public:
  vtkThresholdTextureCoords();
  char *GetClassName() {return "vtkThresholdTextureCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void ThresholdByLower(float lower);
  void ThresholdByUpper(float upper);
  void ThresholdBetween(float lower, float upper);
  
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

  // Description:
  // Set the desired dimension of the texture map.
  vtkSetClampMacro(TextureDimension,int,1,3);
  vtkGetMacro(TextureDimension,int);

  // Description:
  // Set the texture coordinate value for point satisfying threshold criterion.
  vtkSetVector3Macro(InTextureCoord,float);
  vtkGetVectorMacro(InTextureCoord,float,3);

  // Description:
  // Set the texture coordinate value for point NOT satisfying threshold
  //  criterion.
  vtkSetVector3Macro(OutTextureCoord,float);
  vtkGetVectorMacro(OutTextureCoord,float,3);

protected:
  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  int TextureDimension;

  float InTextureCoord[3];
  float OutTextureCoord[3];

  //BTX
  int (vtkThresholdTextureCoords::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif


