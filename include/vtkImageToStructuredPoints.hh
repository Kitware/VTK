/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkImageToStructurePoints - read pnm (i.e., portable anymap) files
// .SECTION Description
// vtkImageToStructurePoints changes an image region format to
// a structured points dataset.  It was modeled after vtkPNMReader.


#ifndef __vtkImageToStructurePoints_h
#define __vtkImageToStructurePoints_h

#include "vtkStructuredPointsSource.hh"
#include "vtkGraymap.hh"
#include "vtkImageSource.hh"
#include "vtkImageRegion.hh"


class vtkImageToStructuredPoints : public vtkStructuredPointsSource
{
public:
  vtkImageToStructuredPoints();
  char *GetClassName() {return "vtkImageToStructurePoints";};

  // Description:
  // Set/Get the input object from the image pipline.
  vtkSetObjectMacro(Input,vtkImageSource);
  vtkGetObjectMacro(Input,vtkImageSource);
  // Description:
  // Set/Get the region to request
  vtkSetVector3Macro(Offset,int);
  vtkGetVector3Macro(Offset,int);
  vtkSetVector3Macro(Size,int);
  vtkGetVector3Macro(Size,int);
  // Description:
  // Set/Get the flag that tells the object toconvert the whole image or not.
  vtkSetMacro(WholeImageFlag,int);
  vtkGetMacro(WholeImageFlag,int);
  vtkBooleanMacro(WholeImageFlag,int);
  // Description:
  // Set/Get the flag that flips the Y axis (origin upper left?)
  vtkSetMacro(FlipYFlag,int);
  vtkGetMacro(FlipYFlag,int);
  vtkBooleanMacro(FlipYFlag,int);

  void ConditionalUpdate(int forcedFlag);
  
  
protected:
  vtkImageSource *Input;
  int Offset[3];
  int Size[3];
  int WholeImageFlag;
  int FlipYFlag;

  void Execute();
  void Generate(vtkImageRegion *region, vtkGraymap *scalars);
};

#endif


