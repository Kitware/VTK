/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QVTKOpenGLWidget
 * @brief Wrapper for QVTKOpenGLStereoWidget for legacy support purpose
 *
 * QVTKOpenGLWidget is only a wrapper for QVTKOpenGLStereoWidget
 * so old code can still be built.
 * Please use QVTKOpenGLStereoWidget if Quad-buffer stereo
 * rendering capabilities or needed or QVTKOpenGLNativeWidget
 * for a more versatile implementation.
 *
 * @sa QVTKOpenGLStereoWidget QVTKOpenGLNativeWidget QVTKRenderWidget
 */
#ifndef QVTKOpenGLWidget_h
#define QVTKOpenGLWidget_h

#include "QVTKOpenGLStereoWidget.h"

#ifndef VTK_LEGACY_REMOVE
typedef QVTKOpenGLStereoWidget VTK_LEGACY(QVTKOpenGLWidget);
#endif

#endif
