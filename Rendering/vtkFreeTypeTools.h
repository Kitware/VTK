/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFreeTypeTools - FreeType library support
// .SECTION Description
// vtkFreeTypeTools provides a low-level interface to the FreeType2 library,
// including font-cache and rasterization.
//
// .Section Caveats
// Internal use only.

#ifndef __vtkFreeTypeTools_h
#define __vtkFreeTypeTools_h

#include "vtkObject.h"

class vtkImageData;
class vtkTextProperty;
class vtkStdString;
class vtkUnicodeString;

// FreeType
#include "vtk_freetype.h"  //since ft2build.h could be in the path
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H

class FTFont;

//----------------------------------------------------------------------------
// Singleton cleanup
class VTK_RENDERING_EXPORT vtkFreeTypeToolsCleanup
{
public:
  vtkFreeTypeToolsCleanup();
  ~vtkFreeTypeToolsCleanup();
};

//----------------------------------------------------------------------------
// Singleton font cache
class VTK_RENDERING_EXPORT vtkFreeTypeTools : public vtkObject
{
public:
  vtkTypeMacro(vtkFreeTypeTools, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the singleton instance with no reference counting.
  static vtkFreeTypeTools* GetInstance();

  // Description:
  // Supply a user defined instance. Call Delete() on the supplied
  // instance after setting it to fix the reference count.
  static void SetInstance(vtkFreeTypeTools *instance);

  // Description:
  // Get the FreeType library singleton.
  FT_Library* GetLibrary();

  // Description:
  // Set/Get the maximum number of faces (FT_Face), sizes (FT_Size) and
  // bytes used by the cache. These settings can be changed as long as
  // it is done prior to accessing any of the caches or the cache manager.
  vtkSetClampMacro(MaximumNumberOfFaces,unsigned int,1,VTK_UNSIGNED_INT_MAX);
  vtkGetMacro(MaximumNumberOfFaces, unsigned int);
  vtkSetClampMacro(MaximumNumberOfSizes,unsigned int,1,VTK_UNSIGNED_INT_MAX);
  vtkGetMacro(MaximumNumberOfSizes, unsigned int);
  vtkSetClampMacro(MaximumNumberOfBytes,unsigned long,1,VTK_UNSIGNED_LONG_MAX);
  vtkGetMacro(MaximumNumberOfBytes, unsigned long);

  // Description:
  // Given a text property and a string, get the bounding box [xmin, xmax] x
  // [ymin, ymax]. Note that this is the bounding box of the area
  // where actual pixels will be written, given a text/pen/baseline location
  // of (0,0).
  // For example, if the string starts with a 'space', or depending on the
  // orientation, you can end up with a [-20, -10] x [5, 10] bbox (the math
  // to get the real bbox is straightforward).
  // Return 1 on success, 0 otherwise.
  // You can use IsBoundingBoxValid() to test if the computed bbox
  // is valid (it may not if GetBoundingBox() failed or if the string
  // was empty).
  bool GetBoundingBox(vtkTextProperty *tprop, const vtkStdString& str,
                      int bbox[4]);
  bool GetBoundingBox(vtkTextProperty *tprop, const vtkUnicodeString& str,
                      int bbox[4]);
  bool IsBoundingBoxValid(int bbox[4]);

  // Description:
  // Given a text property and a string, this function initializes the
  // vtkImageData *data and renders it in a vtkImageData.
  bool RenderString(vtkTextProperty *tprop, const vtkStdString& str,
                    vtkImageData *data);
  bool RenderString(vtkTextProperty *tprop, const vtkUnicodeString& str,
                    vtkImageData *data);

  // Description:
  // Given a text property 'tprop', get its unique ID in our cache framework.
  // In the same way, given a unique ID in our cache, retrieve the
  // corresponding text property and assign its parameters to 'tprop'.
  // Warning: there is no one to one mapping between a single text property
  // the corresponding ID, and vice-versa. The ID is just a fast hash, a
  // binary mask concatenating the attributes of the text property that are
  // relevant to our cache (Color, Opacity, Justification setting are not
  // stored).
  void MapTextPropertyToId(vtkTextProperty *tprop, unsigned long *tprop_cache_id);
  void MapIdToTextProperty(unsigned long tprop_cache_id, vtkTextProperty *tprop);

  // Description:
  // Set whether the image produced should be scaled up to the nearest power of
  // 2. This is normally required for older graphics cards where all textures
  // must be a power of 2. This defaults to false, and should be fine on modern
  // hardware.
  vtkSetMacro(ScaleToPowerTwo, bool);
  vtkGetMacro(ScaleToPowerTwo, bool);
  vtkBooleanMacro(ScaleToPowerTwo, bool);

protected:
  // Description:
  // This function initializes calculates the size of the required bounding box.
  template <typename T>
  bool CalculateBoundingBox(vtkTextProperty *tprop, const T& str, int bbox[4]);

  // Description:
  // This function initializes the extent of the ImageData to eventually
  // receive the text stored in str
  template <typename T>
  void PrepareImageData(vtkImageData *data,
                        vtkTextProperty *tprop,
                        const T& str,
                        int *x, int *y);

  // Description:
  // Internal helper method called by RenderString
  template <typename T>
  bool PopulateImageData(vtkTextProperty *tprop, const T& str,
                         int x, int y, vtkImageData *data);

  // Description:
  // Given a text property, get the corresponding FreeType size object
  // (a structure storing both a face and a specific size metric).
  // The size setting of the text property is used to set the size's face
  // to the corresponding size.
  // Return true on success, false otherwise.
  bool GetSize(vtkTextProperty *tprop, FT_Size *size);

  // Description:
  // Given a text property, get the corresponding FreeType face.
  // The size parameter of the text property is ignored and a face with
  // unknown current size is returned. Use GetSize() to get a specific size.
  // Return true on success, false otherwise
  bool GetFace(vtkTextProperty *tprop, FT_Face *face);

  // Description:
  // Given a text property and a character, get the corresponding FreeType
  // glyph index.
  // Return true on success, false otherwise
  bool GetGlyphIndex(vtkTextProperty *tprop, FT_UInt32 c, FT_UInt *gindex);

  // Description:
  // Given a text property and a character, get the corresponding FreeType
  // glyph. The 'request' parameter can be used to request the glyph to be
  // in a specific format. If GLYPH_REQUEST_DEFAULT, the glyph might be either
  // an outline (most of the time) or a bitmap if the face includes a set of
  // pre-rendered glyphs (called "strikes") for a given size.
  // If GLYPH_REQUEST_BITMAP, the glyph is rendered immediately and can
  // be safely cast to a FT_BitmapGlyph. If GLYPH_REQUEST_OUTLINE, no
  // pre-rendered "strike" is considered, the glyph is an outline and can be
  // safely cast to a FT_OutlineGlyph.
  // Return true on success, false otherwise
  enum
  {
    GLYPH_REQUEST_DEFAULT = 0,
    GLYPH_REQUEST_BITMAP  = 1,
    GLYPH_REQUEST_OUTLINE = 2
  };
  bool GetGlyph(vtkTextProperty *tprop,
                FT_UInt32 c,
                FT_Glyph *glyph,
                int request = GLYPH_REQUEST_DEFAULT);
  bool GetSize(unsigned long tprop_cache_id, int font_size, FT_Size *size);
  bool GetFace(unsigned long tprop_cache_id, FT_Face *face);
  bool GetGlyphIndex(unsigned long tprop_cache_id, FT_UInt32 c,
                     FT_UInt *gindex);
  bool GetGlyph(unsigned long tprop_cache_id,
                int font_size,
                FT_UInt gindex,
                FT_Glyph *glyph,
                int request = GLYPH_REQUEST_DEFAULT);

  // Description:
  // Should the image be scaled to the next highest power of 2?
  bool ScaleToPowerTwo;

  vtkFreeTypeTools();
  virtual ~vtkFreeTypeTools();

private:
  vtkFreeTypeTools(const vtkFreeTypeTools&);  // Not implemented.
  void operator=(const vtkFreeTypeTools&);  // Not implemented.

  // Description:
  // Attempt to get the typeface of the specified font.
  bool GetFace(vtkTextProperty *prop, unsigned long &prop_cache_id,
               FT_Face &face, bool &face_has_kerning);

  // Description:
  // Now attempt to get the bitmap for the specified character.
  FT_Bitmap* GetBitmap(FT_UInt32 c, unsigned long prop_cache_id,
                       int prop_font_size, FT_UInt &gindex,
                       FT_BitmapGlyph &bitmap_glyph);

  // The singleton instance and the singleton cleanup instance
  static vtkFreeTypeTools* Instance;
  static vtkFreeTypeToolsCleanup Cleanup;

  // The cache manager, image cache and charmap cache
  FTC_Manager *CacheManager;
  FTC_ImageCache *ImageCache;
  FTC_CMapCache  *CMapCache;

  // Description:
  // Get the FreeType cache manager, image cache and charmap cache
  FTC_Manager* GetCacheManager();
  FTC_ImageCache* GetImageCache();
  FTC_CMapCache* GetCMapCache();

  unsigned int MaximumNumberOfFaces;
  unsigned int MaximumNumberOfSizes;
  unsigned long MaximumNumberOfBytes;

  void InitializeCacheManager();
  void ReleaseCacheManager();
};

#endif
