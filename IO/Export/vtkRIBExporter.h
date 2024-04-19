// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRIBExporter
 * @brief   export a scene into RenderMan RIB format.
 *
 * vtkRIBExporter is a concrete subclass of vtkExporter that writes a
 * Renderman .RIB files. The input specifies a vtkRenderWindow. All
 * visible actors and lights will be included in the rib file. The
 * following file naming conventions apply:
 *   rib file - FilePrefix.rib
 *   image file created by RenderMan - FilePrefix.tif
 *   texture files - TexturePrefix_0xADDR_MTIME.tif
 * This object does NOT generate an image file. The user must run either
 * RenderMan or a RenderMan emulator like Blue Moon Ray Tracer (BMRT).
 * vtk properties are convert to Renderman shaders as follows:
 *   Normal property, no texture map - plastic.sl
 *   Normal property with texture map - txtplastic.sl
 * These two shaders must be compiled by the rendering package being
 * used.  vtkRIBExporter also supports custom shaders. The shaders are
 * written using the Renderman Shading Language. See "The Renderman
 * Companion", ISBN 0-201-50868, 1989 for details on writing shaders.
 * vtkRIBProperty specifies the declarations and parameter settings for
 * custom shaders.
 *
 * @sa
 * vtkExporter vtkRIBProperty vtkRIBLight
 */

#ifndef vtkRIBExporter_h
#define vtkRIBExporter_h

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkCamera;
class vtkLight;
class vtkPolyData;
class vtkProperty;
class vtkRenderer;
class vtkTexture;
class vtkUnsignedCharArray;

class VTKIOEXPORT_EXPORT vtkRIBExporter : public vtkExporter
{
public:
  static vtkRIBExporter* New();
  vtkTypeMacro(vtkRIBExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the size of the image for RenderMan. If none is specified, the
   * size of the render window will be used.
   */
  vtkSetVector2Macro(Size, int);
  vtkGetVectorMacro(Size, int, 2);
  ///@}

  ///@{
  /**
   * Specify the sampling rate for the rendering. Default is 2 2.
   */
  vtkSetVector2Macro(PixelSamples, int);
  vtkGetVectorMacro(PixelSamples, int, 2);
  ///@}

  ///@{
  /**
   * Specify the prefix of the files to write out. The resulting file names
   * will have .rib appended to them.
   */
  vtkSetFilePathMacro(FilePrefix);
  vtkGetFilePathMacro(FilePrefix);
  ///@}

  ///@{
  /**
   * Specify the prefix of any generated texture files.
   */
  vtkSetStringMacro(TexturePrefix);
  vtkGetStringMacro(TexturePrefix);
  ///@}

  ///@{
  /**
   * Set/Get the background flag. Default is 0 (off).
   * If set, the rib file will contain an
   * image shader that will use the renderer window's background
   * color. Normally, RenderMan does generate backgrounds. Backgrounds are
   * composited into the scene with the tiffcomp program that comes with
   * Pixar's RenderMan Toolkit.  In fact, Pixar's Renderman will accept an
   * image shader but only sets the alpha of the background. Images created
   * this way will still have a black background but contain an alpha of 1
   * at all pixels and CANNOT be subsequently composited with other images
   * using tiffcomp.  However, other RenderMan compliant renderers like
   * Blue Moon Ray Tracing (BMRT) do allow image shaders and properly set
   * the background color. If this sounds too confusing, use the following
   * rules: If you are using Pixar's Renderman, leave the Background
   * off. Otherwise, try setting BackGroundOn and see if you get the
   * desired results.
   */
  vtkSetMacro(Background, vtkTypeBool);
  vtkGetMacro(Background, vtkTypeBool);
  vtkBooleanMacro(Background, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set or get the ExportArrays. If ExportArrays is set, then
   * all point data, field data, and cell data arrays will get
   * exported together with polygons. Default is Off (0).
   */
  vtkSetClampMacro(ExportArrays, vtkTypeBool, 0, 1);
  vtkBooleanMacro(ExportArrays, vtkTypeBool);
  vtkGetMacro(ExportArrays, vtkTypeBool);
  ///@}

protected:
  vtkRIBExporter();
  ~vtkRIBExporter() override;

  vtkTypeBool Background;
  int Size[2];
  int PixelSamples[2];

  /**
   * This variable defines whether the arrays are exported or not.
   */
  vtkTypeBool ExportArrays;

  ///@{
  /**
   * Write the RIB header.
   */
  void WriteHeader(vtkRenderer* aRen);
  void WriteTrailer();
  void WriteTexture(vtkTexture* aTexture);
  void WriteViewport(vtkRenderer* aRenderer, int size[2]);
  void WriteCamera(vtkCamera* aCamera);
  void WriteLight(vtkLight* aLight, int count);
  void WriteAmbientLight(int count);
  void WriteProperty(vtkProperty* aProperty, vtkTexture* aTexture);
  void WritePolygons(vtkPolyData* pd, vtkUnsignedCharArray* colors, vtkProperty* aProperty);
  void WriteStrips(vtkPolyData* pd, vtkUnsignedCharArray* colors, vtkProperty* aProperty);
  ///@}

  void WriteData() override;
  void WriteActor(vtkActor* anActor);

  /**
   * Since additional variables are sent to the shader as
   * variables, and their names are used in the shader, these
   * names have to follow C naming convention. This method
   * modifies array name so that you can use it in shader.
   */
  void ModifyArrayName(char* newname, const char* name);

  char* GetTextureName(vtkTexture* aTexture);
  char* GetTIFFName(vtkTexture* aTexture);
  char* FilePrefix;
  FILE* FilePtr;
  char* TexturePrefix;

private:
  vtkRIBExporter(const vtkRIBExporter&) = delete;
  void operator=(const vtkRIBExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
