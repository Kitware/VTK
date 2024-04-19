cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(SimpleView)

find_package(VTK
  COMPONENTS
    CommonCore
    GUISupportQt
    InfovisCore
    RenderingFreeType
    ViewsQt)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

find_package("Qt${VTK_QT_VERSION}" COMPONENTS Widgets Gui)
if (NOT TARGET "Qt${VTK_QT_VERSION}::Widgets" OR NOT TARGET "Qt${VTK_QT_VERSION}::Gui")
  message("Skipping example: ${Qt${VTK_QT_VERSION}_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# Set your files and resources here
set(Srcs
  SimpleView.cxx
  main.cxx)

set(Hdrs
  SimpleView.h)

set(UIs
  SimpleView.ui)

set(QRCs
  Icons/icons.qrc)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

source_group("Resources" FILES
  ${UIs}
  ${QRCs})

add_executable(SimpleView MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${UIs} ${QRCs})
target_link_libraries(SimpleView
  PRIVATE
    ${VTK_LIBRARIES}
    "Qt${VTK_QT_VERSION}::Gui"
    "Qt${VTK_QT_VERSION}::Widgets")
