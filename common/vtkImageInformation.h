/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInformation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageInformation - Image specific info (like spacing).
// .SECTION Description
// Note:  This object is under development an might change in the future.


#ifndef __vtkImageInformation_h
#define __vtkImageInformation_h

#include "vtkStructuredInformation.h"


class VTK_EXPORT vtkImageInformation : public vtkStructuredInformation
{
public:
  static vtkImageInformation *New();

  vtkTypeMacro(vtkImageInformation,vtkStructuredInformation);
  void PrintSelf(vtkOstream& os, vtkIndent indent);

  // Description:
  // Makes an empty similar type object.
  vtkDataInformation *MakeObject() 
    {return vtkImageInformation::New();}
  
  // Description:
  // Subclasses override this method, and try to be smart
  // if the types are different.
  void Copy(vtkDataInformation *in);
  
  // Description:
  // Set the spacing (width,height,length) of the cubical cells that
  // compose the data set.
  vtkSetVector3Macro(Spacing,float);
  vtkGetVectorMacro(Spacing,float,3);
  
  // Description:
  // Set the origin of the data. The origin plus spacing determine the
  // position in space of the points.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);
  
  // Description:
  // Set/Get the data scalar type (i.e VTK_FLOAT).
  vtkSetMacro(ScalarType, int);
  vtkGetMacro(ScalarType, int);

  // Description:
  // Set/Get the number of scalar components for points.
  vtkSetMacro(NumberOfScalarComponents,int);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  // Description:
  // This method is passed a ClassName and returns 1 if the object is
  // a subclass of the class arg.  It is an attempt at making a smarter copy.
  int GetClassCheck(char *className);
  
  // Description:
  // Serialization provided for the multi-process ports.
  void ReadSelf(vtkIstream& is);
  void WriteSelf(vtkOstream& os);

protected:
  vtkImageInformation();
  ~vtkImageInformation() {};
  vtkImageInformation(vtkImageInformation&) {};
  void operator=(vtkImageInformation&) {};

  float Origin[3];
  float Spacing[3];
  int ScalarType;
  int NumberOfScalarComponents;
};


#endif




