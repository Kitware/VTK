/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.h
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
// .NAME vtkMergeFilter - extract separate components of data from different datasets
// .SECTION Description
// vtkMergeFilter is a filter that extracts separate components of data from
// different datasets and merges them into a single dataset. The output from
// this filter is of the same type as the input (i.e., vtkDataSet.)

#ifndef __vtkMergeFilter_h
#define __vtkMergeFilter_h

#include "vtkDataSet.h"
#include "vtkFilter.h"

class VTK_EXPORT vtkMergeFilter : public vtkFilter
{
public:
  vtkMergeFilter();
  static vtkMergeFilter *New() {return new vtkMergeFilter;};
  char *GetClassName() {return "vtkMergeFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Filter interface
  void Update();

  // Description:
  // Specify object from which to extract geometry information.
  void SetGeometry(vtkDataSet *input);
  void SetGeometry(vtkDataSet &input) {this->SetGeometry(&input);};
  vtkDataSet *GetGeometry() {return this->Input;};

  // Description:
  // Get the output of this source.
  vtkDataSet *GetOutput() {return this->Output;};

  // Description:
  // Specify object from which to extract scalar information.
  vtkSetObjectMacro(Scalars,vtkDataSet);
  vtkGetObjectMacro(Scalars,vtkDataSet);
  
  // Description:
  // Specify object from which to extract vector information.
  vtkSetObjectMacro(Vectors,vtkDataSet);
  vtkGetObjectMacro(Vectors,vtkDataSet);
  
  // Description:
  // Specify object from which to extract normal information.
  vtkSetObjectMacro(Normals,vtkDataSet);
  vtkGetObjectMacro(Normals,vtkDataSet);
  
  // Description:
  // Specify object from which to extract texture coordinates information.
  vtkSetObjectMacro(TCoords,vtkDataSet);
  vtkGetObjectMacro(TCoords,vtkDataSet);

  // Description:
  // Specify object from which to extract tensor data.
  vtkSetObjectMacro(Tensors,vtkDataSet);
  vtkGetObjectMacro(Tensors,vtkDataSet);

  // Description:
  // Specify object from which to extract user defined data.
  vtkSetObjectMacro(UserDefined,vtkDataSet);
  vtkGetObjectMacro(UserDefined,vtkDataSet);

protected:
  // Usual data generation method
  void Execute();

  vtkDataSet *Geometry; // output geometry
  vtkDataSet *Scalars;  // scalars to merge
  vtkDataSet *Vectors;  // vectors
  vtkDataSet *Normals;  // normals
  vtkDataSet *TCoords;  // texture coords
  vtkDataSet *Tensors;  // tensors
  vtkDataSet *UserDefined;  // user defined
};

#endif


