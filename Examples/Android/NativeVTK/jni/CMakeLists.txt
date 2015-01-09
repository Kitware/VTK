include_directories(
  "${ANDROID_NDK}/platforms/android-${ANDROID_NATIVE_API_LEVEL}/arch-${ANDROID_ARCH_NAME}/usr/include"
  "${ANDROID_NDK}/sources/android/native_app_glue"
)

set(sources
  main.cxx
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

add_library(NativeVTK SHARED ${sources})
target_link_libraries(NativeVTK ${VTK_LIBRARIES}
  android
  log
)
