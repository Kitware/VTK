/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterJavaHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkX3DExporterJavaHelper - create an x3d file
// .SECTION Description

#ifndef __vtkX3DExporterJavaHelper_h
#define __vtkX3DExporterJavaHelper_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkObject.h"

class vtkX3DExporterJavaHelperInternal;

class VTKIOEXPORT_EXPORT vtkX3DExporterJavaHelper : public vtkObject
{
public:
  static vtkX3DExporterJavaHelper *New();
  vtkTypeMacro(vtkX3DExporterJavaHelper,vtkObject);

  int OpenFile(const char* fileName);
  int Write(const char* data, vtkIdType length);
  int Close();

  // Description:
  // Set the location of the FastInfoset JAR file
  static void SetFastInfosetJarLocation(const char* location);

protected:
  vtkX3DExporterJavaHelper();
  ~vtkX3DExporterJavaHelper();


  vtkX3DExporterJavaHelperInternal* Internal;
  static char* FastInfosetJarLocation;

private:
  vtkX3DExporterJavaHelper(const vtkX3DExporterJavaHelper&); // Not implemented.
  void operator=(const vtkX3DExporterJavaHelper&); // Not implemented.
};


#endif

