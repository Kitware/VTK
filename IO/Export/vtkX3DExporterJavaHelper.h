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
/**
 * @class   vtkX3DExporterJavaHelper
 * @brief   create an x3d file
 *
*/

#ifndef vtkX3DExporterJavaHelper_h
#define vtkX3DExporterJavaHelper_h

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

  /**
   * Set the location of the FastInfoset JAR file
   */
  static void SetFastInfosetJarLocation(const char* location);

protected:
  vtkX3DExporterJavaHelper();
  ~vtkX3DExporterJavaHelper();


  vtkX3DExporterJavaHelperInternal* Internal;
  static char* FastInfosetJarLocation;

private:
  vtkX3DExporterJavaHelper(const vtkX3DExporterJavaHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkX3DExporterJavaHelper&) VTK_DELETE_FUNCTION;
};


#endif

