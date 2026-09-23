// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @brief Private implementation details shared between vtkFreeTypeTools and
 *        subclasses that need access to the complete FTThreadLocalData type.
 *
 * This header is intentionally NOT installed.
 */

#ifndef vtkFreeTypeToolsPrivate_h
#define vtkFreeTypeToolsPrivate_h

#include "vtkFreeTypeTools.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

VTK_ABI_NAMESPACE_BEGIN

/**
 * Per-thread RAII wrapper for a FreeType library instance and its associated
 * FTC caches. One instance is created the first time a thread calls
 * vtkFreeTypeTools::GetThreadLocalData(), and is automatically destroyed when
 * the thread exits.
 */
struct vtkFreeTypeTools::FTThreadLocalData
{
  FT_Library Library;
  FTC_Manager* CacheManager;
  FTC_ImageCache* ImageCache;
  FTC_CMapCache* CMapCache;

  FTThreadLocalData();
  ~FTThreadLocalData();

  ///@{
  /**
   * Initialise the FT_Library for this thread.
   * Returns true on success.
   */
  bool InitLibrary();
  ///@}

  ///@{
  /**
   * Release all FreeType cache objects owned by this thread.
   */
  void ReleaseCaches();
  ///@}

  FTThreadLocalData(const FTThreadLocalData&) = delete;
  FTThreadLocalData& operator=(const FTThreadLocalData&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkFreeTypeToolsPrivate_h
