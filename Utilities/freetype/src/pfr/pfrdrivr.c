/***************************************************************************/
/*                                                                         */
/*  pfrdrivr.c                                                             */
/*                                                                         */
/*    FreeType PFR driver interface (body).                                */
/*                                                                         */
/*  Copyright 2002 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include "pfrdrivr.h"
#include "pfrobjs.h"


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  pfr_driver_class =
  {
    {
      ft_module_font_driver      |
      ft_module_driver_scalable,

      sizeof( FT_DriverRec ),

      "pfr",
      0x10000L,
      0x20000L,

      0,   /* format interface */

      (FT_Module_Constructor)NULL,
      (FT_Module_Destructor) NULL,
      (FT_Module_Requester)  NULL
    },

    sizeof( PFR_FaceRec ),
    sizeof( PFR_SizeRec ),
    sizeof( PFR_SlotRec ),

    (FT_Face_InitFunc)        pfr_face_init,
    (FT_Face_DoneFunc)        pfr_face_done,
    (FT_Size_InitFunc)        NULL,
    (FT_Size_DoneFunc)        NULL,
    (FT_Slot_InitFunc)        pfr_slot_init,
    (FT_Slot_DoneFunc)        pfr_slot_done,

    (FT_Size_ResetPointsFunc) NULL,
    (FT_Size_ResetPixelsFunc) NULL,
    (FT_Slot_LoadFunc)        pfr_slot_load,
    (FT_CharMap_CharIndexFunc)NULL,

    (FT_Face_GetKerningFunc)  0,
    (FT_Face_AttachFunc)      0,
    (FT_Face_GetAdvancesFunc) 0,

    (FT_CharMap_CharNextFunc) NULL
  };


/* END */
