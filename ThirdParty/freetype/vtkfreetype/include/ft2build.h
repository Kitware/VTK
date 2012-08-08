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

#include <vtkfreetype/include/vtk_freetype_mangle.h>

#ifndef FT_CONFIG_MODULES_H
#define FT_CONFIG_MODULES_H  <vtkfreetype/include/vtk_ftmodule.h>
#endif

#include <vtkfreetype/include/freetype/config/ftheader.h>

#include <vtkfreetype/include/vtkFreeTypeConfig.h>

#endif /* __FT2_BUILD_VTK_H__ */
