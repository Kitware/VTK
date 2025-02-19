// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOBJImporter
 * @brief   import from .obj wavefront files
 *
 * This importer doesn't support scene hierarchy API
 *
 * This importer supports the collection API
 *
 *                        from Wavefront .obj & associated .mtl files.
 * @par Thanks - Peter Karasev (Georgia Tech / Keysight Technologies Inc),:
 *                   Allen Tannenbaum (SUNY Stonybrook), Patricio Vela (Georgia Tech)
 * @sa
 *  vtkImporter
 */

#ifndef vtkOBJImporter_h
#define vtkOBJImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"
#include "vtkSmartPointer.h" // for ivars
#include <string>            // for string

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindow;
class vtkRenderer;
class vtkPolydata;
class vtkOBJPolyDataProcessor;

/** @note{updated by peter karasev, 2015 to read texture coordinates + material properties}
    @note{An example of a supported (*).mtl file is show below.
             Lighting values and texture images are specified, and a corresponding vtkActor
             with properties and vtkTexture will be created upon import. }

        # Wavefront material file saved from Meshlab
        newmtl material_0
        Ka 0.400000 0.400000 0.400000
        Kd 0.5 0.5 0.5
        Ks 0.85 0.9 0.9
        illum 2
        Ns 0.000000
        map_Kd map1024.png

        newmtl material_1
        Ka 0.200000 0.200000 0.200000
        Kd 0.666667 0.666667 0.666667
        Ks 1.000000 0.9 1.000000
        illum 2
        Ns 0.000000
        map_Kd flare.jpg

   **/
class VTKIOIMPORT_EXPORT vtkOBJImporter : public vtkImporter
{
public:
  static vtkOBJImporter* New();

  vtkTypeMacro(vtkOBJImporter, vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the file to read.
   * FileName must be provided.
   * FileNameMTL can be provided, if not provided, we will do, in order:
   *  - Use mtllib is provided in the .obj file
   *  - Check for a FileName.mtl and use it if it exists
   *  - Check for a FileStem.mtl and use it if it exists
   * TexturePath can be provided, it not provided, the folder containing FileName will be used
   */
  void SetFileName(VTK_FILEPATH const char* arg);
  void SetFileNameMTL(VTK_FILEPATH const char* arg);
  void SetTexturePath(VTK_FILEPATH const char* path);
  VTK_FILEPATH const char* GetFileName() const;
  VTK_FILEPATH const char* GetFileNameMTL() const;
  VTK_FILEPATH const char* GetTexturePath() const;
  ///@}

  /**
   * Get a printable string describing all outputs
   */
  std::string GetOutputsDescription() override;

  /**
   * Get a string describing an output
   */
  std::string GetOutputDescription(int idx);

protected:
  vtkOBJImporter();
  ~vtkOBJImporter() override;

  int ImportBegin() override /*override*/;
  void ImportEnd() override /*override*/;
  void ReadData() override /* override */;

  vtkSmartPointer<vtkOBJPolyDataProcessor> Impl;

private:
  vtkOBJImporter(const vtkOBJImporter&) = delete;
  void operator=(const vtkOBJImporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
