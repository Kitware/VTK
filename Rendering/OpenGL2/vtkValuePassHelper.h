/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePassHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkValuePassHelper
 *
 *
 * Implementation of both rendering modes of vtkValuePass for the
 * vtkOpenGLPolyDataMapper. The mapper is intended to call various methods
 * of this helper in order to setup the appropriate rendering state.
 *
 * * INVERTIBLE_LUT  It uses a texture as a color LUT to map the values to RGB
 * data. Texture size constraints limit its precision (currently 12-bit). Most
 * of the implementation of this mode is in vtkMapper (InvertibleLookupTable)
 * where the color LUT is generated.
 *
 * * FLOATING_POINT Resources are allocated on demand. When rendering point data
 * values are uploaded to the GPU as vertex attributes. When rendering cell data
 * values are uploaded as a texture buffer. Custom vertex and fragment shaders are
 * defined in order to adjust its behavior for either type of data.
 *
 * @sa
 * vtkValuePass vtkOpenGLPolyDataMapper vtkMapper
*/

#ifndef vtkValuePassHelper_h
#define vtkValuePassHelper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"


class vtkActor;
class vtkDataSet;
class vtkMapper;
class vtkOpenGLHelper;
class vtkRenderer;
class vtkWindow;
class vtkPolyData;

class VTKRENDERINGOPENGL2_EXPORT vtkValuePassHelper : public vtkObject
{
  friend class vtkOpenGLPolyDataMapper;

public:

  static vtkValuePassHelper* New();
  vtkTypeMacro(vtkValuePassHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkValuePassHelper();
  ~vtkValuePassHelper();

  vtkGetMacro(RenderingMode, int);

  /**
   * Configure the internal state depending on the settings defined by the
   * vtkValuePass (RenderingMode,  RENDER_VALUES, SCALAR_MODE, etc.).
   */
  void UpdateConfiguration(vtkRenderer* ren, vtkActor* act, vtkMapper* mapper,
    vtkPolyData* input);

  /**
   * Upload new data if necessary, bind textures, etc.
   */
  void RenderPieceStart(vtkActor* actor, vtkDataSet* input);

  /**
   * Add necessary shader definitions.
   */
  void UpdateShaders(std::string & VSSource, std::string & FSSource,
    std::string & required);

  //@{
  /**
   * Bind shader variables.
   */
  void BindAttributes(vtkOpenGLHelper& cellBO);
  void BindUniforms(vtkOpenGLHelper& cellBO);
  //@}

  /**
   * Unbind textures, etc.
   */
  void RenderPieceFinish();

  /**
   * Query whether a shader rebuild will be required.
   */
  bool RequiresShaderRebuild();

  void ReleaseGraphicsResources(vtkWindow* win);

private:

  void AllocateGraphicsResources(vtkRenderer* ren);

  vtkValuePassHelper(const vtkValuePassHelper &) VTK_DELETE_FUNCTION;
  void operator=(const vtkValuePassHelper &) VTK_DELETE_FUNCTION;


  class vtkInternals;
  vtkInternals* Impl;
  int RenderingMode;
};
#endif // vtkValuePassHelper_h
