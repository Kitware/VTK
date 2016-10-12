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
bool vtkMathTextFreeTypeTextRenderer::FreeTypeIsSupported()
{
  return this->FreeTypeTools != NULL;
}

//------------------------------------------------------------------------------
bool vtkMathTextFreeTypeTextRenderer::MathTextIsSupported()
{
  return this->MathTextUtilities != NULL &&
         this->MathTextUtilities->IsAvailable();
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
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->GetBoundingBox(tprop, str.c_str(), dpi,
                                                    bbox))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 GetBoundingBox overload:
      return this->FreeTypeTools->GetBoundingBox(
            tprop, vtkUnicodeString::from_utf8(cleanString), dpi, bbox);
    }
    case Default:
    case UserBackend:
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
      if (this->MathTextIsSupported())
      {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetBoundingBox(tprop, str.utf8_str(), dpi,
                                                    bbox))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetBoundingBox(tprop, cleanString, dpi, bbox);
    }
    case Default:
    case UserBackend:
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
bool vtkMathTextFreeTypeTextRenderer::GetMetricsInternal(
    vtkTextProperty *tprop, const vtkStdString &str,
    vtkTextRenderer::Metrics &metrics, int dpi, int backend)
{
  if (!tprop)
  {
    vtkErrorMacro("No text property supplied!");
    return false;
  }

  metrics = Metrics();
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
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->GetMetrics(tprop, str.c_str(), dpi,
                                                metrics))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 GetBoundingBox overload:
      return this->FreeTypeTools->GetMetrics(
            tprop, vtkUnicodeString::from_utf8(cleanString), dpi, metrics);
    }
    case Default:
    case UserBackend:
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
bool vtkMathTextFreeTypeTextRenderer::GetMetricsInternal(
    vtkTextProperty *tprop, const vtkUnicodeString &str,
    vtkTextRenderer::Metrics &metrics, int dpi, int backend)
{
  if (!tprop)
  {
    vtkErrorMacro("No text property supplied!");
    return false;
  }

  metrics = Metrics();
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
      if (this->MathTextIsSupported())
      {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->GetMetrics(tprop, str.utf8_str(), dpi,
                                                metrics))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetMetrics(tprop, cleanString, dpi, metrics);
    }
    case Default:
    case UserBackend:
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
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->RenderString(str.c_str(), data, tprop,
                                                  dpi, textDims))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      // Interpret string as UTF-8, use the UTF-16 RenderString overload:
      return this->FreeTypeTools->RenderString(
            tprop, vtkUnicodeString::from_utf8(cleanString), dpi, data,
            textDims);
    }
    case Default:
    case UserBackend:
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
      if (this->MathTextIsSupported())
      {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->RenderString(str.utf8_str(), data, tprop,
                                                  dpi, textDims))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->RenderString(tprop, cleanString, dpi, data,
                                               textDims);
    }
    case Default:
    case UserBackend:
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
      if (this->MathTextIsSupported())
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
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetConstrainedFontSize(cleanString, tprop,
                                                         dpi, targetWidth,
                                                         targetHeight);
    }
    case Default:
    case UserBackend:
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
      if (this->MathTextIsSupported())
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
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->GetConstrainedFontSize(cleanString, tprop,
                                                         dpi, targetWidth,
                                                         targetHeight);
    }
    case Default:
    case UserBackend:
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
    vtkTextProperty *tprop, const vtkStdString &str, vtkPath *path, int dpi,
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
      if (this->MathTextIsSupported())
      {
        if (this->MathTextUtilities->StringToPath(str.c_str(), path, tprop,
                                                  dpi))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkStdString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->StringToPath(tprop, str, dpi, path);
    }
    case Default:
    case UserBackend:
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
    vtkTextProperty *tprop, const vtkUnicodeString &str, vtkPath *path, int dpi,
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
      if (this->MathTextIsSupported())
      {
        vtkDebugMacro("Converting UTF16 to UTF8 for MathText rendering.");
        if (this->MathTextUtilities->StringToPath(str.utf8_str(), path, tprop,
                                                  dpi))
        {
          return true;
        }
      }
      vtkDebugMacro("MathText unavailable. Falling back to FreeType.");
      VTK_FALLTHROUGH;
    case FreeType:
    {
      vtkUnicodeString cleanString(str);
      this->CleanUpFreeTypeEscapes(cleanString);
      return this->FreeTypeTools->StringToPath(tprop, str, dpi, path);
    }
    case Default:
    case UserBackend:
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
}

//------------------------------------------------------------------------------
vtkMathTextFreeTypeTextRenderer::~vtkMathTextFreeTypeTextRenderer()
{
}
