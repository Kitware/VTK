/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.h
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
// .NAME vtkExtractVectorComponents - extract components of vector as separate scalars
// .SECTION Description
// vtkExtractVectorComponents is a filter that extracts vector components as
// separate scalars. This is accomplished by creating three different outputs.
// Each output is the same as the input, except that the scalar values will be
// one of the three components of the vector. These can be found in the
// VxComponent, VyComponent, and VzComponent.

// .SECTION Caveats
// This filter is unusual in that it creates multiple outputs. As a result,
// it cannot take advantage of the convenience classes (e.g., 
// vtkPolyToPolyFilter) for deriving concrete filters. Instead, it overloads 
// the Update() method of its superclasses and provides methods for retrieving
// the output.
//
// If you use the GetOutput() method, you will be retrieving the x vector component.

#ifndef __vtkExtractVectorComponents_h
#define __vtkExtractVectorComponents_h

#include "vtkFilter.h"

class VTK_EXPORT vtkExtractVectorComponents : public vtkFilter
{
public:
  vtkExtractVectorComponents();
  vtkExtractVectorComponents *New() {return new vtkExtractVectorComponents;};
  char *GetClassName() {return "vtkExtractVectorComponents";};

  // filter interface (need to overload because of multiple output
  void Update();
  virtual void SetInput(vtkDataSet *input);
  void SetInput(vtkDataSet &input) {this->SetInput(&input);};

  vtkDataSet *GetVxComponent();
  vtkDataSet *GetVyComponent();
  vtkDataSet *GetVzComponent();

  vtkDataSet *GetOutput(int i=0); //default extracts vector component.

protected:
  void Execute();

  //Note; inverited ivar Output serves as pointer to VxComponent
  vtkDataSet *VyComponent;
  vtkDataSet *VzComponent;
};

#endif


