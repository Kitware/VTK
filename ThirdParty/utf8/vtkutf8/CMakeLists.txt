if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    "utf8.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkutf8"
    COMPONENT Development)
  install(FILES
    "utf8/checked.h"
    "utf8/core.h"
    "utf8/unchecked.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkutf8/utf8"
    COMPONENT Development)
endif()
