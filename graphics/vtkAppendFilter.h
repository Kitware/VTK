/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.h
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
// .NAME vtkAppendFilter - appends one or more datasets together into a single unstructured grid
// .SECTION Description
// vtkAppendFilter is a filter that appends one of more datasets into a single
// unstructured grid. All geometry is extracted and appended, but point 
// attributes (i.e., scalars, vectors, normals) are extracted and appended
// only if all datasets have the point attributes available. (For example, 
// if one dataset has scalars but another does not, scalars will not be 
// appended.)
// .SECTION See Also
// vtkAppendPolyData

#ifndef __vtkAppendFilter_h
#define __vtkAppendFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"
#include "vtkDataSetCollection.h"

class VTK_EXPORT vtkAppendFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkAppendFilter();
  static vtkAppendFilter *New() {return new vtkAppendFilter;};
  char *GetClassName() {return "vtkAppendFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddInput(vtkDataSet *in);
  void AddInput(vtkDataSet& in) {this->AddInput(&in);};
  void RemoveInput(vtkDataSet *in);
  void RemoveInput(vtkDataSet& in) {this->RemoveInput(&in);};
  vtkDataSetCollection *GetInputList() {return &(this->InputList);};

  // filter interface
  void Update();

protected:
  // Usual data generation method
  void Execute();
  // list of data sets to append together
  vtkDataSetCollection InputList;
};

#endif


