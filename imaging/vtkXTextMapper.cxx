#include "vtkXTextMapper.h"

vtkXTextMapper::vtkXTextMapper()
{

}

vtkXTextMapper::~vtkXTextMapper()
{

}

void vtkXTextMapper::SetFontSize(int size)
{
  // Make sure that the font size matches an available X font size.
  // This routine assumes that some standard X fonts are installed.
  switch (size)
    {  
    // available X Font sizes
    case 8:
    case 10:
    case 12:
    case 14:
    case 18:
    case 24:
      this->FontSize = size;
      return;

    // In between sizes use next larger size
    case 9:
      this->FontSize = 10;
      break;
    case 11:
      this->FontSize = 12;
      break;
    case 13:
      this->FontSize = 14;
      break;
    case 15:
    case 16:
    case 17:
      this->FontSize = 18;
      break;
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      this->FontSize = 24;
      break;

    // catch the values outside the font range 
    default:
      if (size < 8) this->FontSize = 8;
      else if (size > 24)this->FontSize  = 24;
      else this->FontSize = 12;   // just in case we missed something above
      break;
    }

  vtkErrorMacro (<< size << " point font is not available.  Using " 
                 << this->FontSize << " point.");

  return;
}

int vtkXTextMapper::GetCompositingMode(vtkActor2D* actor)
{

  vtkProperty2D* tempProp = actor->GetProperty();
  int compositeMode = tempProp->GetCompositingOperator();

  switch (compositeMode)
  {
  case VTK_BLACK:
	  return GXclear;
  case VTK_NOT_DEST:
	  return GXinvert;
  case VTK_SRC_AND_DEST:
	  return GXand;
  case VTK_SRC_OR_DEST:
	  return  GXor;
  case VTK_NOT_SRC:
	  return GXcopyInverted;
  case VTK_SRC_XOR_DEST:
	  return GXxor;
  case VTK_SRC_AND_notDEST:
	  return GXandReverse;
  case VTK_SRC:
	  return GXcopy;
  case VTK_WHITE:
	  return GXset;
  default:
	  return GXcopy;
  }

}

void vtkXTextMapper::Render(vtkViewport* viewport, vtkActor2D* actor)
{
  if (this->Input == NULL) vtkErrorMacro (<<"vtkXTextMapper::Render - No input");

  // Get the window info
  vtkWindow*  window = viewport->GetVTKWindow();
  Display* displayId = (Display*) window->GetGenericDisplayId();
  GC gc = (GC) window->GetGenericContext();
  Window windowId = (Window) window->GetGenericWindowId();

  int* actorPos = actor->GetComputedDisplayPosition(viewport);

  // Set up the font color
  float* actorColor = actor->GetProperty()->GetColor();
  unsigned char red = (unsigned char) (actorColor[0] * 255.0);
  unsigned char green = (unsigned char) (actorColor[1] * 255.0);
  unsigned char  blue = (unsigned char)  (actorColor[2] * 255.0);

  // Use the color masks from the visual
  unsigned long rmask = 0;
  unsigned long gmask = 0;
  unsigned long bmask = 0;

  XWindowAttributes winAttribs;
  XGetWindowAttributes(displayId, windowId, &winAttribs);
 
  XVisualInfo temp1;
  temp1.visualid = winAttribs.visual->visualid;

  int nvisuals = 0;
  XVisualInfo* visuals = XGetVisualInfo(displayId, VisualIDMask, &temp1, &nvisuals);   

  if (nvisuals == 0)  vtkErrorMacro(<<"Could not get color masks");

  rmask = visuals->red_mask;
  gmask = visuals->green_mask;
  bmask = visuals->blue_mask;
  
  XFree(visuals);

  // Compute the shifts to match up the pixel bits with the mask bits
  int rshift = 0;

  while ( ((rmask & 0x80000000) == 0) && (rshift < 32))
    {
    rmask = rmask << 1;
    rshift++;
    }

  int gshift = 0;

  while ( ((gmask & 0x80000000) == 0) && (gshift < 32))
    {
    gmask = gmask << 1;
    gshift++;
    }

  int bshift = 0;

  while ( ((bmask & 0x80000000) == 0) && (bshift < 32))
    {
    bmask = bmask << 1;
    bshift++;
    }

  // Mask the colors into the foreground variable
  unsigned long foreground = 0;
  foreground = foreground | ((rmask & (red << 24)) >> rshift);
  foreground = foreground | ((gmask & (green << 24)) >> gshift);
  foreground = foreground | ((bmask & (blue << 24)) >> bshift);

  XSetForeground(displayId, gc, foreground);

  // Set up the font name string. Note: currently there is no checking to see
  // if we've exceeded the fontname length.

  // Foundry
  char fontname[256]= "*";
 
  // Family
  switch (this->FontFamily)
    {
    case VTK_ARIAL:
      strcat(fontname, "helvetica-");
      break;
    case VTK_COURIER:
      strcat(fontname, "courier-");
      break;
    case VTK_TIMES:
      strcat(fontname, "times-");
      break;
    default:
      strcat(fontname, "helvetica-");
    }

  // Weight
  if (this->Bold == 1)
    {
    strcat(fontname, "bold-");
    }
  else
    {
    strcat (fontname, "medium-");
    }

  // Slant
  if (this->Italic == 1)
    {
    if (this->FontFamily == VTK_TIMES) strcat(fontname, "i-");
    else strcat(fontname, "o-");
    }
  else
    {
    strcat(fontname, "r-");
    }

  char tempString[100];
 
  // Set width, pixels, point size
  sprintf(tempString, "*-%d-*\0", 10*this->FontSize);

  strcat(fontname, tempString);

  vtkDebugMacro(<<"vtkXTextMapper::Render - Font specifier: " << fontname);

  // Set the font
  Font font = XLoadFont(displayId,  fontname );
  XSetFont(displayId, gc, font);

  // Set the compositing mode for the actor
  int compositeMode = this->GetCompositingMode(actor);
  XSetFunction(displayId, gc, compositeMode);
 
  // Get the drawable to draw into
  Drawable drawable = (Drawable) window->GetGenericDrawable();
  if (!drawable) vtkErrorMacro(<<"Window returned NULL drawable!");

  // Draw the string
  XDrawString(displayId, drawable, gc, actorPos[0], actorPos[1], this->Input, strlen(this->Input));
 
  // Flush the X queue
  XFlush(displayId);
  XSync(displayId, False);
 
}







