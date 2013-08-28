/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextFreeTypeTextRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextFreeTypeTextRenderer.h"

#include "vtkFreeTypeTools.h"
#include "vtkMathTextUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkUnicodeString.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkMathTextFreeTypeTextRenderer)

//------------------------------------------------------------------------------
void vtkMathTextFreeTypeTextRenderer::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FreeTypeTools)
    {
    os << indent << "FreeTypeTools:" << endl;
    this->FreeTypeTools->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "FreeTypeTools: (NULL)" << endl;
    }

  if (this->MathTextUtilities)
    {
    os << indent << "MathTextUtilities:" << endl;
    this->MathTextUtilities->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "MathTextUtilities: (NULL)" << endl;
    }
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::GetBoundingBoxInternal(
    vtkTextProperty *tprop, const vtkStdString &str, int bbox[4], int dpi,
    int backend)
{
  if (!bbox || !tprop)
    {
    vtkErrorMacro("No bounding box container and/or text property supplied!");
    return false;
    }

  memset(bbox, 0, 4 * sizeof(int));
  if (str.empty())
    {
    return true;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        if (this->MathTextUtilities->GetBoundingBox(tprop, str.c_str(), dpi,
                                                    bbox))
          {
          return true;
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 GetBoundingBox overload:
      return this->FreeTypeTools->GetBoundingBox(
            tprop, vtkUnicodeString::from_utf8(cleanString), bbox);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::GetBoundingBoxInternal(
    vtkTextProperty *tprop, const vtkUnicodeString &str, int bbox[], int dpi,
    int backend)
{
  if (!bbox || !tprop)
    {
    vtkErrorMacro("No bounding box container and/or text property supplied!");
    return false;
    }

  memset(bbox, 0, 4 * sizeof(int));
  if (str.empty())
    {
    return true;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetBoundingBox(tprop, str.utf8_str(), dpi,
                                                    bbox))
          {
          return true;
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetBoundingBox(tprop, cleanString, bbox);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::RenderStringInternal(
    vtkTextProperty *tprop, const vtkStdString &str, vtkImageData *data,
    int textDims[2], int dpi, int backend)
{
  if (!data || !tprop)
    {
    vtkErrorMacro("No image container and/or text property supplied!");
    return false;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        if (this->MathTextUtilities->RenderString(str.c_str(), data, tprop,
                                                  dpi, textDims))
          {
          return true;
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 RenderString overload:
      return this->FreeTypeTools->RenderString(
            tprop, vtkUnicodeString::from_utf8(cleanString), data, textDims);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::RenderStringInternal(
    vtkTextProperty *tprop, const vtkUnicodeString &str, vtkImageData *data,
    int textDims[], int dpi, int backend)
{
  if (!data || !tprop)
    {
    vtkErrorMacro("No image container and/or text property supplied!");
    return false;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->RenderString(str.utf8_str(), data, tprop,
                                                  dpi, textDims))
          {
          return true;
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->RenderString(tprop, cleanString, data,
                                               textDims);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
int vtkMathTextFreeTypeTextRenderer::GetConstrainedFontSizeInternal(
    const vtkStdString &str, vtkTextProperty *tprop, int targetWidth,
    int targetHeight, int dpi, int backend)
{
  if (!tprop)
    {
    vtkErrorMacro("No text property supplied!");
    return false;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        if (this->MathTextUtilities->GetConstrainedFontSize(str.c_str(), tprop,
                                                            targetWidth,
                                                            targetHeight,
                                                            dpi) != -1)
          {
          return tprop->GetFontSize();
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetConstrainedFontSize(cleanString, tprop,
                                                         targetWidth,
                                                         targetHeight);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
int vtkMathTextFreeTypeTextRenderer::GetConstrainedFontSizeInternal(
    const vtkUnicodeString &str, vtkTextProperty *tprop, int targetWidth,
    int targetHeight, int dpi, int backend)
{
  if (!tprop)
    {
    vtkErrorMacro("No text property supplied!");
    return false;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetConstrainedFontSize(str.utf8_str(),
                                                            tprop, targetWidth,
                                                            targetHeight,
                                                            dpi) != -1)
          {
          return tprop->GetFontSize();
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetConstrainedFontSize(cleanString, tprop,
                                                         targetWidth,
                                                         targetHeight);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::StringToPathInternal(
    vtkTextProperty *tprop, const vtkStdString &str, vtkPath *path, int backend)
{
  if (!path || !tprop)
    {
    vtkErrorMacro("No path container and/or text property supplied!");
    return false;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        if (this->MathTextUtilities->StringToPath(str.c_str(), path, tprop))
          {
          return true;
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->StringToPath(tprop, str, path);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::StringToPathInternal(
    vtkTextProperty *tprop, const vtkUnicodeString &str, vtkPath *path,
    int backend)
{
  if (!path || !tprop)
    {
    vtkErrorMacro("No path container and/or text property supplied!");
    return false;
    }

  if (static_cast<Backend>(backend) == Default)
    {
    backend = this->DefaultBackend;
    }

  if (static_cast<Backend>(backend) == Detect)
    {
    backend = static_cast<int>(this->DetectBackend(str));
    }

  switch (static_cast<Backend>(backend))
    {
    case MathText:
      if (this->HasMathText)
        {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->StringToPath(str.utf8_str(), path, tprop))
          {
          return true;
          }
        }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
    case FreeType:
      {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->StringToPath(tprop, str, path);
      }
    default:
      vtkDebugMacro("Unrecognized backend requested: " << backend);
      break;
    case Detect:
      vtkDebugMacro("Unhandled 'Detect' backend requested!");
      break;
    }
  return false;
}

//------------------------------------------------------------------------------
void vtkMathTextFreeTypeTextRenderer::SetScaleToPowerOfTwoInternal(bool scale)
{
  if (this->FreeTypeTools)
    {
    this->FreeTypeTools->SetScaleToPowerTwo(scale);
    }
  if (this->MathTextUtilities)
    {
    this->MathTextUtilities->SetScaleToPowerOfTwo(scale);
    }
}

//------------------------------------------------------------------------------
vtkMathTextFreeTypeTextRenderer::vtkMathTextFreeTypeTextRenderer()
{
  this->FreeTypeTools = vtkFreeTypeTools::GetInstance();
  this->MathTextUtilities = vtkMathTextUtilities::GetInstance();

  this->HasFreeType = (this->FreeTypeTools != NULL);
  this->HasMathText = (this->MathTextUtilities != NULL);
}

//------------------------------------------------------------------------------
vtkMathTextFreeTypeTextRenderer::~vtkMathTextFreeTypeTextRenderer()
{
}
