// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class QVTKRenderWidget
 * @brief Generic QWidget for displaying a vtkRenderWindow in a Qt Application.
 *
 * QVTKRenderWidget is intended as a generic widget for displaying VTK rendering
 * results in a Qt application. It is only a wrapper around other specific
 * classes. For now, this only wraps the QVTKOpenGLNativeWidget which is the
 * most versatile implementation. This may evolve in the future.
 * See QVTKOpenGLNativeWidget for implementation details.
 *
 * For Quad-buffer stereo support, please use directly the QVTKOpenGLStereoWidget.
 *
 * @note QVTKRenderWidget requires Qt version 5.9 and above.
 * @sa QVTKOpenGLStereoWidget QVTKOpenGLNativeWidget
 */
#ifndef QVTKRenderWidget_h
#define QVTKRenderWidget_h

// For now, only wraps the QVTKOpenGLNativeWidget
#include "QVTKOpenGLNativeWidget.h"
using QVTKRenderWidget = QVTKOpenGLNativeWidget;

#endif
