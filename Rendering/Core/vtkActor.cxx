/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <cmath>

vtkCxxSetObjectMacro(vtkActor,Texture,vtkTexture);
vtkCxxSetObjectMacro(vtkActor,Mapper,vtkMapper);
vtkCxxSetObjectMacro(vtkActor,BackfaceProperty,vtkProperty);
vtkCxxSetObjectMacro(vtkActor,Property,vtkProperty);

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkActor)

// Creates an actor with the following defaults: origin(0,0,0)
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.
vtkActor::vtkActor()
{
  this->Mapper = NULL;
  this->Property = NULL;
  this->BackfaceProperty = NULL;
  this->Texture = NULL;

  this->ForceOpaque = false;
  this->ForceTranslucent = false;

  // The mapper bounds are cache to know when the bounds must be recomputed
  // from the mapper bounds.
  vtkMath::UninitializeBounds(this->MapperBounds);
}

//----------------------------------------------------------------------------
vtkActor::~vtkActor()
{
  if ( this->Property != NULL)
  {
    this->Property->UnRegister(this);
    this->Property = NULL;
  }

  if ( this->BackfaceProperty != NULL)
  {
    this->BackfaceProperty->UnRegister(this);
    this->BackfaceProperty = NULL;
  }

  if (this->Mapper)
  {
    this->Mapper->UnRegister(this);
    this->Mapper = NULL;
  }
  this->SetTexture(NULL);
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkActor::ShallowCopy(vtkProp *prop)
{
  vtkActor *a = vtkActor::SafeDownCast(prop);
  if ( a != NULL )
  {
    this->SetMapper(a->GetMapper());
    this->SetProperty(a->GetProperty());
    this->SetBackfaceProperty(a->GetBackfaceProperty());
    this->SetTexture(a->GetTexture());
  }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkActor::GetActors(vtkPropCollection *ac)
{
  ac->AddItem(this);
}

//----------------------------------------------------------------------------
// should be called from the render methods only
int vtkActor::GetIsOpaque()
{
  if (this->ForceOpaque)
  {
    return 1;
  }

  if (this->ForceTranslucent)
  {
    return 0;
  }

  // make sure we have a property
  if(!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }
  bool is_opaque = (this->Property->GetOpacity() >= 1.0);

  // are we using an opaque texture, if any?
  is_opaque = is_opaque &&
    (this->Texture ==NULL || this->Texture->IsTranslucent() == 0);

  // are we using an opaque scalar array, if any?
  is_opaque = is_opaque &&
    (this->Mapper == NULL || this->Mapper->GetIsOpaque());

  return is_opaque? 1 : 0;
}

//----------------------------------------------------------------------------
// This causes the actor to be rendered. It in turn will render the actor's
// property, texture map and then mapper. If a property hasn't been
// assigned, then the actor will create one automatically. Note that a
// side effect of this method is that the visualization network is updated.
int vtkActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  int          renderedSomething = 0;
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);

  if ( ! this->Mapper )
  {
    return 0;
  }

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  // is this actor opaque
  // Do this check only when not in selection mode
  if (this->GetIsOpaque() ||
    (ren->GetSelector() && this->Property->GetOpacity() > 0.0))
  {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
    {
      this->BackfaceProperty->BackfaceRender(this, ren);
    }

    // render the texture
    if (this->Texture)
    {
      this->Texture->Render(ren);
      if (this->Texture->GetTransform())
      {
        vtkInformation *info = this->GetPropertyKeys();
        if (!info)
        {
          info = vtkInformation::New();
          this->SetPropertyKeys(info);
          info->Delete();
        }
        info->Set(vtkProp::GeneralTextureTransform(),
          &(this->Texture->GetTransform()->GetMatrix()->Element[0][0])
          ,16);
      }
    }
    this->Render(ren,this->Mapper);
    this->Property->PostRender(this, ren);
    if (this->Texture)
    {
      this->Texture->PostRender(ren);
      if (this->Texture->GetTransform())
      {
        vtkInformation *info = this->GetPropertyKeys();
        info->Remove(vtkProp::GeneralTextureTransform());
      }
    }
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    renderedSomething = 1;
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkActor::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  int          renderedSomething = 0;
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);

  if ( ! this->Mapper )
  {
    return 0;
  }

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  // is this actor opaque ?
  if (!this->GetIsOpaque())
  {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
    {
      this->BackfaceProperty->BackfaceRender(this, ren);
    }

    // render the texture
    if (this->Texture)
    {
      this->Texture->Render(ren);
      if (this->Texture->GetTransform())
      {
        vtkInformation *info = this->GetPropertyKeys();
        if (!info)
        {
          info = vtkInformation::New();
          this->SetPropertyKeys(info);
          info->Delete();
        }
        info->Set(vtkProp::GeneralTextureTransform(),
          &(this->Texture->GetTransform()->GetMatrix()->Element[0][0])
          ,16);
      }
    }
    this->Render(ren,this->Mapper);
    this->Property->PostRender(this, ren);
    if (this->Texture)
    {
      this->Texture->PostRender(ren);
      if (this->Texture->GetTransform())
      {
        vtkInformation *info = this->GetPropertyKeys();
        info->Remove(vtkProp::GeneralTextureTransform());
      }
    }
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();

    renderedSomething = 1;
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkActor::HasTranslucentPolygonalGeometry()
{
  if ( ! this->Mapper )
  {
    return 0;
  }
  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  // is this actor opaque ?
  return !this->GetIsOpaque();
}

//----------------------------------------------------------------------------
void vtkActor::ReleaseGraphicsResources(vtkWindow *win)
{
  vtkRenderWindow *renWin = static_cast<vtkRenderWindow *>(win);

  // pass this information onto the mapper
  if (this->Mapper)
  {
    this->Mapper->ReleaseGraphicsResources(renWin);
  }

  // pass this information onto the texture
  if (this->Texture)
  {
    this->Texture->ReleaseGraphicsResources(renWin);
  }

  // pass this information to the properties
  if (this->Property)
  {
    this->Property->ReleaseGraphicsResources(renWin);
  }
  if (this->BackfaceProperty)
  {
    this->BackfaceProperty->ReleaseGraphicsResources(renWin);
  }
}

//----------------------------------------------------------------------------
vtkProperty* vtkActor::MakeProperty()
{
  return vtkProperty::New();
}

//----------------------------------------------------------------------------
vtkProperty *vtkActor::GetProperty()
{
  if ( this->Property == NULL )
  {
    vtkProperty *p = this->MakeProperty();
    this->SetProperty(p);
    p->Delete();
  }
  return this->Property;
}

//----------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkActor::GetBounds()
{
  int i,n;
  double bbox[24], *fptr;

  vtkDebugMacro( << "Getting Bounds" );

  // get the bounds of the Mapper if we have one
  if (!this->Mapper)
  {
    return this->Bounds;
  }

  const double *bounds = this->Mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
  {
    return NULL;
  }

  // Check for the special case when the actor is empty.
  if (!vtkMath::AreBoundsInitialized(bounds))
  {
    memcpy( this->MapperBounds, bounds, 6*sizeof(double) );
    vtkMath::UninitializeBounds(this->Bounds);
    this->BoundsMTime.Modified();
    return this->Bounds;
  }

  // Check if we have cached values for these bounds - we cache the
  // values returned by this->Mapper->GetBounds() and we store the time
  // of caching. If the values returned this time are different, or
  // the modified time of this class is newer than the cached time,
  // then we need to rebuild.
  if ( ( memcmp( this->MapperBounds, bounds, 6*sizeof(double) ) != 0 ) ||
       ( this->GetMTime() > this->BoundsMTime ) )
  {
    vtkDebugMacro( << "Recomputing bounds..." );

    memcpy( this->MapperBounds, bounds, 6*sizeof(double) );

    // fill out vertices of a bounding box
    bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
    bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
    bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
    bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
    bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
    bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
    bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
    bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];

    // make sure matrix (transform) is up-to-date
    this->ComputeMatrix();

    // and transform into actors coordinates
    fptr = bbox;
    for (n = 0; n < 8; n++)
    {
      double homogeneousPt[4] = {fptr[0], fptr[1], fptr[2], 1.0};
      this->Matrix->MultiplyPoint(homogeneousPt, homogeneousPt);
      fptr[0] = homogeneousPt[0] / homogeneousPt[3];
      fptr[1] = homogeneousPt[1] / homogeneousPt[3];
      fptr[2] = homogeneousPt[2] / homogeneousPt[3];
      fptr += 3;
    }

    // now calc the new bounds
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
    for (i = 0; i < 8; i++)
    {
      for (n = 0; n < 3; n++)
      {
        if (bbox[i*3+n] < this->Bounds[n*2])
        {
          this->Bounds[n*2] = bbox[i*3+n];
        }
        if (bbox[i*3+n] > this->Bounds[n*2+1])
        {
          this->Bounds[n*2+1] = bbox[i*3+n];
        }
      }
    }
    this->BoundsMTime.Modified();
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkActor::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if ( this->Property != NULL )
  {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  if ( this->BackfaceProperty != NULL )
  {
    time = this->BackfaceProperty->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  if ( this->Texture != NULL )
  {
    time = this->Texture->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkActor::GetRedrawMTime()
{
  vtkMTimeType mTime=this->GetMTime();
  vtkMTimeType time;

  vtkMapper *myMapper = this->GetMapper();
  if ( myMapper != NULL )
  {
    time = myMapper->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (myMapper->GetInput() != NULL)
    {
      myMapper->GetInputAlgorithm()->Update();
      time = myMapper->GetInput()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Mapper )
  {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Mapper: (none)\n";
  }

  if ( this->Property )
  {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if ( this->BackfaceProperty )
  {
    os << indent << "BackfaceProperty:\n";
    this->BackfaceProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "BackfaceProperty: (none)\n";
  }

  if ( this->Texture )
  {
    os << indent << "Texture: " << this->Texture << "\n";
  }
  else
  {
    os << indent << "Texture: (none)\n";
  }

  os << indent << "ForceOpaque: " << (this->ForceOpaque ? "true" : "false") << "\n";
  os << indent << "ForceTranslucent: " << (this->ForceTranslucent ? "true" : "false") << "\n";

}

//----------------------------------------------------------------------------
bool vtkActor::GetSupportsSelection()
{
  if (this->Mapper)
  {
    return this->Mapper->GetSupportsSelection();
  }

  return false;
}
