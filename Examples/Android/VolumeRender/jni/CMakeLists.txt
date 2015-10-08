include_directories(
  "${ANDROID_NDK}/platforms/android-${ANDROID_NATIVE_API_LEVEL}/arch-${ANDROID_ARCH_NAME}/usr/include"
)

set(sources
  main.cxx
)

add_library(VolumeRender SHARED ${sources})
target_link_libraries(VolumeRender ${VTK_LIBRARIES}
  android
  log
)
