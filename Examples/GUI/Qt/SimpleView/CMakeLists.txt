project(SimpleView)

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

# Use the include path and library for Qt that is used by VTK.
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)


# Set your files and resources here
set(SimpleViewSrcs main.cxx SimpleView.cxx)
set(SimpleViewUI SimpleView.ui)
set(SimpleViewHeaders SimpleView.h)
set(SimpleViewResources Icons/icons.qrc)

# The rest should just work (sure...)
qt4_wrap_ui(UISrcs ${SimpleViewUI})
qt4_wrap_cpp(MOCSrcs ${SimpleViewHeaders} )
qt4_add_resources(ResourceSrcs ${SimpleViewResources})

source_group("Resources" FILES
  ${SimpleViewUI}
  ${SimpleViewResources}
  ${EXE_ICON}
)

source_group("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
  ${ResourceSrcs}
  ${RCS_SOURCES}
)

add_definitions(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)

add_executable(SimpleView MACOSX_BUNDLE ${SimpleViewSrcs} ${UISrcs} ${MOCSrcs}
  ${ResourceSrcs})

target_link_libraries(SimpleView
  ${QT_LIBRARIES}
  ${VTK_LIBRARIES}
)



