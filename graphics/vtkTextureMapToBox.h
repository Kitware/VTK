/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToBox.h
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
// .NAME vtkTextureMapToBox - generate 3D texture coordinates by mapping points into bounding box
// .SECTION Description
// vtkTextureMapToBox is a filter that generates 3D texture coordinates
// by mapping input dataset points onto a bounding box. The bounding box
// can either be user specified or generated automatically. If the box
// is generated automatically, all points will lie inside of it. If a
// point lies outside the bounding box (only for manual box 
// specification), its generated texture coordinate will be mapped
// into the r-s-t texture coordinate range.

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToCylinder vtkTextureMapToSphere
// vtkThresholdTextureCoords

#ifndef __vtkTextureMapToBox_h
#define __vtkTextureMapToBox_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkTextureMapToBox : public vtkDataSetToDataSetFilter 
{
public:
  vtkTextureMapToBox();
  static vtkTextureMapToBox *New() {return new vtkTextureMapToBox;};
  const char *GetClassName() {return "vtkTextureMapToBox";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetBox(float *box);
  void SetBox(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(Box,float,6);

  // Description:
  // Specify r-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(RRange,float);
  vtkGetVectorMacro(RRange,float,2);

  // Description:
  // Specify s-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic bounding box generation.
  vtkSetMacro(AutomaticBoxGeneration,int);
  vtkGetMacro(AutomaticBoxGeneration,int);
  vtkBooleanMacro(AutomaticBoxGeneration,int);

protected:
  void Execute();
  float Box[6];
  float RRange[2];
  float SRange[2];
  float TRange[2];
  int AutomaticBoxGeneration;
};

#endif


