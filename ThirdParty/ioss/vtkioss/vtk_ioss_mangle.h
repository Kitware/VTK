#ifndef vtkioss_mangle_h
#define vtkioss_mangle_h

#define io_getline_int vtkioss_io_getline_int
#define io_gl_buf vtkioss_io_gl_buf
#define io_gl_ellipses_during_completion vtkioss_io_gl_ellipses_during_completion
#define io_gl_filename_quote_characters vtkioss_io_gl_filename_quote_characters
#define io_gl_filename_quoting_desired vtkioss_io_gl_filename_quoting_desired
#define io_gl_histadd vtkioss_io_gl_histadd
#define io_gl_setwidth vtkioss_io_gl_setwidth
#define new_termios vtkioss_new_termios
#define old_termios vtkioss_old_termios

// Mangle all global namespaces
#define Iocatalyst vtkioss_Iocatalyst
#define Iocgns vtkioss_Iocgns
#define Ioex vtkioss_Ioex
#define Iogn vtkioss_Iogn
#define Iogs vtkioss_Iogs
#define Iohb vtkioss_Iohb
#define Ioss vtkioss_Ioss
#define Iotr vtkioss_Iotr
#define SmartAssert vtkioss_SmartAssert

#endif
