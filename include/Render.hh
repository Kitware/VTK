/*=========================================================================

  Program:   OSCAR 
  Module:    Render.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __Render_hh
#define __Render_hh

/*
 * shading models
 */
#define FLAT    0
#define GOURAUD 1
#define PHONG   2

/*
 * representation models
 */
#define POINTS    0
#define WIREFRAME 1
#define SURFACE   2

/*
 * some general macros
 */
#define RANGE_CLIP(a,x,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))

#endif

