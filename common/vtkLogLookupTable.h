/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogLookupTable.h
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
// .NAME vtkLogLookupTable - map scalar values into colors using logarithmic (base 10) color table
// .SECTION Description
// vtkLogLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The difference between this class and its
// superclass vtkLookupTable is that this class performs scalar mapping based
// on a logarithmic lookup process. (Uses log base 10.)
//
// If non-positive ranges are encountered, then they are converted to 
// positive values using absolute value.
// .SECTION See Also
// vtkLookupTable

#ifndef __vtkLogLookupTable_h
#define __vtkLogLookupTable_h

#include "vtkLookupTable.h"

class vtkLogLookupTable : public vtkLookupTable
{
public:
  vtkLogLookupTable(int sze=256, int ext=256);
  char *GetClassName() {return "vtkLogLookupTable";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetTableRange(float min, float max);
  unsigned char *MapValue(float v);

protected:
  float LogMinRange;
  float LogMaxRange;
  float UseAbsoluteValue;
};

#endif
