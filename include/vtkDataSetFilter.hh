/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetFilter.hh
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
// .NAME vtkDataSetFilter - filter that takes vtkDataSet as input
// .SECTION Description
// vtkDataSetFilter is a filter that takes a single vtkDataSet data object
// as input.

// SECTION See Also
// vtkDataSetToDataSetFilter vtkDataSetToPolyFilter vtkDataSetToStructuredGridFilter
// vtkOBBFilter vtkSpatialRepFilter

#ifndef __vtkDataSetFilter_h
#define __vtkDataSetFilter_h

#include "vtkFilter.hh"
#include "vtkDataSet.hh"

class vtkDataSetFilter : public vtkFilter 
{
public:
  vtkDataSetFilter() {};
  char *GetClassName() {return "vtkDataSetFilter";};

  virtual void SetInput(vtkDataSet *input);
  void SetInput(vtkDataSet &input) {this->SetInput(&input);};
  vtkDataSet *GetInput() {return this->Input;};
};

#endif


