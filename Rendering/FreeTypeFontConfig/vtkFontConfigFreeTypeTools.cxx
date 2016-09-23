/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFontConfigFreeTypeTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFontConfigFreeTypeTools.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

#ifndef _MSC_VER
# include <stdint.h>
#endif

#include <fontconfig/fontconfig.h>

vtkStandardNewMacro(vtkFontConfigFreeTypeTools)

namespace
{
// The FreeType face requester callback:
FT_CALLBACK_DEF(FT_Error)
vtkFontConfigFreeTypeToolsFaceRequester(FTC_FaceID face_id,
                                        FT_Library lib,
                                        FT_Pointer request_data,
                                        FT_Face* face)
{
  // Get a pointer to the current vtkFontConfigFreeTypeTools object
  vtkFontConfigFreeTypeTools *self =
    reinterpret_cast<vtkFontConfigFreeTypeTools*>(request_data);

  // Map the ID to a text property
  vtkSmartPointer<vtkTextProperty> tprop =
      vtkSmartPointer<vtkTextProperty>::New();
  self->MapIdToTextProperty(reinterpret_cast<intptr_t>(face_id), tprop);

  bool faceIsSet =
      self->GetForceCompiledFonts() || tprop->GetFontFamily() == VTK_FONT_FILE
      ? false : self->LookupFaceFontConfig(tprop, lib, face);

  // Fall back to compiled fonts if lookup fails/compiled fonts are forced:
  if (!faceIsSet)
  {
    faceIsSet = self->Superclass::LookupFace(tprop, lib, face);
  }

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
} // end anon namespace

void vtkFontConfigFreeTypeTools::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkFontConfigFreeTypeTools::vtkFontConfigFreeTypeTools()
{
}

vtkFontConfigFreeTypeTools::~vtkFontConfigFreeTypeTools()
{
}

FT_Error vtkFontConfigFreeTypeTools::CreateFTCManager()
{
  return FTC_Manager_New(*this->GetLibrary(),
                         this->MaximumNumberOfFaces,
                         this->MaximumNumberOfSizes,
                         this->MaximumNumberOfBytes,
                         vtkFontConfigFreeTypeToolsFaceRequester,
                         static_cast<FT_Pointer>(this),
                         this->CacheManager);
}

bool vtkFontConfigFreeTypeTools::LookupFaceFontConfig(vtkTextProperty *tprop,
                                                      FT_Library lib,
                                                      FT_Face *face)
{
  if (!FcInit())
  {
    return false;
  }

  // Query tprop
  const FcChar8 *family = reinterpret_cast<const FcChar8*>(
        tprop->GetFontFamilyAsString());
  const double pointSize = static_cast<double>(tprop->GetFontSize());
  const int weight = tprop->GetBold() ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM;
  const int slant = tprop->GetItalic() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN;

  // Build pattern
  FcPattern *pattern = FcPatternCreate();
  FcPatternAddString(pattern, FC_FAMILY, family);
  FcPatternAddDouble(pattern, FC_SIZE, pointSize);
  FcPatternAddInteger(pattern, FC_WEIGHT, weight);
  FcPatternAddInteger(pattern, FC_SLANT, slant);
  FcPatternAddBool(pattern, FC_SCALABLE, true);

  // Prefer fonts that have at least greek characters:
  FcCharSet *charSet = FcCharSetCreate();
  FcCharSetAddChar(charSet, static_cast<FcChar32>(948)); // lowercase delta
  FcPatternAddCharSet(pattern, FC_CHARSET, charSet);

  // Replace common font names, e.g. arial, times, etc -> sans, serif, etc
  FcConfigSubstitute(NULL, pattern, FcMatchPattern);

  // Fill in any missing defaults:
  FcDefaultSubstitute(pattern);

  // Match pattern
  FcResult result;
  FcFontSet *fontMatches = FcFontSort(NULL, pattern, false, NULL, &result);
  FcPatternDestroy(pattern);
  pattern = NULL;
  if (!fontMatches || fontMatches->nfont == 0)
  {
    if (fontMatches)
    {
      FcFontSetDestroy(fontMatches);
    }
    return false;
  }

  // Grab the first match that is scalable -- even though we've requested
  // scalable fonts in the match, FC seems to not weigh that option very heavily
  FcPattern *match = NULL;
  for (int i = 0; i < fontMatches->nfont; ++i)
  {
    match = fontMatches->fonts[i];

    // Ensure that the match is scalable
    FcBool isScalable;
    if (FcPatternGetBool(match, FC_SCALABLE, 0, &isScalable) != FcResultMatch ||
        !isScalable)
    {
      continue;
    }

    FcCharSet *currentFontCharSet;
    if (FcPatternGetCharSet(match, FC_CHARSET, 0, &currentFontCharSet)
        != FcResultMatch ||
        FcCharSetIntersectCount(charSet, currentFontCharSet) == 0)
    {
      continue;
    }

    break;
  }

  if (!match)
  {
    FcFontSetDestroy(fontMatches);
    FcCharSetDestroy(charSet);
    return false;
  }

  // Get filename. Do not free the filename string -- it is owned by FcPattern
  // "match". Likewise, do not use the filename after match is freed.
  FcChar8 *filename;
  result = FcPatternGetString(match, FC_FILE, 0, &filename);

  FT_Error error = FT_New_Face(lib, reinterpret_cast<const char*>(filename), 0,
                               face);

  if (!error)
  {
    vtkDebugWithObjectMacro(vtkFreeTypeTools::GetInstance(),
                            <<"Loading system font: "
                            << reinterpret_cast<const char*>(filename));
  }

  FcCharSetDestroy(charSet);
  charSet = NULL;
  FcFontSetDestroy(fontMatches);
  fontMatches = NULL;

  if (error)
  {
    return false;
  }

  return true;
}
