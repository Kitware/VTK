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
#include "vtkNew.h"
#include "vtkPath.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtkStdString.h"
#include "vtkUnicodeString.h"

// The embedded fonts
#include "fonts/vtkEmbeddedFonts.h"

#ifndef _MSC_VER
# include <stdint.h>
#endif

#include <limits>
#include <cassert>
#include <algorithm>
#include <map>
#include <vector>
#include <sstream>
#include <limits>

// Print debug info
#define VTK_FTFC_DEBUG 0
#define VTK_FTFC_DEBUG_CD 0

namespace {
// Some helper functions:
void rotateVector2i(vtkVector2i &vec, float sinTheta, float cosTheta)
{
  vec = vtkVector2i(vtkMath::Round(cosTheta * vec[0] - sinTheta * vec[1]),
                    vtkMath::Round(sinTheta * vec[0] + cosTheta * vec[1]));
}

} // end anon namespace

class vtkTextPropertyLookup
    : public std::map<size_t, vtkSmartPointer<vtkTextProperty> >
{
public:
  bool contains(const size_t id) {return this->find(id) != this->end();}
};

class vtkFreeTypeTools::MetaData
{
public:
  // Set by PrepareMetaData
  vtkTextProperty *textProperty;
  size_t textPropertyCacheId;
  size_t unrotatedTextPropertyCacheId;
  FTC_ScalerRec scaler;
  FTC_ScalerRec unrotatedScaler;
  FT_Face face;
  bool faceHasKerning;
  bool faceIsRotated;
  FT_Matrix rotation;
  FT_Matrix inverseRotation;

  // Set by CalculateBoundingBox
  int ascent;    // position of the highest point of character from baseline which
                 // has position 0. Negative if below baseline.
  int descent;   // position of the the lowest point of character from baseline which
                 // has position 0. Negative if below baseline
  int height;
  struct LineMetrics {
    vtkVector2i origin;
    int width;
    // bbox relative to origin[XY]:
    int xmin;
    int xmax;
    int ymin;
    int ymax;
  };
  vtkVector2i dx; // Vector representing the data width after rotation
  vtkVector2i dy; // Vector representing the data height after rotation
  vtkVector2i TL; // Top left corner of the rotated data
  vtkVector2i TR; // Top right corner of the rotated data
  vtkVector2i BL; // Bottom left corner of the rotated data
  vtkVector2i BR; // Bottom right corner of the rotated data
  std::vector<LineMetrics> lineMetrics;
  int maxLineWidth;
  vtkTuple<int, 4> bbox;
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
// The singleton, and the singleton cleanup counter
vtkFreeTypeTools* vtkFreeTypeTools::Instance;
static unsigned int vtkFreeTypeToolsCleanupCounter;

//----------------------------------------------------------------------------
// The embedded fonts
// Create a lookup table between the text mapper attributes
// and the font buffers.
struct EmbeddedFontStruct
{
  size_t length;
  unsigned char *ptr;
};

//------------------------------------------------------------------------------
// Clean up the vtkFreeTypeTools instance at exit. Using a separate class allows
// us to delay initialization of the vtkFreeTypeTools class.
vtkFreeTypeToolsCleanup::vtkFreeTypeToolsCleanup()
{
  vtkFreeTypeToolsCleanupCounter++;
}

vtkFreeTypeToolsCleanup::~vtkFreeTypeToolsCleanup()
{
  if (--vtkFreeTypeToolsCleanupCounter == 0)
  {
    vtkFreeTypeTools::SetInstance(NULL);
  }
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
      vtkFreeTypeTools::Instance->InitializeObjectBase();
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
  this->DebugTextures = false;
  this->MaximumNumberOfFaces = 30; // combinations of family+bold+italic
  this->MaximumNumberOfSizes = this->MaximumNumberOfFaces * 20; // sizes
  this->MaximumNumberOfBytes = 300000UL * this->MaximumNumberOfSizes;
  this->TextPropertyLookup = new vtkTextPropertyLookup ();
  this->CacheManager = NULL;
  this->ImageCache   = NULL;
  this->CMapCache    = NULL;
  this->ScaleToPowerTwo = true;

  // Ideally this should be thread-local to support SMP:
  FT_Error err;
  this->Library = new FT_Library;
  err = FT_Init_FreeType(this->Library);
  if (err)
  {
    vtkErrorMacro("FreeType library initialization failed with error code: "
                  << err << ".");
    delete this->Library;
    this->Library = NULL;
  }
}

//----------------------------------------------------------------------------
vtkFreeTypeTools::~vtkFreeTypeTools()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::~vtkFreeTypeTools\n");
#endif
  this->ReleaseCacheManager();
  delete TextPropertyLookup;

  FT_Done_FreeType(*this->Library);
  delete this->Library;
  this->Library = NULL;
}

//----------------------------------------------------------------------------
FT_Library* vtkFreeTypeTools::GetLibrary()
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetLibrary\n");
#endif

  return this->Library;
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

  delete this->ImageCache;
  this->ImageCache = NULL;

  delete this->CMapCache;
  this->CMapCache = NULL;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetBoundingBox(vtkTextProperty *tprop,
                                      const vtkStdString& str, int dpi,
                                      int bbox[4])
{
  // We need the tprop and bbox
  if (!tprop || !bbox)
  {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL or zero");
    return false;
  }

  if (str.empty())
  {
    std::fill(bbox, bbox + 4, 0);
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      memcpy(bbox, metaData.bbox.GetData(), sizeof(int) * 4);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetBoundingBox(vtkTextProperty *tprop,
                                      const vtkUnicodeString& str, int dpi,
                                      int bbox[4])
{
  // We need the tprop and bbox
  if (!tprop || !bbox)
  {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL or zero");
    return false;
  }

  if (str.empty())
  {
    std::fill(bbox, bbox + 4, 0);
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      memcpy(bbox, metaData.bbox.GetData(), sizeof(int) * 4);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetMetrics(vtkTextProperty *tprop,
                                  const vtkStdString &str, int dpi,
                                  vtkTextRenderer::Metrics &metrics)
{
  if (!tprop)
  {
    vtkErrorMacro(<< "NULL text property.");
    return false;
  }

  if (str.empty())
  {
    metrics = vtkTextRenderer::Metrics();
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      metrics.BoundingBox = metaData.bbox;
      metrics.TopLeft     = metaData.TL;
      metrics.TopRight    = metaData.TR;
      metrics.BottomLeft  = metaData.BL;
      metrics.BottomRight = metaData.BR;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetMetrics(vtkTextProperty *tprop,
                                  const vtkUnicodeString &str, int dpi,
                                  vtkTextRenderer::Metrics &metrics)
{
  if (!tprop)
  {
    vtkErrorMacro(<< "NULL text property.");
    return false;
  }

  if (str.empty())
  {
    metrics = vtkTextRenderer::Metrics();
    return true;
  }

  MetaData metaData;
  bool result = this->PrepareMetaData(tprop, dpi, metaData);
  if (result)
  {
    result = this->CalculateBoundingBox(str, metaData);
    if (result)
    {
      metrics.BoundingBox = metaData.bbox;
      metrics.TopLeft     = metaData.TL;
      metrics.TopRight    = metaData.TR;
      metrics.BottomLeft  = metaData.BL;
      metrics.BottomRight = metaData.BR;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::RenderString(vtkTextProperty *tprop,
                                    const vtkStdString& str, int dpi,
                                    vtkImageData *data, int textDims[2])
{
  return this->RenderStringInternal(tprop, str, dpi, data, textDims);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::RenderString(vtkTextProperty *tprop,
                                    const vtkUnicodeString& str, int dpi,
                                    vtkImageData *data, int textDims[2])
{
  return this->RenderStringInternal(tprop, str, dpi, data, textDims);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::StringToPath(vtkTextProperty *tprop,
                                    const vtkStdString &str, int dpi,
                                    vtkPath *path)
{
  return this->StringToPathInternal(tprop, str, dpi, path);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::StringToPath(vtkTextProperty *tprop,
                                    const vtkUnicodeString &str, int dpi,
                                    vtkPath *path)
{
  return this->StringToPathInternal(tprop, str, dpi, path);
}

//----------------------------------------------------------------------------
int vtkFreeTypeTools::GetConstrainedFontSize(const vtkStdString &str,
                                             vtkTextProperty *tprop, int dpi,
                                             int targetWidth, int targetHeight)
{
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, dpi, metaData))
  {
    vtkErrorMacro(<<"Could not prepare metadata.");
    return false;
  }
  return this->FitStringToBBox(str, metaData, targetWidth, targetHeight);
}

//----------------------------------------------------------------------------
int vtkFreeTypeTools::GetConstrainedFontSize(const vtkUnicodeString &str,
                                             vtkTextProperty *tprop, int dpi,
                                             int targetWidth, int targetHeight)
{
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, dpi, metaData))
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
vtkTypeUInt32 vtkFreeTypeTools::HashBuffer(const void *buffer, size_t n, vtkTypeUInt32 hash)
{
  if (buffer == NULL)
  {
    return 0;
  }

  const char* key = reinterpret_cast<const char*>(buffer);

  // Jenkins hash function
  for (size_t i = 0; i < n; ++i)
  {
    hash += key[i];
    hash += (hash << 10);
    hash += (hash << 15);
  }

  return hash;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::MapTextPropertyToId(vtkTextProperty *tprop,
                                           size_t *id)
{
  if (!tprop || !id)
  {
    vtkErrorMacro(<< "Wrong parameters, one of them is NULL");
    return;
  }

  // The font family is hashed into 16 bits (= 17 bits so far)
  const char* fontFamily = tprop->GetFontFamily() != VTK_FONT_FILE
    ? tprop->GetFontFamilyAsString()
    : tprop->GetFontFile();
  size_t fontFamilyLength = 0;
  if (fontFamily)
  {
    fontFamilyLength = strlen(fontFamily);
  }
  vtkTypeUInt32 hash =
    vtkFreeTypeTools::HashBuffer(fontFamily, fontFamilyLength);

  // Create a "string" of text properties
  unsigned char ucValue = tprop->GetBold();
  hash = vtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  ucValue = tprop->GetItalic();
  hash = vtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  ucValue = tprop->GetShadow();
  hash = vtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  hash = vtkFreeTypeTools::HashBuffer(
    tprop->GetColor(), 3*sizeof(double), hash);
  double dValue = tprop->GetOpacity();
  hash = vtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  hash = vtkFreeTypeTools::HashBuffer(
    tprop->GetBackgroundColor(), 3*sizeof(double), hash);
  dValue = tprop->GetBackgroundOpacity();
  hash = vtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  hash = vtkFreeTypeTools::HashBuffer(
    tprop->GetFrameColor(), 3*sizeof(double), hash);
  ucValue = tprop->GetFrame();
  hash = vtkFreeTypeTools::HashBuffer(&ucValue, sizeof(unsigned char), hash);
  int iValue = tprop->GetFrameWidth();
  hash = vtkFreeTypeTools::HashBuffer(&iValue, sizeof(int), hash);
  iValue = tprop->GetFontSize();
  hash = vtkFreeTypeTools::HashBuffer(&iValue, sizeof(int), hash);
  hash = vtkFreeTypeTools::HashBuffer(
    tprop->GetShadowOffset(), 2*sizeof(int), hash);
  dValue = tprop->GetOrientation();
  hash = vtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  hash = vtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  dValue = tprop->GetLineSpacing();
  hash = vtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);
  dValue = tprop->GetLineOffset();
  hash = vtkFreeTypeTools::HashBuffer(&dValue, sizeof(double), hash);

  // Set the first bit to avoid id = 0
  // (the id will be mapped to a pointer, FTC_FaceID, so let's avoid NULL)
  *id = 1;

  // Add in the hash.
  // We're dropping a bit here, but that should be okay.
  *id |= hash << 1;

  // Insert the TextProperty into the lookup table
  if (!this->TextPropertyLookup->contains(*id))
    (*this->TextPropertyLookup)[*id] = tprop;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::MapIdToTextProperty(size_t id,
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
bool vtkFreeTypeTools::GetSize(size_t tprop_cache_id,
                               int font_size,
                               FT_Size *size)
{
  if (!size || font_size <= 0)
  {
    vtkErrorMacro(<< "Wrong parameters, size is NULL or invalid font size");
    return 0;
  }

  // Map the id of a text property in the cache to a FTC_FaceID
  FTC_FaceID face_id = reinterpret_cast<FTC_FaceID>(tprop_cache_id);

  FTC_ScalerRec scaler_rec;
  scaler_rec.face_id = face_id;
  scaler_rec.width = font_size;
  scaler_rec.height = font_size;
  scaler_rec.pixel = 1;

  return this->GetSize(&scaler_rec, size);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetSize(FTC_Scaler scaler, FT_Size *size)
{
#if VTK_FTFC_DEBUG_CD
  printf("vtkFreeTypeTools::GetSize()\n");
#endif

  if (!size)
  {
    vtkErrorMacro(<< "Size is NULL.");
    return 0;
  }

  FTC_Manager *manager = this->GetCacheManager();
  if (!manager)
  {
    vtkErrorMacro(<< "Failed querying the cache manager !");
    return 0;
  }

  FT_Error error = FTC_Manager_LookupSize(*manager, scaler, size);
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
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetSize(tprop_cache_id, tprop->GetFontSize(), size);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetFace(size_t tprop_cache_id,
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
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetFace(tprop_cache_id, face);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetGlyphIndex(size_t tprop_cache_id,
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
  size_t tprop_cache_id;
  this->MapTextPropertyToId(tprop, &tprop_cache_id);

  return this->GetGlyphIndex(tprop_cache_id, c, gindex);
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::GetGlyph(size_t tprop_cache_id,
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
bool vtkFreeTypeTools::GetGlyph(FTC_Scaler scaler, FT_UInt gindex,
                                FT_Glyph *glyph, int request)
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

  FT_ULong loadFlags = FT_LOAD_DEFAULT;
  if (request == GLYPH_REQUEST_BITMAP)
  {
    loadFlags |= FT_LOAD_RENDER;
  }
  else if (request == GLYPH_REQUEST_OUTLINE)
  {
    loadFlags |= FT_LOAD_NO_BITMAP;
  }

  // Lookup the glyph
  FT_Error error = FTC_ImageCache_LookupScaler(
        *image_cache, scaler, loadFlags, gindex, glyph, NULL);

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
  else if (family == VTK_FONT_FILE)
  {
    vtkDebugWithObjectMacro(tprop,
                            << "Attempting to load font from file: "
                            << tprop->GetFontFile());

    if (FT_New_Face(lib, tprop->GetFontFile(), 0, face) == 0)
    {
      return true;
    }

    vtkDebugWithObjectMacro(
          tprop,
          << "Error loading font from file '" << tprop->GetFontFile()
          << "'. Falling back to arial.");
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
  size_t tprop_cache_id;
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
inline bool vtkFreeTypeTools::PrepareMetaData(vtkTextProperty *tprop, int dpi,
                                              MetaData &metaData)
{
  // Text properties
  metaData.textProperty = tprop;
  this->MapTextPropertyToId(tprop, &metaData.textPropertyCacheId);

  metaData.scaler.face_id =
      reinterpret_cast<FTC_FaceID>(metaData.textPropertyCacheId);
  metaData.scaler.width = tprop->GetFontSize() * 64; // 26.6 format point size
  metaData.scaler.height = tprop->GetFontSize() * 64;
  metaData.scaler.pixel = 0;
  metaData.scaler.x_res = dpi;
  metaData.scaler.y_res = dpi;

  FT_Size size;
  if (!this->GetSize(&metaData.scaler, &size))
  {
    return false;
  }

  metaData.face = size->face;
  metaData.faceHasKerning = (FT_HAS_KERNING(metaData.face) != 0);

  // Store an unrotated version of this font, as we'll need this to get accurate
  // ascenders/descenders (see CalculateBoundingBox).
  if (tprop->GetOrientation() != 0.0)
  {
    vtkNew<vtkTextProperty> unrotatedTProp;
    unrotatedTProp->ShallowCopy(tprop);
    unrotatedTProp->SetOrientation(0);
    this->MapTextPropertyToId(unrotatedTProp.GetPointer(),
                              &metaData.unrotatedTextPropertyCacheId);

    metaData.unrotatedScaler.face_id =
        reinterpret_cast<FTC_FaceID>(metaData.unrotatedTextPropertyCacheId);
    metaData.unrotatedScaler.width = tprop->GetFontSize() * 64; // 26.6 format point size
    metaData.unrotatedScaler.height = tprop->GetFontSize() * 64;
    metaData.unrotatedScaler.pixel = 0;
    metaData.unrotatedScaler.x_res = dpi;
    metaData.unrotatedScaler.y_res = dpi;
  }
  else
  {
    metaData.unrotatedTextPropertyCacheId = metaData.textPropertyCacheId;
    metaData.unrotatedScaler = metaData.scaler;
  }

  // Rotation matrices:
  metaData.faceIsRotated =
      (fabs(metaData.textProperty->GetOrientation()) > 1e-5);
  if (metaData.faceIsRotated)
  {
    float angle = vtkMath::RadiansFromDegrees(
          static_cast<float>(metaData.textProperty->GetOrientation()));
    // 0 -> orientation (used to adjust kerning, PR#15301)
    float c = cos(angle);
    float s = sin(angle);
    metaData.rotation.xx = (FT_Fixed)( c * 0x10000L);
    metaData.rotation.xy = (FT_Fixed)(-s * 0x10000L);
    metaData.rotation.yx = (FT_Fixed)( s * 0x10000L);
    metaData.rotation.yy = (FT_Fixed)( c * 0x10000L);

    // orientation -> 0 (used for width calculations)
    c = cos(-angle);
    s = sin(-angle);
    metaData.inverseRotation.xx = (FT_Fixed)( c * 0x10000L);
    metaData.inverseRotation.xy = (FT_Fixed)(-s * 0x10000L);
    metaData.inverseRotation.yx = (FT_Fixed)( s * 0x10000L);
    metaData.inverseRotation.yy = (FT_Fixed)( c * 0x10000L);
  }

  return true;
}

//----------------------------------------------------------------------------
template <typename StringType>
bool vtkFreeTypeTools::RenderStringInternal(vtkTextProperty *tprop,
                                            const StringType &str,
                                            int dpi,
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
    data->Initialize();
    if (textDims)
    {
      textDims[0] = 0;
      textDims[1] = 0;
    }
    return true;
  }

  ImageMetaData metaData;

  // Setup the metadata cache
  if (!this->PrepareMetaData(tprop, dpi, metaData))
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
  this->PrepareImageData(data, metaData.bbox.GetData());

  // Setup the image metadata
  if (!this->PrepareImageMetaData(tprop, data, metaData))
  {
    vtkErrorMacro(<<"Error prepare image metadata.");
    return false;
  }

  // Render the background:
  this->RenderBackground(tprop, data, metaData);

  // Render shadow if needed
  if (metaData.textProperty->GetShadow())
  {
    // Modify the line offsets with the shadow offset
    vtkVector2i shadowOffset;
    metaData.textProperty->GetShadowOffset(shadowOffset.GetData());
    std::vector<MetaData::LineMetrics> origMetrics = metaData.lineMetrics;
    metaData.lineMetrics.clear();
    for (std::vector<MetaData::LineMetrics>::const_iterator
         it = origMetrics.begin(), itEnd = origMetrics.end(); it < itEnd; ++it)
    {
      MetaData::LineMetrics line = *it;
      line.origin = line.origin + shadowOffset;
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

  // Mark the image data as modified, as it is possible that only
  // vtkImageData::Get*Pointer methods will be called, which do not update the
  // MTime.
  data->Modified();

  // Render image
  if (!this->PopulateData(str, data, metaData))
  {
    vtkErrorMacro(<<"Error rendering text.");
    return false;
  }

  // Draw a red dot at the anchor point:
  if (this->DebugTextures)
  {
    unsigned char *ptr =
        static_cast<unsigned char *>(data->GetScalarPointer(0, 0, 0));
    if (ptr)
    {
      ptr[0] = 255;
      ptr[1] = 0;
      ptr[2] = 0;
      ptr[3] = 255;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
template <typename StringType>
bool vtkFreeTypeTools::StringToPathInternal(vtkTextProperty *tprop,
                                            const StringType &str,
                                            int dpi,
                                            vtkPath *path)
{
  // Setup the metadata
  MetaData metaData;
  if (!this->PrepareMetaData(tprop, dpi, metaData))
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

  return true;
}

namespace
{
const char* DEFAULT_HEIGHT_STRING = "_/7Agfy";
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::CalculateBoundingBox(const vtkUnicodeString& str, MetaData &metaData)
{
  return CalculateBoundingBox(str, metaData, vtkUnicodeString::from_utf8(DEFAULT_HEIGHT_STRING));
}

//----------------------------------------------------------------------------
bool vtkFreeTypeTools::CalculateBoundingBox(const vtkStdString& str, MetaData &metaData)
{
  return CalculateBoundingBox(str, metaData, vtkStdString(DEFAULT_HEIGHT_STRING));
}

//----------------------------------------------------------------------------
template <typename T>
bool vtkFreeTypeTools::CalculateBoundingBox(const T& str,
                                            MetaData &metaData, const T& defaultHeightString)
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

  int numLines = metaData.lineMetrics.size();
  T heightString;
  if (metaData.textProperty->GetUseTightBoundingBox() && numLines == 1)
  {
    // Calculate line hight from actual characters. This works only for single line text
    // and may result in a hight that does not include descent. It is used to get
    // a centered label.
    heightString = str;
  }
  else
  {
    // Calculate line height from a reference set of characters, since the global
    // face values are usually way too big.
    heightString = defaultHeightString;
  }
  metaData.ascent = std::numeric_limits<int>::min();
  metaData.descent = std::numeric_limits<int>::max();
  typename T::const_iterator it = heightString.begin();
  while (it != heightString.end())
  {
    FT_BitmapGlyph bitmapGlyph;
    FT_UInt glyphIndex;
    // Use the unrotated face to get correct metrics:
    FT_Bitmap *bitmap = this->GetBitmap(
          *it, &metaData.unrotatedScaler, glyphIndex, bitmapGlyph);
    if (bitmap)
    {
      metaData.ascent = std::max(bitmapGlyph->top - 1, metaData.ascent);
      metaData.descent = std::min(-static_cast<int>((bitmap->rows -
                                                     bitmapGlyph->top)),
                                  metaData.descent);
    }
    ++it;
  }
  // Set line height. Descent is negative.
  metaData.height = metaData.ascent - metaData.descent + 1;

  // The unrotated height of the text
  int interLineSpacing = (metaData.textProperty->GetLineSpacing() - 1) * metaData.height;
  int fullHeight = numLines * metaData.height +
                   (numLines - 1) * interLineSpacing +
                   metaData.textProperty->GetLineOffset();

  // Will we be rendering a background?
  bool hasBackground = (static_cast<unsigned char>(
        metaData.textProperty->GetBackgroundOpacity() * 255) > 0);
  bool hasFrame = metaData.textProperty->GetFrame() && metaData.textProperty->GetFrameWidth() > 0;
  int padWidth = hasFrame ? 1 + metaData.textProperty->GetFrameWidth() : 2;

  int pad = (hasBackground || hasFrame) ? padWidth : 0; // pixels on each side.

  // sin, cos of orientation
  float angle = vtkMath::RadiansFromDegrees(
        metaData.textProperty->GetOrientation());
  float c = cos(angle);
  float s = sin(angle);

  // The width and height of the text + background/frame, as rotated vectors:
  metaData.dx = vtkVector2i(metaData.maxLineWidth + 2 * pad, 0);
  metaData.dy = vtkVector2i(0, fullHeight + 2 * pad);
  rotateVector2i(metaData.dx, s, c);
  rotateVector2i(metaData.dy, s, c);

  // The rotated padding on the text's vertical and horizontal axes:
  vtkVector2i hPad(pad, 0);
  vtkVector2i vPad(0, pad);
  vtkVector2i hOne(1, 0);
  vtkVector2i vOne(0, 1);
  rotateVector2i(hPad, s, c);
  rotateVector2i(vPad, s, c);
  rotateVector2i(hOne, s, c);
  rotateVector2i(vOne, s, c);

  // Calculate the bottom left corner of the data rect. Start at anchor point
  // (0, 0) and subtract out justification. Account for background/frame padding to
  // ensure that we're aligning to the text, not the background/frame.
  metaData.BL = vtkVector2i(0, 0);
  switch (metaData.textProperty->GetJustification())
  {
    case VTK_TEXT_CENTERED:
      metaData.BL = metaData.BL - (metaData.dx * 0.5);
      break;
    case VTK_TEXT_RIGHT:
      metaData.BL = metaData.BL - metaData.dx + hPad + hOne;
      break;
    case VTK_TEXT_LEFT:
      metaData.BL = metaData.BL - hPad;
      break;
    default:
      vtkErrorMacro(<< "Bad horizontal alignment flag: "
                    << metaData.textProperty->GetJustification());
      break;
  }
  switch (metaData.textProperty->GetVerticalJustification())
  {
    case VTK_TEXT_CENTERED:
      metaData.BL = metaData.BL - (metaData.dy * 0.5);
      break;
    case VTK_TEXT_BOTTOM:
      metaData.BL = metaData.BL - vPad;
      break;
    case VTK_TEXT_TOP:
      metaData.BL = metaData.BL - metaData.dy + vPad + vOne;
      break;
    default:
      vtkErrorMacro(<< "Bad vertical alignment flag: "
                    << metaData.textProperty->GetVerticalJustification());
      break;
  }

  // Compute the other corners of the data:
  metaData.TL = metaData.BL + metaData.dy - vOne;
  metaData.TR = metaData.TL + metaData.dx - hOne;
  metaData.BR = metaData.BL + metaData.dx - hOne;

  // First baseline offset from top-left corner.
  vtkVector2i penOffset(pad, -pad);
  // Account for line spacing to center the text vertically in the bbox:
  penOffset[1] -= metaData.ascent;
  penOffset[1] -= metaData.textProperty->GetLineOffset();
  rotateVector2i(penOffset, s, c);

  vtkVector2i pen = metaData.TL + penOffset;

  // Calculate bounding box of text:
  vtkTuple<int, 4> textBbox;
  textBbox[0] = textBbox[1] = pen[0];
  textBbox[2] = textBbox[3] = pen[1];

  // Calculate line offset:
  vtkVector2i lineFeed(0, -(metaData.height + interLineSpacing));
  rotateVector2i(lineFeed, s, c);

  // Compile the metrics data to determine the final bounding box. Set line
  // origins here, too.
  vtkVector2i origin;
  int justification = metaData.textProperty->GetJustification();
  for (size_t i = 0; i < metaData.lineMetrics.size(); ++i)
  {
    MetaData::LineMetrics &metrics = metaData.lineMetrics[i];

    // Apply justification
    origin = pen;
    if (justification != VTK_TEXT_LEFT)
    {
      int xShift = metaData.maxLineWidth - metrics.width;
      if (justification == VTK_TEXT_CENTERED)
      {
        xShift /= 2;
      }
      origin[0] += vtkMath::Round(c * xShift);
      origin[1] += vtkMath::Round(s * xShift);
    }

    // Set line origin
    metrics.origin = origin;

    // Merge bounding boxes
    textBbox[0] = std::min(textBbox[0], metrics.xmin + origin[0]);
    textBbox[1] = std::max(textBbox[1], metrics.xmax + origin[0]);
    textBbox[2] = std::min(textBbox[2], metrics.ymin + origin[1]);
    textBbox[3] = std::max(textBbox[3], metrics.ymax + origin[1]);

    // Update pen position
    pen = pen + lineFeed;
  }

  // Adjust for shadow
  if (metaData.textProperty->GetShadow())
  {
    int shadowOffset[2];
    metaData.textProperty->GetShadowOffset(shadowOffset);
    if (shadowOffset[0] < 0)
    {
      textBbox[0] += shadowOffset[0];
    }
    else
    {
      textBbox[1] += shadowOffset[0];
    }
    if (shadowOffset[1] < 0)
    {
      textBbox[2] += shadowOffset[1];
    }
    else
    {
      textBbox[3] += shadowOffset[1];
    }
  }

  // Compute the background/frame bounding box.
  vtkTuple<int, 4> bgBbox;
  bgBbox[0] = std::min(std::min(metaData.TL[0], metaData.TR[0]),
                       std::min(metaData.BL[0], metaData.BR[0]));
  bgBbox[1] = std::max(std::max(metaData.TL[0], metaData.TR[0]),
                       std::max(metaData.BL[0], metaData.BR[0]));
  bgBbox[2] = std::min(std::min(metaData.TL[1], metaData.TR[1]),
                       std::min(metaData.BL[1], metaData.BR[1]));
  bgBbox[3] = std::max(std::max(metaData.TL[1], metaData.TR[1]),
                       std::max(metaData.BL[1], metaData.BR[1]));

  // Calculate the final bounding box (should just be the bg, but just in
  // case...)
  metaData.bbox[0] = std::min(textBbox[0], bgBbox[0]);
  metaData.bbox[1] = std::max(textBbox[1], bgBbox[1]);
  metaData.bbox[2] = std::min(textBbox[2], bgBbox[2]);
  metaData.bbox[3] = std::max(textBbox[3], bgBbox[3]);

  return true;
}

//----------------------------------------------------------------------------
void vtkFreeTypeTools::PrepareImageData(vtkImageData *data, int textBbox[4])
{
  // Calculate the bbox's dimensions
  int textDims[2];
  textDims[0] = (textBbox[1] - textBbox[0] + 1);
  textDims[1] = (textBbox[3] - textBbox[2] + 1);

  // Calculate the size the image needs to be.
  int targetDims[3];
  targetDims[0] = textDims[0];
  targetDims[1] = textDims[1];
  targetDims[2] = 1;
  // Scale to the next highest power of 2 if required.
  if (this->ScaleToPowerTwo)
  {
    targetDims[0] = vtkMath::NearestPowerOfTwo(targetDims[0]);
    targetDims[1] = vtkMath::NearestPowerOfTwo(targetDims[1]);
  }

  // Calculate the target extent of the image.
  int targetExtent[6];
  targetExtent[0] = textBbox[0];
  targetExtent[1] = textBbox[0] + targetDims[0] - 1;
  targetExtent[2] = textBbox[2];
  targetExtent[3] = textBbox[2] + targetDims[1] - 1;
  targetExtent[4] = 0;
  targetExtent[5] = 0;

  // Get the actual image extents and increments
  int imageExtent[6];
  double imageSpacing[3];
  data->GetExtent(imageExtent);
  data->GetSpacing(imageSpacing);

  // Do we need to reallocate the image memory?
  if (data->GetScalarType() != VTK_UNSIGNED_CHAR ||
      data->GetNumberOfScalarComponents() != 4 ||
      imageExtent[0] != targetExtent[0] ||
      imageExtent[1] != targetExtent[1] ||
      imageExtent[2] != targetExtent[2] ||
      imageExtent[3] != targetExtent[3] ||
      imageExtent[4] != targetExtent[4] ||
      imageExtent[5] != targetExtent[5] ||
      fabs(imageSpacing[0] - 1.0) > 1e-10 ||
      fabs(imageSpacing[1] - 1.0) > 1e-10 ||
      fabs(imageSpacing[2] - 1.0) > 1e-10 )
  {
    data->SetSpacing(1.0, 1.0, 1.0);
    data->SetExtent(targetExtent);
    data->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  }

  // Clear the image buffer
  memset(data->GetScalarPointer(), this->DebugTextures ? 64 : 0,
         (data->GetNumberOfPoints() * data->GetNumberOfScalarComponents()));
}

// Helper functions for rasterizing the background/frame quad:
namespace RasterScanQuad {

// Return true and set t1 (if 0 <= t1 <= 1) for the intersection of lines:
//
// P1(t1) = p1 + t1 * v1 and
// P2(t2) = p2 + t2 * v2.
//
// This method is specialized for the case of P2(t2) always being a horizontal
// line (v2 = {1, 0}) with p1 defined as {0, y}.
//
// If the lines do not intersect or t1 is outside of the specified range, return
// false.
inline bool getIntersectionParameter(const vtkVector2i &p1,
                                     const vtkVector2i &v1,
                                     int y, float &t1)
{
  // First check if the input vector is parallel to the scan line, returning
  // false if it is:
  if (v1[1] == 0)
  {
    return false;
  }

  // Given the lines:
  // P1(t1) = p1 + t1 * v1 (The polygon edge)
  // P2(t2) = p2 + t2 * v2 (The horizontal scan line)
  //
  // And defining the vector:
  // w = p1 - p2
  //
  // The value of t1 at the intersection of P1 and P2 is:
  // t1 = (v2[1] * w[0] - v2[0] * w[1]) / (v2[0] * v1[1] - v2[1] * v1[0])
  //
  // We know that p2 = {0, y} and v2 = {1, 0}, since we're scanning along the
  // x axis, so the above becomes:
  // t1 = (-w[1]) / (v1[1])
  //
  // Expanding the definition of w, w[1] --> (p1[1] - p2[1]) --> p1[1] - y,
  // resulting in the final:
  // t1 = -(p1[1] - y) / v1[1], or
  // t1 = (y - p1[1]) / v1[1]

  t1 = (y - p1[1]) / static_cast<float>(v1[1]);
  return t1 >= 0.f && t1 <= 1.f;
}

// Evaluate the line equation P(t) = p + t * v at the supplied t, and return
// the x value of the resulting point.
inline int evaluateLineXOnly(const vtkVector2i &p, const vtkVector2i &v,
                             float t)
{
  return p.GetX() + vtkMath::Round(v.GetX() * t);
}

// Given the corners of a rectangle (TL, TR, BL, BR), the vectors that
// separate them (dx = TR - TL = BR - BL, dy = TR - BR = TL - BL), and the
// y value to scan, return the minimum and maximum x values that the rectangle
// contains.
bool findScanRange(const vtkVector2i &TL, const vtkVector2i &TR,
                   const vtkVector2i &BL, const vtkVector2i &BR,
                   const vtkVector2i &dx, const vtkVector2i &dy,
                   int y, int &min, int &max)
{
  // Initialize the min and max to a known invalid range using the bounds of the
  // rectangle:
  min = std::max(std::max(TL[0], TR[0]), std::max(BL[0], BR[0]));
  max = std::min(std::min(TL[0], TR[0]), std::min(BL[0], BR[0]));

  float lineParam;
  int numIntersections = 0;

  // Top
  if (getIntersectionParameter(TL, dx, y, lineParam))
  {
    int x = evaluateLineXOnly(TL, dx, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }
  // Bottom
  if (getIntersectionParameter(BL, dx, y, lineParam))
  {
    int x = evaluateLineXOnly(BL, dx, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }
  // Left
  if (getIntersectionParameter(BL, dy, y, lineParam))
  {
    int x = evaluateLineXOnly(BL, dy, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }
  // Right
  if (getIntersectionParameter(BR, dy, y, lineParam))
  {
    int x = evaluateLineXOnly(BR, dy, lineParam);
    min = std::min(min, x);
    max = std::max(max, x);
    ++numIntersections;
  }

  return numIntersections != 0;
}

// Clamp value to stay between the minimum and maximum extent for the
// specified dimension.
inline void clampToExtent(int extent[6], int dim, int &value)
{
  value = std::min(extent[2*dim+1], std::max(extent[2*dim], value));
}

} // end namespace RasterScanQuad

//----------------------------------------------------------------------------
void vtkFreeTypeTools::RenderBackground(vtkTextProperty *tprop,
                                        vtkImageData *image,
                                        ImageMetaData &metaData)
{
  unsigned char* color;
  unsigned char backgroundColor[4] = {
    static_cast<unsigned char>(tprop->GetBackgroundColor()[0] * 255),
    static_cast<unsigned char>(tprop->GetBackgroundColor()[1] * 255),
    static_cast<unsigned char>(tprop->GetBackgroundColor()[2] * 255),
    static_cast<unsigned char>(tprop->GetBackgroundOpacity()  * 255)
  };
  unsigned char frameColor[4] = {
    static_cast<unsigned char>(tprop->GetFrameColor()[0] * 255),
    static_cast<unsigned char>(tprop->GetFrameColor()[1] * 255),
    static_cast<unsigned char>(tprop->GetFrameColor()[2] * 255),
    static_cast<unsigned char>(tprop->GetFrame() ? 255 : 0)
  };

  if (backgroundColor[3] == 0 && frameColor[3] == 0)
  {
    return;
  }

  const vtkVector2i &dx = metaData.dx;
  const vtkVector2i &dy = metaData.dy;
  const vtkVector2i &TL = metaData.TL;
  const vtkVector2i &TR = metaData.TR;
  const vtkVector2i &BL = metaData.BL;
  const vtkVector2i &BR = metaData.BR;

  // Find the minimum and maximum y values:
  int yMin = std::min(std::min(TL[1], TR[1]), std::min(BL[1], BR[1]));
  int yMax = std::max(std::max(TL[1], TR[1]), std::max(BL[1], BR[1]));

  // Clamp these to prevent out of bounds errors:
  int extent[6];
  image->GetExtent(extent);
  RasterScanQuad::clampToExtent(extent, 1, yMin);
  RasterScanQuad::clampToExtent(extent, 1, yMax);

  // Scan from yMin to yMax, finding the x values on that horizontal line that
  // are contained by the data rectangle, then paint them with the background
  // color.
  int frameWidth = tprop->GetFrameWidth();
  for (int y = yMin; y <= yMax; ++y)
  {
    int xMin, xMax;
    if (RasterScanQuad::findScanRange(TL, TR, BL, BR, dx, dy, y, xMin, xMax))
    {
      // Clamp to prevent out of bounds errors:
      RasterScanQuad::clampToExtent(extent, 0, xMin);
      RasterScanQuad::clampToExtent(extent, 0, xMax);

      // Get a pointer into the image data:
      unsigned char *dataPtr = static_cast<unsigned char*>(
            image->GetScalarPointer(xMin, y, 0));
      for (int x = xMin; x <= xMax; ++x)
      {
        color =
          (frameColor[3] != 0 && (y < (yMin + frameWidth) || y > (yMax - frameWidth)
            || x < (xMin + frameWidth) || x > (xMax - frameWidth))) ?
          frameColor : backgroundColor;
        *(dataPtr++) = color[0];
        *(dataPtr++) = color[1];
        *(dataPtr++) = color[2];
        *(dataPtr++) = color[3];
      }
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
  int x = metaData.lineMetrics[lineIndex].origin.GetX();
  int y = metaData.lineMetrics[lineIndex].origin.GetY();

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
  FT_BitmapGlyph bitmapGlyph = NULL;
  FT_UInt glyphIndex;
  FT_Bitmap *bitmap = this->GetBitmap(character, &iMetaData->scaler,
                                      glyphIndex, bitmapGlyph);

  // Add the kerning
  if (iMetaData->faceHasKerning && previousGlyphIndex && glyphIndex)
  {
    FT_Vector kerningDelta;
    if (FT_Get_Kerning(iMetaData->face, previousGlyphIndex, glyphIndex,
                       FT_KERNING_DEFAULT, &kerningDelta) == 0)
    {
      if (metaData.faceIsRotated) // PR#15301
      {
        FT_Vector_Transform(&kerningDelta, &metaData.rotation);
      }
      x += kerningDelta.x >> 6;
      y += kerningDelta.y >> 6;
    }
  }
  previousGlyphIndex = glyphIndex;

  if (!bitmap)
  {
    // TODO This should draw an empty rectangle.
    return false;
  }

  if (bitmap->width && bitmap->rows)
  {
    // Starting position given the bearings.
    // Subtract 1 to the bearing Y, because this is the vertical distance
    // from the glyph origin (0,0) to the topmost pixel of the glyph bitmap
    // (more precisely, to the pixel just above the bitmap). This distance is
    // expressed in integer pixels, and is positive for upwards y.
    vtkVector2i pen(x + bitmapGlyph->left, y + bitmapGlyph->top - 1);

    // Render the current glyph into the image
    unsigned char *ptr = static_cast<unsigned char *>(
          image->GetScalarPointer(pen[0], pen[1], 0));
    if (ptr)
    {
      int dataPitch = (-iMetaData->imageDimensions[0] - bitmap->width) *
          iMetaData->imageIncrements[0];
      unsigned char *glyphPtrRow = bitmap->buffer;
      unsigned char *glyphPtr;
      const unsigned char *fgRGB = iMetaData->rgba;
      const float fgA = iMetaData->rgba[3] / 255.f;

      for (int j = 0; j < static_cast<int>(bitmap->rows); ++j)
      {
        glyphPtr = glyphPtrRow;

        for (int i = 0; i < static_cast<int>(bitmap->width); ++i)
        {
          if (*glyphPtr == 0)
          {
            ptr += 4;
          }
          else if (ptr[3] > 0)
          {
            // This is a pixel we've drawn before since it has non-zero alpha.
            // We must therefore blend the colors.
            const float val = *glyphPtr / 255.f;
            const float bgA = ptr[3] / 255.0;

            const float fg_blend = fgA * val;
            const float bg_blend = 1.f - fg_blend;

            float r(bg_blend * ptr[0] + fg_blend * fgRGB[0]);
            float g(bg_blend * ptr[1] + fg_blend * fgRGB[1]);
            float b(bg_blend * ptr[2] + fg_blend * fgRGB[2]);
            float a(255 * (fg_blend + bgA * bg_blend));

            // Figure out the color.
            ptr[0] = static_cast<unsigned char>(r);
            ptr[1] = static_cast<unsigned char>(g);
            ptr[2] = static_cast<unsigned char>(b);
            ptr[3] = static_cast<unsigned char>(a);

            ptr += 4;
          }
          else
          {
            *ptr = fgRGB[0];
            ++ptr;
            *ptr = fgRGB[1];
            ++ptr;
            *ptr = fgRGB[2];
            ++ptr;
            *ptr = static_cast<unsigned char>((*glyphPtr) * fgA);
            ++ptr;
          }
          ++glyphPtr;
        }
        glyphPtrRow += bitmap->pitch;
        ptr += dataPitch;
      }
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
  FT_OutlineGlyph outlineGlyph = NULL;
  FT_Outline *outline = this->GetOutline(character, &metaData.scaler,
                                         glyphIndex, outlineGlyph);

  // Add the kerning
  if (metaData.faceHasKerning && previousGlyphIndex && glyphIndex)
  {
    FT_Vector kerningDelta;
    FT_Get_Kerning(metaData.face, previousGlyphIndex, glyphIndex,
                   FT_KERNING_DEFAULT, &kerningDelta);
    if (metaData.faceIsRotated) // PR#15301
    {
      FT_Vector_Transform(&kerningDelta, &metaData.rotation);
    }
    x += kerningDelta.x >> 6;
    y += kerningDelta.y >> 6;
  }
  previousGlyphIndex = glyphIndex;

  if (!outline)
  {
    // TODO render an empty box.
    return false;
  }

  if (outline->n_points > 0)
  {
    int pen_x = x;
    int pen_y = y;

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

        // Mask the tag and convert to our known-good control types:
        // (0x3 mask is because these values often have trailing garbage --
        // see note above controlType enum).
        switch (fttag & 0x3)
        {
          case (FT_CURVE_TAG_ON & 0x3): // 0b01
            tag = ON_POINT;
            break;
          case (FT_CURVE_TAG_CUBIC & 0x3): // 0b11
            tag = CUBIC_POINT;
            break;
          case (FT_CURVE_TAG_CONIC & 0x3): // 0b00
            tag = CONIC_POINT;
            break;
          default:
            vtkWarningMacro("Invalid control code returned from FreeType: "
                            << static_cast<int>(fttag) << " (masked: "
                            << static_cast<int>(fttag & 0x3));
            return false;
        }

        double vec[2];
        vec[0] = ftvec.x / 64.0 + pen_x;
        vec[1] = ftvec.y / 64.0 + pen_y;

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
  double fontSize = metaData.textProperty->GetFontSize();
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
    metaData.textProperty->SetFontSize(static_cast<int>(fontSize));
    metaData.scaler.height = fontSize * 64; // 26.6 format points
    metaData.scaler.width = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.height = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.width = fontSize * 64; // 26.6 format points
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
    fontSize += 1.;
    metaData.textProperty->SetFontSize(fontSize);
    metaData.scaler.height = fontSize * 64; // 26.6 format points
    metaData.scaler.width = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.height = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.width = fontSize * 64; // 26.6 format points
    if (!this->CalculateBoundingBox(str, metaData))
    {
      return -1;
    }
    size[0] = metaData.bbox[1] - metaData.bbox[0];
    size[1] = metaData.bbox[3] - metaData.bbox[2];
  }

  while ((size[0] > targetWidth || size[1] > targetHeight) && fontSize > 0)
  {
    fontSize -= 1.;
    metaData.textProperty->SetFontSize(fontSize);
    metaData.scaler.height = fontSize * 64; // 26.6 format points
    metaData.scaler.width = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.height = fontSize * 64; // 26.6 format points
    metaData.unrotatedScaler.width = fontSize * 64; // 26.6 format points
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
                                      size_t &prop_cache_id,
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
                                              size_t prop_cache_id,
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
FT_Bitmap *vtkFreeTypeTools::GetBitmap(FT_UInt32 c, FTC_Scaler scaler,
                                       FT_UInt &gindex,
                                       FT_BitmapGlyph &bitmap_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(reinterpret_cast<size_t>(scaler->face_id), c,
                           &gindex))
  {
    return 0;
  }

  // Get the glyph as a bitmap
  FT_Glyph glyph;
  if (!this->GetGlyph(scaler, gindex, &glyph,
                      vtkFreeTypeTools::GLYPH_REQUEST_BITMAP)
      || glyph->format != ft_glyph_format_bitmap)
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
                                                size_t prop_cache_id,
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
FT_Outline *vtkFreeTypeTools::GetOutline(FT_UInt32 c, FTC_Scaler scaler,
                                         FT_UInt &gindex,
                                         FT_OutlineGlyph &outline_glyph)
{
  // Get the glyph index
  if (!this->GetGlyphIndex(reinterpret_cast<size_t>(scaler->face_id), c,
                           &gindex))
  {
    return 0;
  }

  // Get the glyph as a outline
  FT_Glyph glyph;
  if (!this->GetGlyph(scaler, gindex, &glyph,
                      vtkFreeTypeTools::GLYPH_REQUEST_OUTLINE)
      || glyph->format != ft_glyph_format_outline)
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
  FT_BitmapGlyph bitmapGlyph = NULL;
  FT_UInt gindex = 0;
  FT_UInt gindexLast = 0;
  FT_Vector delta;
  width = 0;
  int pen[2] = {0, 0};
  bbox[0] = bbox[1] = pen[0];
  bbox[2] = bbox[3] = pen[1];

  for (; begin != end; ++begin)
  {
    // Get the bitmap and glyph index:
    FT_Bitmap *bitmap = this->GetBitmap(*begin, &metaData.scaler, gindex,
                                        bitmapGlyph);

    // Adjust the pen location for kerning
    if (metaData.faceHasKerning && gindexLast && gindex)
    {
      if (FT_Get_Kerning(metaData.face, gindexLast, gindex, FT_KERNING_DEFAULT,
                         &delta) == 0)
      {
        // Kerning is not rotated with the face, no need to rotate/adjust for
        // width:
        width += delta.x >> 6;
        // But we do need to rotate for pen location (see PR#15301)
        if (metaData.faceIsRotated)
        {
          FT_Vector_Transform(&delta, &metaData.rotation);
        }
        pen[0] += delta.x >> 6;
        pen[1] += delta.y >> 6;
      }
    }
    gindexLast = gindex;

    // Use the dimensions of the bitmap glyph to get a tight bounding box.
    if (bitmap)
    {
      bbox[0] = std::min(bbox[0], pen[0] + bitmapGlyph->left);
      bbox[1] = std::max(bbox[1], pen[0] + bitmapGlyph->left + static_cast<int>(bitmap->width));
      bbox[2] = std::min(bbox[2], pen[1] + bitmapGlyph->top - 1 - static_cast<int>(bitmap->rows));
      bbox[3] = std::max(bbox[3], pen[1] + bitmapGlyph->top - 1);
    }
    else
    {
      // FIXME: do something more elegant here.
      // We should render an empty rectangle to adhere to the specs...
      vtkDebugMacro(<<"Unrecognized character: " << *begin);
      continue;
    }

    // Update advance.
    delta = bitmapGlyph->root.advance;
    pen[0] += (delta.x + 0x8000) >> 16;
    pen[1] += (delta.y + 0x8000) >> 16;

    if (metaData.faceIsRotated)
    {
      FT_Vector_Transform(&delta, &metaData.inverseRotation);
    }
    width += (delta.x + 0x8000) >> 16;
  }
}
