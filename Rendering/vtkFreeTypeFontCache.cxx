/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeFontCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFreeTypeFontCache.h"

#include "vtkTextProperty.h"
#include <sys/stat.h>

#include "vtkfreetypeConfig.h"
#include "vtkftglConfig.h"
#include "FTLibrary.h"
#include "FTGLPixmapFont.h" // Use pixmaps for antialiased fonts
#include "FTGLBitmapFont.h" // Use bitmaps for normal (jaggy but faster) fonts

//----------------------------------------------------------------------------
// Print debug info

#define VTK_FTFC_DEBUG 0
#define VTK_FTFC_DEBUG_CD 0

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup

vtkFreeTypeFontCache* vtkFreeTypeFontCache::Instance = 0;
vtkFreeTypeFontCacheCleanup vtkFreeTypeFontCache::Cleanup;

//----------------------------------------------------------------------------
// The embedded fonts
// Create a lookup table between the text mapper attributes 
// and the font buffers.

#include "fonts/vtkEmbeddedFonts.h"

struct EmbeddedFontStruct
{
  size_t length;
  unsigned char *ptr;
};

//----------------------------------------------------------------------------
// This callback will be called by the FTGLibrary singleton cleanup destructor
// if it happens to be destroyed before our singleton (this order is not 
// deterministic). It will destroy our singleton, if needed.

void vtkFreeTypeFontCacheCleanupCallback ()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCacheCleanupCallback\n");
#endif
  vtkFreeTypeFontCache::SetInstance(0);
}

//----------------------------------------------------------------------------
// Create the singleton cleanup
// Register our singleton cleanup callback against the FTLibrary so that
// it might be called before the FTLibrary singleton is destroyed.

vtkFreeTypeFontCacheCleanup::vtkFreeTypeFontCacheCleanup()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCacheCleanup::vtkFreeTypeFontCacheCleanup\n");
#endif
  FTLibraryCleanup::AddDependency(&vtkFreeTypeFontCacheCleanupCallback);
}

//----------------------------------------------------------------------------
// Delete the singleton cleanup 
// The callback called here might have been called by the FTLibrary singleton
// cleanup first (depending on the destruction order), but in case ours is
// destroyed first, let's call it too.

vtkFreeTypeFontCacheCleanup::~vtkFreeTypeFontCacheCleanup()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCacheCleanup::~vtkFreeTypeFontCacheCleanup\n");
#endif
  vtkFreeTypeFontCacheCleanupCallback();
}

//----------------------------------------------------------------------------
// (static) Return the single instance

vtkFreeTypeFontCache* vtkFreeTypeFontCache::GetInstance()
{
  if(!vtkFreeTypeFontCache::Instance)
    {
    vtkFreeTypeFontCache::Instance = new vtkFreeTypeFontCache;
    }
  return vtkFreeTypeFontCache::Instance;
}

//----------------------------------------------------------------------------
// (static) Set the singleton instance

void vtkFreeTypeFontCache::SetInstance(vtkFreeTypeFontCache* instance)
{
  if (vtkFreeTypeFontCache::Instance == instance)
    {
    return;
    }

  if (vtkFreeTypeFontCache::Instance)
    {
    delete vtkFreeTypeFontCache::Instance;
    }

  vtkFreeTypeFontCache::Instance = instance;
}

//----------------------------------------------------------------------------
// Create/Delete cache

vtkFreeTypeFontCache::vtkFreeTypeFontCache() 
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCache::vtkFreeTypeFontCache\n");
#endif
  this->NumberOfEntries = 0;
  this->InitializeCache();
}

//----------------------------------------------------------------------------
vtkFreeTypeFontCache::~vtkFreeTypeFontCache() 
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCache::~vtkFreeTypeFontCache\n");
#endif
  this->ReleaseCache();
}

//----------------------------------------------------------------------------
// Print entry

void vtkFreeTypeFontCache::PrintEntry(int i, char *msg) 
{
  if (!this->Entries[i])
    {
    return;
    }

  printf("%s: [%2d] =", msg, i);

  printf(" [S: %2d]", this->Entries[i]->FontSize);

#if VTK_FTFC_CACHE_BY_RGBA
  printf(" [RGBA: %2X/%2X/%2X (%2X)]", 
         this->Entries[i]->Red, 
         this->Entries[i]->Green, 
         this->Entries[i]->Blue, 
         this->Entries[i]->Alpha);
#endif

  if (this->Entries[i]->FaceFileName)
    {
    printf(" [F: %s]", this->Entries[i]->FaceFileName);
    }
  else
    {
    printf(" [F: %d] [I: %d] [B: %d]", 
           this->Entries[i]->FontFamily, 
           this->Entries[i]->Italic, 
           this->Entries[i]->Bold);
    }

  if (this->Entries[i]->Font)
    {
    printf(" [F: %p]", this->Entries[i]->Font);
    printf("\n                                                [f: %p]", *(this->Entries[i]->Font->Face()->Face()));
    }
  
  printf("\n");
  fflush(stdout);
}

//----------------------------------------------------------------------------
// Release entry

void vtkFreeTypeFontCache::ReleaseEntry(int i) 
{
  if (!this->Entries[i])
    {
    return;
    }

#if VTK_FTFC_DEBUG
  this->PrintEntry(this->NumberOfEntries, "Rl");
#endif

  if (this->Entries[i]->Font)
    {
    delete this->Entries[i]->Font;
    this->Entries[i]->Font = 0;
    }
  
  if (this->Entries[i]->FaceFileName)
    {
    delete [] this->Entries[i]->FaceFileName;
    this->Entries[i]->FaceFileName = 0;
    }

  delete this->Entries[i];
  this->Entries[i] = 0;
}

//----------------------------------------------------------------------------
// Initialize cache

void vtkFreeTypeFontCache::InitializeCache() 
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCache::InitializeCache()\n");
#endif  
  this->ReleaseCache();

  int i;
  for (i = 0; i < VTK_FTFC_CACHE_CAPACITY; i++)
    {
    this->Entries[i] = 0;
    }
}

//----------------------------------------------------------------------------
// Release cache

void vtkFreeTypeFontCache::ReleaseCache() 
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeFontCache::ReleaseCache()\n");
#endif  

  int i;
  for (i = 0; i < this->NumberOfEntries; i++)
    {
#if VTK_FTFC_DEBUG
    this->PrintEntry(i, "Rl");
#endif
    this->ReleaseEntry(i);
    }

  this->NumberOfEntries = 0;
}

//----------------------------------------------------------------------------
// Get a font from the cache given the text property. If no font is
// found in the cache, one is created and stored with the given color
// parameters.  If AntiAliasing is Off, the font is a bitmap, thus
// color are not used in the cache (since glBitmap honors glColor*) If
// override_color is true, then red, green, blue are used as text color
// instead of the colors found in the vtkTextProperty.

vtkFreeTypeFontCache::Entry* vtkFreeTypeFontCache::GetFont(vtkTextProperty *tprop, 
                                           int override_color,
                                           unsigned char red,
                                           unsigned char green,
                                           unsigned char blue)
{
  int i;
#if VTK_FTFC_REORDER
  int j;
#endif

  int antialiasing_requested = 
    (tprop->GetGlobalAntiAliasing() == VTK_TEXT_GLOBAL_ANTIALIASING_ALL || 
     (tprop->GetGlobalAntiAliasing() == VTK_TEXT_GLOBAL_ANTIALIASING_SOME 
      && tprop->GetAntiAliasing())) ? 1 : 0;

#if VTK_FTFC_CACHE_BY_RGBA
  float opacity = tprop->GetOpacity();
  unsigned char alpha = (opacity < 0.0) ? 255 : (unsigned char)(opacity*255.0);
  if (!override_color)
    {
    double* tpropColor = tprop->GetColor();
    red   = (tpropColor[0] < 0.0) ? 0 : (unsigned char)(tpropColor[0] * 255.0);
    green = (tpropColor[1] < 0.0) ? 0 : (unsigned char)(tpropColor[1] * 255.0);
    blue  = (tpropColor[2] < 0.0) ? 0 : (unsigned char)(tpropColor[2] * 255.0);
    }
#endif
  
  // Has the font been cached ?
  
  for (i = 0; i < this->NumberOfEntries; i++)
    {
    if (
      // If a face file name has been specified, it overrides the 
      // font family as well as italic and bold attributes

      ((!tprop->GetFaceFileName() && 
        !this->Entries[i]->FaceFileName &&
        this->Entries[i]->FontFamily == tprop->GetFontFamily() &&
        this->Entries[i]->Italic     == tprop->GetItalic() &&
        this->Entries[i]->Bold       == tprop->GetBold()) ||

       (tprop->GetFaceFileName() && 
        this->Entries[i]->FaceFileName && 
        !strcmp(tprop->GetFaceFileName(), this->Entries[i]->FaceFileName))) &&

      this->Entries[i]->AntiAliasing == antialiasing_requested

#if VTK_FTFC_CACHE_BY_RGBA
      && (!antialiasing_requested ||
          (this->Entries[i]->Red     == red &&
           this->Entries[i]->Green   == green &&
           this->Entries[i]->Blue    == blue &&
           this->Entries[i]->Alpha   == alpha))
#endif
      && this->Entries[i]->FontSize  == tprop->GetFontSize())
      {
#if VTK_FTFC_REORDER
      // Make this the most recently used
      if (i != 0)
        {
        vtkFreeTypeFontCache::Entry *tmp = this->Entries[i];
        for (j = i - 1; j >= 0; j--)
          {
          this->Entries[j+1] = this->Entries[j];
          }
        this->Entries[0] = tmp;
        }
      return this->Entries[0];
#else
      return this->Entries[i];
#endif
      }
    }

  // OK the font is not cached, try to create one

  FTFont *font;
  if (antialiasing_requested)
    {
    font = new FTGLPixmapFont;
    }
  else
    {
    font = new FTGLBitmapFont;
    }

  // A face file name has been provided, try to load it, otherwise
  // just use the embedded fonts (i.e. font family, bold and italic attribs)

  if (tprop->GetFaceFileName())
    {
    if (!font->Open(tprop->GetFaceFileName(), false))
      {
      vtkErrorWithObjectMacro(tprop,<< "Unable to load font " << tprop->GetFaceFileName());
      delete font;
      return 0;
      }
    // Try to load an AFM metrics file for the PFB/PFA Postscript fonts
    int length = strlen(tprop->GetFaceFileName());
    if (length > 4 &&
        (!strcmp(tprop->GetFaceFileName() + length - 4, ".pfb") ||
         !strcmp(tprop->GetFaceFileName() + length - 4, ".pfa")))
      {
      char *metrics = new char[length + 1];
      strncpy(metrics, tprop->GetFaceFileName(), length - 3);
      strcpy(metrics + length - 3, "afm");
      struct stat fs;
      if (stat(metrics, &fs) == 0)
        {
        font->Attach(metrics); 
        }
      delete [] metrics;
      }
    }
  else
    {
    // Fonts, organized by [Family][Bold][Italic]
    
    static EmbeddedFontStruct EmbeddedFonts[3][2][2] = 
    {
      {
        {
          { // VTK_ARIAL: Bold [ ] Italic [ ]
        
            face_arial_buffer_length, face_arial_buffer
          },
          { // VTK_ARIAL: Bold [ ] Italic [x]
            face_arial_italic_buffer_length, face_arial_italic_buffer
          }
        },
        {
          { // VTK_ARIAL: Bold [x] Italic [ ]
            face_arial_bold_buffer_length, face_arial_bold_buffer
          },
          { // VTK_ARIAL: Bold [x] Italic [x]
            face_arial_bold_italic_buffer_length, face_arial_bold_italic_buffer
          }
        }
      },
      {
        {
          { // VTK_COURIER: Bold [ ] Italic [ ]
            face_courier_buffer_length, face_courier_buffer
          },
          { // VTK_COURIER: Bold [ ] Italic [x]
            face_courier_italic_buffer_length, face_courier_italic_buffer
          }
        },
        {
          { // VTK_COURIER: Bold [x] Italic [ ]
            face_courier_bold_buffer_length, face_courier_bold_buffer
          },
          { // VTK_COURIER: Bold [x] Italic [x]
            face_courier_bold_italic_buffer_length, 
            face_courier_bold_italic_buffer
          }
        }
      },
      {
        {
          { // VTK_TIMES: Bold [ ] Italic [ ]
            face_times_buffer_length, face_times_buffer
          },
          { // VTK_TIMES: Bold [ ] Italic [x]
            face_times_italic_buffer_length, face_times_italic_buffer
          }
        },
        {
          { // VTK_TIMES: Bold [x] Italic [ ]
            face_times_bold_buffer_length, face_times_bold_buffer
          },
          { // VTK_TIMES: Bold [x] Italic [x]
            face_times_bold_italic_buffer_length, face_times_bold_italic_buffer
          }
        }
      }
    };
    
    size_t length = EmbeddedFonts[tprop->GetFontFamily()][tprop->GetBold()][tprop->GetItalic()].length;
    unsigned char *ptr = EmbeddedFonts[tprop->GetFontFamily()][tprop->GetBold()][tprop->GetItalic()].ptr;

    if (!font->Open(ptr, length, false))
      {
      vtkErrorWithObjectMacro(tprop,<< "Unable to create font !" << " (family: " <<  tprop->GetFontFamily() << ", bold: " << tprop->GetBold() << ", italic: " << tprop->GetItalic() << ", length: " << length << ")");
      delete font;
      return 0;
      }
    }

  // Set face size

  font->FaceSize(tprop->GetFontSize());
  
  // We need to make room for a new font

  if (this->NumberOfEntries == VTK_FTFC_CACHE_CAPACITY)
    {
#if VTK_FTFC_DEBUG
    printf("Cache is full, deleting last!\n");
#endif
    this->NumberOfEntries--;
    }

  // Add the new font

  if (this->Entries[this->NumberOfEntries])
    {
    this->ReleaseEntry(this->NumberOfEntries);
    }
  this->Entries[this->NumberOfEntries] = new vtkFreeTypeFontCache::Entry;
  
  // Set the other info

  if (tprop->GetFaceFileName())
    {
    this->Entries[this->NumberOfEntries]->FaceFileName = 
      new char [strlen(tprop->GetFaceFileName()) + 1];
    strcpy(this->Entries[this->NumberOfEntries]->FaceFileName, 
           tprop->GetFaceFileName());
    }
  else
    {
    this->Entries[this->NumberOfEntries]->FontFamily  = tprop->GetFontFamily();
    this->Entries[this->NumberOfEntries]->Bold        = tprop->GetBold();
    this->Entries[this->NumberOfEntries]->Italic      = tprop->GetItalic();
    this->Entries[this->NumberOfEntries]->FaceFileName = 0;
    }

  this->Entries[this->NumberOfEntries]->AntiAliasing  = antialiasing_requested;
  this->Entries[this->NumberOfEntries]->FontSize      = tprop->GetFontSize();

#if VTK_FTFC_CACHE_BY_RGBA
  if (antialiasing_requested)
    {
    this->Entries[this->NumberOfEntries]->Red         = red;
    this->Entries[this->NumberOfEntries]->Green       = green;
    this->Entries[this->NumberOfEntries]->Blue        = blue;
    this->Entries[this->NumberOfEntries]->Alpha       = alpha;
    }
#endif

  this->Entries[this->NumberOfEntries]->Font          = font;

  this->Entries[this->NumberOfEntries]->LargestAscender  =
  this->Entries[this->NumberOfEntries]->LargestDescender = -1;

#if VTK_FTFC_DEBUG
  this->PrintEntry(this->NumberOfEntries, "Cr");
#endif

  vtkFreeTypeFontCache::Entry *tmp = this->Entries[this->NumberOfEntries];

#if VTK_FTFC_DO_NOT_REORDER
  // Now resort the list

  for (i = this->NumberOfEntries - 1; i >= 0; i--)
    {
    this->Entries[i+1] = this->Entries[i];
    }
  this->Entries[0] = tmp;
#endif

  this->NumberOfEntries++;
  return tmp;
}
