/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVtkJSViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVtkJSViewNodeFactory.h"

#include <vtkActor.h>
#include <vtkActorNode.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkGlyph3DMapper.h>
#include <vtkMapper.h>
#include <vtkMapperNode.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererNode.h>
#include <vtkWindowNode.h>

#include "vtkVtkJSSceneGraphSerializer.h"

#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
#include <vtkCompositePolyDataMapper2.h>
#endif

#include <type_traits>

namespace
{
// A template for performing a compile-time check if a scene element inherits
// from vtkAlgorithm and calling Update on it if it does.
class UpdateIfAlgorithm
{
  template <typename X>
  static typename std::enable_if<std::is_base_of<vtkAlgorithm, X>::value>::type MaybeUpdate(X* x)
  {
    x->Update();
  }

  template <typename X>
  static typename std::enable_if<!std::is_base_of<vtkAlgorithm, X>::value>::type MaybeUpdate(X*)
  {
  }

public:
  template <typename MaybeAlgorithm>
  static void Update(MaybeAlgorithm* maybeAlgorithm)
  {
    UpdateIfAlgorithm::MaybeUpdate(maybeAlgorithm);
  }
};

// A template for constructing view nodes associated with scene elements and
// their associated renderables.
template <typename Base, typename Renderable>
class vtkVtkJSViewNode : public Base
{
public:
  static vtkViewNode* New()
  {
    vtkVtkJSViewNode* result = new vtkVtkJSViewNode;
    result->InitializeObjectBase();
    return result;
  }

  void Synchronize(bool prepass) override
  {
    this->Base::Synchronize(prepass);
    if (prepass)
    {
      auto factory = vtkVtkJSViewNodeFactory::SafeDownCast(this->GetMyFactory());
      if (factory != nullptr)
      {
        factory->GetSerializer()->Add(this, Renderable::SafeDownCast(this->GetRenderable()));
      }
    }
  }

  void Render(bool prepass) override
  {
    this->Base::Render(prepass);
    UpdateIfAlgorithm::Update(Renderable::SafeDownCast(this->GetRenderable()));
  }
};
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVtkJSViewNodeFactory);
vtkCxxSetObjectMacro(vtkVtkJSViewNodeFactory, Serializer, vtkVtkJSSceneGraphSerializer);

//----------------------------------------------------------------------------
vtkVtkJSViewNodeFactory::vtkVtkJSViewNodeFactory()
{
  this->Serializer = vtkVtkJSSceneGraphSerializer::New();

  // Since a view node is constructed if an override exists for one of its base
  // classes, we only need to span the set of base renderable types and provide
  // specializations when custom logic is required by vtk-js.

  // These overrides span the base renderable types.
  this->RegisterOverride("vtkActor", vtkVtkJSViewNode<vtkActorNode, vtkActor>::New);
  this->RegisterOverride("vtkMapper", vtkVtkJSViewNode<vtkMapperNode, vtkMapper>::New);
  this->RegisterOverride("vtkRenderWindow", vtkVtkJSViewNode<vtkWindowNode, vtkRenderWindow>::New);
  this->RegisterOverride("vtkRenderer", vtkVtkJSViewNode<vtkRendererNode, vtkRenderer>::New);

  // These overrides are necessary to accommodate custom logic that must be
  // performed when converting these renderables to vtk-js.
  this->RegisterOverride(
    "vtkCompositePolyDataMapper", vtkVtkJSViewNode<vtkMapperNode, vtkCompositePolyDataMapper>::New);
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
  this->RegisterOverride("vtkCompositePolyDataMapper2",
    vtkVtkJSViewNode<vtkMapperNode, vtkCompositePolyDataMapper2>::New);
#endif
  this->RegisterOverride(
    "vtkGlyph3DMapper", vtkVtkJSViewNode<vtkMapperNode, vtkGlyph3DMapper>::New);
}

//----------------------------------------------------------------------------
vtkVtkJSViewNodeFactory::~vtkVtkJSViewNodeFactory()
{
  this->SetSerializer(nullptr);
}

//----------------------------------------------------------------------------
void vtkVtkJSViewNodeFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
