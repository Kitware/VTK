/***************************************************************************/
/*                                                                         */
/*  t42types.h                                                             */
/*                                                                         */
/*    Type 42 font data types (specification only).                        */
/*                                                                         */
/*  Copyright 2002 by Roberto Alameda.                                     */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __T42TYPES_H__
#define __T42TYPES_H__


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_INTERNAL_TYPE1_TYPES_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H


FT_BEGIN_HEADER


  typedef struct  T42_FontRec_ 
  {
    /* font info dictionary */
    PS_FontInfoRec   font_info; 

    /* top-level dictionary */
    FT_String*       font_name;

    T1_EncodingType  encoding_type; 
    T1_EncodingRec   encoding;

    FT_Byte*         charstrings_block;
    FT_Byte*         glyph_names_block;

    FT_Int           num_glyphs;
    FT_String**      glyph_names;       /* array of glyph names       */
    FT_Byte**        charstrings;       /* array of glyph charstrings */
    FT_Int*          charstrings_len;

    FT_Byte          paint_type;
    FT_Byte          font_type;
    FT_Matrix        font_matrix; /* From FontMatrix field: a, b, c, d */
    FT_Vector        font_offset; /* From FontMatrix field: tx, ty */
    FT_BBox          font_bbox;

    FT_Int           stroke_width;  

  } T42_FontRec, *T42_Font;


  typedef struct  T42_FaceRec_
  {
    FT_FaceRec     root;
    T42_FontRec    type42;
    const void*    psnames;
    const void*    psaux;
    const void*    afm_data;
    FT_Byte*       ttf_data;
    FT_ULong       ttf_size;
    FT_Face        ttf_face;
    FT_CharMapRec  charmaprecs[2];
    FT_CharMap     charmaps[2];
    PS_Unicodes    unicode_map;

  } T42_FaceRec, *T42_Face;


FT_END_HEADER

#endif /* __T1TYPES_H__ */


/* END */
