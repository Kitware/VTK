/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLinesToImage.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkPolyLinesToImage - Convert lines in poly data to an image.
// .SECTION Description
// vtkPolyLinesToImage creates an image by drawing the polylines in
// an image.  It ignores the Z axis, and assumes the two origins
// coincide.


#ifndef __vtkPolyLinesToImage_h
#define __vtkPolyLinesToImage_h

#include <iostream.h>
#include <fstream.h>
#include "vtkPolyData.h"
#include "vtkImageSource.h"
#include "vtkImageCanvasSource2D.h"

class vtkPolyLinesToImage : public vtkImageSource
{
public:
  vtkPolyLinesToImage();
  ~vtkPolyLinesToImage();
  static vtkPolyLinesToImage *New() {return new vtkPolyLinesToImage;};
  const char *GetClassName() {return "vtkPolyLinesToImage";};
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  void UpdateImageInformation();
  void Update();
  unsigned long GetPipelineMTime();
  
  // Description:
  // Set/Get the input
  vtkSetObjectMacro(Input,vtkPolyData);
  vtkGetObjectMacro(Input,vtkPolyData);

  // Description:
  // Sets the maximum extent that can be requested.
  // If this is not set by the first update, it defaults to 
  // an extent large enough to contain all the polylines
  void SetWholeExtent(int min0, int max0, int min1, int max1);

  // Description:
  // Sets the aspect ratio (resolution) of the output.
  vtkSetVector2Macro(Spacing, float);
  
  // Description:
  // Sets the Origin of the output image.
  vtkSetVector2Macro(Origin, float);
  
protected:
  vtkPolyData *Input;
  vtkImageCanvasSource2D *Paint;
  
  int WholeExtent[8];
  float Spacing[4];
  float Origin[4];
  
  void UpdateInput();
  void ExecuteImageInformation();

  void Execute(vtkImageRegion *region);
  
};

#endif


