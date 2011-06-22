/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSlice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSlice.h"

#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"
#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkImageSlice);

//----------------------------------------------------------------------------
class vtkImageToImageMapper3DFriendship
{
public:
  static void SetCurrentProp(vtkImageMapper3D *mapper, vtkImageSlice *prop)
    {
    mapper->CurrentProp = prop;
    }
  static void SetCurrentRenderer(vtkImageMapper3D *mapper, vtkRenderer *ren)
    {
    mapper->CurrentRenderer = ren;
    }
  static void SetStackedImagePass(vtkImageMapper3D *mapper, int pass)
    {
    switch (pass)
      {
      case 0:
        mapper->MatteEnable = true;
        mapper->ColorEnable = false;
        mapper->DepthEnable = false;
        break;
      case 1:
        mapper->MatteEnable = false;
        mapper->ColorEnable = true;
        mapper->DepthEnable = false;
        break;
      case 2:
        mapper->MatteEnable = false;
        mapper->ColorEnable = false;
        mapper->DepthEnable = true;
        break;
      default:
        mapper->MatteEnable = true;
        mapper->ColorEnable = true;
        mapper->DepthEnable = true;
        break;
      }
    }

};

//----------------------------------------------------------------------------
vtkImageSlice::vtkImageSlice()
{
  this->Mapper = NULL;
  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkImageSlice::~vtkImageSlice()
{
  if (this->Property)
    {
    this->Property->UnRegister(this);
    }

  this->SetMapper(NULL);
}

//----------------------------------------------------------------------------
void vtkImageSlice::GetImages(vtkPropCollection *vc)
{
  vc->AddItem(this);
}

//----------------------------------------------------------------------------
void vtkImageSlice::ShallowCopy(vtkProp *prop)
{
  vtkImageSlice *v = vtkImageSlice::SafeDownCast(prop);

  if (v != NULL)
    {
    this->SetMapper(v->GetMapper());
    this->SetProperty(v->GetProperty());
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkImageSlice::SetMapper(vtkImageMapper3D *mapper)
{
  if (this->Mapper != mapper)
    {
    if (this->Mapper != NULL)
      {
      vtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, NULL);
      this->Mapper->UnRegister(this);
      }
    this->Mapper = mapper;
    if (this->Mapper != NULL)
      {
      this->Mapper->Register(this);
      vtkImageToImageMapper3DFriendship::SetCurrentProp(mapper, this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkImageSlice::GetBounds()
{
  int i,n;
  double *bounds, bbox[24], *fptr;

  // get the bounds of the Mapper if we have one
  if (!this->Mapper)
    {
    return this->Bounds;
    }

  bounds = this->Mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
    {
    return bounds;
    }

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

  return this->Bounds;
}

//----------------------------------------------------------------------------
// Get the minimum X bound
double vtkImageSlice::GetMinXBound()
{
  this->GetBounds();
  return this->Bounds[0];
}

// Get the maximum X bound
double vtkImageSlice::GetMaxXBound()
{
  this->GetBounds();
  return this->Bounds[1];
}

// Get the minimum Y bound
double vtkImageSlice::GetMinYBound()
{
  this->GetBounds();
  return this->Bounds[2];
}

// Get the maximum Y bound
double vtkImageSlice::GetMaxYBound()
{
  this->GetBounds();
  return this->Bounds[3];
}

// Get the minimum Z bound
double vtkImageSlice::GetMinZBound()
{
  this->GetBounds();
  return this->Bounds[4];
}

// Get the maximum Z bound
double vtkImageSlice::GetMaxZBound()
{
  this->GetBounds();
  return this->Bounds[5];
}

//----------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
int vtkImageSlice::HasTranslucentPolygonalGeometry()
{
  // Always render during opaque pass, to keep the behavior
  // predictable and because depth-peeling kills alpha-blending.
  // In the future, the Renderer should render images in layers,
  // i.e. where each image will have a layer number assigned to it,
  // and the Renderer will do the images in their own pass. 
  return 0;
}

//----------------------------------------------------------------------------
int vtkImageSlice::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageSlice::RenderTranslucentPolygonalGeometry");

  if (this->HasTranslucentPolygonalGeometry())
    {
    this->Render(vtkRenderer::SafeDownCast(viewport));
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkImageSlice::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageSlice::RenderOpaqueGeometry");

  if (!this->HasTranslucentPolygonalGeometry())
    {
    this->Render(vtkRenderer::SafeDownCast(viewport));
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkImageSlice::RenderOverlay(vtkViewport* vtkNotUsed(viewport))
{
  vtkDebugMacro(<< "vtkImageSlice::RenderOverlay");

  // Render the image as an underlay

  return 0;
}

//----------------------------------------------------------------------------
void vtkImageSlice::Render(vtkRenderer *ren)
{
  // Force the creation of a property
  if (!this->Property)
    {
    this->GetProperty();
    }

  if (!this->Property)
    {
    vtkErrorMacro( << "Error generating a property!\n" );
    return;
    }

  if (!this->Mapper)
    {
    vtkErrorMacro( << "You must specify a mapper!\n" );
    return;
    }

  vtkImageToImageMapper3DFriendship::SetCurrentRenderer(this->Mapper, ren);

  this->Update();

  // only call the mapper if it has an input
  if (this->Mapper->GetInput())
    {
    this->Mapper->Render(ren, this);
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    }

  vtkImageToImageMapper3DFriendship::SetCurrentRenderer(this->Mapper, NULL);
}

//----------------------------------------------------------------------------
void vtkImageSlice::ReleaseGraphicsResources(vtkWindow *win)
{
  // pass this information onto the mapper
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
void vtkImageSlice::Update()
{
  if (this->Mapper)
    {
    vtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, this);
    this->Mapper->Update();
    }
}

//----------------------------------------------------------------------------
void vtkImageSlice::SetProperty(vtkImageProperty *property)
{
  if (this->Property != property)
    {
    if (this->Property != NULL)
      {
      this->Property->UnRegister(this);
      }
    this->Property = property;
    if (this->Property != NULL)
      {
      this->Property->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkImageProperty *vtkImageSlice::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
    }
  return this->Property;
}

//----------------------------------------------------------------------------
unsigned long int vtkImageSlice::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserMatrix != NULL )
    {
    time = this->UserMatrix->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserTransform != NULL )
    {
    time = this->UserTransform->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
unsigned long vtkImageSlice::GetRedrawMTime()
{
  unsigned long mTime = this->GetMTime();
  unsigned long time;

  if ( this->Mapper != NULL )
    {
    time = this->Mapper->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->GetMapper()->GetInput() != NULL)
      {
      this->GetMapper()->GetInput()->Update();
      time = this->Mapper->GetInput()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );

    if ( this->Property->GetLookupTable() != NULL )
      {
      // check the lookup table mtime
      time = this->Property->GetLookupTable()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageSlice::SetStackedImagePass(int pass)
{
  if (this->Mapper)
    {
    vtkImageToImageMapper3DFriendship::SetStackedImagePass(
      this->Mapper, pass);
    }
}

//----------------------------------------------------------------------------
void vtkImageSlice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (not defined)\n";
    }

  if( this->Mapper )
    {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Mapper: (not defined)\n";
    }

  // make sure our bounds are up to date
  if ( this->Mapper )
    {
    this->GetBounds();
    os << indent << "Bounds: (" << this->Bounds[0] << ", "
       << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
       << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
       << this->Bounds[5] << ")\n";
    }
  else
    {
    os << indent << "Bounds: (not defined)\n";
    }
}
