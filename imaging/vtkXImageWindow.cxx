/*=========================================================================*/
#include "vtkXImageWindow.h"


unsigned char *vtkXImageWindow::GetPixelData(int x1, int y1, int x2, int y2, int)
{
  vtkDebugMacro (<< "Getting pixel data...");

  int width  = (abs(x2 - x1)+1);
  int height = (abs(y2 - y1)+1);

  // Get the XImage
  XImage* image = XGetImage(this->DisplayId, this->WindowId, x1, y1,
			    width, height, AllPlanes, XYPixmap);

  // Allocate space for the data
  unsigned char*  data = new unsigned char[width*height*3];
  if (!data) 
    {
    vtkErrorMacro(<< "Failed to malloc space for pixel data!");
    return (unsigned char*) NULL;
    }

  // Pointer to index through the data
  unsigned char* p_data = data;

  // Set up the loop indices
  int yloop = 0;
  int xloop = 0;
  unsigned long pixel = 0; 
  int y_low = 0;
  int y_hi = 0;
  int x_low = 0;
  int x_hi = 0;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  for (yloop = y_hi; yloop >= y_low; yloop--)
    {
    for (xloop = x_low; xloop <= x_hi ; xloop++)
      {
      pixel = XGetPixel(image, xloop, yloop);
      *p_data = (pixel & 0x000000ff); p_data++;
      *p_data = (pixel & 0x0000ff00) >> 8; p_data++;
      *p_data = (pixel & 0x00ff0000) >> 16; p_data++;
      }
    }
 
  return data;

}

void vtkXImageWindow::SwapBuffers()
{
  static int swapFlag = 0;

  if (swapFlag == 0)
    {
    swapFlag = 1;
    }
  else
    {
    XCopyArea(this->DisplayId, this->Drawable, this->WindowId, this->Gc, 
	      0, 0, this->Size[0], this->Size[1], 0, 0);
    XSync(this->DisplayId, False);
    XFlush(this->DisplayId);
    swapFlag = 0;
    }

}

void *vtkXImageWindow::GetGenericDrawable()
{
  static int pixWidth = 0;
  static int pixHeight = 0;

  if (this->DoubleBuffer)
    {
    if (!this->Drawable)
      {
      this->Drawable = XCreatePixmap(this->DisplayId, this->WindowId, 
				     this->Size[0], this->Size[1],
				     this->VisualDepth);
      pixWidth = this->Size[0];
      pixHeight = this->Size[1];
      }
    else if ((pixWidth != this->Size[0]) || (pixHeight != this->Size[1]))
      {
      XFreePixmap(this->DisplayId, this->Drawable);
      this->Drawable = XCreatePixmap(this->DisplayId, this->WindowId, 
				      this->Size[0], this->Size[1],
				      this->VisualDepth);       
      }
    return (void *) this->Drawable;
    }
  else
    {
    return (void *) this->WindowId;
    }

}


//----------------------------------------------------------------------------
vtkXImageWindow::vtkXImageWindow()
{
  vtkDebugMacro(<< "vtkXImageWindow::vtkXImageWindow");
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)(NULL);
  this->ParentId = (Window)(NULL);
  this->ColorMap = (Colormap) NULL;
  this->Drawable = (Pixmap) NULL;
  this->IconPixmap = (Pixmap) NULL;
  this->Offset = 0;
}


//----------------------------------------------------------------------------
vtkXImageWindow::~vtkXImageWindow()
{
  vtkDebugMacro(<< "vtkXImageWindow::vtkXImageWindow");
}


//----------------------------------------------------------------------------
void vtkXImageWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageWindow::PrintSelf(os, indent);
  os << indent << "Parent Id: " << this->ParentId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
  os << indent << "Display Id: " << this->DisplayId << "\n";
  os << indent << "Visual Id: " << this->VisualId << "\n";
  os << indent << "Visual Depth: " << this->VisualDepth << "\n";
  os << indent << "Visual Class: " << this->VisualClass << "\n";
  os << indent << "ColorMap: " << this->ColorMap << "\n";
  os << indent << "GC: " << this->Gc << "\n";
  os << indent << "Icon Pixmap: " << this->IconPixmap << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Colors: " << this->Colors << "\n";
  os << indent << "Number Of Colors: " << this->Colors << "\n";
  os << indent << "Drawable: " << this->Drawable << "\n";

}


void vtkXImageWindow::SetBackgroundColor(float r, float g, float b)
{
  unsigned long background = 0;

  // I think these need to be unsigned char
  unsigned short red = 0;
  unsigned short green = 0;
  unsigned short blue = 0;

  // Check if colors are > 1 ???
  red = (unsigned short) (r * 255.0);
  green = (unsigned short) (g * 255.0);
  blue = (unsigned short)  (b * 255.0);

  background = background | (blue << 16);
  background = background | (green << 8);
  background = background | red;

  vtkDebugMacro(<<"vtkXImageWindow::SetBackgroundColor - value: " << background);

  vtkDebugMacro(<<"vtkXImageWindow::SetBackgroundColor - red: " << red << ", green: " << green <<
  ", blue: " << blue);

  XSetWindowBackground(this->DisplayId, this->WindowId, background);

  XClearWindow(this->DisplayId, this->WindowId);

  XFlush(this->DisplayId);
   
  XSync(this->DisplayId, False); 
}

void vtkXImageWindow::EraseWindow()
{
  // Erase the drawable if double buffering is on
  // and the drawable exists
  if (this->DoubleBuffer && this->Drawable)
    {
    // Get the old foreground and background
    XGCValues vals;
    XGetGCValues(this->DisplayId, this->Gc, GCForeground, &vals);
    unsigned long oldForeground = vals.foreground;
    unsigned long background = vals.background;

    // Set the foreground color to the background so the rectangle
    // matches the background color
    XSetForeground(this->DisplayId, this->Gc, 0x0);
    XFillRectangle(this->DisplayId, this->Drawable, this->Gc, 0, 0, 
	           this->Size[0], this->Size[1]);

    // Reset the foreground to it's previous color
    XSetForeground (this->DisplayId, this->Gc, oldForeground);
    }
  // otherwise, erase the window
  else
    {
    XClearWindow(this->DisplayId, this->WindowId);
    XFlush(this->DisplayId); 
    XSync(this->DisplayId, False); 
    }

}


// Description:
// Get this RenderWindow's X window id.
Window vtkXImageWindow::GetWindowId()
{
//  vtkDebugMacro(<< "vtkXImageWindow::GetWindowID ");

  return this->WindowId;
}

// Description:
// Get this RenderWindow's parent X window id.
Window vtkXImageWindow::GetParentId()
{
//  vtkDebugMacro(<< "vtkXImageWindow::GetParentID ");
  return this->ParentId;
}

// Description:
// Sets the parent of the window that WILL BE created.
void vtkXImageWindow::SetParentId(Window arg)
{
  if (this->ParentId)
    {
    vtkErrorMacro("ParentId is already set.");
    return;
    }
  
//  vtkDebugMacro(<< "vtkXImageWindow::SetParentId ");

  this->ParentId = arg;
}

void vtkXImageWindow::SetParentId(void* arg)
{
  this->SetParentId((Window)arg);
}


// Description:
// Get the position in screen coordinates (pixels) of the window.
int *vtkXImageWindow::GetPosition(void)
{
  XWindowAttributes attribs;
  int x,y;
  Window child;
 
//  vtkDebugMacro (<< "vtkXImageWindow::GetPosition");
 
  // if we aren't mapped then just return the ivar 
  if ( ! this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);
  x = attribs.x;
  y = attribs.y;

  XTranslateCoordinates(this->DisplayId,this->WindowId,
			RootWindowOfScreen(ScreenOfDisplay(this->DisplayId,0)),
			x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

// Description:
// Move the window to a new position on the display.
void vtkXImageWindow::SetPosition(int x, int y)
{
//  vtkDebugMacro (<< "vtkXImageWindow::SetPosition");

  // if we aren't mapped then just set the ivars
  if (!this->Mapped)
    {
    if ((this->Position[0] != x)||(this->Position[1] != y))
      {
      this->Modified();
      }
    this->Position[0] = x;
    this->Position[1] = y;
    return;
    }

  XMoveResizeWindow(this->DisplayId,this->WindowId,x,y,
                    this->Size[0], this->Size[1]);
  XSync(this->DisplayId,False);
}


void vtkXImageWindow::SetSize(int x, int y)
{
//  vtkDebugMacro (<< "vtkXImageWindow::SetSize");

  // If the values have changed, change the ivars
  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;

    }

  // If the window isn't displayed, return
  if (!this->Mapped)
    {
    return;
    }

   // Make the X window call 
   XResizeWindow(this->DisplayId, this->WindowId, this->Size[0], this->Size[1]);


   // Need the XFlush to clear the X queue or else there are X timing problems
   // i.e. The first image may not display correctly
   XFlush(this->DisplayId);
   
   XSync(this->DisplayId, False); 

   //this->Render();

}


int* vtkXImageWindow::GetSize()
{
  XWindowAttributes attribs;
 
  vtkDebugMacro (<< "vtkXImageWindow::GetSize");
 
  // if we aren't mapped then just return the ivar 
  if ( ! this->Mapped)
    {
    vtkDebugMacro (<< "vtkXImageWindow::GetSize - Window not mapped");
    return(this->Size);
    }

  //  Find the current window size 
  XFlush(this->DisplayId);
  XSync(this->DisplayId, False);

  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);


  // May want to put in call to update Size ivars if different than 
  // what X thinks the size is

  this->Size[0] = attribs.width;
  this->Size[1] = attribs.height;

  return this->Size; 

}

//----------------------------------------------------------------------------
void vtkXImageWindow::MakeDefaultWindow() 
{
  char name[80];
  int screen;
  XVisualInfo info;
  XSetWindowAttributes values;
  Window window;

  vtkDebugMacro (<< "vtkXImageWindow::MakeDefaultWindow" ); 
  
  strcpy(name,"vtk - X Viewer Window");

  // make sure we have a connection to the X server.
  if ( ! this->DisplayId)
    {
    if ( ( this->DisplayId = XOpenDisplay((char *)NULL)) == NULL) 
      {
      cerr <<"cannot connect to X server"<< XDisplayName((char *)NULL)<< endl;
      exit(-1);
      }
    }
  
  
  screen = DefaultScreen(this->DisplayId);
  this->GetDefaultVisualInfo(&info);
  
  // Create a window 
  // If this is a pseudocolor visual, create a color map.
  values.colormap = this->GetDesiredColormap();
  
  values.background_pixel = BlackPixel(this->DisplayId, screen);
  values.border_pixel = None;
  values.event_mask = 0;
  values.override_redirect = False;
  //  if ((w > 0) && (x >= 0) && (!borders))
  //  values.override_redirect = True;
  XFlush(this->DisplayId);

  // get a default parent if one has not been set.
  if (! this->ParentId)
    {
    this->ParentId = RootWindow(this->DisplayId, screen);
    }

  window = XCreateWindow(this->DisplayId, this->ParentId,
			 0, 0, this->Size[0], this->Size[1], 0, info.depth, 
			 InputOutput, info.visual,
			 CWEventMask | CWBackPixel | CWBorderPixel | 
			 CWColormap | CWOverrideRedirect, 
			 &values);
  XSetStandardProperties(this->DisplayId, window, name, name, None, 0, 0, 0);
  XSync(this->DisplayId, False);
  
  // Select event types wanted 
  XSelectInput(this->DisplayId, window,
	       ExposureMask | KeyPressMask | ButtonPressMask |
	       PointerMotionMask | StructureNotifyMask | PropertyChangeMask);
  
  // Map Window onto Screen and sysc
  XMapWindow(this->DisplayId, window);
  
  XSync(this->DisplayId,0);
 
// ####

  XVisualInfo templ;
  XVisualInfo *visuals;
  int nvisuals;
  XWindowAttributes attributes;
 
  this->WindowId = (Window)(window);
 
  // Create a graphics contect for this window
  this->Gc = XCreateGC(this->DisplayId, this->WindowId, 0, NULL);
  XSetForeground(this->DisplayId, this->Gc, 0XFFFFFF);
  XSetBackground(this->DisplayId, this->Gc, 0X000000);

  // Get the visual
  if ( ! XGetWindowAttributes(this->DisplayId, this->WindowId, &attributes))
    {
    vtkErrorMacro(<< "SetWindow: Could not get window attributes.");
    return;
    }
  this->VisualId = attributes.visual;
  this->VisualDepth = attributes.depth;
  this->ColorMap = attributes.colormap;
 
  //####
  if (this->ColorMap == None) 
           vtkDebugMacro(<<"vtkXImageWindow::MakeDefaultWindow - No colormap!");
  if (attributes.map_installed == False) 
           vtkDebugMacro(<<"vtkXImageWindow::MakeDefaultWindow - Colormap not installed!");

  // Get the visual class
  templ.visualid = this->VisualId->visualid;
  visuals = XGetVisualInfo(this->DisplayId,
                           VisualIDMask,
                           &templ, &nvisuals);
  if (nvisuals == 0)
    {
    vtkErrorMacro(<< "Could not get visual class");
    }
  this->VisualClass = visuals->c_class;

  XFree(visuals);

  // Make sure the color map is set up properly.
  if (this->VisualClass == DirectColor)
    {
    vtkDebugMacro(<< "vtkXImageWindow::MakeDefaultWindow - Allocating direct color map");
    this->AllocateDirectColorMap();
    }

  this->Mapped = 1;
  this->WindowCreated = 1;


// ####
 
  return;
}


//----------------------------------------------------------------------------
void vtkXImageWindow::GetDefaultVisualInfo(XVisualInfo *info) 
{
  int screen;
  XVisualInfo templ;
  XVisualInfo *visuals, *v;
  XVisualInfo *best = NULL;
  int nvisuals;
  int i, rate, bestRate = 100;
  
//  vtkDebugMacro (<< "vtkImageWindow::GetDefaultVisualInfo" );

  screen = DefaultScreen(this->DisplayId);  
  templ.screen = screen;
  //templ.depth = 24;
  //templ.c_class = DirectColor;

  // Get a list of all the possible visuals for this screen.
  visuals = XGetVisualInfo(this->DisplayId,
			   // VisualScreenMask | VisualClassMask,
			   VisualScreenMask,
			   &templ, &nvisuals);
  
  if (nvisuals == 0)
    {
    vtkErrorMacro(<< "Could not get a visual");
    }
  
  for (v = visuals, i = 0; i < nvisuals; v++, i++)
    {
    // which are available

#if 0
    if (this->Debug)
      {
      if (v->c_class == TrueColor)
	vtkDebugMacro(<< "Available: " << v->depth << " bit TrueColor");
      if (v->c_class == DirectColor)
	vtkDebugMacro(<< "Available: " << v->depth << " bit DirectColor");
      if (v->c_class == PseudoColor)
	vtkDebugMacro(<< "Available: " << v->depth << " bit PseudoColor");
      }
#endif 


    // We only handle three types of visuals now.
    // Rate the visual
    if (v->depth == 24 && v->c_class == TrueColor)
      {
      rate = 1;
      }
    else if (v->depth == 24 && v->c_class == DirectColor)
      {
      rate = 2;
      }
    else if (v->depth == 8 && v->c_class == PseudoColor)
      {
      rate = 3;
      }
    else
      {
      rate = 50;
      }
    
    if (rate < bestRate)
      {
      bestRate = rate;
      best = v;
      }
    }

  if (bestRate >= 50)
    {
    vtkWarningMacro("Could not find a visual I like");
    }
 
#if 0 
  if (this->Debug)
    {
    if (best->c_class == TrueColor)
      vtkDebugMacro(<< "Chose: " << best->depth << " bit TrueColor");
    if (best->c_class == DirectColor)
      vtkDebugMacro(<< "Chose: " << best->depth << " bit DirectColor");
    if (best->c_class == PseudoColor)
      vtkDebugMacro(<< "Chose: " << best->depth << " bit PseudoColor");
    }
#endif

  
  // Copy visual
  *info = *best;
  
  XFree(visuals);
}

int vtkXImageWindow::GetDesiredDepth()
{
  XVisualInfo v;

//  vtkDebugMacro (<< "vtkXImageWindow::GetDesiredDepth");

  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  return v.depth;  
}

// Description:
// Get a visual from the windowing system.
Visual *vtkXImageWindow::GetDesiredVisual ()
{
  XVisualInfo v;

//  vtkDebugMacro (<< "vtkXImageWindow::GetDesiredVisual");

  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  return v.visual;  
}


// Description:
// Get a colormap from the windowing system.
Colormap vtkXImageWindow::GetDesiredColormap ()
{
  XVisualInfo v;

//  vtkDebugMacro (<< "vtkXImageWindow::GetDesiredColorMap");

  if (this->ColorMap) return this->ColorMap;
  
  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  if (v.depth == 8)
    {
    this->ColorMap = this->MakeColorMap(v.visual);
    }
  else
    {
    this->ColorMap = 
      XCreateColormap(this->DisplayId, RootWindow(this->DisplayId, v.screen),
		      v.visual, AllocNone);
    }
  
  return this->ColorMap;  
}


void vtkXImageWindow::SetWindowId(void *arg)
{
//  vtkDebugMacro (<< "vtkXImageWindow::SetWindowId");

  this->SetWindowId((Window)arg);
}

void vtkXImageWindow::SetWindowId(Window arg)
{
  // Here to allow me to compile
}

// Description:
// Set the X display id for this ImageXWindow to use to a pre-existing 
// X display id.
void vtkXImageWindow::SetDisplayId(Display  *arg)
{
//  vtkDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
}
void vtkXImageWindow::SetDisplayId(void *arg)
{
//  vtkDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 
  this->SetDisplayId((Display *)arg);
}



Display *vtkXImageWindow::GetDisplayId()
{
//  vtkDebugMacro(<< "vtkXImageWindow::GetDisplayId\n"); 
  return(this->DisplayId);
}
GC vtkXImageWindow::GetGC()
{
//  vtkDebugMacro(<< "vtkXImageWindow::GetGC\n"); 
  return(this->Gc);
}
//----------------------------------------------------------------------------
Colormap vtkXImageWindow::MakeColorMap(Visual *visual) 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  int screen;
  Colormap  defaultMap, newMap;
  XColor    defccells[256];
  
//  vtkDebugMacro(<< "vtkXImageWindow::MakeColorMap\n"); 
  
  this->Offset = 0;

  screen = DefaultScreen(this->DisplayId);
  defaultMap = DefaultColormap(this->DisplayId, screen);
  
  if ( !XAllocColorCells(this->DisplayId, defaultMap, 0, &planeMask, 0, 
			 pval, (unsigned int) this->NumberOfColors))
    {
    // can't allocate NUM_COLORS from Def ColorMap
    // create new ColorMap ... but first cp some def ColorMap
    
    newMap = XCreateColormap(this->DisplayId, 
			     RootWindow(this->DisplayId, screen),
			     visual, AllocNone);
    this->Offset = 100;
    if (! XAllocColorCells(this->DisplayId, newMap, 1, &planeMask, 0, pval,
			   (unsigned int)256))
      {
      vtkErrorMacro(<< "Sorry cann't allocate any more Colors");
      return (Colormap)(NULL);
      }
    
    for ( idx = 0 ; idx < 256; idx++) 
      {
      defccells[idx].pixel = idx; 
      }
    XQueryColors(this->DisplayId, defaultMap, defccells, 256);
    
    for (idx = 0 ; idx < 256; idx++)
      {
      // Value should range between ? and ?
      value = 1000 + (int)(60000.0 * (float)(idx - this->Offset) / (float)(this->NumberOfColors));
    
      if ( (idx < this->Offset)) 
	{
	this->Colors[idx].pixel = defccells[idx].pixel;
	this->Colors[idx].red   = defccells[idx].red ;
	this->Colors[idx].green = defccells[idx].green ;
	this->Colors[idx].blue  = defccells[idx].blue ;
	this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
	XStoreColor(this->DisplayId, newMap, &(this->Colors[idx]));
	}
      else 
	{
	this->Colors[idx].pixel = pval[idx];
	this->Colors[idx].red   = value ;
	this->Colors[idx].green = value ; 
	this->Colors[idx].blue  = value ;
	this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
	XStoreColor(this->DisplayId, newMap, &(this->Colors[idx]));
	}
      }
    XInstallColormap(this->DisplayId, newMap);
    return newMap;
    }
  else
    {
    for (idx = 0 ; idx < this->NumberOfColors ; idx++)
      {
      if (idx) 
	{
	value = (((192 * idx)/(this->NumberOfColors -1)) << 8)  + 16000;
	}
      else 
	{
	value = 0;
	}
      this->Colors[idx].pixel = pval[idx];
      this->Colors[idx].red   = value ;
      this->Colors[idx].green = value ;
      this->Colors[idx].blue  = value ;
      this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
      XStoreColor(this->DisplayId, defaultMap, &(this->Colors[idx]));
      }

    return defaultMap;
    } 
}



//----------------------------------------------------------------------------
void vtkXImageWindow::AllocateDirectColorMap() 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  Colormap newMap;
  
//  vtkDebugMacro(<< "vtkXImageWindow::AllocateDirectColorMap\n"); 
  
  this->Offset = 100;

  // Get the colors in the current color map.
  for ( idx = 0 ; idx < 256; idx++) 
    {
    this->Colors[idx].pixel = idx; 
    }
  XQueryColors(this->DisplayId, this->ColorMap, this->Colors, 256);
    
  
  newMap = XCreateColormap(this->DisplayId, this->WindowId,
			   this->VisualId, AllocNone);
  if (! XAllocColorCells(this->DisplayId, newMap, 1, &planeMask, 0, pval,
			 (unsigned int)256))
    {
    vtkErrorMacro(<< "Sorry cann't allocate any more Colors");
    return;
    }
  
  // Set up the colors
  for (idx = 0; idx < 100; ++idx)
    {
    this->Colors[idx].pixel = pval[idx];
    this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
    XStoreColor(this->DisplayId, newMap, &(this->Colors[idx]));
    }
  for (idx = 0 ; idx < this->NumberOfColors; ++idx)
    {
    // Value should range between 0 and 65000
    value = 1000 + (int)(60000.0 * (float)(idx)/(float)(this->NumberOfColors));
    this->Colors[idx+100].pixel = pval[idx];
    this->Colors[idx+100].red   = value ;
    this->Colors[idx+100].green = value ; 
    this->Colors[idx+100].blue  = value ;
    this->Colors[idx+100].flags = DoRed | DoGreen | DoBlue ;
    XStoreColor(this->DisplayId, newMap, &(this->Colors[idx+100]));
    }
  XInstallColormap(this->DisplayId, newMap);
  this->ColorMap = newMap;
  XSetWindowColormap(this->DisplayId, this->WindowId, this->ColorMap);
}
















