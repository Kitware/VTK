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
// .NAME vtkOBJExporter - export a scene into Wavefront format.
// .SECTION Description
// vtkOBJExporter is a concrete subclass of vtkExporter that writes wavefront
// .OBJ files in ASCII form. It also writes out a mtl file that contains the
// material properties. The filenames are derived by appending the .obj and
// .mtl suffix onto the user specified FilePrefix.
//
// .SECTION See Also
// vtkExporter


#ifndef __vtkOBJExporter_h
#define __vtkOBJExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkActor;

class VTKIOEXPORT_EXPORT vtkOBJExporter : public vtkExporter
{
public:
  static vtkOBJExporter *New();
  vtkTypeMacro(vtkOBJExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the prefix of the files to write out. The resulting filenames
  // will have .obj and .mtl appended to them.
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);

protected:
  vtkOBJExporter();
  ~vtkOBJExporter();

  void WriteData();
  void WriteAnActor(vtkActor *anActor, FILE *fpObj, FILE *fpMat, int &id);
  char *FilePrefix;
private:
  vtkOBJExporter(const vtkOBJExporter&);  // Not implemented.
  void operator=(const vtkOBJExporter&);  // Not implemented.
};

#endif

