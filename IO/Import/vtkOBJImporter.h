// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOBJImporter
 * @brief   import from .obj wavefront files
 *
 * This importer doesn't support scene hierarchy API
 * This importer supports reading streams
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
   * Specify the name of the file or the stream to read as MTL file.
   * Stream or FileNameMTL can be provided, if both are provided, Stream will be used.
   * if none is provided, we will do, in order:
   *  - Use mtllib is provided in the .obj file
   *  - Check for a FileName.mtl and use it if it exists
   *  - Check for a FileStem.mtl and use it if it exists
   */
  void SetMTLStream(vtkResourceStream* stream);
  void SetFileNameMTL(VTK_FILEPATH const char* arg);
  VTK_FILEPATH const char* GetFileNameMTL() const;
  ///@}

  ///@{

  /**
   * Set TexturePath or TextureStreams.
   * TextureStreams is a map where the filename are the string keys.
   * If both are provided, the streams will be used.
   * If none is provided, the folder containing FileName will be used
   */
  void SetTextureStreams(std::map<std::string, vtkResourceStream*> streamMap);
  void SetTexturePath(VTK_FILEPATH const char* path);
  VTK_FILEPATH const char* GetTexturePath() const;
  ///@}

  ///@{
  /**
   * Return true if, after a quick check of file header, it looks like the provided stream
   * can be read. Return false if it is sure it cannot be read. The stream version can move the
   * stream cursor, the filename version calls the stream version.
   *
   * This only check that the first non-commented non-empty line start with either of:
   * "mtllib "
   * "usemtl "
   * "v "
   * "vt "
   * "vn "
   * "p "
   * "l "
   * "f "
   * "o "
   * "s "
   */
  static bool CanReadFile(const std::string& filename);
  static bool CanReadFile(vtkResourceStream* stream);
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

private:
  vtkOBJImporter(const vtkOBJImporter&) = delete;
  void operator=(const vtkOBJImporter&) = delete;

  vtkSmartPointer<vtkOBJPolyDataProcessor> Impl;
};

VTK_ABI_NAMESPACE_END
#endif
