/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2DSF.h
  Language:  C++
  Date:      2/17/94
  Version:   1.8


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
// .NAME vtkDataSetToDataSetFilter - abstract filter class
// .SECTION Description
// vtkDataSetToDataSetFilter is an abstract filter class. Subclasses of 
// vtkDataSetToDataSetFilter take a dataset as input and create a dataset 
// as output. The form of the input geometry is not changed in these 
// filters, only the point attributes (e.g. scalars, vectors, etc.).

// .SECTION See Also
// vtkBrownianPoints vtkProbeFilter vtkThresholdTextureCoords vtkDicer
// vtkElevationFilter vtkImplicitTextureCoords vtkTextureMapToBox vtkTextureMapToPlane
// vtkVectorDot vtkVectorNorm

#ifndef __vtkDataSetToDataSetFilter_h
#define __vtkDataSetToDataSetFilter_h

#include "vtkDataSetFilter.h"
#include "vtkDataSet.h"

class VTK_EXPORT vtkDataSetToDataSetFilter : public vtkDataSetFilter
{

public:
  char *GetClassName() {return "vtkDataSetToDataSetFilter";};
  vtkDataSetToDataSetFilter() {this->Output = NULL;};

  void SetInput(vtkDataSet *input);

  // filter interface (need to overload because of abstract interface)
  void Update();

  vtkDataSet *GetOutput();
};

#endif
