/*
 *  VTK_FREETYPE_CHANGE this file is totally changed for VTK.
 *
 *  This is used to override the default module headers.
 *
 *  This file is based on freetype's ft2build.h.
 *
 *  Please read `docs/CUSTOMIZE' section IV-3 for more information.
 *
 */

#ifndef __FT2_BUILD_VTK_H__
#define __FT2_BUILD_VTK_H__

#define FT2_BUILD_LIBRARY 1
#include "vtk_freetype_mangle.h"

#define FT_CONFIG_MODULES_H  <vtk_ftmodule.h>
#include <freetype/config/ftheader.h>

#if defined(VTKFREETYPE)
#include "vtkFreeTypeConfig.h"
#endif

#endif /* __FT2_BUILD_VTK_H__ */
