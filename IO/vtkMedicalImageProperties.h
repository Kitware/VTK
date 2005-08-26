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

class vtkMedicalImagePropertiesInternals;

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
  // Image Date
  vtkSetStringMacro(ImageDate);
  vtkGetStringMacro(ImageDate);

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

  // Description:
  // Add/Remove/Query the window/level presets that may have been associated
  // to a medical image.
  virtual void AddWindowLevelPreset(double w, double l);
  virtual void RemoveWindowLevelPreset(double w, double l);
  virtual void RemoveAllWindowLevelPresets();
  virtual int GetNumberOfWindowLevelPresets();
  virtual int HasWindowLevelPreset(double w, double l);
  virtual int GetNthWindowLevelPreset(int idx, double *w, double *l);
  virtual double* GetNthWindowLevelPreset(int idx);
  
protected:
  vtkMedicalImageProperties();
  ~vtkMedicalImageProperties();

  char *PatientName;
  char *PatientID;
  char *ImageDate;
  char *ImageNumber;
  char *Study;
  char *Series;
  char *Modality;

  // Description:
  // PIMPL Encapsulation for STL containers
  //BTX
  vtkMedicalImagePropertiesInternals *Internals;
  //ETX
  
private:
  vtkMedicalImageProperties(const vtkMedicalImageProperties&); // Not implemented.
  void operator=(const vtkMedicalImageProperties&); // Not implemented.
};

#endif
