#include "vtkWin32TextMapper.h"
#include "vtkWin32ImageWindow.h"

vtkWin32TextMapper::vtkWin32TextMapper()
{
}

vtkWin32TextMapper::~vtkWin32TextMapper()
{
}

int vtkWin32TextMapper::GetCompositingMode(vtkActor2D* actor)
{
  vtkProperty2D* tempProp = actor->GetProperty();
  int compositeMode = tempProp->GetCompositingOperator();

  switch (compositeMode)
  {
  case VTK_BLACK:
	  return R2_BLACK;
	  break;
  case VTK_NOT_DEST:
	  return R2_NOT;
	  break;
  case VTK_SRC_AND_DEST:
	  return R2_MASKPEN;
	  break;
  case VTK_SRC_OR_DEST:
	  return  R2_MERGEPEN;
	  break;
  case VTK_NOT_SRC:
	  return R2_NOTCOPYPEN;
	  break;
  case VTK_SRC_XOR_DEST:
	  return R2_XORPEN;
      break;
  case VTK_SRC_AND_notDEST:
	  return R2_MASKPENNOT;
	  break;
  case VTK_SRC:
	  return R2_COPYPEN;
	  break;
  case VTK_WHITE:
	  return R2_WHITE;
	  break;
  default:
	  return R2_COPYPEN;
	  break;
  }

}


void vtkWin32TextMapper::Render(vtkViewport* viewport, vtkActor2D* actor)
{

  vtkDebugMacro (<< "vtkWin32TextMapper::Render");

  // Check for input
  if (this->Input == NULL) vtkErrorMacro (<<"vtkWin32TextMapper::Render - No input");

  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  HWND windowId = (HWND) window->GetGenericWindowId();

  // Get the device context from the window
  
  HDC hdc = (HDC) window->GetGenericContext();
 
  // Get the position of the text actor
  POINT ptDestOff;
  int xOffset = 0;
  int yOffset = 0;
//  this->GetActorOffset(viewport, actor, &xOffset, &yOffset);  
  int* actorPos = actor->GetComputedDisplayPosition(viewport);
  xOffset = actorPos[0];
  yOffset = actorPos[1];

  ptDestOff.x = xOffset;
  ptDestOff.y = yOffset;

  // Set up the font color from the text actor
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  float*  actorColor = actor->GetProperty()->GetColor();
  red = (unsigned char) (actorColor[0] * 255.0);
  green = (unsigned char) (actorColor[1] * 255.0);
  blue = (unsigned char) (actorColor[2] * 255.0);
  long status = SetTextColor(hdc, RGB(red, green, blue));

  if (status == CLR_INVALID)  vtkErrorMacro(<<"vtkWin32TextMapper::Render - SetTextColor failed!");

  // Set the background mode to transparent
  SetBkMode(hdc, TRANSPARENT);

#if 0
  // Display the text
  BOOL val = TextOut( hdc, ptDestOff.x , ptDestOff.y , this->Input, strlen(this->Input));

  if (val == FALSE) vtkErrorMacro (<<"vtkWin32TextMapper::Render - TextOut failed!");
#endif

  // Create the font
  LOGFONT fontStruct;
  char fontname[32];
  DWORD family;
  switch (this->FontFamily)
    {
    case VTK_ARIAL:
      strcpy(fontname, "Arial");
	  family = FF_SWISS;
	  break;
	case VTK_TIMES:
      strcpy(fontname, "Times Roman");
	  family = FF_ROMAN;
	  break;
	case VTK_COURIER:
      strcpy(fontname, "Courier");
	  family = FF_MODERN;
	  break;
	default:
      strcpy(fontname, "Arial");
	  family = FF_SWISS;
	  break;
  }

  fontStruct.lfHeight = MulDiv(this->FontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);  // height in logical units
  fontStruct.lfWidth = 0;  // default width
  fontStruct.lfEscapement = 0;
  fontStruct.lfOrientation = 0;
  if (this->Bold == 1)
    {
    fontStruct.lfWeight = FW_BOLD;
    }
  else 
    {
	fontStruct.lfWeight = FW_NORMAL;
    }
  fontStruct.lfItalic = this->Italic;
  fontStruct.lfUnderline = 0;
  fontStruct.lfStrikeOut = 0;
  fontStruct.lfCharSet = ANSI_CHARSET;
  fontStruct.lfOutPrecision = OUT_DEFAULT_PRECIS;
  fontStruct.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  fontStruct.lfQuality = DEFAULT_QUALITY;
  fontStruct.lfPitchAndFamily = DEFAULT_PITCH | family;
  strcpy(fontStruct.lfFaceName, fontname);
   
  HFONT hFont = CreateFontIndirect(&fontStruct);
  HFONT hOldFont = (HFONT) SelectObject(hdc, hFont);

  // Set the compositing operator
  int compositeMode = this->GetCompositingMode(actor);
  SetROP2(hdc, compositeMode);
  // For Debug
  int op = GetROP2(hdc);
  if (op != compositeMode) vtkErrorMacro(<<"vtkWin32TextMapper::Render - ROP not set!");

  // Define bounding rectangle
  RECT rect;
  rect.left = ptDestOff.x;
  rect.top = ptDestOff.y;
  rect.bottom = ptDestOff.y;
  rect.right = ptDestOff.x;

  // Calculate the size of the bounding rectangle
  DrawText(hdc, this->Input, strlen(this->Input), &rect, DT_CALCRECT|DT_LEFT|DT_NOPREFIX);
  // Draw the text
  DrawText(hdc, this->Input, strlen(this->Input), &rect, DT_LEFT|DT_NOPREFIX);

  SelectObject(hdc, hOldFont);
  DeleteObject(hFont);

}

