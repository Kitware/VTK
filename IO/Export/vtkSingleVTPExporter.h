/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSingleVTPExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSingleVTPExporter
 * @brief   export a scene into a single vtp file and png texture
 *
 * vtkSingleVTPExporter is a concrete subclass of vtkExporter that writes
 * a .vtp file and a .png file containing the polydata and texture
 * elements of the scene.
 *
 * @sa
 * vtkExporter
*/

#ifndef vtkSingleVTPExporter_h
#define vtkSingleVTPExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"
#include <vector> // for method args

class vtkActor;
class vtkTexture;

class VTKIOEXPORT_EXPORT vtkSingleVTPExporter : public vtkExporter
{
public:
  static vtkSingleVTPExporter *New();
  vtkTypeMacro(vtkSingleVTPExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the prefix of the files to write out. The resulting filenames
   * will have .vtp and .png appended to them.
   */
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);
  //@}

protected:
  vtkSingleVTPExporter();
  ~vtkSingleVTPExporter() override;

  void WriteData() override;

  class actorData
  {
  public:
    vtkActor *Actor;
    vtkTexture *Texture;
    int ImagePosition[2];
  };
  int TextureSize[2];
  void WriteTexture(std::vector<actorData> &actors);
  void WriteVTP(std::vector<actorData> &actors);
  char *FilePrefix;

private:
  vtkSingleVTPExporter(const vtkSingleVTPExporter&) = delete;
  void operator=(const vtkSingleVTPExporter&) = delete;
};

#endif
