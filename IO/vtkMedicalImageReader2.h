/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// vtkImageReader2 vtkGESignaReader vtkMedicalImageProperties

#ifndef __vtkMedicalImageReader2_h
#define __vtkMedicalImageReader2_h

#include "vtkImageReader2.h"

class vtkMedicalImageProperties;

class VTK_IO_EXPORT vtkMedicalImageReader2 : public vtkImageReader2
{
public:
  static vtkMedicalImageReader2 *New();
  vtkTypeMacro(vtkMedicalImageReader2,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Get the medical image properties object
  vtkGetObjectMacro(MedicalImageProperties, vtkMedicalImageProperties);
  
  // Description:
  // For backward compatibility, propagate calls to the MedicalImageProperties
  // object.
  virtual void SetPatientName(const char*);
  virtual const char* GetPatientName();
  virtual void SetPatientID(const char*);
  virtual const char* GetPatientID();
  virtual void SetDate(const char*);
  virtual const char* GetDate();
  virtual void SetSeries(const char*);
  virtual const char* GetSeries();
  virtual void SetStudy(const char*);
  virtual const char* GetStudy();
  virtual void SetImageNumber(const char*);
  virtual const char* GetImageNumber();
  virtual void SetModality(const char*);
  virtual const char* GetModality();
  
protected:
  vtkMedicalImageReader2();
  ~vtkMedicalImageReader2();

  // Description:
  // Medical Image properties
  vtkMedicalImageProperties *MedicalImageProperties;

private:
  vtkMedicalImageReader2(const vtkMedicalImageReader2&); // Not implemented.
  void operator=(const vtkMedicalImageReader2&); // Not implemented.
};

#endif
