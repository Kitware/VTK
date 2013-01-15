/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextRenderer.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkUnicodeString.h"

#include <vtksys/RegularExpression.hxx>

#ifdef VTK_DEBUG_LEAKS
#include "vtkDebugLeaks.h"
#endif

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup
vtkTextRenderer *vtkTextRenderer::Instance = NULL;
vtkTextRendererCleanup vtkTextRenderer::Cleanup;

//----------------------------------------------------------------------------
vtkTextRendererCleanup::vtkTextRendererCleanup()
{
}

//----------------------------------------------------------------------------
vtkTextRendererCleanup::~vtkTextRendererCleanup()
{
  vtkTextRenderer::SetInstance(NULL);
}

//----------------------------------------------------------------------------
void vtkTextRenderer::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Instance: " << vtkTextRenderer::Instance << endl;
  os << indent << "HasFreeType: " << this->HasFreeType << endl;
  os << indent << "HasMathText: " << this->HasMathText << endl;
  os << indent << "MathTextRegExp: " << this->MathTextRegExp << endl;
  os << indent << "MathTextRegExp2: " << this->MathTextRegExp2 << endl;
}

//----------------------------------------------------------------------------
vtkTextRenderer *vtkTextRenderer::New()
{
  vtkTextRenderer *instance = vtkTextRenderer::GetInstance();
  if (instance)
    {
    instance->Register(NULL);
    }
  return instance;
}

//----------------------------------------------------------------------------
vtkTextRenderer *vtkTextRenderer::GetInstance()
{
  if (vtkTextRenderer::Instance)
    {
    return vtkTextRenderer::Instance;
    }

  vtkTextRenderer::Instance = static_cast<vtkTextRenderer*>(
        vtkObjectFactory::CreateInstance("vtkTextRenderer"));

  // Clean up any leaked references from vtkDebugLeaks if needed
#ifdef VTK_DEBUG_LEAKS
  if (!vtkTextRenderer::Instance)
    {
    vtkDebugLeaks::DestructClass("vtkTextRenderer");
    }
#endif

  return vtkTextRenderer::Instance;
}

//----------------------------------------------------------------------------
void vtkTextRenderer::SetInstance(vtkTextRenderer *instance)
{
  if (vtkTextRenderer::Instance == instance)
    {
    return;
    }

  if (vtkTextRenderer::Instance)
    {
    vtkTextRenderer::Instance->Delete();
    }

  vtkTextRenderer::Instance = instance;

  if (instance)
    {
    instance->Register(NULL);
    }
}

//----------------------------------------------------------------------------
vtkTextRenderer::vtkTextRenderer()
  : MathTextRegExp(new vtksys::RegularExpression("[^\\]\\$.+[^\\]\\$")),
    MathTextRegExp2(new vtksys::RegularExpression("^\\$.+[^\\]\\$")),
    HasFreeType(false),
    HasMathText(false),
    DefaultBackend(Detect)
{
}

//----------------------------------------------------------------------------
vtkTextRenderer::~vtkTextRenderer()
{
  delete this->MathTextRegExp;
  delete this->MathTextRegExp2;
}

//----------------------------------------------------------------------------
int vtkTextRenderer::DetectBackend(const vtkStdString &str)
{
  if (!str.empty())
    {
    // the vtksys::RegularExpression class doesn't support {...|...} "or"
    // branching, so we check the first character to see which regexp to use:
    //
    // Find unescaped "$...$" patterns where "$" is not the first character:
    //   MathTextRegExp  = "[^\\]\\$.+[^\\]\\$"
    // Find unescaped "$...$" patterns where "$" is the first character:
    //   MathTextRegExp2 = "^\\$.+[^\\]\\$"
    if ((str[0] == '$' && this->MathTextRegExp2->find(str)) ||
        this->MathTextRegExp->find(str))
      {
      return static_cast<int>(MathText);
      }
    }
  return static_cast<int>(FreeType);
}

//----------------------------------------------------------------------------
int vtkTextRenderer::DetectBackend(const vtkUnicodeString &str)
{
  if (!str.empty())
    {
    // the vtksys::RegularExpression class doesn't support {...|...} "or"
    // branching, so we check the first character to see which regexp to use:
    //
    // Find unescaped "$...$" patterns where "$" is not the first character:
    //   MathTextRegExp  = "[^\\]\\$.+[^\\]\\$"
    // Find unescaped "$...$" patterns where "$" is the first character:
    //   MathTextRegExp2 = "^\\$.+[^\\]\\$"
    if ((str[0] == '$' && this->MathTextRegExp2->find(str.utf8_str())) ||
        this->MathTextRegExp->find(str.utf8_str()))
      {
      return static_cast<int>(MathText);
      }
    }
  return static_cast<int>(FreeType);
}

//----------------------------------------------------------------------------
void vtkTextRenderer::CleanUpFreeTypeEscapes(vtkStdString &str)
{
  size_t ind = str.find("\\$");
  while (ind != std::string::npos)
    {
    str.replace(ind, 2, "$");
    ind = str.find("\\$", ind + 1);
    }
}

//----------------------------------------------------------------------------
void vtkTextRenderer::CleanUpFreeTypeEscapes(vtkUnicodeString &str)
{
  // vtkUnicodeString has only a subset of the std::string API available, so
  // this method is more complex than the std::string overload.
  vtkUnicodeString::const_iterator begin = str.begin();
  vtkUnicodeString::const_iterator end = str.end();
  vtkUnicodeString tmp;

  for (vtkUnicodeString::const_iterator it = begin; it != end; ++it)
    {
    if (*it != '\\')
      {
      continue;
      }

    // No operator+ in the unicode string iterator. Copy and advance it:
    vtkUnicodeString::const_iterator nextChar = it;
    std::advance(nextChar, 1);
    if (*nextChar != '$')
      {
      continue;
      }

    // We found a "\$" in the string. Append [begin, it) into tmp.
    tmp.append(begin, it);

    // Add the dollar sign
    tmp.append(vtkUnicodeString::from_utf8("$"));

    // Reset the iterators to continue checking the rest of the string.
    begin = it;
    std::advance(it, 1);
    std::advance(begin, 2);
    }

  // Append the last bit of the string to tmp
  tmp.append(begin, end);

  // Update the input with the cleaned up string:
  str = tmp;
}
