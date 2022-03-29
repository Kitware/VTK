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

#include "vtkDebugLeaks.h" // Must be included before any singletons
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"

#include <vtksys/RegularExpression.hxx>

//------------------------------------------------------------------------------
// The singleton, and the singleton cleanup
vtkTextRenderer* vtkTextRenderer::Instance = nullptr;
vtkTextRendererCleanup vtkTextRenderer::Cleanup;

//------------------------------------------------------------------------------
vtkTextRendererCleanup::vtkTextRendererCleanup() = default;

//------------------------------------------------------------------------------
vtkTextRendererCleanup::~vtkTextRendererCleanup()
{
  vtkTextRenderer::SetInstance(nullptr);
}

//------------------------------------------------------------------------------
void vtkTextRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Instance: " << vtkTextRenderer::Instance << endl;
  os << indent << "MathTextRegExp: " << this->MathTextRegExp << endl;
  os << indent << "MathTextRegExp2: " << this->MathTextRegExp2 << endl;
}

//------------------------------------------------------------------------------
vtkTextRenderer* vtkTextRenderer::New()
{
  vtkTextRenderer* instance = vtkTextRenderer::GetInstance();
  if (instance)
  {
    instance->Register(nullptr);
  }
  return instance;
}

//------------------------------------------------------------------------------
vtkTextRenderer* vtkTextRenderer::GetInstance()
{
  if (vtkTextRenderer::Instance)
  {
    return vtkTextRenderer::Instance;
  }

  vtkTextRenderer::Instance =
    static_cast<vtkTextRenderer*>(vtkObjectFactory::CreateInstance("vtkTextRenderer"));

  return vtkTextRenderer::Instance;
}

//------------------------------------------------------------------------------
void vtkTextRenderer::SetInstance(vtkTextRenderer* instance)
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
    instance->Register(nullptr);
  }
}

//------------------------------------------------------------------------------
vtkTextRenderer::vtkTextRenderer()
  : MathTextRegExp(new vtksys::RegularExpression("[^\\]\\$.*[^\\]\\$"))
  , MathTextRegExp2(new vtksys::RegularExpression("^\\$.*[^\\]\\$"))
  , MathTextRegExpColumn(new vtksys::RegularExpression("[^\\]\\|"))
  , DefaultBackend(Detect)
{
}

//------------------------------------------------------------------------------
vtkTextRenderer::~vtkTextRenderer()
{
  delete this->MathTextRegExp;
  delete this->MathTextRegExp2;
  delete this->MathTextRegExpColumn;
}

//------------------------------------------------------------------------------
int vtkTextRenderer::DetectBackend(const vtkStdString& str)
{
  if (!str.empty())
  {
    // the vtksys::RegularExpression class doesn't support {...|...} "or"
    // branching, so we check the first character to see which regexp to use:
    //
    // Find unescaped "$...$" patterns where "$" is not the first character:
    //   MathTextRegExp  = "[^\\]\\$.*[^\\]\\$"
    // Find unescaped "$...$" patterns where "$" is the first character:
    //   MathTextRegExp2 = "^\\$.*[^\\]\\$"
    // Find unescaped "|" character that defines a multicolumn line
    //  MathTextRegExpColumn = "[^\\]|"
    if ((str[0] == '$' && this->MathTextRegExp2->find(str)) || this->MathTextRegExp->find(str) ||
      this->MathTextRegExpColumn->find(str))
    {
      return static_cast<int>(MathText);
    }
  }
  return static_cast<int>(FreeType);
}

//------------------------------------------------------------------------------
void vtkTextRenderer::CleanUpFreeTypeEscapes(vtkStdString& str)
{
  size_t ind = str.find("\\$");
  while (ind != std::string::npos)
  {
    str.replace(ind, 2, "$");
    ind = str.find("\\$", ind + 1);
  }
}
