/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageReader2.cxx
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
#include "vtkMedicalImageReader2.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMedicalImageReader2, "1.1");
vtkStandardNewMacro(vtkMedicalImageReader2);

vtkMedicalImageReader2::vtkMedicalImageReader2()
{
  this->PatientName = 0;
  this->PatientID = 0;
  this->Date = 0;
  this->ImageNumber = 0;
  this->Series = 0;
  this->Study = 0;
}

vtkMedicalImageReader2::~vtkMedicalImageReader2()
{
  delete [] this->PatientName;
  this->PatientName = NULL;
  delete [] this->PatientID;
  this->PatientID = NULL;
  delete [] this->Date;
  this->Date = NULL;
  delete [] this->ImageNumber;
  this->ImageNumber = NULL;
  delete [] this->Series;
  this->Series = NULL;
  delete [] this->Study;
  this->Study = NULL;
}

void vtkMedicalImageReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "\n" << indent << "PatientName: ";
  if (this->PatientName)
    {
    os << this->PatientName;
    }
  os << "\n" << indent << "PatientID: ";
  if (this->PatientID)
    {
    os << this->PatientID;
    }
  os << "\n" << indent << "Date: ";
  if (this->Date)
    {
    os << this->Date;
    }
  os << "\n" << indent << "ImageNumber: ";
  if (this->ImageNumber)
    {
    os << this->ImageNumber;
    }
  os << "\n" << indent << "Series: ";
  if (this->Series)
    {
    os << this->Series;
    }
  os << "\n" << indent << "Study: ";
  if (this->Study)
    {
    os << this->Study;
    }
}
