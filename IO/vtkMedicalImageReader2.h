/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageReader2.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
