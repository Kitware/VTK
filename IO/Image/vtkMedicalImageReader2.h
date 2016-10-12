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
/**
 * @class   vtkMedicalImageReader2
 * @brief   vtkImageReader2 with medical meta data.
 *
 * vtkMedicalImageReader2 is a parent class for medical image readers.
 * It provides a place to store patient information that may be stored
 * in the image header.
 * @sa
 * vtkImageReader2 vtkGESignaReader vtkMedicalImageProperties
*/

#ifndef vtkMedicalImageReader2_h
#define vtkMedicalImageReader2_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class vtkMedicalImageProperties;

class VTKIOIMAGE_EXPORT vtkMedicalImageReader2 : public vtkImageReader2
{
public:
  static vtkMedicalImageReader2 *New();
  vtkTypeMacro(vtkMedicalImageReader2,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the medical image properties object
   */
  vtkGetObjectMacro(MedicalImageProperties, vtkMedicalImageProperties);
  //@}

  //@{
  /**
   * For backward compatibility, propagate calls to the MedicalImageProperties
   * object.
   */
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
  //@}

protected:
  vtkMedicalImageReader2();
  ~vtkMedicalImageReader2();

  /**
   * Medical Image properties
   */
  vtkMedicalImageProperties *MedicalImageProperties;

private:
  vtkMedicalImageReader2(const vtkMedicalImageReader2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMedicalImageReader2&) VTK_DELETE_FUNCTION;
};

#endif
