/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformStructuredPoints.h
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
// .NAME vtkTransformStructuredPoints - transform (and resample) vtkStructuredPoints
// .SECTION Description
// vtkTransformStructuredPoints is a filter that samples an input structured 
// point set with a "transformed" structured point set. The sampling process
// occurs as follows: each output point (or voxel) is transformed according
// to a user  specified transformation object. The point is used to sample
// the input. If the point does not fall inside the input structured point 
// set, then the point is assigned a fill value (user specified). Otherwise,
// tri-linear interpolation is used to assign the value.

#ifndef __vtkTransformStructuredPoints_h
#define __vtkTransformStructuredPoints_h

#include "vtkStructuredPointsToStructuredPointsFilter.hh"
#include "vtkTransform.hh"

class vtkTransformStructuredPoints : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  vtkTransformStructuredPoints();
  char *GetClassName() {return "vtkTransformStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k dimensions to sample input with.
  vtkSetVector3Macro(SampleDimensions,int);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // All voxels not within input structured point set are assigned this value.
  vtkSetMacro(FillValue,float);
  vtkGetMacro(FillValue,float);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Specify object to transform output voxels prior to sampling.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

  unsigned long int GetMTime();

protected:
  void Execute();

  int SampleDimensions[3];
  float FillValue;
  float ModelBounds[6];

  vtkTransform *Transform;
};

#endif


