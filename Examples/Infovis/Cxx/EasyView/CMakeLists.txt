project(EasyView)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set your files and resources here
set(Srcs
  EasyView.cxx
  main.cxx)
set(UIs
  EasyView.ui)
set(Hdrs
  EasyView.h)
set(Rcs
  Icons/icons.qrc)

# We have ui files
find_package("Qt${VTK_QT_VERSION}" COMPONENTS Widgets)
if (NOT TARGET "Qt${VTK_QT_VERSION}::Widgets")
  message("Skipping example: ${Qt${VTK_QT_VERSION}_NOT_FOUND_MESSAGE}")
  return ()
endif ()

source_group("Resources" FILES
  ${UIs}
  ${Rcs}
)

add_executable(EasyView
  MACOSX_BUNDLE  ${Srcs} ${Hdrs} ${UIs} ${Rcs})
target_link_libraries(EasyView
  PRIVATE
    ${VTK_LIBRARIES}
    "Qt${VTK_QT_VERSION}::Widgets")
vtk_module_autoinit(
  TARGETS EasyView
  MODULES ${VTK_LIBRARIES})
