/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolume.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkVolumeCollection.h"
#include "vtkVolumeProperty.h"

#include <math.h>

vtkStandardNewMacro(vtkVolume);

// Creates a Volume with the following defaults: origin(0,0,0)
// position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
vtkVolume::vtkVolume()
{
  this->Mapper                      = NULL;
  this->Property                    = NULL;

  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
    {
    this->ScalarOpacityArray[i]          = NULL;
    this->RGBArray[i]                    = NULL;
    this->GrayArray[i]                   = NULL;
    this->CorrectedScalarOpacityArray[i] = NULL;
    this->GradientOpacityConstant[i]     = 0;
    }

  this->CorrectedStepSize           = -1;
  this->ArraySize                   =  0;
}

// Destruct a volume
vtkVolume::~vtkVolume()
{
  if (this->Property )
    {
    this->Property->UnRegister(this);
    }

  this->SetMapper(NULL);

  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
    {
    delete [] this->ScalarOpacityArray[i];
    delete [] this->RGBArray[i];
    delete [] this->GrayArray[i];
    delete [] this->CorrectedScalarOpacityArray[i];
    }
}

void vtkVolume::GetVolumes(vtkPropCollection *vc)
{
  vc->AddItem(this);
}

// Shallow copy of an volume.
void vtkVolume::ShallowCopy(vtkProp *prop)
{
  vtkVolume *v = vtkVolume::SafeDownCast(prop);

  if ( v != NULL )
    {
    this->SetMapper(v->GetMapper());
    this->SetProperty(v->GetProperty());
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

float *vtkVolume::GetScalarOpacityArray(int index)
{
  if ( index < 0 || index >= VTK_MAX_VRCOMP )
    {
    vtkErrorMacro("Index out of range [0-" << VTK_MAX_VRCOMP <<
                  "]: " << index );
    return NULL;
    }
  return this->ScalarOpacityArray[index];
}

float *vtkVolume::GetCorrectedScalarOpacityArray(int index)
{
  if ( index < 0 || index >= VTK_MAX_VRCOMP )
    {
    vtkErrorMacro("Index out of range [0-" << VTK_MAX_VRCOMP <<
                  "]: " << index );
    return NULL;
    }
  return this->CorrectedScalarOpacityArray[index];
}

float *vtkVolume::GetGradientOpacityArray(int index)
{
  if ( index < 0 || index >= VTK_MAX_VRCOMP )
    {
    vtkErrorMacro("Index out of range [0-" << VTK_MAX_VRCOMP <<
                  "]: " << index );
    return NULL;
    }
  return this->GradientOpacityArray[index];
}

float vtkVolume::GetGradientOpacityConstant(int index)
{
  if ( index < 0 || index >= VTK_MAX_VRCOMP )
    {
    vtkErrorMacro("Index out of range [0-" << VTK_MAX_VRCOMP <<
                  "]: " << index );
    return 0;
    }
  return this->GradientOpacityConstant[index];
}

float *vtkVolume::GetGrayArray(int index)
{
  if ( index < 0 || index >= VTK_MAX_VRCOMP )
    {
    vtkErrorMacro("Index out of range [0-" << VTK_MAX_VRCOMP <<
                  "]: " << index );
    return NULL;
    }
  return this->GrayArray[index];
}

float *vtkVolume::GetRGBArray(int index)
{
  if ( index < 0 || index >= VTK_MAX_VRCOMP )
    {
    vtkErrorMacro("Index out of range [0-" << VTK_MAX_VRCOMP <<
                  "]: " << index );
    return NULL;
    }
  return this->RGBArray[index];
}

void vtkVolume::SetMapper(vtkAbstractVolumeMapper *mapper)
{
  if (this->Mapper != mapper)
    {
    if (this->Mapper != NULL)
      {
      this->Mapper->UnRegister(this);
      }
    this->Mapper = mapper;
    if (this->Mapper != NULL)
      {
      this->Mapper->Register(this);
      }
    this->Modified();
    }
}

double vtkVolume::ComputeScreenCoverage( vtkViewport *vp )
{
  double coverage = 1.0;

  vtkRenderer *ren = vtkRenderer::SafeDownCast( vp );

  if ( ren )
    {
    vtkCamera *cam = ren->GetActiveCamera();
    ren->ComputeAspect();
    double *aspect = ren->GetAspect();
    vtkMatrix4x4 *mat = cam->GetCompositeProjectionTransformMatrix(
      aspect[0]/aspect[1], 0.0, 1.0 );
    double *bounds = this->GetBounds();
    double minX =  1.0;
    double maxX = -1.0;
    double minY =  1.0;
    double maxY = -1.0;
    int i, j, k;
    double p[4];
    for ( k = 0; k < 2; k++ )
      {
      for ( j = 0; j < 2; j++ )
        {
        for ( i = 0; i < 2; i++ )
          {
          p[0] = bounds[i];
          p[1] = bounds[2+j];
          p[2] = bounds[4+k];
          p[3] = 1.0;
          mat->MultiplyPoint( p, p );
          if ( p[3] )
            {
            p[0] /= p[3];
            p[1] /= p[3];
            p[2] /= p[3];
            }

          minX = (p[0] < minX)?(p[0]):(minX);
          minY = (p[1] < minY)?(p[1]):(minY);
          maxX = (p[0] > maxX)?(p[0]):(maxX);
          maxY = (p[1] > maxY)?(p[1]):(maxY);
          }
        }
      }

    coverage = (maxX-minX)*(maxY-minY)*.25;
    coverage = (coverage > 1.0 )?(1.0):(coverage);
    coverage = (coverage < 0.0 )?(0.0):(coverage);
    }

  return coverage;
}

// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkVolume::GetBounds()
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

// Get the minimum X bound
double vtkVolume::GetMinXBound( )
{
  this->GetBounds();
  return this->Bounds[0];
}

// Get the maximum X bound
double vtkVolume::GetMaxXBound( )
{
  this->GetBounds();
  return this->Bounds[1];
}

// Get the minimum Y bound
double vtkVolume::GetMinYBound( )
{
  this->GetBounds();
  return this->Bounds[2];
}

// Get the maximum Y bound
double vtkVolume::GetMaxYBound( )
{
  this->GetBounds();
  return this->Bounds[3];
}

// Get the minimum Z bound
double vtkVolume::GetMinZBound( )
{
  this->GetBounds();
  return this->Bounds[4];
}

// Get the maximum Z bound
double vtkVolume::GetMaxZBound( )
{
  this->GetBounds();
  return this->Bounds[5];
}

// If the volume mapper is of type VTK_FRAMEBUFFER_VOLUME_MAPPER, then
// this is its opportunity to render
int vtkVolume::RenderVolumetricGeometry( vtkViewport *vp )
{
  this->Update();

  if ( !this->Mapper )
    {
    vtkErrorMacro( << "You must specify a mapper!\n" );
    return 0;
    }

  // If we don't have any input return silently
  if ( !this->Mapper->GetDataObjectInput() )
    {
    return 0;
    }

  // Force the creation of a property
  if( !this->Property )
    {
    this->GetProperty();
    }

  if( !this->Property )
    {
    vtkErrorMacro( << "Error generating a property!\n" );
    return 0;
    }

  this->Mapper->Render( static_cast<vtkRenderer *>(vp), this );
  this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();

  return 1;
}

void vtkVolume::ReleaseGraphicsResources(vtkWindow *win)
{
  // pass this information onto the mapper
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(win);
    }
}

void vtkVolume::Update()
{
  if ( this->Mapper )
    {
    this->Mapper->Update();
    }
}

void vtkVolume::SetProperty(vtkVolumeProperty *property)
{
  if( this->Property != property )
    {
    if (this->Property != NULL) {this->Property->UnRegister(this);}
    this->Property = property;
    if (this->Property != NULL)
      {
      this->Property->Register(this);
      this->Property->UpdateMTimes();
      }
    this->Modified();
    }
}

vtkVolumeProperty *vtkVolume::GetProperty()
{
  if( this->Property == NULL )
    {
    this->Property = vtkVolumeProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
    }
  return this->Property;
}

unsigned long int vtkVolume::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
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

unsigned long int vtkVolume::GetRedrawMTime()
{
  unsigned long mTime=this->GetMTime();
  unsigned long time;

  if ( this->Mapper != NULL )
    {
    time = this->Mapper->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->GetMapper()->GetDataSetInput() != NULL)
      {
      this->GetMapper()->GetInputAlgorithm()->Update();
      time = this->Mapper->GetDataSetInput()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );

    int numComponents;

    if ( this->Mapper && this->Mapper->GetDataSetInput() &&
         this->Mapper->GetDataSetInput()->GetPointData() &&
         this->Mapper->GetDataSetInput()->GetPointData()->GetScalars() )
      {
      numComponents = this->Mapper->GetDataSetInput()->GetPointData()->
        GetScalars()->GetNumberOfComponents();
      }
    else
      {
      numComponents = 0;
      }

    for ( int i = 0; i < numComponents; i++ )
      {
      // Check the color transfer function (gray or rgb)
      if ( this->Property->GetColorChannels(i) == 1 )
        {
        time = this->Property->GetGrayTransferFunction(i)->GetMTime();
        mTime = ( time > mTime ? time : mTime );
        }
      else
        {
        time = this->Property->GetRGBTransferFunction(i)->GetMTime();
        mTime = ( time > mTime ? time : mTime );
        }

      // check the scalar opacity function
      time = this->Property->GetScalarOpacity(i)->GetMTime();
      mTime = ( time > mTime ? time : mTime );

      // check the gradient opacity function
      time = this->Property->GetGradientOpacity(i)->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  return mTime;
}

void vtkVolume::UpdateTransferFunctions( vtkRenderer *vtkNotUsed(ren) )
{
  int                        dataType;
  vtkPiecewiseFunction      *sotf;
  vtkPiecewiseFunction      *gotf;
  vtkPiecewiseFunction      *graytf;
  vtkColorTransferFunction  *rgbtf;
  int                        colorChannels;

  int                        arraySize;

  // Check that we have scalars
  if ( this->Mapper == NULL ||
       this->Mapper->GetDataSetInput() == NULL ||
       this->Mapper->GetDataSetInput()->GetPointData() == NULL ||
       this->Mapper->GetDataSetInput()->GetPointData()->GetScalars() == NULL )
    {
    vtkErrorMacro(<<"Need scalar data to volume render");
    return;
    }

  // What is the type of the data?
  dataType = this->Mapper->GetDataSetInput()->
    GetPointData()->GetScalars()->GetDataType();

  if (dataType == VTK_UNSIGNED_CHAR)
    {
    arraySize = 256;
    }
  else if (dataType == VTK_UNSIGNED_SHORT)
    {
    arraySize = 65536;
    }
  else
    {
    vtkErrorMacro("Unsupported data type");
    return;
    }

  int numComponents = this->Mapper->GetDataSetInput()->GetPointData()->
    GetScalars()->GetNumberOfComponents();

  for ( int c = 0; c < numComponents; c++ )
    {

    // Did our array size change? If so, free up all our previous arrays
    // and create new ones for the scalar opacity and corrected scalar
    // opacity
    if ( arraySize != this->ArraySize )
      {
      delete [] this->ScalarOpacityArray[c];
      this->ScalarOpacityArray[c] = NULL;

      delete [] this->CorrectedScalarOpacityArray[c];
      this->CorrectedScalarOpacityArray[c] = NULL;

      delete [] this->GrayArray[c];
      this->GrayArray[c] = NULL;

      delete [] this->RGBArray[c];
      this->RGBArray[c] = NULL;

      // Allocate these two because we know we need them
      this->ScalarOpacityArray[c] = new float[arraySize];
      this->CorrectedScalarOpacityArray[c] = new float[arraySize];
    }

    // How many color channels for this component?
    colorChannels = this->Property->GetColorChannels(c);

    // If we have 1 color channel and no gray array, create it.
    // Free the rgb array if there is one.
    if ( colorChannels == 1 )
      {
      delete [] this->RGBArray[c];
      this->RGBArray[c] = NULL;

      if ( !this->GrayArray[c] )
        {
        this->GrayArray[c] = new float[arraySize];
        }
      }

    // If we have 3 color channels and no rgb array, create it.
    // Free the gray array if there is one.
    if ( colorChannels == 3 )
      {
      delete [] this->GrayArray[c];
      this->GrayArray[c] = NULL;

      if ( !this->RGBArray[c] )
        {
        this->RGBArray[c] = new float[3*arraySize];
        }
      }

    // Get the various functions for this index. There is no chance of
    // these being NULL since the property will create them if they were
    // not defined
    sotf          = this->Property->GetScalarOpacity(c);
    gotf          = this->Property->GetGradientOpacity(c);

    if ( colorChannels == 1 )
      {
      rgbtf         = NULL;
      graytf        = this->Property->GetGrayTransferFunction(c);
      }
    else
      {
      rgbtf         = this->Property->GetRGBTransferFunction(c);
      graytf        = NULL;
      }


    // Update the scalar opacity array if necessary
    if ( sotf->GetMTime() >
         this->ScalarOpacityArrayMTime[c] ||
         this->Property->GetScalarOpacityMTime(c) >
         this->ScalarOpacityArrayMTime[c] )
      {
      sotf->GetTable( 0.0, static_cast<double>(arraySize-1),
                      arraySize, this->ScalarOpacityArray[c] );
      this->ScalarOpacityArrayMTime[c].Modified();
      }

    // Update the gradient opacity array if necessary
    if ( gotf->GetMTime() >
         this->GradientOpacityArrayMTime[c] ||
         this->Property->GetGradientOpacityMTime(c) >
         this->GradientOpacityArrayMTime[c] )
      {
      // Get values according to scale/bias from mapper 256 values are
      // in the table, the scale / bias values control what those 256 values
      // mean.
      float scale = this->Mapper->GetGradientMagnitudeScale(c);
      float bias  = this->Mapper->GetGradientMagnitudeBias(c);

      float low   = -bias;
      float high  = 255 / scale - bias;

      gotf->GetTable(low, high, static_cast<int>(0x100),
                     this->GradientOpacityArray[c] );

      if ( !strcmp(gotf->GetType(), "Constant") )
        {
        this->GradientOpacityConstant[c] = this->GradientOpacityArray[c][0];
        }
      else
        {
        this->GradientOpacityConstant[c] = -1.0;
        }

      this->GradientOpacityArrayMTime[c].Modified();
      }

    // Update the RGB or Gray transfer function if necessary
    if ( colorChannels == 1 )
      {
      if ( graytf->GetMTime() >
           this->GrayArrayMTime[c] ||
           this->Property->GetGrayTransferFunctionMTime(c) >
           this->GrayArrayMTime[c] )
        {
        graytf->GetTable( 0.0, static_cast<float>(arraySize-1),
                          arraySize, this->GrayArray[c] );
        this->GrayArrayMTime[c].Modified();
        }
      }
    else
      {
      if ( rgbtf->GetMTime() >
           this->RGBArrayMTime[c] ||
           this->Property->GetRGBTransferFunctionMTime(c) >
           this->RGBArrayMTime[c] )
        {
        rgbtf->GetTable( 0.0, static_cast<float>(arraySize-1),
                         arraySize, this->RGBArray[c] );
        this->RGBArrayMTime[c].Modified();
        }
      }
    }

  // reset the array size to the current size
  this->ArraySize = arraySize;
}

// This method computes the corrected alpha blending for a given
// step size.  The ScalarOpacityArray reflects step size 1.
// The CorrectedScalarOpacityArray reflects step size CorrectedStepSize.
void vtkVolume::UpdateScalarOpacityforSampleSize( vtkRenderer *vtkNotUsed(ren),
                                                  float sample_distance )
{
  int i;
  int needsRecomputing;
  float originalAlpha,correctedAlpha;
  float ray_scale;

  ray_scale = sample_distance;

  // step size changed
  needsRecomputing =
      this->CorrectedStepSize-ray_scale >  0.0001;

  needsRecomputing = needsRecomputing ||
      this->CorrectedStepSize-ray_scale < -0.0001;

  // Check that we have scalars
  if ( this->Mapper == NULL ||
       this->Mapper->GetDataSetInput() == NULL ||
       this->Mapper->GetDataSetInput()->GetPointData() == NULL ||
       this->Mapper->GetDataSetInput()->GetPointData()->GetScalars() == NULL )
    {
    vtkErrorMacro(<<"Need scalar data to volume render");
    return;
    }

  int numComponents = this->Mapper->GetDataSetInput()->GetPointData()->
    GetScalars()->GetNumberOfComponents();

  if ( needsRecomputing )
    {
    this->CorrectedStepSize = ray_scale;
    }

  for ( int c = 0; c < numComponents; c++ )
    {
    if (needsRecomputing ||
        this->ScalarOpacityArrayMTime[c] >
        this->CorrectedScalarOpacityArrayMTime[c])
      {
      this->CorrectedScalarOpacityArrayMTime[c].Modified();

      for (i = 0; i < this->ArraySize; i++)
        {
        originalAlpha = *(this->ScalarOpacityArray[c]+i);

        // this test is to accelerate the Transfer function correction
        if (originalAlpha > 0.0001)
          {
          correctedAlpha =
            1.0f-static_cast<float>(
              pow(static_cast<double>(1.0f-originalAlpha),
                  static_cast<double>(this->CorrectedStepSize)));
          }
        else
          {
          correctedAlpha = originalAlpha;
          }
        *(this->CorrectedScalarOpacityArray[c]+i) = correctedAlpha;
        }
      }
    }
}


void vtkVolume::PrintSelf(ostream& os, vtkIndent indent)
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

