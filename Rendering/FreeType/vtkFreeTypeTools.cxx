/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFreeTypeTools.h"

#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPath.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"

#include "vtkStdString.h"
#include "vtkUnicodeString.h"

// FTGL
#include "vtkftglConfig.h"
#include "FTLibrary.h"

// The embedded fonts
#include "fonts/vtkEmbeddedFonts.h"

#ifndef _MSC_VER
# include <stdint.h>
#endif

#include <algorithm>
#include <map>
#include <vector>

#ifdef FTGL_USE_NAMESPACE
using namespace ftgl;
#endif

// Print debug info
#define VTK_FTFC_DEBUG 0
#define VTK_FTFC_DEBUG_CD 0

class vtkTextPropertyLookup
    : public std::map<unsigned long, vtkSmartPointer<vtkTextProperty> >
{
public:
  bool contains(const unsigned long id) {return this->find(id) != this->end();}
};

class vtkFreeTypeTools::MetaData
{
public:
  // Set by PrepareMetaData
  vtkTextProperty *textProperty;
  unsigned long textPropertyCacheId;
  FT_Face face;
  bool faceHasKerning;

  // Set by CalculateBoundingBox
  int ascent;
  int descent;
  int height;
  struct LineMetrics {
    int originX;
    int originY;
    int width;
    // bbox relative to origin[XY]:
    int xmin;
    int xmax;
    int ymin;
    int ymax;
  };
  std::vector<LineMetrics> lineMetrics;
  int maxLineWidth;
  int bbox[4];
};

class vtkFreeTypeTools::ImageMetaData : public vtkFreeTypeTools::MetaData
{
public:
  // Set by PrepareImageMetaData
  int imageDimensions[3];
  vtkIdType imageIncrements[3];
  unsigned char rgba[4];
};

//----------------------------------------------------------------------------
vtkInstantiatorNewMacro(vtkFreeTypeTools);

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup
vtkFreeTypeTools* vtkFreeTypeTools::Instance = NULL;
vtkFreeTypeToolsCleanup vtkFreeTypeTools::Cleanup;

//----------------------------------------------------------------------------
// The embedded fonts
// Create a lookup table between the text mapper attributes
// and the font buffers.
struct EmbeddedFontStruct
{
  size_t length;
  unsigned char *ptr;
};

//----------------------------------------------------------------------------
// This callback will be called by the FTGLibrary singleton cleanup destructor
// if it happens to be destroyed before our singleton (this order is not
// deterministic). It will destroy our singleton, if needed.
void vtkFreeTypeToolsCleanupCallback ()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeToolsCleanupCallback\n");
#endif
  vtkFreeTypeTools::SetInstance(NULL);
}

//----------------------------------------------------------------------------
// Create the singleton cleanup
// Register our singleton cleanup callback against the FTLibrary so that
// it might be called before the FTLibrary singleton is destroyed.
vtkFreeTypeToolsCleanup::vtkFreeTypeToolsCleanup()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeToolsCleanup::vtkFreeTypeToolsCleanup\n");
#endif
  FTLibraryCleanup::AddDependency(&vtkFreeTypeToolsCleanupCallback);
}

//----------------------------------------------------------------------------
// Delete the singleton cleanup
// The callback called here might have been called by the FTLibrary singleton
// cleanup first (depending on the destruction order), but in case ours is
// destroyed first, let's call it too.
vtkFreeTypeToolsCleanup::~vtkFreeTypeToolsCleanup()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeToolsCleanup::~vtkFreeTypeToolsCleanup\n");
#endif
  vtkFreeTypeToolsCleanupCallback();
}

//----------------------------------------------------------------------------
vtkFreeTypeTools* vtkFreeTypeTools::GetInstance()
{
  if (!vtkFreeTypeTools::Instance)
    {
    vtkFreeTypeTools::Instance = static_cast<vtkFreeTypeTools *>(
      vtkObjectFactory::CreateInstance("vtkFreeTypeTools"));
    if (!vtkFreeTypeTools::Instance)
      {
      vtkFreeTypeTools::Instance = new vtkFreeTypeTools;
      }
    }
  return vtkFreeTypeTools::Instance;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::SetInstance(vtkFreeTypeTools* instance)
{
  if (vtkFreeTypeTools::Instance == instance)
    {
    return;
    }

  if (vtkFreeTypeTools::Instance)
    {
    vtkFreeTypeTools::Instance->Delete();
    }

  vtkFreeTypeTools::Instance = instance;

  // User will call ->Delete() after setting instance

  if (instance)
    {
    instance->Register(NULL);
    }
}

//----------------------------------------------------------------------------
vtkFreeTypeTools::vtkFreeTypeTools()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::vtkFreeTypeTools\n");
#endif
  // Force use of compiled fonts by default.
  this->ForceCompiledFonts = true;
  this->MaximumNumberOfFaces = 30; // combinations of family+bold+italic
  this->MaximumNumberOfSizes = this->MaximumNumberOfFaces * 20; // sizes
  this->MaximumNumberOfBytes = 300000UL * this->MaximumNumberOfSizes;
  this->TextPropertyLookup = new vtkTextPropertyLookup ();
  this->CacheManager = NULL;
  this->ImageCache   = NULL;
  this->CMapCache    = NULL;
  this->ScaleToPowerTwo = true;
}

//----------------------------------------------------------------------------
vtkFreeTypeTools::~vtkFreeTypeTools()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::~vtkFreeTypeTools\n");
#endif
  this->ReleaseCacheManager();
  delete TextPropertyLookup;
}

//----------------------------------------------------------------------------
FT_Library* vtkFreeTypeTools::GetLibrary()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetLibrary\n");
#endif

  FTLibrary * ftgl_lib = FTLibrary::GetInstance();
  if (ftgl_lib)
    {
    return ftgl_lib->GetLibrary();
    }

  return NULL;
}

//----------------------------------------------------------------------------
FTC_Manager* vtkFreeTypeTools::GetCacheManager()
{
  if (!this->CacheManager)
    {
    this->InitializeCacheManager();
    }

  return this->CacheManager;
}

//----------------------------------------------------------------------------
FTC_ImageCache* vtkFreeTypeTools::GetImageCache()
{
  if (!this->ImageCache)
    {
    this->InitializeCacheManager();
    }

  return this->ImageCache;
}

//----------------------------------------------------------------------------
FTC_CMapCache* vtkFreeTypeTools::GetCMapCache()
{
  if (!this->CMapCache)
    {
    this->InitializeCacheManager();
    }

  return this->CMapCache;
}

//----------------------------------------------------------------------------
FT_CALLBACK_DEF(FT_Error)
vtkFreeTypeToolsFaceRequester(FTC_FaceID face_id,
                              FT_Library lib,
                              FT_Pointer request_data,
                              FT_Face* face)
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeToolsFaceRequester()\n");
#endif

  // Get a pointer to the current vtkFreeTypeTools object
  vtkFreeTypeTools *self =
    reinterpret_cast<vtkFreeTypeTools*>(request_data);

  // Map the ID to a text property
  vtkSmartPointer<vtkTextProperty> tprop =
      vtkSmartPointer<vtkTextProperty>::New();
  self->MapIdToTextProperty(reinterpret_cast<intptr_t>(face_id), tprop);

  bool faceIsSet = self->LookupFace(tprop, lib, face);

  if (!faceIsSet)
    {
    return static_cast<FT_Error>(1);
    }

  if ( tprop->GetOrientation() != 0.0 )
    {
    // FreeType documentation says that the transform should not be set
    // but we cache faces also by transform, so that there is a unique
    // (face, orientation) cache entry
    FT_Matrix matrix;
    float angle = vtkMath::RadiansFromDegrees( tprop->GetOrientation() );
    matrix.xx = (FT_Fixed)( cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed)( sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed)( cos(angle) * 0x10000L);
    FT_Set_Transform(*face, &matrix, NULL);
    }

  return static_cast<FT_Error>(0);
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::InitializeCacheManager()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::InitializeCacheManager()\n");
#endif

  this->ReleaseCacheManager();

  FT_Error error;

  // Create the cache manager itself
  this->CacheManager = new FTC_Manager;

  error = this->CreateFTCManager();

  if (error)
    {
    vtkErrorMacro(<< "Failed allocating a new FreeType Cache Manager");
    }

  // The image cache
  this->ImageCache = new FTC_ImageCache;
  error = FTC_ImageCache_New(*this->CacheManager, this->ImageCache);

  if (error)
    {
    vtkErrorMacro(<< "Failed allocating a new FreeType Image Cache");
    }

  // The charmap cache
  this->CMapCache = new FTC_CMapCache;
  error = FTC_CMapCache_New(*this->CacheManager, this->CMapCache);

  if (error)
    {
    vtkErrorMacro(<< "Failed allocating a new FreeType CMap Cache");
    }
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::ReleaseCacheManager()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::ReleaseCacheManager()\n");
#endif

  if (this->CacheManager)
    {
    FTC_Manager_Done(*this->CacheManager);

    delete this->CacheManager;
    this->CacheManager = NULL;
    }

  if (this->ImageCache)
    {
    delete this->ImageCache;
    this->ImageCache = NULL;
    }

  if (this->CMapCache)
    {
    delete this->CMapCache;
    this->CMapCache = NULL;
    }
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::IsBoundingBoxValid(int bbox[4])
{
  return (!bbox ||
          bbox[0] == VTK_INT_MAX || bbox[1] == VTK_INT_MIN ||
          bbox[2] == VTK_INT_MAX || bbox[3] == VTK_INT_MIN) ? false : true;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetBoundingBox(vtkTextProperty *tprop,
                                      const vtkStdString& str,
                                      int bbox[4])
{
  // We need the tprop and bbox
  if (!tprop || !bbox)
    {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL or zero");
    return false;
    }

  // No string to render, bail out now
  if (str.empty())
    {
    return false;
    }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, metaData);
  if (result)
    {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
      {
      memcpy(bbox, metaData.bbox, sizeof(int) * 4);
      }
    }
  return result;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetBoundingBox(vtkTextProperty *tprop,
                                      const vtkUnicodeString& str,
                                      int bbox[4])
{
  // We need the tprop and bbox
  if (!tprop || !bbox)
    {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL or zero");
    return false;
    }

  // No string to render, bail out now
  if (str.empty())
    {
    return false;
    }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, metaData);
  if (result)
    {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
      {
      memcpy(bbox, metaData.bbox, sizeof(int) * 4);
      }
    }
  return result;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::RenderString(vtkTextProperty *tprop,
                                    const vtkStdString& str,
                                    vtkImageData *data, int textDims[2])
{
  return this->RenderStringInternal(tprop, str, data, textDims);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::RenderString(vtkTextProperty *tprop,
                                    const vtkUnicodeString& str,
                                    vtkImageData *data, int textDims[2])
{
  return this->RenderStringInternal(tprop, str, data, textDims);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::StringToPath(vtkTextProperty *tprop,
                                    const vtkStdString &str, vtkPath *path)
{
  return this->StringToPathInternal(tprop, str, path);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::StringToPath(vtkTextProperty *tprop,
                                    const vtkUnicodeString &str, vtkPath *path)
{
  return this->StringToPathInternal(tprop, str, path);
}

//----------------------------------------------------------------------------
int vtkFreeTypeTools::GetConstrainedFontSize(const vtkStdString &str,
                                             vtkTextProperty *tprop,
                                             int targetWidth, int targetHeight)
{
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, metaData))
    {
    vtkErrorMacro(<<"Could not prepare metadata.");
    return false;
    }
  return this->FitStringToBBox(str, metaData, targetWidth, targetHeight);
}

//----------------------------------------------------------------------------
int vtkFreeTypeTools::GetConstrainedFontSize(const vtkUnicodeString &str,
                                             vtkTextProperty *tprop,
                                             int targetWidth, int targetHeight)
{
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, metaData))
    {
    vtkErrorMacro(<<"Could not prepare metadata.");
    return false;
    }
  return this->FitStringToBBox(str, metaData, targetWidth, targetHeight);
}

//----------------------------------------------------------------------------
vtkTypeUInt16 vtkFreeTypeTools::HashString(const char *str)
{
  if (str == NULL)
    return 0;

  vtkTypeUInt16 hash = 0;
  while (*str != 0)
    {
    vtkTypeUInt8 high = ((hash<<8)^hash) >> 8;
    vtkTypeUInt8 low = tolower(*str)^(hash<<2);
    hash = (high<<8) ^ low;
    ++str;
    }

  return hash;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::MapTextPropertyToId(vtkTextProperty *tprop,
                                           unsigned long *id)
{
  if (!tprop || !id)
    {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL");
    return;
    }

  // Set the first bit to avoid id = 0
  // (the id will be mapped to a pointer, FTC_FaceID, so let's avoid NULL)
  *id = 1;
  int bits = 1;

  // The font family is hashed into 16 bits (= 17 bits so far)
  *id |= vtkFreeTypeTools::HashString(tprop->GetFontFamilyAsString()) << bits;
  bits += 16;

  // Bold is in 1 bit (= 18 bits so far)
  int bold = (tprop->GetBold() ? 1 : 0) << bits;
  ++bits;

  // Italic is in 1 bit (= 19 bits so far)
  int italic = (tprop->GetItalic() ? 1 : 0) << bits;
  ++bits;

  // Orientation (in degrees)
  // We need 9 bits for 0 to 360. What do we need for more precisions:
  // - 1/10th degree: 12 bits (11.8) (31 bits)
  int angle = (vtkMath::Round(tprop->GetOrientation() * 10.0) % 3600) << bits;

  // We really should not use more than 32 bits
  // Now final id
  *id |= bold | italic | angle;

  // Insert the TextProperty into the lookup table
  if (!this->TextPropertyLookup->contains(*id))
    (*this->TextPropertyLookup)[*id] = tprop;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::MapIdToTextProperty(unsigned long id,
                                           vtkTextProperty *tprop)
{
  if (!tprop)
    {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL");
    return;
    }

  vtkTextPropertyLookup::const_iterator tpropIt =
      this->TextPropertyLookup->find(id);

  if (tpropIt == this->TextPropertyLookup->end())
    {
    vtkErrorMacro(<<"Unknown id; call MapTextPropertyToId first!");
    return;
    }

  tprop->ShallowCopy(tpropIt->second);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetSize(unsigned long tprop_cache_id,
                               int font_size,
                               FT_Size *size)
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetSize()\n");
#endif

  if (!size || font_size <= 0)
    {
    vtkErrorMacro(<< "Wrong parameters, size is NULL or invalid font size");
    return 0;
    }

  FTC_Manager *manager = this->GetCacheManager();
  if (!manager)
    {
    vtkErrorMacro(<< "Failed querying the cache manager !");
    return 0;
    }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  FTC_ScalerRec scaler_rec;
  scaler_rec.face_id = face_id;
  scaler_rec.width = font_size;
  scaler_rec.height = font_size;
  scaler_rec.pixel = 1;

  FT_Error error = FTC_Manager_LookupSize(*manager, &scaler_rec, size);
  if (error)
    {
    vtkErrorMacro(<< "Failed looking up a FreeType Size");
    }

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetSize(vtkTextProperty *tprop,
                               FT_Size *size)
{
  if (!tprop)
    {
    vtkErrorMacro(<< "Wrong parameters, text property is NULL");
    return 0;
    }

  // Map the text property to a unique id that will be used as face id
  unsigned long tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetSize(tprop_cache_id, tprop->GetFontSize(), size);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetFace(unsigned long tprop_cache_id,
                               FT_Face *face)
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetFace()\n");
#endif

  if (!face)
    {
    vtkErrorMacro(<< "Wrong parameters, face is NULL");
    return false;
    }

  FTC_Manager *manager = this->GetCacheManager();
  if (!manager)
    {
    vtkErrorMacro(<< "Failed querying the cache manager !");
    return false;
    }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  FT_Error error = FTC_Manager_LookupFace(*manager, face_id, face);
  if (error)
    {
    vtkErrorMacro(<< "Failed looking up a FreeType Face");
    }

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetFace(vtkTextProperty *tprop,
                               FT_Face *face)
{
  if (!tprop)
    {
    vtkErrorMacro(<< "Wrong parameters, face is NULL");
    return 0;
    }

  // Map the text property to a unique id that will be used as face id
  unsigned long tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetFace(tprop_cache_id, face);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetGlyphIndex(unsigned long tprop_cache_id,
                                     FT_UInt32 c,
                                     FT_UInt *gindex)
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetGlyphIndex()\n");
#endif

  if (!gindex)
    {
    vtkErrorMacro(<< "Wrong parameters, gindex is NULL");
    return 0;
    }

  FTC_CMapCache *cmap_cache = this->GetCMapCache();
  if (!cmap_cache)
    {
    vtkErrorMacro(<< "Failed querying the charmap cache manager !");
    return 0;
    }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  // Lookup the glyph index
  *gindex = FTC_CMapCache_Lookup(*cmap_cache, face_id, 0, c);

  return *gindex ? true : false;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetGlyphIndex(vtkTextProperty *tprop,
                                     FT_UInt32 c,
                                     FT_UInt *gindex)
{
  if (!tprop)
    {
    vtkErrorMacro(<< "Wrong parameters, text property is NULL");
    return 0;
    }

  // Map the text property to a unique id that will be used as face id
  unsigned long tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetGlyphIndex(tprop_cache_id, c, gindex);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetGlyph(unsigned long tprop_cache_id,
                                int font_size,
                                FT_UInt gindex,
                                FT_Glyph *glyph,
                                int request)
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetGlyph()\n");
#endif

  if (!glyph)
    {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL");
    return false;
    }

  FTC_ImageCache *image_cache = this->GetImageCache();
  if (!image_cache)
    {
    vtkErrorMacro(<< "Failed querying the image cache manager !");
    return false;
    }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  // Which font are we looking for
  FTC_ImageTypeRec image_type_rec;
  image_type_rec.face_id = face_id;
  image_type_rec.width = font_size;
  image_type_rec.height = font_size;
  image_type_rec.flags = FT_LOAD_DEFAULT;
  if (request == GLYPH_REQUEST_BITMAP)
    {
    image_type_rec.flags |= FT_LOAD_RENDER;
    }
  else if (request == GLYPH_REQUEST_OUTLINE)
    {
    image_type_rec.flags |= FT_LOAD_NO_BITMAP;
    }

  // Lookup the glyph
  FT_Error error = FTC_ImageCache_Lookup(
    *image_cache, &image_type_rec, gindex, glyph, NULL);

  return error ? false : true;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::LookupFace(vtkTextProperty *tprop, FT_Library lib,
                                  FT_Face *face)
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

  int family = tprop->GetFontFamily();
  // If font family is unknown, fall back to Arial.
  if (family == VTK_UNKNOWN_FONT)
    {
    vtkDebugWithObjectMacro(
          tprop,
          << "Requested font '" << tprop->GetFontFamilyAsString() << "'"
          " unavailable. Substituting Arial.");
    family = VTK_ARIAL;
    }

  FT_Long length = EmbeddedFonts
    [family][tprop->GetBold()][tprop->GetItalic()].length;
  FT_Byte *ptr = EmbeddedFonts
    [family][tprop->GetBold()][tprop->GetItalic()].ptr;

  // Create a new face from the embedded fonts if possible
  FT_Error error = FT_New_Memory_Face(lib, ptr, length, 0, face);

  if (error)
    {
    vtkErrorWithObjectMacro(
          tprop,
          << "Unable to create font !" << " (family: " << family
          << ", bold: " << tprop->GetBold() << ", italic: " << tprop->GetItalic()
          << ", length: " << length << ")");
    return false;
    }
  else
    {
#if VTK_FTFC_DEBUG
    cout << "Requested: " << *face
         << " (F: " << tprop->GetFontFamily()
         << ", B: " << tprop->GetBold()
         << ", I: " << tprop->GetItalic()
         << ", O: " << tprop->GetOrientation() << ")" << endl;
#endif
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetGlyph(vtkTextProperty *tprop,
                                FT_UInt32 c,
                                FT_Glyph *glyph,
                                int request)
{
  if (!tprop)
    {
    vtkErrorMacro(<< "Wrong parameters, text property is NULL");
    return 0;
    }

  // Map the text property to a unique id that will be used as face id
  unsigned long tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  // Get the character/glyph index
  FT_UInt gindex;
  if (!this->GetGlyphIndex(tprop_cache_id, c, &gindex))
    {
    vtkErrorMacro(<< "Failed querying a glyph index");
    return false;
    }

  // Get the glyph
  return this->GetGlyph(
    tprop_cache_id, tprop->GetFontSize(), gindex, glyph, request);
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MaximumNumberOfFaces: "
     << this->MaximumNumberOfFaces << endl;

  os << indent << "MaximumNumberOfSizes: "
     << this->MaximumNumberOfSizes << endl;

  os << indent << "MaximumNumberOfBytes: "
     << this->MaximumNumberOfBytes << endl;

  os << indent << "Scale to nearest power of 2 for image sizes: "
     << this->ScaleToPowerTwo << endl;
}

//----------------------------------------------------------------------------
FT_Error vtkFreeTypeTools::CreateFTCManager()
{
  return FTC_Manager_New(*this->GetLibrary(),
                         this->MaximumNumberOfFaces,
                         this->MaximumNumberOfSizes,
                         this->MaximumNumberOfBytes,
                         vtkFreeTypeToolsFaceRequester,
                         static_cast<FT_Pointer>(this),
                         this->CacheManager);
}

//----------------------------------------------------------------------------
inline bool vtkFreeTypeTools::PrepareImageMetaData(vtkTextProperty *tprop,
                                                   vtkImageData *image,
                                                   ImageMetaData &metaData)
{
  // Image properties
  image->GetIncrements(metaData.imageIncrements);
  image->GetDimensions(metaData.imageDimensions);

  double color[3];
  tprop->GetColor(color);
  metaData.rgba[0] = static_cast<unsigned char>(color[0] * 255);
  metaData.rgba[1] = static_cast<unsigned char>(color[1] * 255);
  metaData.rgba[2] = static_cast<unsigned char>(color[2] * 255);
  metaData.rgba[3] = static_cast<unsigned char>(tprop->GetOpacity() * 255);

  return true;
}

//----------------------------------------------------------------------------
inline bool vtkFreeTypeTools::PrepareMetaData(vtkTextProperty *tprop,
                                              MetaData &metaData)
{
  // Text properties
  metaData.textProperty = tprop;
  if (!this->GetFace(tprop, metaData.textPropertyCacheId, metaData.face,
                     metaData.faceHasKerning))
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
template <typename StringType>
bool vtkFreeTypeTools::RenderStringInternal(vtkTextProperty *tprop,
                                            const StringType &str,
                                            vtkImageData *data,
                                            int textDims[2])
{
  // Check parameters
  if (!tprop || !data)
    {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL or zero");
    return false;
    }

  if (data->GetNumberOfScalarComponents() > 4)
    {
    vtkErrorMacro("The image data must have a maximum of four components");
    return false;
    }

  if (str.empty())
    {
    return false;
    }

  ImageMetaData metaData;

  // Setup the metadata cache
  if (!this->PrepareMetaData(tprop, metaData))
    {
    vtkErrorMacro(<<"Error prepare text metadata.");
    return false;
    }

  // Calculate the bounding box.
  if (!this->CalculateBoundingBox(str, metaData))
    {
    vtkErrorMacro(<<"Could not get a valid bounding box.");
    return false;
    }

  // Calculate the text dimensions:
  if (textDims)
    {
    textDims[0] = metaData.bbox[1] - metaData.bbox[0] + 1;
    textDims[1] = metaData.bbox[3] - metaData.bbox[2] + 1;
    }

  // Prepare the ImageData to receive the text
  this->PrepareImageData(data, metaData.bbox);

  // Setup the image metadata
  if (!this->PrepareImageMetaData(tprop, data, metaData))
    {
    vtkErrorMacro(<<"Error prepare image metadata.");
    return false;
    }

  // Render shadow if needed
  if (metaData.textProperty->GetShadow())
    {
    // Modify the line offsets with the shadow offset
    int shadowOffset[2];
    metaData.textProperty->GetShadowOffset(shadowOffset);
    std::vector<MetaData::LineMetrics> origMetrics = metaData.lineMetrics;
    metaData.lineMetrics.clear();
    for (std::vector<MetaData::LineMetrics>::const_iterator
         it = origMetrics.begin(), itEnd = origMetrics.end(); it < itEnd; ++it)
      {
      MetaData::LineMetrics line = *it;
      line.originX += shadowOffset[0];
      line.originY += shadowOffset[1];
      metaData.lineMetrics.push_back(line);
      }

    // Set the color
    unsigned char origColor[3] = {metaData.rgba[0], metaData.rgba[1],
                                  metaData.rgba[2]};
    double shadowColor[3];
    metaData.textProperty->GetShadowColor(shadowColor);
    metaData.rgba[0] = static_cast<unsigned char>(shadowColor[0] * 255);
    metaData.rgba[1] = static_cast<unsigned char>(shadowColor[1] * 255);
    metaData.rgba[2] = static_cast<unsigned char>(shadowColor[2] * 255);

    if (!this->PopulateData(str, data, metaData))
      {
      vtkErrorMacro(<<"Error rendering shadow");
      return false;
      }

    // Restore color and line metrics
    metaData.lineMetrics = origMetrics;
    memcpy(metaData.rgba, origColor, 3 * sizeof(unsigned char));
    }

  // Render image
  return this->PopulateData(str, data, metaData);
}

//----------------------------------------------------------------------------
template <typename StringType>
bool vtkFreeTypeTools::StringToPathInternal(vtkTextProperty *tprop,
                                            const StringType &str,
                                            vtkPath *path)
{
  // Setup the metadata
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, metaData))
    {
    vtkErrorMacro(<<"Could not prepare metadata.");
    return false;
    }

  // Layout the text, calculate bounding box
  if (!this->CalculateBoundingBox(str, metaData))
    {
    vtkErrorMacro(<<"Could not calculate bounding box.");
    return false;
    }

  // Create the path
  if (!this->PopulateData(str, path, metaData))
    {
    vtkErrorMacro(<<"Could not populate path.");
    return false;
    }

  // Adjust for justification
  this->JustifyPath(path, metaData);
  return true;
}

//----------------------------------------------------------------------------
template <typename T>
bool vtkFreeTypeTools::CalculateBoundingBox(const T& str,
                                            MetaData &metaData)
{
  // Calculate the metrics for each line. These will be used to calculate
  // a bounding box, but first we need to know the maximum line length to
  // get justification right.
  metaData.lineMetrics.clear();
  metaData.maxLineWidth = 0;

  // Go through the string, line by line, and build the metrics data.
  typename T::const_iterator beginLine = str.begin();
  typename T::const_iterator endLine = std::find(beginLine, str.end(), '\n');
  while (endLine != str.end())
    {
    metaData.lineMetrics.push_back(MetaData::LineMetrics());
    this->GetLineMetrics(beginLine, endLine, metaData,
                         metaData.lineMetrics.back().width,
                         &metaData.lineMetrics.back().xmin);
    metaData.maxLineWidth = std::max(metaData.maxLineWidth,
                                     metaData.lineMetrics.back().width);
    beginLine = endLine;
    ++beginLine;
    endLine = std::find(beginLine, str.end(), '\n');
    }
  // Last line...
  metaData.lineMetrics.push_back(MetaData::LineMetrics());
  this->GetLineMetrics(beginLine, endLine, metaData,
                       metaData.lineMetrics.back().width,
                       &metaData.lineMetrics.back().xmin);
  metaData.maxLineWidth = std::max(metaData.maxLineWidth,
                                   metaData.lineMetrics.back().width);

  // Calculate line height from a reference set of characters, since the global
  // face values are usually way too big. This is the same string used to
  // determine height in vtkFreeTypeUtilities.
  const char *heightString = "_/7Agfy";
  int fontSize = metaData.textProperty->GetFontSize();
  metaData.ascent = 0;
  metaData.descent = 0;
  while (*heightString)
    {
    FT_BitmapGlyph bitmapGlyph;
    FT_UInt glyphIndex;
    FT_Bitmap *bitmap = this->GetBitmap(
          *heightString, metaData.textPropertyCacheId, fontSize, glyphIndex,
          bitmapGlyph);
    if (bitmap)
      {
      metaData.ascent = std::max(bitmapGlyph->top - 1, metaData.ascent);
      metaData.descent = std::min(-(bitmap->rows - (bitmapGlyph->top - 1)),
                                  metaData.descent);
      }
    ++heightString;
    }
  // Set line height. Descent is negative.
  metaData.height = metaData.ascent - metaData.descent;

  // sin, cos of orientation
  float angle = vtkMath::RadiansFromDegrees(
        metaData.textProperty->GetOrientation());
  float c = cos(angle);
  float s = sin(angle);

  // The base of the current line, and temp vars for line offsets and origins
  int pen[2] = {0, 0};
  int offset[2] = {0, 0};
  int origin[2] = {0, 0};

  // Initialize bbox
  metaData.bbox[0] = metaData.bbox[1] = pen[0];
  metaData.bbox[2] = metaData.bbox[3] = pen[1];

  // Compile the metrics data to determine the final bounding box. Set line
  // origins here, too.
  int justification = metaData.textProperty->GetJustification();
  for (size_t i = 0; i < metaData.lineMetrics.size(); ++i)
    {
    MetaData::LineMetrics &metrics = metaData.lineMetrics[i];

    // Apply justification
    origin[0] = pen[0];
    origin[1] = pen[1];
    if (justification != VTK_TEXT_LEFT)
      {
      int xShift = metaData.maxLineWidth - metrics.width;
      if (justification == VTK_TEXT_CENTERED)
        {
        xShift /= 2;
        }
      origin[0] += vtkMath::Floor(c * xShift + 0.5);
      origin[1] += vtkMath::Floor(s * xShift + 0.5);
      }

    // Set line origin
    metrics.originX = origin[0];
    metrics.originY = origin[1];

    // Merge bounding boxes
    metaData.bbox[0] = std::min(metaData.bbox[0], metrics.xmin + origin[0]);
    metaData.bbox[1] = std::max(metaData.bbox[1], metrics.xmax + origin[0]);
    metaData.bbox[2] = std::min(metaData.bbox[2], metrics.ymin + origin[1]);
    metaData.bbox[3] = std::max(metaData.bbox[3], metrics.ymax + origin[1]);

    // Calculate offset of next line
    offset[0] = 0;
    offset[1] = -std::ceil(metaData.height *
                           metaData.textProperty->GetLineSpacing());

    // Update pen position
    pen[0] += vtkMath::Floor(c * offset[0] - s * offset[1] + 0.5);
    pen[1] += vtkMath::Floor(s * offset[0] + c * offset[1] + 0.5);
    }

  // Adjust for shadow
  if (metaData.textProperty->GetShadow())
    {
    int shadowOffset[2];
    metaData.textProperty->GetShadowOffset(shadowOffset);
    if (shadowOffset[0] < 0)
      {
      metaData.bbox[0] += shadowOffset[0];
      }
    else
      {
      metaData.bbox[1] += shadowOffset[0];
      }
    if (shadowOffset[1] < 0)
      {
      metaData.bbox[2] += shadowOffset[1];
      }
    else
      {
      metaData.bbox[3] += shadowOffset[1];
      }
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::PrepareImageData(vtkImageData *data, int text_bbox[4])
{
  // The bounding box was the area that is going to be filled with pixels
  // given a text origin of (0, 0). Now get the real size we need, i.e.
  // the full extent from the origin to the bounding box.
  int text_size[2];
  text_size[0] = (text_bbox[1] - text_bbox[0] + 1);
  text_size[1] = (text_bbox[3] - text_bbox[2] + 1);

  // If the current image data is too small to render the text,
  // or more than twice as big (too hungry), then resize
  int img_dims[3], new_img_dims[3];
  data->GetDimensions(img_dims);

  if (data->GetScalarType() != VTK_UNSIGNED_CHAR ||
      data->GetNumberOfScalarComponents() != 4 ||
      img_dims[0] < text_size[0] || img_dims[1] < text_size[1] ||
      text_size[0] * 2 < img_dims[0] || text_size[1] * 2 < img_dims[1])
    {
    // Scale to the next highest power of 2 if required.
    if (this->ScaleToPowerTwo)
      {
      new_img_dims[0] =
          1 << static_cast<int>(ceil(log(static_cast<double>(text_size[0]+1)) /
                                     log(2.0)));
      new_img_dims[1] =
          1 << static_cast<int>(ceil(log(static_cast<double>(text_size[1]+1)) /
                                     log(2.0)));
      }
    else
      {
      new_img_dims[0] = text_size[0];
      new_img_dims[1] = text_size[1];
      }
    new_img_dims[2] = 1;

    // Set the extents to match the bbox + padding
    data->SetExtent(text_bbox[0], text_bbox[0] + new_img_dims[0] - 1,
                    text_bbox[2], text_bbox[2] + new_img_dims[1] - 1,
                    0, 0);

    if (data->GetScalarType() != VTK_UNSIGNED_CHAR ||
        data->GetNumberOfScalarComponents() != 4 ||
        new_img_dims[0] != img_dims[0] ||
        new_img_dims[1] != img_dims[1] ||
        new_img_dims[2] != img_dims[2])
      {
      data->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
      }
    }

  // Clear the image
  memset(data->GetScalarPointer(), 0,
         (data->GetNumberOfPoints() * data->GetNumberOfScalarComponents()));
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::JustifyPath(vtkPath *path, MetaData &metaData)
{
  double delta[2] = {0.0, 0.0};
  double bounds[6];

  vtkPoints *points = path->GetPoints();
  points->GetBounds(bounds);

  // Apply justification:
  switch (metaData.textProperty->GetJustification())
    {
    default:
    case VTK_TEXT_LEFT:
      delta[0] = -bounds[0];
      break;
    case VTK_TEXT_CENTERED:
      delta[0] = -(bounds[0] + bounds[1]) * 0.5;
      break;
    case VTK_TEXT_RIGHT:
      delta[0] = -bounds[1];
      break;
    }
  switch (metaData.textProperty->GetVerticalJustification())
    {
    default:
    case VTK_TEXT_BOTTOM:
      delta[1] = -bounds[2];
      break;
    case VTK_TEXT_CENTERED:
      delta[1] = -(bounds[2] + bounds[3]) * 0.5;
      break;
    case VTK_TEXT_TOP:
      delta[1] = -bounds[3];
    }

  if (delta[0] != 0 || delta[1] != 0) {
    for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
      {
      double *point = points->GetPoint(i);
      point[0] += delta[0];
      point[1] += delta[1];
      points->SetPoint(i, point);
      }
    }
}

//----------------------------------------------------------------------------
template <typename StringType, typename DataType>
bool vtkFreeTypeTools::PopulateData(const StringType &str, DataType data,
                                    MetaData &metaData)
{
  // Go through the string, line by line
  typename StringType::const_iterator beginLine = str.begin();
  typename StringType::const_iterator endLine =
      std::find(beginLine, str.end(), '\n');

  int lineIndex = 0;
  while (endLine != str.end())
    {
    if (!this->RenderLine(beginLine, endLine, lineIndex, data, metaData))
      {
      return false;
      }

    beginLine = endLine;
    ++beginLine;
    endLine = std::find(beginLine, str.end(), '\n');
    ++lineIndex;
    }

  // Render the last line:
  return this->RenderLine(beginLine, endLine, lineIndex, data, metaData);
}

//----------------------------------------------------------------------------
template <typename IteratorType, typename DataType>
bool vtkFreeTypeTools::RenderLine(IteratorType begin, IteratorType end,
                                  int lineIndex, DataType data,
                                  MetaData &metaData)
{
  int x = metaData.lineMetrics[lineIndex].originX;
  int y = metaData.lineMetrics[lineIndex].originY;

  // Render char by char
  FT_UInt previousGlyphIndex = 0; // for kerning
  for (; begin != end; ++begin)
    {
    this->RenderCharacter(*begin, x, y, previousGlyphIndex, data, metaData);
    }

  return true;
}

//----------------------------------------------------------------------------
template <typename CharType>
bool vtkFreeTypeTools::RenderCharacter(CharType character, int &x, int &y,
                                       FT_UInt &previousGlyphIndex,
                                       vtkImageData *image,
                                       MetaData &metaData)
{
  ImageMetaData *iMetaData = reinterpret_cast<ImageMetaData*>(&metaData);
  FT_BitmapGlyph bitmapGlyph;
  FT_UInt glyphIndex;
  FT_Bitmap *bitmap = this->GetBitmap(character, iMetaData->textPropertyCacheId,
                                      iMetaData->textProperty->GetFontSize(),
                                      glyphIndex, bitmapGlyph);

  if (!bitmap)
    {
    // TODO This should draw an empty rectangle.
    previousGlyphIndex = glyphIndex;
    return false;
    }

  if (bitmap->width && bitmap->rows)
    {
    // Starting position given the bearings.
    // Subtract 1 to the bearing Y, because this is the vertical distance
    // from the glyph origin (0,0) to the topmost pixel of the glyph bitmap
    // (more precisely, to the pixel just above the bitmap). This distance is
    // expressed in integer pixels, and is positive for upwards y.
    int penX = x + bitmapGlyph->left;
    int penY = y + bitmapGlyph->top - 1;

    // Add the kerning
    if (iMetaData->faceHasKerning && previousGlyphIndex && glyphIndex)
      {
      FT_Vector kerningDelta;
      if (FT_Get_Kerning(iMetaData->face, previousGlyphIndex, glyphIndex,
                         FT_KERNING_DEFAULT, &kerningDelta) == 0)
        {
        penX += kerningDelta.x >> 6;
        penY += kerningDelta.y >> 6;
        }
      }
    previousGlyphIndex = glyphIndex;

    // Render the current glyph into the image
    unsigned char *dataPtr =
        static_cast<unsigned char *>(image->GetScalarPointer(penX, penY, 0));
    if (!dataPtr)
      {
      return false;
      }

    int dataPitch = (-iMetaData->imageDimensions[0] - bitmap->width) *
        iMetaData->imageIncrements[0];
    unsigned char *glyphPtrRow = bitmap->buffer;
    unsigned char *glyphPtr;
    float tpropAlpha = iMetaData->rgba[3] / 255.0;

    for (int j = 0; j < bitmap->rows; ++j)
      {
      glyphPtr = glyphPtrRow;

      for (int i = 0; i < bitmap->width; ++i)
        {
        if (*glyphPtr != 0)
          {
          *dataPtr = iMetaData->rgba[0];
          ++dataPtr;
          *dataPtr = iMetaData->rgba[1];
          ++dataPtr;
          *dataPtr = iMetaData->rgba[2];
          ++dataPtr;
          *dataPtr = static_cast<unsigned char>((*glyphPtr) * tpropAlpha);
          ++dataPtr;
          ++glyphPtr;
          }
        else
          {
          dataPtr += 4;
          ++glyphPtr;
          }
        }
      glyphPtrRow += bitmap->pitch;
      dataPtr += dataPitch;
      }
    }

  // Advance to next char
  x += (bitmapGlyph->root.advance.x + 0x8000) >> 16;
  y += (bitmapGlyph->root.advance.y + 0x8000) >> 16;
  return true;
}

//----------------------------------------------------------------------------
template <typename CharType>
bool vtkFreeTypeTools::RenderCharacter(CharType character, int &x, int &y,
                                       FT_UInt &previousGlyphIndex,
                                       vtkPath *path, MetaData &metaData)
{
  // The FT_CURVE defines don't really work in a switch...only the first two
  // bits are meaningful, and the rest appear to be garbage. We'll convert them
  // into values in the enum below:
  enum controlType
    {
    FIRST_POINT,
    ON_POINT,
    CUBIC_POINT,
    CONIC_POINT
    };

  FT_UInt glyphIndex;
  FT_OutlineGlyph outlineGlyph;
  FT_Outline *outline = this->GetOutline(character,
                                         metaData.textPropertyCacheId,
                                         metaData.textProperty->GetFontSize(),
                                         glyphIndex, outlineGlyph);
  if (!outline)
    {
    // TODO render an empty box.
    return false;
    }

  if (outline->n_points > 0)
    {
    int pen_x = x;
    int pen_y = y;

    // Add the kerning
    if (metaData.faceHasKerning && previousGlyphIndex && glyphIndex)
      {
      FT_Vector kerningDelta;
      FT_Get_Kerning(metaData.face, previousGlyphIndex, glyphIndex,
                     FT_KERNING_DEFAULT, &kerningDelta);
      pen_x += kerningDelta.x >> 6;
      pen_y += kerningDelta.y >> 6;
      }
    previousGlyphIndex = glyphIndex;

    short point = 0;
    for (short contour = 0; contour < outline->n_contours; ++contour)
      {
      short contourEnd = outline->contours[contour];
      controlType lastTag = FIRST_POINT;
      double contourStartVec[2];
      contourStartVec[0] = contourStartVec[1] = 0.0;
      double lastVec[2];
      lastVec[0] = lastVec[1] = 0.0;
      for (; point <= contourEnd; ++point)
        {
        FT_Vector ftvec = outline->points[point];
        char fttag = outline->tags[point];
        controlType tag = FIRST_POINT;
        if (fttag & FT_CURVE_TAG_ON)
          {
          tag = ON_POINT;
          }
        else if (fttag & FT_CURVE_TAG_CUBIC)
          {
          tag = CUBIC_POINT;
          }
        else if (fttag & FT_CURVE_TAG_CONIC)
          {
          tag = CONIC_POINT;
          }

        double vec[2];
        vec[0] = ftvec.x / 64.0 + x;
        vec[1] = ftvec.y / 64.0 + y;

        // Handle the first point here, unless it is a CONIC point, in which
        // case the switches below handle it.
        if (lastTag == FIRST_POINT && tag != CONIC_POINT)
          {
          path->InsertNextPoint(vec[0], vec[1], 0.0, vtkPath::MOVE_TO);
          lastTag = tag;
          lastVec[0] = vec[0];
          lastVec[1] = vec[1];
          contourStartVec[0] = vec[0];
          contourStartVec[1] = vec[1];
          continue;
          }

        switch (tag)
          {
          case ON_POINT:
            switch(lastTag)
              {
              case ON_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0, vtkPath::LINE_TO);
                break;
              case CONIC_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0,
                    vtkPath::CONIC_CURVE);
                break;
              case CUBIC_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0,
                    vtkPath::CUBIC_CURVE);
                break;
              case FIRST_POINT:
              default:
                break;
              }
            break;
          case CONIC_POINT:
            switch(lastTag)
              {
              case ON_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0,
                    vtkPath::CONIC_CURVE);
                break;
              case CONIC_POINT: {
                // Two conic points indicate a virtual "ON" point between
                // them. Insert both points.
                double virtualOn[2] = {(vec[0] + lastVec[0]) * 0.5,
                                       (vec[1] + lastVec[1]) * 0.5};
                path->InsertNextPoint(virtualOn[0], virtualOn[1], 0.0,
                    vtkPath::CONIC_CURVE);
                path->InsertNextPoint(vec[0], vec[1], 0.0,
                    vtkPath::CONIC_CURVE);
                }
                break;
              case FIRST_POINT: {
                // The first point in the contour can be a conic control
                // point. Use the last point of the contour as the starting
                // point. If the last point is a conic point as well, start
                // on a virtual point between the two:
                FT_Vector lastContourFTVec = outline->points[contourEnd];
                double lastContourVec[2] = {lastContourFTVec.x / 64.0 + x,
                                            lastContourFTVec.y / 64.0 + y};
                char lastContourFTTag = outline->tags[contourEnd];
                if (lastContourFTTag & FT_CURVE_TAG_CONIC)
                  {
                  double virtualOn[2] = {(vec[0] + lastContourVec[0]) * 0.5,
                                         (vec[1] + lastContourVec[1]) * 0.5};
                  path->InsertNextPoint(virtualOn[0], virtualOn[1],
                      0.0, vtkPath::MOVE_TO);
                  path->InsertNextPoint(vec[0], vec[1], 0.0,
                      vtkPath::CONIC_CURVE);
                  }
                else
                  {
                  path->InsertNextPoint(lastContourVec[0], lastContourVec[1],
                      0.0, vtkPath::MOVE_TO);
                  path->InsertNextPoint(vec[0], vec[1], 0.0,
                      vtkPath::CONIC_CURVE);
                  }
                }
                break;
              case CUBIC_POINT:
              default:
                break;
              }
            break;
          case CUBIC_POINT:
            switch(lastTag)
              {
              case ON_POINT:
              case CUBIC_POINT:
                path->InsertNextPoint(vec[0], vec[1], 0.0,
                    vtkPath::CUBIC_CURVE);
                break;
              case CONIC_POINT:
              case FIRST_POINT:
              default:
                break;
              }
            break;
          case FIRST_POINT:
          default:
            break;
          } // end switch

        lastTag = tag;
        lastVec[0] = vec[0];
        lastVec[1] = vec[1];
        } // end contour

      // The contours are always implicitly closed to the start point of the
      // contour:
      switch (lastTag)
        {
        case ON_POINT:
          path->InsertNextPoint(contourStartVec[0], contourStartVec[1], 0.0,
              vtkPath::LINE_TO);
          break;
        case CUBIC_POINT:
          path->InsertNextPoint(contourStartVec[0], contourStartVec[1], 0.0,
              vtkPath::CUBIC_CURVE);
          break;
        case CONIC_POINT:
          path->InsertNextPoint(contourStartVec[0], contourStartVec[1], 0.0,
              vtkPath::CONIC_CURVE);
          break;
        case FIRST_POINT:
        default:
          break;
        } // end switch (lastTag)
      } // end contour points iteration
    } // end contour iteration

  // Advance to next char
  x += (outlineGlyph->root.advance.x + 0x8000) >> 16;
  y += (outlineGlyph->root.advance.y + 0x8000) >> 16;
  return true;
}

//----------------------------------------------------------------------------
// Similar to implementations in vtkFreeTypeUtilities and vtkTextMapper.
template <typename T>
int vtkFreeTypeTools::FitStringToBBox(const T &str, MetaData &metaData,
                                      int targetWidth, int targetHeight)
{
  if (str.empty() || targetWidth == 0 || targetHeight == 0 ||
      metaData.textProperty == 0)
    {
    return 0;
    }

  // Use the current font size as a first guess
  int size[2];
  int fontSize = metaData.textProperty->GetFontSize();
  if (!this->CalculateBoundingBox(str, metaData))
    {
    return -1;
    }
  size[0] = metaData.bbox[1] - metaData.bbox[0];
  size[1] = metaData.bbox[3] - metaData.bbox[2];

  // Bad assumption but better than nothing -- assume the bbox grows linearly
  // with the font size:
  if (size[0] != 0 && size[1] != 0)
    {
    fontSize *= std::min(
          static_cast<double>(targetWidth)  / static_cast<double>(size[0]),
        static_cast<double>(targetHeight) / static_cast<double>(size[1]));
    metaData.textProperty->SetFontSize(fontSize);
    if (!this->CalculateBoundingBox(str, metaData))
      {
      return -1;
      }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
    }

  // Now just step up/down until the bbox matches the target.
  while (size[0] < targetWidth && size[1] < targetHeight && fontSize < 200)
    {
    metaData.textProperty->SetFontSize(++fontSize);
    if (!this->CalculateBoundingBox(str, metaData))
      {
      return -1;
      }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
    }

  while ((size[0] > targetWidth || size[1] > targetHeight) && fontSize > 0)
    {
    metaData.textProperty->SetFontSize(--fontSize);
    if (!this->CalculateBoundingBox(str, metaData))
      {
      return -1;
      }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
    }

  return fontSize;
}

//----------------------------------------------------------------------------
inline bool vtkFreeTypeTools::GetFace(vtkTextProperty *prop,
                                      unsigned long &prop_cache_id,
                                      FT_Face &face, bool &face_has_kerning)
{
  this->MapTextPropertyToId(prop, &prop_cache_id);
  if (!this->GetFace(prop_cache_id, &face))
    {
    vtkErrorMacro(<< "Failed retrieving the face");
    return false;
    }
  face_has_kerning = (FT_HAS_KERNING(face) != 0);
  return true;
}

//----------------------------------------------------------------------------
inline FT_Bitmap* vtkFreeTypeTools::GetBitmap(FT_UInt32 c,
                                              unsigned long prop_cache_id,
                                              int prop_font_size,
                                              FT_UInt &gindex,
                                              FT_BitmapGlyph &bitmap_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(prop_cache_id, c, &gindex))
    {
    return 0;
    }
  FT_Glyph glyph;
  // Get the glyph as a bitmap
  if (!this->GetGlyph(prop_cache_id,
                      prop_font_size,
                      gindex,
                      &glyph,
                      vtkFreeTypeTools::GLYPH_REQUEST_BITMAP) ||
                        glyph->format != ft_glyph_format_bitmap)
    {
    return 0;
    }

  bitmap_glyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
  FT_Bitmap *bitmap = &bitmap_glyph->bitmap;

  if (bitmap->pixel_mode != ft_pixel_mode_grays)
    {
    return 0;
    }

  return bitmap;
}

//----------------------------------------------------------------------------
inline FT_Outline *vtkFreeTypeTools::GetOutline(FT_UInt32 c,
                                                unsigned long prop_cache_id,
                                                int prop_font_size,
                                                FT_UInt &gindex,
                                                FT_OutlineGlyph &outline_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(prop_cache_id, c, &gindex))
    {
    return 0;
    }
  FT_Glyph glyph;
  // Get the glyph as a outline
  if (!this->GetGlyph(prop_cache_id,
                      prop_font_size,
                      gindex,
                      &glyph,
                      vtkFreeTypeTools::GLYPH_REQUEST_OUTLINE) ||
                        glyph->format != ft_glyph_format_outline)
    {
    return 0;
    }

  outline_glyph = reinterpret_cast<FT_OutlineGlyph>(glyph);
  FT_Outline *outline= &outline_glyph->outline;

  return outline;
}

//----------------------------------------------------------------------------
template<typename T>
void vtkFreeTypeTools::GetLineMetrics(T begin, T end, MetaData &metaData,
                                      int &width, int bbox[4])
{
  FT_Matrix inverseRotation;
  bool isRotated = (fabs(metaData.textProperty->GetOrientation()) > 1e-5);
  if (isRotated)
    {
    float angle = -vtkMath::RadiansFromDegrees(
          static_cast<float>(metaData.textProperty->GetOrientation()));
    float c = cos(angle);
    float s = sin(angle);
    inverseRotation.xx = (FT_Fixed)( c * 0x10000L);
    inverseRotation.xy = (FT_Fixed)(-s * 0x10000L);
    inverseRotation.yx = (FT_Fixed)( s * 0x10000L);
    inverseRotation.yy = (FT_Fixed)( c * 0x10000L);
    }

  FT_BitmapGlyph bitmapGlyph;
  FT_UInt gindex = 0;
  FT_UInt gindexLast = 0;
  FT_Vector delta;
  width = 0;
  int pen[2] = {0, 0};
  bbox[0] = bbox[1] = pen[0];
  bbox[2] = bbox[3] = pen[1];
  int fontSize = metaData.textProperty->GetFontSize();
  for (; begin != end; ++begin)
    {
    // Adjust the pen location for kerning
    if (metaData.faceHasKerning && gindexLast && gindex)
      {
      if (FT_Get_Kerning(metaData.face, gindexLast, gindex, FT_KERNING_DEFAULT,
                         &delta) == 0)
        {
        pen[0] += delta.x >> 6;
        pen[1] += delta.y >> 6;
        if (isRotated)
          {
          FT_Vector_Transform(&delta, &inverseRotation);
          }
        width += delta.x >> 6;
        }
      }

    // Use the dimensions of the bitmap glyph to get a tight bounding box.
    FT_Bitmap *bitmap = this->GetBitmap(*begin, metaData.textPropertyCacheId,
                                        fontSize, gindex, bitmapGlyph);
    if (bitmap)
      {
      bbox[0] = std::min(bbox[0], pen[0] + bitmapGlyph->left);
      bbox[1] = std::max(bbox[1], pen[0] + bitmapGlyph->left + bitmap->width);
      bbox[2] = std::min(bbox[2], pen[1] + bitmapGlyph->top - 1 - bitmap->rows);
      bbox[3] = std::max(bbox[3], pen[1] + bitmapGlyph->top - 1);
      }
    else
      {
      // FIXME: do something more elegant here.
      // We should render an empty rectangle to adhere to the specs...
      continue;
      }

    // Update advance.
    delta = bitmapGlyph->root.advance;
    pen[0] += (delta.x + 0x8000) >> 16;
    pen[1] += (delta.y + 0x8000) >> 16;

    if (isRotated)
      {
      FT_Vector_Transform(&delta, &inverseRotation);
      }
    width += (delta.x + 0x8000) >> 16;
    }
}
