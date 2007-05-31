/*
 *  VTK_FREETYPE_CHANGE this file is new for VTK.
 *
 *  This is used to override the default module headers.
 *
 *  This file is based on freetype's ftmodule.h.
 *
 *  Please read `docs/CUSTOMIZE' section IV-3 for more information.
 *
 */

/* VTK_FREETYPE_CHANGE removed the line below */
/* FT_USE_MODULE(autofit_module_class) */
FT_USE_MODULE(tt_driver_class)
FT_USE_MODULE(t1_driver_class)
FT_USE_MODULE(cff_driver_class)
FT_USE_MODULE(t1cid_driver_class)
FT_USE_MODULE(pfr_driver_class)
FT_USE_MODULE(t42_driver_class)
FT_USE_MODULE(winfnt_driver_class)
FT_USE_MODULE(pcf_driver_class)
FT_USE_MODULE(psaux_module_class)
FT_USE_MODULE(psnames_module_class)
FT_USE_MODULE(pshinter_module_class)
FT_USE_MODULE(ft_raster1_renderer_class)
FT_USE_MODULE(sfnt_module_class)
FT_USE_MODULE(ft_smooth_renderer_class)
FT_USE_MODULE(ft_smooth_lcd_renderer_class)
FT_USE_MODULE(ft_smooth_lcdv_renderer_class)
FT_USE_MODULE(bdf_driver_class)
