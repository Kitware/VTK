/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkStructuredPoints - A subclass of ImageData.
// .SECTION Description
// StructuredPoints is a subclass of ImageData that requires the data extent
// to exactly match the update extent. Normall image data allows that the
// data extent may be larger than the update extent.
// StructuredPoints also defines the origin differently that vtkImageData.
// For structured points the origin is the location of first point. 
// Whereas images define the origin as the location of point 0, 0, 0.
// Image Origin is stored in ivar, and structured points
// have special methods for setting/getting the origin/extents.


#ifndef __vtkStructuredPoints_h
#define __vtkStructuredPoints_h

#include "vtkImageData.h"

  
class VTK_COMMON_EXPORT vtkStructuredPoints : public vtkImageData
{
public:
  static vtkStructuredPoints *New();
  vtkTypeMacro(vtkStructuredPoints,vtkImageData);
  
  // Description:
  // Create a similar type object
  vtkDataObject *MakeObject() {return vtkStructuredPoints::New();}

  // Description:
  // To simplify filter superclasses,
  int GetDataObjectType() {return VTK_STRUCTURED_POINTS;}

protected:
  vtkStructuredPoints();
  ~vtkStructuredPoints() {};
  vtkStructuredPoints(const vtkStructuredPoints&);
  void operator=(const vtkStructuredPoints&);
};

#endif



