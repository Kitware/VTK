/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageProperties.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMedicalImageProperties - some medical image properties.
// .SECTION Description
// vtkMedicalImageProperties is a helper class that can be used by medical
// image readers and applications to encapsulate medical image/acquisition
// properties. Later on, this should probably be extended to add
// any user-defined property.
// .SECTION See Also
// vtkMedicalImageReader2

#ifndef __vtkMedicalImageProperties_h
#define __vtkMedicalImageProperties_h

#include "vtkObject.h"

class VTK_IO_EXPORT vtkMedicalImageProperties : public vtkObject
{
public:
  static vtkMedicalImageProperties *New();
  vtkTypeRevisionMacro(vtkMedicalImageProperties,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Convenience method to reset all fields to an emptry string/value
  virtual void Clear();

  // Description:
  // Patient name
  vtkSetStringMacro(PatientName);
  vtkGetStringMacro(PatientName);

  // Description:
  // Patient ID
  vtkSetStringMacro(PatientID);
  vtkGetStringMacro(PatientID);

  // Description:
  // Date
  vtkSetStringMacro(Date);
  vtkGetStringMacro(Date);

  // Description:
  // Series
  vtkSetStringMacro(Series);
  vtkGetStringMacro(Series);

  // Description:
  // Study
  vtkSetStringMacro(Study);
  vtkGetStringMacro(Study);

  // Description:
  // Image number
  vtkSetStringMacro(ImageNumber);
  vtkGetStringMacro(ImageNumber);

  // Description:
  // Modality
  vtkSetStringMacro(Modality);
  vtkGetStringMacro(Modality);

  // Description:
  // Copy the contents of p to this instance. 
  virtual void DeepCopy(vtkMedicalImageProperties *p);
  
protected:
  vtkMedicalImageProperties();
  ~vtkMedicalImageProperties();

  char *PatientName;
  char *PatientID;
  char *Date;
  char *ImageNumber;
  char *Study;
  char *Series;
  char *Modality;
  
private:
  vtkMedicalImageProperties(const vtkMedicalImageProperties&); // Not implemented.
  void operator=(const vtkMedicalImageProperties&); // Not implemented.
};

#endif
