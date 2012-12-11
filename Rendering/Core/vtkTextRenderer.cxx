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
    HasFreeType(false),
    HasMathText(false),
    DefaultBackend(Detect)
{
}

//----------------------------------------------------------------------------
vtkTextRenderer::~vtkTextRenderer()
{
  delete this->MathTextRegExp;
}

//----------------------------------------------------------------------------
int vtkTextRenderer::DetectBackend(const vtkStdString &str)
{
  if (this->MathTextRegExp->find(str))
    {
    return static_cast<int>(MathText);
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
