/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.h
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
// .NAME vtkReverseSense - reverse the ordering of polygonal cells and/or vertex normals
// .SECTION Description
// vtkReverseSense is a filter that reverses the order of polygonal cells
// and/or reverses the direction of vertex normals. Two flags are used to
// control these operations. Cell reversal means reversing the order of 
// indices in the cell connectivity list. Normal reversal means multiplying
// the normal vector by -1.

// .SECTION Caveats
// Normals can be operated on only if they are present in the data.

#ifndef __vtkReverseSense_h
#define __vtkReverseSense_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkReverseSense : public vtkPolyDataToPolyDataFilter
{
public:
  vtkReverseSense();
  static vtkReverseSense *New() {return new vtkReverseSense;};
  char *GetClassName() {return "vtkReverseSense";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Flag controls whether to reverse cell ordering.
  vtkSetMacro(ReverseCells,int);
  vtkGetMacro(ReverseCells,int);
  vtkBooleanMacro(ReverseCells,int);

  // Description:
  // Flag controls whether to reverse normal orientation.
  vtkSetMacro(ReverseNormals,int);
  vtkGetMacro(ReverseNormals,int);
  vtkBooleanMacro(ReverseNormals,int);


protected:
  // Usual data generation method
  void Execute();

  int ReverseCells;
  int ReverseNormals;
};

#endif


