project(QtEvents)

if(NOT VTK_BINARY_DIR)
  find_package(VTK)
  if(NOT VTK_DIR)
    message(FATAL_ERROR "Please set VTK_DIR.")
  endif()
  include(${VTK_USE_FILE})
endif()

# Use what VTK built with
set(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
set(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
set(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

set(SRCS main.cxx)

# Use the include path and library for Qt that is used by VTK.
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(${VTK_TDX_INCLUDE_PATH})

qt4_wrap_ui(UI_SRCS GUI4.ui)
qt4_wrap_cpp(MOC_SRCS GUI4.h)
list(APPEND SRCS ${MOC_SRCS} GUI4.cxx GUI4.h)

add_executable(qtevents MACOSX_BUNDLE ${SRCS} ${UI_SRCS})

target_link_libraries(qtevents
  ${QT_LIBRARIES}
  ${VTK_LIBRARIES}
)


