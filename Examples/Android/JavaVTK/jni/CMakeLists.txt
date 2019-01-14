set(sources
  main.cxx
)

add_library(JavaVTK SHARED ${sources})
target_link_libraries(JavaVTK ${VTK_LIBRARIES}
  android
  log
)
