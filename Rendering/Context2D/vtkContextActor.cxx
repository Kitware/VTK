/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContextActor.h"

#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"

#include "vtkContext3D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkTransform2D.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

#include <algorithm>

namespace
{
  // vtkViewportSpecification is a helper class that makes it easier to do some
  // of the arithmetic for dealing with tiled displays (in VTK, for saving large
  // images and in ParaView for actual tiled display).
  template <class T>
    class vtkViewportSpecification
  {
    public:
      vtkViewportSpecification(const T* input)
      {
        this->Data[0] = input[0];
        this->Data[1] = input[1];
        this->Data[2] = input[2];
        this->Data[3] = input[3];
      }

      vtkViewportSpecification(const vtkViewportSpecification<T> &other)
      {
        this->Data[0] = other.Data[0];
        this->Data[1] = other.Data[1];
        this->Data[2] = other.Data[2];
        this->Data[3] = other.Data[3];
      }

      // returns false is intersection results in an empty box, otherwise returns true.
      // here we assume that typename T handles signed values correctly.
      bool intersect(const vtkViewportSpecification<T> &other)
      {
        vtkViewportSpecification<T> clone(*this);

        this->Data[0] = std::max(other.x(), clone.x());
        this->Data[1] = std::max(other.y(), clone.y());

        T _width = std::min(clone.x() + clone.width(),
          other.x() + other.width()) - this->Data[0];

        T _height = std::min(clone.y() + clone.height(),
          other.y() + other.height()) - this->Data[1];

        _width = std::max(0, _width);
        _height = std::max(0, _height);

        this->Data[2] = this->x() + _width;
        this->Data[3] = this->y() + _height;
        return (_width !=0 && _height != 0);
      }

      const T* data() const { return this->Data; }
      const T& x() const { return this->Data[0]; }
      const T& y() const { return this->Data[1]; }
      T width() const { return this->Data[2] - this->Data[0]; }
      T height() const { return this->Data[3] - this->Data[1]; }

    private:
      T Data[4];
  };

  template <class T>
    ostream& operator << (ostream& str, const vtkViewportSpecification<T>& other)
  {
      str << other.data()[0] << ", "
        << other.data()[1] << ", "
        << other.data()[2] << ", "
        << other.data()[3];
      return str;
  }

  // use this method to convert from normalized-to-display space i.e. [0.0, 1.0]
  // to screen pixels.
  vtkViewportSpecification<int> convert(
    const vtkViewportSpecification<double>& other,
    int width, int height)
  {
    int value[4];
    value[0] = static_cast<int>(other.data()[0] * width);
    value[1] = static_cast<int>(other.data()[1] * height);
    value[2] = static_cast<int>(other.data()[2] * width);
    value[3] = static_cast<int>(other.data()[3] * height);

    return vtkViewportSpecification<int>(value);
  }
}

vtkObjectFactoryNewMacro(vtkContextActor);

//----------------------------------------------------------------------------
vtkContextActor::vtkContextActor()
{
  this->Initialized = false;
  this->Scene = vtkSmartPointer<vtkContextScene>::New();

  this->Context->SetContext3D(this->Context3D.GetPointer());
}

//----------------------------------------------------------------------------
vtkContextActor::~vtkContextActor()
{
  if (this->Context.GetPointer())
  {
    this->Context->End();
  }
  if (this->Context3D.GetPointer())
  {
    this->Context3D->End();
  }
}

//----------------------------------------------------------------------------
vtkContextScene * vtkContextActor::GetScene()
{
  return this->Scene.GetPointer();
}

//----------------------------------------------------------------------------
void vtkContextActor::SetScene(vtkContextScene *scene)
{
  this->Scene = scene;
}

//----------------------------------------------------------------------------
void vtkContextActor::ReleaseGraphicsResources(vtkWindow *)
{
}

//----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int vtkContextActor::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkContextActor::RenderOverlay");

  if (!this->Context.GetPointer())
  {
    vtkErrorMacro(<< "vtkContextActor::Render - No painter set");
    return 0;
  }

  // view_viewport is a normalized viewport specification for this view in a
  // large "single" window where 0.0 is min and 1.0 is max. For multi-tile
  // views, the range (0-1) will span across multiple tiles.
  vtkViewportSpecification<double> view_viewport(
    viewport->GetViewport());

  // tileviewport is a normalized viewport specification for this "window"
  // mapping where in a multi-tile display, does the current window correspond.
  vtkViewportSpecification<double> tile_viewport(
    viewport->GetVTKWindow()->GetTileViewport());

  // this size is already scaled using TileScale.
  const int *tile_size = viewport->GetVTKWindow()->GetSize();

  // convert both to pixel space before we start doing our arithmetic.
  vtkViewportSpecification<int> tile_viewport_pixels =
    convert(tile_viewport, tile_size[0], tile_size[1]);
  vtkViewportSpecification<int> view_viewport_pixels=
    convert(view_viewport, tile_size[0], tile_size[1]);

  // interacted space.
  vtkViewportSpecification<int> actual_viewport_pixels(view_viewport_pixels);
  if (!actual_viewport_pixels.intersect(tile_viewport_pixels))
  {
    return 1;
  }

  vtkTransform2D* transform = this->Scene->GetTransform();
  transform->Identity();
  transform->Translate(
    view_viewport_pixels.x() - actual_viewport_pixels.x(),
    view_viewport_pixels.y() - actual_viewport_pixels.y());

  if (!this->Initialized)
  {
    this->Initialize(viewport);
  }

  // Pass the viewport details onto the context device.
  int size[2];
  size[0] = view_viewport_pixels.width();
  size[1] = view_viewport_pixels.height();
  vtkRecti viewportRect(actual_viewport_pixels.x() - view_viewport_pixels.x(),
                        actual_viewport_pixels.y() - view_viewport_pixels.y(),
                        actual_viewport_pixels.width(),
                        actual_viewport_pixels.height());
  this->Context->GetDevice()->SetViewportSize(vtkVector2i(size));
  this->Context->GetDevice()->SetViewportRect(viewportRect);

  // This is the entry point for all 2D rendering.
  // First initialize the drawing device.

  this->Context->GetDevice()->Begin(viewport);
  this->Scene->SetGeometry(size);
  this->Scene->Paint(this->Context.GetPointer());
  this->Context->GetDevice()->End();

  return 1;
}

//----------------------------------------------------------------------------
void vtkContextActor::Initialize(vtkViewport*)
{
  // Initialization deferred to the derived actor classes.
}

//----------------------------------------------------------------------------
void vtkContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context.GetPointer())
  {
    this->Context->PrintSelf(os, indent.GetNextIndent());
  }
}
