/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageReader2.h
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
// .NAME vtkMedicalImageReader2 - vtkImageReader2 with medical meta data.
// .SECTION Description
// vtkMedicalImageReader2 is a parent class for medical image readers.
// It provides a place to store patient information that may be stored
// in the image header.

// .SECTION See Also
// vtkImageReader2 vtkGESignaReader

#ifndef __vtkMedicalImageReader2_h
#define __vtkMedicalImageReader2_h

#include "vtkImageReader2.h"


class VTK_IO_EXPORT vtkMedicalImageReader2 : public vtkImageReader2
{
public:
  static vtkMedicalImageReader2 *New();
  vtkTypeRevisionMacro(vtkMedicalImageReader2,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Methods to set/get the patient information data.
  vtkSetStringMacro(PatientName);
  vtkGetStringMacro(PatientName);
  vtkSetStringMacro(PatientID);
  vtkGetStringMacro(PatientID);
  vtkSetStringMacro(Date);
  vtkGetStringMacro(Date);
  vtkSetStringMacro(Series);
  vtkGetStringMacro(Series);
  vtkSetStringMacro(Study);
  vtkGetStringMacro(Study);
  vtkSetStringMacro(ImageNumber);
  vtkGetStringMacro(ImageNumber);
  
protected:
  vtkMedicalImageReader2();
  ~vtkMedicalImageReader2();

  // store header info
  char *PatientName;
  char *PatientID;
  char *Date;
  char *ImageNumber;
  char *Study;
  char *Series;
  
private:
  vtkMedicalImageReader2(const vtkMedicalImageReader2&); // Not implemented.
  void operator=(const vtkMedicalImageReader2&); // Not implemented.
};

#endif
