/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkVectorNorm - generate scalars from Euclidean norm of vectors
// .SECTION Description
// vtkVectorNorm is a filter that generates scalar values by computing
// Euclidean norm of vector triplets. Scalars can be normalized 
// 0<=s<=1 if desired.
//
// Note that this filter operates on point or cell attribute data, or
// both.  By default, the filter operates on both point and cell data
// if vector point and cell data, respectively, are available from the
// input. Alternatively, you can choose to generate scalar norm values
// for just cell or point data.

#ifndef __vtkVectorNorm_h
#define __vtkVectorNorm_h

#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkVectorNorm : public vtkDataSetToDataSetFilter 
{
public:

// Description:
// Construct with normalize flag off.
  vtkVectorNorm();

  static vtkVectorNorm *New() {return new vtkVectorNorm;};
  const char *GetClassName() {return "vtkVectorNorm";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify whether to normalize scalar values.
  vtkSetMacro(Normalize,int);
  vtkGetMacro(Normalize,int);
  vtkBooleanMacro(Normalize,int);

  // Description:
  // Control how the filter works to generate scalar data from the
  // input vector data. By default, (AttributeModeToDefault) the
  // filter will generate the scalar norm for point and cell data (if
  // vector data present in the input). Alternatively, you can
  // explicitly set the filter to generate point data
  // (AttributeModeToUsePointData) or cell data
  // (AttributeModeToUseCellData).
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  char *GetAttributeModeAsString();

protected:
  void Execute();

  int Normalize;  // normalize 0<=n<=1 if true.
  int AttributeMode; //control whether to use point or cell data, or both
};

#endif


