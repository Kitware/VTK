/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedAAHexahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLProjectedAAHexahedraMapper - OpenGL implementation of a volume mapper for axis-aligned hexahedra
// .SECTION Description
// High quality volume renderer for axis-aligned hexahedra

// .SECTION Implementation
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This mapper implements the paper
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data",
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.

#ifndef vtkOpenGLProjectedAAHexahedraMapper_h
#define vtkOpenGLProjectedAAHexahedraMapper_h

#include "vtkRenderingVolumeOpenGLModule.h" // For export macro
#include "vtkProjectedAAHexahedraMapper.h"

class vtkFloatArray;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkVisibilitySort;
class vtkVolumeProperty;
class vtkRenderWindow;
class vtkShaderProgram2;

class VTKRENDERINGVOLUMEOPENGL_EXPORT vtkOpenGLProjectedAAHexahedraMapper
  : public vtkProjectedAAHexahedraMapper
{
public:
  vtkTypeMacro(vtkOpenGLProjectedAAHexahedraMapper,
               vtkProjectedAAHexahedraMapper);
  static vtkOpenGLProjectedAAHexahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Check if the required OpenGL extensions are supported by the OpenGL
  // context attached to the render window `w'.
  bool IsRenderSupported(vtkRenderWindow *w);

  void Render(vtkRenderer *renderer, vtkVolume *volume);

  void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkOpenGLProjectedAAHexahedraMapper();
  ~vtkOpenGLProjectedAAHexahedraMapper();

  // Description:
  // Convert the input scalar values to floats.
  float* ConvertScalars(vtkDataArray* inScalars);

  // Description:
  // Convert the input cell coordinates to floats.
  float* ConvertPoints(vtkPoints* inPoints);

  // Description:
  // Iterate over all the hexahedal input cells,
  // sort and render them.
  virtual void ProjectHexahedra(vtkRenderer *renderer, vtkVolume *volume);

  // Description:
  // Load the OpenGL extensions and allocate the vertex arrays.
  void Initialize(vtkRenderer *renderer, vtkVolume *volume);

  // Description:
  // Update the preintegration texture; this is needed whenever the mesh
  // changes.
  void UpdatePreintegrationTexture(vtkVolume *vome, vtkDataArray *scalars);

  // Description:
  // Create the OpenGL geometry/vertex/fragment programs for
  // hexahedral cell rendering.
  void CreateProgram(vtkRenderWindow *w);

  // Description:
  // Set the OpenGL state for hexahedral cell rendering.
  void SetState(double* observer);

  // Description:
  // Render a single axis-aligned hexahedal cell.
  void RenderHexahedron(float min[3], float max[3], float scalars[8]);

  // Description:
  // Restore the OpenGL state touched by SetState().
  void UnsetState();


  bool              Initialized;

  int UsingCellColors;

  float MaxCellSize;
  vtkTimeStamp InputAnalyzedTime;
  vtkTimeStamp PreintTextureTime;
  vtkTimeStamp ColorsMappedTime;

  unsigned int PreintTexture;

  // OpenGL arrays for primitive submission
  float* pos_points;
  float* min_points;
  float* node_data1;
  float* node_data2;

  // number of pending points
  int num_points;
  static const int max_points = 4096;

  // our shader
  vtkShaderProgram2 *Shader;

  int GaveError;

  float ScalarScale, ScalarShift, ScalarResolution;
  float LengthScale;

  vtkVolumeProperty *LastProperty;

  vtkFloatArray *ConvertedPoints;
  vtkFloatArray *ConvertedScalars;

private:
  vtkOpenGLProjectedAAHexahedraMapper(const vtkOpenGLProjectedAAHexahedraMapper &);  // Not Implemented.
  void operator=(const vtkOpenGLProjectedAAHexahedraMapper &);  // Not Implemented.
};

#endif
