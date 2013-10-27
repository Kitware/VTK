project(QtImageViewer)

cmake_minimum_required(VERSION 2.8)

# Eliminate a warning when building in Windows that relates
# to static linking of Qt executables to qtmain.lib.
# This policy was introduced in CMake version 2.8.11.
# CMake version 2.8.11.2 warns when the policy is not set
# and uses OLD behavior.
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

if(NOT VTK_BINARY_DIR)
  find_package(VTK)
  if(NOT VTK_DIR)
    message(FATAL_ERROR "Please set VTK_DIR.")
  endif()
  include(${VTK_USE_FILE})
endif()

if("${VTK_QT_VERSION}" STREQUAL "")
  message(FATAL_ERROR "VTK was not built with Qt")
endif()

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5Core REQUIRED QUIET)
else()
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
  # Use what VTK built with
  set(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
  set(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
  set(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
endif()

set( Srcs main.cxx )

if(VTK_QT_VERSION VERSION_GREATER "4")
  add_executable(qtimageviewer ${Srcs})

  qt5_use_modules(qtimageviewer Core Gui Widgets)

  target_link_libraries(qtimageviewer ${VTK_LIBRARIES})
else()
  include_directories(
    ${QT_INCLUDE_DIR}
  )

  add_executable(qtimageviewer ${Srcs})

  target_link_libraries(qtimageviewer ${QT_LIBRARIES} ${VTK_LIBRARIES})
endif()
