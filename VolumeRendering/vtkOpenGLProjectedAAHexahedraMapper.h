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

// High quality volume renderer for axis-aligned hexahedra
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This file implements the paper 
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data", 
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.


#ifndef __vtkOpenGLProjectedAAHexahedraMapper_h
#define __vtkOpenGLProjectedAAHexahedraMapper_h

#include "vtkUnstructuredGridVolumeMapper.h"

class vtkFloatArray;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkVisibilitySort;
class vtkVolumeProperty;
class vtkRenderWindow;

class VTK_VOLUMERENDERING_EXPORT vtkOpenGLProjectedAAHexahedraMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeMacro(vtkOpenGLProjectedAAHexahedraMapper,
               vtkUnstructuredGridVolumeMapper);
  static vtkOpenGLProjectedAAHexahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void SetVisibilitySort(vtkVisibilitySort *sort);
  vtkGetObjectMacro(VisibilitySort, vtkVisibilitySort);

  bool IsRenderSupported(vtkRenderWindow *w);

  void Render(vtkRenderer *renderer, vtkVolume *volume);

  void ReleaseGraphicsResources(vtkWindow *window);

  float* ConvertScalars(vtkDataArray* inScalars);
  float* ConvertPoints(vtkPoints* inPoints);

protected:
  vtkOpenGLProjectedAAHexahedraMapper();
  ~vtkOpenGLProjectedAAHexahedraMapper();

  vtkVisibilitySort *VisibilitySort;
 
  // Description:
  // The visibility sort will probably make a reference loop by holding a
  // reference to the input.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  virtual void ProjectHexahedra(vtkRenderer *renderer, vtkVolume *volume);
  bool              Initialized;
  void Initialize(vtkRenderer *renderer, vtkVolume *volume);

private:
  void UpdatePreintegrationTexture(vtkVolume *vome, vtkDataArray *scalars);
  vtkOpenGLProjectedAAHexahedraMapper(const vtkOpenGLProjectedAAHexahedraMapper &);  // Not Implemented.
  void operator=(const vtkOpenGLProjectedAAHexahedraMapper &);  // Not Implemented.
  void CreateProgram();
  void SetState(double* observer);
  inline void RenderHexahedron(float min[3], float max[3], float scalars[8]);
  void UnsetState();
 
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
  unsigned int Shader; // vtkgl::handleARB

  static const char *VertSource;
  static const char *GeomSource;
  static const char *FragSource;
  int GaveError;

  float ScalarScale, ScalarShift, ScalarResolution;
  float LengthScale;

  vtkVolumeProperty *LastProperty;

  vtkFloatArray *ConvertedPoints;
  vtkFloatArray *ConvertedScalars;

};

#endif
