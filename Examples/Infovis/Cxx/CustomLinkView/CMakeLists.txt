# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set your files and resources here
set(Srcs
  CustomLinkView.cxx
  main.cxx)
set(UIs
  CustomLinkView.ui)
set(Hdrs
  CustomLinkView.h)
set(Rcs
  Icons/icons.qrc)

find_package("Qt${VTK_QT_VERSION}" COMPONENTS Widgets)
if (NOT TARGET "Qt${VTK_QT_VERSION}::Widgets")
  message("Skipping example: ${Qt${VTK_QT_VERSION}_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(CustomLinkView MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${UIs} ${Rcs})
target_link_libraries(CustomLinkView
  PRIVATE
    ${VTK_LIBRARIES}
    "Qt${VTK_QT_VERSION}::Widgets")
vtk_module_autoinit(
  TARGETS CustomLinkView
  MODULES ${VTK_LIBRARIES})
