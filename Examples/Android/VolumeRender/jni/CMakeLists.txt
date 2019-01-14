set(sources
  main.cxx
)

add_library(VolumeRender SHARED ${sources})
target_link_libraries(VolumeRender ${VTK_LIBRARIES}
  android
  log
)
