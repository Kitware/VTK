/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOOGLExporter.h
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

// .NAME vtkOOGLExporter - export a scene into Geomview OOGL format.
// .SECTION Description
// vtkOOGLExporter is a concrete subclass of vtkExporter that writes
// Geomview OOGL files.
//
// .SECTION See Also
// vtkExporter


#ifndef __vtkOOGLExporter_h
#define __vtkOOGLExporter_h

#include <stdio.h>
#include "vtkExporter.h"

class VTK_RENDERING_EXPORT vtkOOGLExporter : public vtkExporter
{
public:
  static vtkOOGLExporter *New();
  vtkTypeRevisionMacro(vtkOOGLExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the Geomview file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkOOGLExporter();
  ~vtkOOGLExporter();

  void WriteData();
  void WriteALight(vtkLight *aLight, FILE *fp);
  void WriteAnActor(vtkActor *anActor, FILE *fp, int count);
  char *FileName;

private:
  vtkOOGLExporter(const vtkOOGLExporter&);
  void operator=(const vtkOOGLExporter&);
};

#endif

