/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIVExporter - export a scene into OpenInventor 2.0 format.
// .SECTION Description
// vtkIVExporter is a concrete subclass of vtkExporter that writes
// OpenInventor 2.0 files.
//
// .SECTION See Also
// vtkExporter


#ifndef __vtkIVExporter_h
#define __vtkIVExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkLight;
class vtkActor;
class vtkPoints;
class vtkDataArray;
class vtkUnsignedCharArray;

class VTKIOEXPORT_EXPORT vtkIVExporter : public vtkExporter
{
public:
  static vtkIVExporter *New();
  vtkTypeMacro(vtkIVExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the OpenInventor file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkIVExporter();
  ~vtkIVExporter();

  void WriteData();
  void WriteALight(vtkLight *aLight, FILE *fp);
  void WriteAnActor(vtkActor *anActor, FILE *fp);
  void WritePointData(vtkPoints *points, vtkDataArray *normals,
                      vtkDataArray *tcoords, vtkUnsignedCharArray *colors,
                      FILE *fp);
  char *FileName;
private:
  vtkIVExporter(const vtkIVExporter&);  // Not implemented.
  void operator=(const vtkIVExporter&);  // Not implemented.
};

#endif

