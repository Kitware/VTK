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
 * If ActiveRenderer is specified then it exports contents of
 * ActiveRenderer. Otherwise it exports contents of all renderers.
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
class vtkPolyData;
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

  // computes the file prefix from a filename by removing
  // the .vtp extension if present. Useful for APIs that
  // are filename centric.
  void SetFileName(const char *);

protected:
  vtkSingleVTPExporter();
  ~vtkSingleVTPExporter() override;

  void WriteData() override;

  class actorData
  {
  public:
    vtkActor *Actor = nullptr;
    vtkTexture *Texture = nullptr;
    int ImagePosition[2];
    double URange[2];
    double VRange[2];
    bool HaveRepeatingTexture = false;
  };
  int TextureSize[2];
  void WriteTexture(std::vector<actorData> &actors);
  void WriteVTP(std::vector<actorData> &actors);
  char *FilePrefix;

  // handle repeating textures by subdividing triangles
  // so that they do not span mode than 0.0-1.5 of texture
  // range.
  vtkPolyData *FixTextureCoordinates(vtkPolyData *);

  // recursive method that handles one triangle
  void ProcessTriangle(vtkIdType *pts, vtkPolyData *out);

private:
  vtkSingleVTPExporter(const vtkSingleVTPExporter&) = delete;
  void operator=(const vtkSingleVTPExporter&) = delete;
};

#endif
