#include "vtkTextMapper.h"

#ifdef _WIN32
  #include "vtkWin32TextMapper.h"
#else
  #include "vtkXTextMapper.h"
#endif

// Description:
// Creates a new text mapper with
// Font size 12, bold off, italic off,
// and Arial font
vtkTextMapper::vtkTextMapper()
{
  this->Input = (char*) NULL;
  this->FontSize = 12;
  this->Bold = 0;
  this->Italic = 0;
  this->FontFamily = VTK_ARIAL;
  this->FontChanged = 0;
}

vtkTextMapper::~vtkTextMapper()
{

}


vtkTextMapper *vtkTextMapper::New()
{
#ifdef _WIN32
    return vtkWin32TextMapper::New();
#else
    return vtkXTextMapper::New();
#endif

}









