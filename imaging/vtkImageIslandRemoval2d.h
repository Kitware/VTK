/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIslandRemoval2d.h
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
// .NAME vtkImageIslandRemoval2d - Removes small clusters in masks.
// .SECTION Description
// vtkImageIslandRemoval2d computes the area of separate islands in 
// a mask image.  It removes any island that has less than AreaThreshold
// pixels.  Output has the same ScalarType as input.


#ifndef __vtkImageIslandRemoval2d_h
#define __vtkImageIslandRemoval2d_h


#include "vtkImageFilter.h"



typedef struct{
  void *inPtr;
  void *outPtr;
  int idx0;
  int idx1;
  } vtkImage2dIslandPixel;



class vtkImageIslandRemoval2d : public vtkImageFilter
{
public:
  vtkImageIslandRemoval2d();
  char *GetClassName() {return "vtkImageIslandRemoval2d";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void InterceptCacheUpdate(vtkImageRegion *region);
  
  // Description:
  // Set/Get the cutoff area for removal
  vtkSetMacro(AreaThreshold, int);
  vtkGetMacro(AreaThreshold, int);

  // Description:
  // Set/Get whether to use 4 or 8 neighbors
  vtkSetMacro(SquareNeighborhood, int);
  vtkGetMacro(SquareNeighborhood, int);
  vtkBooleanMacro(SquareNeighborhood, int);

  // Description:
  // Set/Get the value to remove.
  vtkSetMacro(IslandValue, float);
  vtkGetMacro(IslandValue, float);

  // Description:
  // Set/Get the value to put in the place of removed pixels.
  vtkSetMacro(ReplaceValue, float);
  vtkGetMacro(ReplaceValue, float);
  
protected:
  int AreaThreshold;
  int SquareNeighborhood;
  float IslandValue;
  float ReplaceValue;
  
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



