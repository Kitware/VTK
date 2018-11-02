# src/CMakeLists.txt
#
# Copyright (C) 2008  Werner Smekal
#
# create library
# =======================================================================
# libharu source files
# =======================================================================
set(
  LIBHPDF_SRCS
  	hpdf_annotation.c
  	hpdf_array.c
  	hpdf_binary.c
  	hpdf_boolean.c
  	hpdf_catalog.c
  	hpdf_destination.c
  	hpdf_dict.c
  	hpdf_doc_png.c
  	hpdf_doc.c
  	hpdf_encoder_cns.c
  	hpdf_encoder_cnt.c
  	hpdf_encoder_jp.c
  	hpdf_encoder_kr.c
  	hpdf_encoder.c
  	hpdf_encrypt.c
  	hpdf_encryptdict.c
  	hpdf_error.c
  	hpdf_ext_gstate.c
  	hpdf_font_cid.c
  	hpdf_font_tt.c
  	hpdf_font_type1.c
  	hpdf_font.c
  	hpdf_fontdef_base14.c
  	hpdf_fontdef_cid.c
  	hpdf_fontdef_cns.c
  	hpdf_fontdef_cnt.c
  	hpdf_fontdef_jp.c
  	hpdf_fontdef_kr.c
  	hpdf_fontdef_tt.c
  	hpdf_fontdef_type1.c
  	hpdf_fontdef.c
  	hpdf_gstate.c
	hpdf_image_ccitt.c
  	hpdf_image_png.c
  	hpdf_image.c
  	hpdf_info.c
  	hpdf_list.c
  	hpdf_mmgr.c
  	hpdf_name.c
  	hpdf_namedict.c
  	hpdf_null.c
  	hpdf_number.c
  	hpdf_objects.c
  	hpdf_outline.c
  	hpdf_page_label.c
  	hpdf_page_operator.c
  	hpdf_pages.c
  	hpdf_real.c
  	hpdf_shading.c
  	hpdf_streams.c
  	hpdf_string.c
  	hpdf_u3d.c
  	hpdf_utils.c
  	hpdf_xref.c
  	hpdf_pdfa.c
	hpdf_3dmeasure.c
	hpdf_exdata.c
	hpdf_encoder_utf.c
)

# =======================================================================
# create static and shared haru library
# =======================================================================
if (FALSE) # XXX(kitware): Use VTK's module system
if(LIBHPDF_STATIC)
  add_library(${LIBHPDF_NAME_STATIC} STATIC ${LIBHPDF_SRCS})
  target_link_libraries(${LIBHPDF_NAME_STATIC} ${ADDITIONAL_LIBRARIES})
  install(
    TARGETS ${LIBHPDF_NAME_STATIC}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )
  if(WIN32 AND NOT CYGWIN)
		foreach(addlib ${ADDITIONAL_LIBRARIES})
			install(FILES ${addlib} DESTINATION lib)
		endforeach(addlib)
  endif(WIN32 AND NOT CYGWIN)
endif(LIBHPDF_STATIC)
if(LIBHPDF_SHARED)
  add_library(${LIBHPDF_NAME} SHARED ${LIBHPDF_SRCS})
  target_link_libraries(${LIBHPDF_NAME} ${ADDITIONAL_LIBRARIES})
  if(WIN32 AND NOT CYGWIN)
  	set_target_properties(${LIBHPDF_NAME} PROPERTIES DEFINE_SYMBOL HPDF_DLL_MAKE)
  endif(WIN32 AND NOT CYGWIN)
  install(
    TARGETS ${LIBHPDF_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )
endif(LIBHPDF_SHARED)
else ()
  vtk_module_add_module(VTK::libharu
    SOURCES ${LIBHPDF_SRCS})
  if (WIN32)
    set_target_properties(libharu
      PROPERTIES
        DEFINE_SYMBOL HPDF_DLL_MAKE)
  endif ()
  target_include_directories(libharu
    PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>")
endif ()
