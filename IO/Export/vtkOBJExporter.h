// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro
#include <fstream>             // For ofstream
#include <map>                 // For map
#include <vector>              // For string

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkTexture;

class VTKIOEXPORT_EXPORT vtkOBJExporter : public vtkExporter
{
public:
  static vtkOBJExporter* New();
  vtkTypeMacro(vtkOBJExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the prefix of the files to write out. The resulting filenames
   * will have .obj and .mtl appended to them.
   */
  vtkSetFilePathMacro(FilePrefix);
  vtkGetFilePathMacro(FilePrefix);
  ///@}

  ///@{
  /**
   * Specify comment string that will be written to the obj file header.
   */
  vtkSetStringMacro(OBJFileComment);
  vtkGetStringMacro(OBJFileComment);
  ///@}

  ///@{
  /**
   * Specify comment string that will be written to the mtl file header.
   */
  vtkSetStringMacro(MTLFileComment);
  vtkGetStringMacro(MTLFileComment);
  ///@}

protected:
  vtkOBJExporter();
  ~vtkOBJExporter() override;

  void WriteData() override;
  void WriteAnActor(
    vtkActor* anActor, std::ostream& fpObj, std::ostream& fpMat, std::string& modelName, int& id);
  char* FilePrefix;
  char* OBJFileComment;
  char* MTLFileComment;
  bool FlipTexture;
  std::map<std::string, vtkTexture*> TextureFileMap;

private:
  vtkOBJExporter(const vtkOBJExporter&) = delete;
  void operator=(const vtkOBJExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
