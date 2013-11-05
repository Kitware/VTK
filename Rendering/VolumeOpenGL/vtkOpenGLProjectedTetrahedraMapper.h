/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkOpenGLProjectedTetrahedraMapper - OpenGL implementation of PT
//
// .SECTION Bugs
// This mapper relies highly on the implementation of the OpenGL pipeline.
// A typical hardware driver has lots of options and some settings can
// cause this mapper to produce artifacts.
//

#ifndef __vtkOpenGLProjectedTetrahedraMapper_h
#define __vtkOpenGLProjectedTetrahedraMapper_h

#include "vtkRenderingVolumeOpenGLModule.h" // For export macro
#include "vtkProjectedTetrahedraMapper.h"

class vtkVisibilitySort;
class vtkUnsignedCharArray;
class vtkFloatArray;
class vtkRenderWindow;
class vtkOpenGLRenderWindow;

class VTKRENDERINGVOLUMEOPENGL_EXPORT vtkOpenGLProjectedTetrahedraMapper
  : public vtkProjectedTetrahedraMapper
{
public:
  vtkTypeMacro(vtkOpenGLProjectedTetrahedraMapper,
                       vtkProjectedTetrahedraMapper);
  static vtkOpenGLProjectedTetrahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void ReleaseGraphicsResources(vtkWindow *window);

  virtual void Render(vtkRenderer *renderer, vtkVolume *volume);

  // Description:
  // Set/get whether to use floating-point rendering buffers rather
  // than the default.
  vtkSetMacro(UseFloatingPointFrameBuffer,bool);
  vtkGetMacro(UseFloatingPointFrameBuffer,bool);
  vtkBooleanMacro(UseFloatingPointFrameBuffer,bool);

  // Description:
  // Return true if the rendering context provides
  // the nececessary functionality to use this class.
  virtual bool IsSupported(vtkRenderWindow *context);

protected:
  vtkOpenGLProjectedTetrahedraMapper();
  ~vtkOpenGLProjectedTetrahedraMapper();

  void Initialize(vtkRenderer *ren);
  bool Initialized;
  int  CurrentFBOWidth, CurrentFBOHeight;
  bool AllocateFBOResources(vtkRenderer *ren);
  bool CanDoFloatingPointFrameBuffer;
  bool FloatingPointFrameBufferResourcesAllocated;
  bool UseFloatingPointFrameBuffer;

  vtkUnsignedCharArray *Colors;
  int UsingCellColors;

  vtkFloatArray *TransformedPoints;

  float MaxCellSize;
  vtkTimeStamp InputAnalyzedTime;
  vtkTimeStamp OpacityTextureTime;
  vtkTimeStamp ColorsMappedTime;

  int GaveError;

  vtkVolumeProperty *LastProperty;

  float *SqrtTable;
  float SqrtTableBias;

  virtual void ProjectTetrahedra(vtkRenderer *renderer, vtkVolume *volume);

  float GetCorrectedDepth(float x, float y, float z1, float z2,
                          const float inverse_projection_mat[16],
                          int use_linear_depth_correction,
                          float linear_depth_correction);

private:
  vtkOpenGLProjectedTetrahedraMapper(const vtkOpenGLProjectedTetrahedraMapper &);  // Not Implemented.
  void operator=(const vtkOpenGLProjectedTetrahedraMapper &);  // Not Implemented.

  class vtkInternals;
  vtkInternals *Internals;
};

#endif
