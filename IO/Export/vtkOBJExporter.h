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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the prefix of the files to write out. The resulting filenames
   * will have .obj and .mtl appended to them.
   */
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
  * Specify comment string that will be written to the obj file header.
  */
  vtkSetStringMacro(OBJFileComment);
  vtkGetStringMacro(OBJFileComment);
  //@}

  //@{
  /**
  * Specify comment string that will be written to the mtl file header.
  */
  vtkSetStringMacro(MTLFileComment);
  vtkGetStringMacro(MTLFileComment);
  //@}

protected:
  vtkOBJExporter();
  ~vtkOBJExporter() override;

  void WriteData() override;
  void WriteAnActor(vtkActor *anActor, FILE *fpObj, FILE *fpMat, int &id);
  char *FilePrefix;
  char *OBJFileComment;
  char *MTLFileComment;
private:
  vtkOBJExporter(const vtkOBJExporter&) = delete;
  void operator=(const vtkOBJExporter&) = delete;
};

#endif

