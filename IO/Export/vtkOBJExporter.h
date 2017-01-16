/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOBJExporter
 * @brief   export a scene into Wavefront format.
 *
 * vtkOBJExporter is a concrete subclass of vtkExporter that writes wavefront
 * .OBJ files in ASCII form. It also writes out a mtl file that contains the
 * material properties. The filenames are derived by appending the .obj and
 * .mtl suffix onto the user specified FilePrefix.
 *
 * @sa
 * vtkExporter
*/

#ifndef vtkOBJExporter_h
#define vtkOBJExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkActor;

class VTKIOEXPORT_EXPORT vtkOBJExporter : public vtkExporter
{
public:
  static vtkOBJExporter *New();
  vtkTypeMacro(vtkOBJExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify the prefix of the files to write out. The resulting filenames
   * will have .obj and .mtl appended to them.
   */
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);
  //@}

protected:
  vtkOBJExporter();
  ~vtkOBJExporter() VTK_OVERRIDE;

  void WriteData() VTK_OVERRIDE;
  void WriteAnActor(vtkActor *anActor, FILE *fpObj, FILE *fpMat, int &id);
  char *FilePrefix;
private:
  vtkOBJExporter(const vtkOBJExporter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOBJExporter&) VTK_DELETE_FUNCTION;
};

#endif

