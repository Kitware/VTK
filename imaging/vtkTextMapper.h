#ifndef __vtkTextMapper_h
#define __vtkTextMapper_h


#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"

#define VTK_ARIAL     0
#define VTK_COURIER   1
#define VTK_TIMES     2

class VTK_EXPORT vtkTextMapper : public vtkMapper2D
{
public:

  vtkTextMapper();
  ~vtkTextMapper();

  static vtkTextMapper *New();

  // Description:
  // Draw the text to the screen.  This function is implemented in
  // the subclasses.
  virtual void Render(vtkViewport* viewport, vtkActor2D* actor) = 0;

  // Description:
  // Set the input to the mapper.  The mapper doesn't parse the string
  // for carriage returns or line feeds.
  vtkSetStringMacro(Input);

  // Description:
  // Set the font size used by the mapper.  The subclasses can override
  // this function since all font sizes may not be available (especially
  // in X).
  virtual void SetFontSize(int size) {this->FontSize = size; this->FontChanged = 1; this->Modified();};

  // Description:
  // Return the font size actually in use by the mapper.  This value may
  // not match the value specified in the last SetFontSize if the last size
  // was unavailable.
  vtkGetMacro(FontSize, int);

  // Description:
  // Set/Get the bold property.
  //  vtkSetMacro(Bold, int);
  void SetBold(int val) {this->Bold = val; this->FontChanged = 1; this->Modified();};
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Set/Get the italic property.
  // vtkSetMacro(Italic, int);
  void SetItalic(int val) {this->Italic = val; this->FontChanged = 1; this->Modified();};
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Set/Get the font family.  Three font types are allowed: Arial (VTK_ARIAL),
  // Courier (VTK_COURIER), and Times (VTK_TIMES).
  // vtkSetMacro(FontFamily, int);
  void SetFontFamily(int val) {this->FontFamily = val; this->FontChanged = 1; this->Modified();};
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {SetFontFamily(VTK_TIMES);};

protected:
  int   Italic;
  int	Bold;
  int   FontSize;
  int   FontFamily;
  char* Input;
  int   FontChanged;  
};


#endif

