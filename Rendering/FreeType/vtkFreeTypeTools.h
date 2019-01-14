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
/**
 * @class   vtkFreeTypeTools
 * @brief   FreeType library support
 *
 * vtkFreeTypeTools provides a low-level interface to the FreeType2 library,
 * including font-cache and rasterization.
 *
 * @warning
 * Internal use only.
*/

#ifndef vtkFreeTypeTools_h
#define vtkFreeTypeTools_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For smart pointer
#include "vtkTextRenderer.h" // For Metrics struct
#include "vtkUnicodeString.h" // For vtkUnicodeStringValueType

#include <array> // for std::array

class vtkImageData;
class vtkPath;
class vtkTextProperty;
class vtkStdString;

// FreeType
#include "vtk_freetype.h"  //since ft2build.h could be in the path
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H

class FTFont;

// PIMPL class for FTC_FaceID->vtkTextProperty lookup
class vtkTextPropertyLookup;

//----------------------------------------------------------------------------
// Singleton cleanup
class VTKRENDERINGFREETYPE_EXPORT vtkFreeTypeToolsCleanup
{
public:
  vtkFreeTypeToolsCleanup();
  ~vtkFreeTypeToolsCleanup();
private:
  vtkFreeTypeToolsCleanup(const vtkFreeTypeToolsCleanup&) = delete;
  vtkFreeTypeToolsCleanup& operator=(const vtkFreeTypeToolsCleanup&) = delete;
};

//----------------------------------------------------------------------------
// Singleton font cache
class VTKRENDERINGFREETYPE_EXPORT vtkFreeTypeTools : public vtkObject
{
public:
  struct FaceMetrics
  {
    int UnitsPerEM;
    int Ascender;
    int Descender;
    int HorizAdvance;
    std::array<int, 4> BoundingBox; // xmin, xmax, ymin, ymax
    std::string FamilyName;
    bool Scalable;
    bool Bold;
    bool Italic;
  };
  struct GlyphOutline
  {
    int HorizAdvance;
    vtkSmartPointer<vtkPath> Path;
  };

  vtkTypeMacro(vtkFreeTypeTools, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the singleton instance with no reference counting.
   */
  static vtkFreeTypeTools* GetInstance();

  /**
   * Supply a user defined instance. Call Delete() on the supplied
   * instance after setting it to fix the reference count.
   */
  static void SetInstance(vtkFreeTypeTools *instance);

  //@{
  /**
   * If true, images produced by RenderString will have a transparent grey
   * background and set the justification anchor texel to bright yellow.
   */
  vtkSetMacro(DebugTextures, bool)
  vtkGetMacro(DebugTextures, bool)
  vtkBooleanMacro(DebugTextures, bool)
  //@}

  /**
   * Get the FreeType library singleton.
   */
  FT_Library* GetLibrary();

  /**
   * Return some metrics about a font face. The information is generic and not
   * tied to a single font size, but describes a scalable font defined on the EM
   * square.
   */
  FaceMetrics GetFaceMetrics(vtkTextProperty *tprop);

  /**
   * Return a generic outline of a glyph with some additional metadata. The
   * information is generic and not tied to a single font size, but describes a
   * scalable font defined on the EM square.
   */
  GlyphOutline GetUnscaledGlyphOutline(vtkTextProperty *tprop,
                                       vtkUnicodeStringValueType charId);

  /**
   * Return a 2D vector detailing the unscaled kerning offset for a pair of
   * characters. If tprop's font size is 0, the information will be generic and
   * not tied to a single font size, but describe a scalable font defined on
   * the EM square.
   */
  std::array<int, 2> GetUnscaledKerning(vtkTextProperty *tprop,
                                        vtkUnicodeStringValueType leftChar,
                                        vtkUnicodeStringValueType rightChar);

  //@{
  /**
   * Set/Get the maximum number of faces (FT_Face), sizes (FT_Size) and
   * bytes used by the cache. These settings can be changed as long as
   * it is done prior to accessing any of the caches or the cache manager.
   */
  vtkSetClampMacro(MaximumNumberOfFaces,unsigned int,1,VTK_UNSIGNED_INT_MAX);
  vtkGetMacro(MaximumNumberOfFaces, unsigned int);
  vtkSetClampMacro(MaximumNumberOfSizes,unsigned int,1,VTK_UNSIGNED_INT_MAX);
  vtkGetMacro(MaximumNumberOfSizes, unsigned int);
  vtkSetClampMacro(MaximumNumberOfBytes,unsigned long,1,VTK_UNSIGNED_LONG_MAX);
  vtkGetMacro(MaximumNumberOfBytes, unsigned long);
  //@}

  //@{
  /**
   * Given a text property and a string, get the bounding box {xmin, xmax,
   * ymin, ymax} of the rendered string in pixels. The origin of the bounding
   * box is the anchor point described by the horizontal and vertical
   * justification text property variables.
   * Returns true on success, false otherwise.
   * @sa GetMetrics
   */
  bool GetBoundingBox(vtkTextProperty *tprop, const vtkStdString& str,
                      int dpi, int bbox[4]);
  bool GetBoundingBox(vtkTextProperty *tprop, const vtkUnicodeString& str,
                      int dpi, int bbox[4]);
  //@}

  //@{
  /**
   * Given a text property and a string, get the metrics of the rendered string.
   * Returns true on success, false otherwise.
   */
  bool GetMetrics(vtkTextProperty *tprop, const vtkStdString& str, int dpi,
                  vtkTextRenderer::Metrics &metrics);
  bool GetMetrics(vtkTextProperty *tprop, const vtkUnicodeString& str, int dpi,
                  vtkTextRenderer::Metrics &metrics);
  //@}

  //@{
  /**
   * Given a text property and a string, this function initializes the
   * vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   * The origin of the image's extents is aligned with the anchor point
   * described by the text property's vertical and horizontal justification
   * options.
   */
  bool RenderString(vtkTextProperty *tprop, const vtkStdString& str, int dpi,
                    vtkImageData *data, int textDims[2] = nullptr);
  bool RenderString(vtkTextProperty *tprop, const vtkUnicodeString& str,
                    int dpi, vtkImageData *data, int textDims[2] = nullptr);
  //@}

  //@{
  /**
   * Given a text property and a string, this function populates the vtkPath
   * path with the outline of the rendered string. The origin of the path
   * coordinates is aligned with the anchor point described by the text
   * property's horizontal and vertical justification options.
   */
  bool StringToPath(vtkTextProperty *tprop, const vtkStdString& str, int dpi,
                    vtkPath *path);
  bool StringToPath(vtkTextProperty *tprop, const vtkUnicodeString& str,
                    int dpi, vtkPath *path);
  //@}

  //@{
  /**
   * This function returns the font size (in points) required to fit the string
   * in the target rectangle. The font size of tprop is updated to the computed
   * value as well. If an error occurs, -1 is returned.
   */
  int GetConstrainedFontSize(const vtkStdString &str, vtkTextProperty *tprop,
                             int dpi, int targetWidth, int targetHeight);
  int GetConstrainedFontSize(const vtkUnicodeString &str,
                             vtkTextProperty *tprop, int dpi,
                             int targetWidth, int targetHeight);
  //@}

  /**
   * Turn a string into a hash. This is not a general purpose hash
   * function, and is only used to generate identifiers for cached fonts.
   */
  static vtkTypeUInt16 HashString(const char *str);

  /**
   * Hash a string of a given length. This function hashes n chars and does
   * not depend on a terminating null character.
   */
  static vtkTypeUInt32 HashBuffer(const void* str, size_t n, vtkTypeUInt32 hash = 0);

  //@{
  /**
   * Given a text property 'tprop', get its unique ID in our cache framework.
   * In the same way, given a unique ID in our cache, retrieve the
   * corresponding text property and assign its parameters to 'tprop'.
   * Warning: there is no one to one mapping between a single text property
   * the corresponding ID, and vice-versa. The ID is just a fast hash, a
   * binary mask concatenating the attributes of the text property that are
   * relevant to our cache (Color, Opacity, Justification setting are not
   * stored).
   */
  void MapTextPropertyToId(vtkTextProperty *tprop, size_t *tprop_cache_id);
  void MapIdToTextProperty(size_t tprop_cache_id, vtkTextProperty *tprop);
  //@}

  //@{
  /**
   * Set whether the image produced should be scaled up to the nearest power of
   * 2. This is normally required for older graphics cards where all textures
   * must be a power of 2. This defaults to false, and should be fine on modern
   * hardware.
   */
  vtkSetMacro(ScaleToPowerTwo, bool);
  vtkGetMacro(ScaleToPowerTwo, bool);
  vtkBooleanMacro(ScaleToPowerTwo, bool);
  //@}

  //@{
  /**
   * Force use of the fonts compiled into VTK, ignoring any FontConfig or
   * embedded fonts. Useful for generating test images consistently across
   * platforms. This flag is on by default.
   */
  vtkSetMacro(ForceCompiledFonts, bool);
  vtkGetMacro(ForceCompiledFonts, bool);
  vtkBooleanMacro(ForceCompiledFonts, bool);
  //@}

  /**
   * Lookup and set the FreeType font face @a face best matching the text
   * property @a tprop using the compiled Arial, Times, and Courier fonts. If
   * an unrecognized font family is requested, Arial will be substituted.
   * Returns true if the face is set, false otherwise.
   */
  static bool LookupFace(vtkTextProperty *tprop, FT_Library lib, FT_Face *face);

protected:
  /**
   * Create the FreeType Cache manager instance and set this->CacheManager
   */
  virtual FT_Error CreateFTCManager();

  //@{
  /**
   * Used to store state about a particular rendering and cache constant values
   */
  class MetaData;
  class ImageMetaData;
  bool PrepareMetaData(vtkTextProperty *tprop, int dpi, MetaData &metaData);
  bool PrepareImageMetaData(vtkTextProperty *tprop, vtkImageData *image,
                            ImageMetaData &metaData);
  //@}

  /**
   * This function initializes the extent of the ImageData to eventually
   * receive the text stored in str
   */
  void PrepareImageData(vtkImageData *data, int bbox[4]);

  /**
   * Draw the background quad on the image.
   */
  void RenderBackground(vtkTextProperty *tprop, vtkImageData *image,
                        ImageMetaData &metaData);

  /**
   * Given a text property, get the corresponding FreeType size object
   * (a structure storing both a face and a specific size metric).
   * The size setting of the text property is used to set the size's face
   * to the corresponding size.
   * Return true on success, false otherwise.
   */
  bool GetSize(vtkTextProperty *tprop, FT_Size *size);

  /**
   * Given a text property, get the corresponding FreeType face.
   * The size parameter of the text property is ignored and a face with
   * unknown current size is returned. Use GetSize() to get a specific size.
   * Return true on success, false otherwise
   */
  bool GetFace(vtkTextProperty *tprop, FT_Face *face);

  /**
   * Given a text property and a character, get the corresponding FreeType
   * glyph index.
   * Return true on success, false otherwise
   */
  bool GetGlyphIndex(vtkTextProperty *tprop, FT_UInt32 c, FT_UInt *gindex);

  //@{
  /**
   * Given a text property and a character, get the corresponding FreeType
   * glyph. The 'request' parameter can be used to request the glyph to be
   * in a specific format. If GLYPH_REQUEST_DEFAULT, the glyph might be either
   * an outline (most of the time) or a bitmap if the face includes a set of
   * pre-rendered glyphs (called "strikes") for a given size.
   * If GLYPH_REQUEST_BITMAP, the glyph is rendered immediately and can
   * be safely cast to a FT_BitmapGlyph. If GLYPH_REQUEST_OUTLINE, no
   * pre-rendered "strike" is considered, the glyph is an outline and can be
   * safely cast to a FT_OutlineGlyph.
   * Return true on success, false otherwise
   */
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
  bool GetSize(size_t tprop_cache_id, int font_size, FT_Size *size);
  bool GetSize(FTC_Scaler scaler, FT_Size *size);
  bool GetFace(size_t tprop_cache_id, FT_Face *face);
  bool GetGlyphIndex(size_t tprop_cache_id, FT_UInt32 c,
                     FT_UInt *gindex);
  bool GetGlyph(size_t tprop_cache_id,
                int font_size,
                FT_UInt gindex,
                FT_Glyph *glyph,
                int request = GLYPH_REQUEST_DEFAULT);
  bool GetGlyph(FTC_Scaler scaler,
                FT_UInt gindex,
                FT_Glyph *glyph,
                int request = GLYPH_REQUEST_DEFAULT);
  //@}

  /**
   * Should the image be scaled to the next highest power of 2?
   */
  bool ScaleToPowerTwo;

  vtkFreeTypeTools();
  ~vtkFreeTypeTools() override;

  /**
   * Attempt to get the typeface of the specified font.
   */
  bool GetFace(vtkTextProperty *prop, size_t &prop_cache_id,
               FT_Face &face, bool &face_has_kerning);

  //@{
  /**
   * Now attempt to get the bitmap for the specified character.
   */
  FT_Bitmap* GetBitmap(FT_UInt32 c, size_t prop_cache_id,
                       int prop_font_size, FT_UInt &gindex,
                       FT_BitmapGlyph &bitmap_glyph);
  FT_Bitmap* GetBitmap(FT_UInt32 c, FTC_Scaler scaler, FT_UInt &gindex,
                       FT_BitmapGlyph &bitmap_glyph);
  //@}

  //@{
  /**
   * Attempt to get the outline for the specified character.
   */
  FT_Outline* GetOutline(FT_UInt32 c, size_t prop_cache_id,
                         int prop_font_size, FT_UInt &gindex,
                         FT_OutlineGlyph &outline_glyph);
  FT_Outline* GetOutline(FT_UInt32 c, FTC_Scaler scaler, FT_UInt &gindex,
                         FT_OutlineGlyph &outline_glyph);
  //@}

  /**
   * The singleton instance
   */
  static vtkFreeTypeTools* Instance;

  /**
   * Lookup table that maps free type font cache face ids to vtkTextProperties
   */
  vtkTextPropertyLookup *TextPropertyLookup;

  /**
   * FreeType library instance.
   */
  FT_Library *Library;

  //@{
  /**
   * The cache manager, image cache and charmap cache
   */
  FTC_Manager *CacheManager;
  FTC_ImageCache *ImageCache;
  FTC_CMapCache  *CMapCache;
  //@}

  //@{
  /**
   * Get the FreeType cache manager, image cache and charmap cache
   */
  FTC_Manager* GetCacheManager();
  FTC_ImageCache* GetImageCache();
  FTC_CMapCache* GetCMapCache();
  //@}

  unsigned int MaximumNumberOfFaces;
  unsigned int MaximumNumberOfSizes;
  unsigned long MaximumNumberOfBytes;

  bool ForceCompiledFonts;
  bool DebugTextures;

  void InitializeCacheManager();
  void ReleaseCacheManager();

private:
  vtkFreeTypeTools(const vtkFreeTypeTools&) = delete;
  void operator=(const vtkFreeTypeTools&) = delete;

  /**
   * Internal helper called by RenderString methods
   */
  template <typename StringType>
  bool RenderStringInternal(vtkTextProperty *tprop, const StringType &str,
                            int dpi, vtkImageData *data, int textDims[2]);

  /**
   * Internal helper method called by StringToPath methods
   */
  template <typename StringType>
  bool StringToPathInternal(vtkTextProperty *tprop, const StringType &str,
                            int dpi, vtkPath *path);

  //@{
  /**
   * This function initializes calculates the size of the required bounding box
   * and stores it in the MetaData provided.
   */
  bool CalculateBoundingBox(const vtkStdString& str, MetaData &metaData);
  bool CalculateBoundingBox(const vtkUnicodeString& str, MetaData &metaData);
  template <typename T>
  bool CalculateBoundingBox(const T& str, MetaData &metaData, const T& defaultHeightString);
  //@}

  /**
   * Internal helper method called by RenderString.
   * metaData is passed through the character renderer and caches properties
   * about data (e.g. range, dimensions, increments, etc).
   */
  template <typename StringType, typename DataType>
  bool PopulateData(const StringType& str, DataType data, MetaData &metaData);

  /**
   * Renders a single line of text (between begin and end) to the image data.
   */
  template <typename IteratorType, typename DataType>
  bool RenderLine(IteratorType begin, IteratorType end, int lineIndex,
                  DataType data, MetaData &metaData);

  //@{
  /**
   * Implementations for rendering a single character to a specific target.
   */
  template <typename CharType>
  bool RenderCharacter(CharType character, int &x, int &y,
                       FT_UInt &previousGlyphIndex, vtkImageData *image,
                       MetaData &metaData);
  template <typename CharType>
  bool RenderCharacter(CharType character, int &x, int &y,
                       FT_UInt &previousGlyphIndex, vtkPath *path,
                       MetaData &metaData);
  //@}

  void OutlineToPath(int x, int y, FT_Outline *outline, vtkPath *path);

  /**
   * Internal helper method called by GetConstrainedFontSize. Returns the
   * fontsize (in points) that will fit the return string @a str into the @a
   * targetWidth and @a targetHeight.
   */
  template <typename T>
  int FitStringToBBox(const T &str, MetaData &metaData, int targetWidth,
                      int targetHeight);

  //@{
  /**
   * Get the width of the rendered string between iterators
   * begin and end. Width is calculated as the sum of advances and kernings
   * along the baseline (i.e. rotations are ignored), while bbox is the
   * is a tight fitting bbox around the rendering string, assuming (0, 0)
   * is the pen origin.
   */
  template<typename T>
  void GetLineMetrics(T begin, T end, MetaData &metaData, int &width,
                      int bbox[4]);
};
  //@}

// This is here to implement the Schwarz counter idiom.
static vtkFreeTypeToolsCleanup vtkFreeTypeToolsCleanupInstance;

#endif
