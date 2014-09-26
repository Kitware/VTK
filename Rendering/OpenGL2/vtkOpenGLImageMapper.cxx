/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLImageMapper.h"

#include "vtk_glew.h"

#include "vtkActor2D.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkViewport.h"
#
#include "vtkWindow.h"

#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkTrivialProducer.h"
#include "vtkTexturedActor2D.h"
#include "vtkTexture.h"

#include "vtkNew.h"

#include "vtkOpenGLError.h"

vtkStandardNewMacro(vtkOpenGLImageMapper);

vtkOpenGLImageMapper::vtkOpenGLImageMapper()
{
  this->Actor = vtkTexturedActor2D::New();
  vtkNew<vtkPolyDataMapper2D> mapper;
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  polydata->SetPoints(points.Get());

  vtkNew<vtkCellArray> tris;
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(2);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(3);
  polydata->SetPolys(tris.Get());

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(polydata.Get());

  // Set some properties.
  mapper->SetInputConnection(prod->GetOutputPort());
  this->Actor->SetMapper(mapper.Get());

  vtkNew<vtkTexture> texture;
  texture->RepeatOff();
  this->Actor->SetTexture(texture.Get());

  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(4);
  polydata->GetPointData()->SetTCoords(tcoords.Get());
}

vtkOpenGLImageMapper::~vtkOpenGLImageMapper()
{
  this->Actor->UnRegister(this);
}


//----------------------------------------------------------------------------
// I know #define can be evil, but this macro absolutely ensures
// that the code will be inlined.  The macro expects 'val' to
// be predefined to the same type as y

#define vtkClampToUnsignedChar(x,y) \
{ \
  val = (y); \
  if (val < 0) \
    { \
    val = 0; \
    } \
  if (val > 255) \
    { \
    val = 255; \
    } \
  (x) = static_cast<unsigned char>(val); \
}
/* should do proper rounding, as follows:
  (x) = (unsigned char)(val + 0.5f); \
*/

// the bit-shift must be done after the comparison to zero
// because bit-shift is implemenation dependent for negative numbers
#define vtkClampIntToUnsignedChar(x,y,shift) \
{ \
  val = (y); \
  if (val < 0) \
    { \
    val = 0; \
    } \
  val >>= shift; \
  if (val > 255) \
    { \
    val = 255; \
    } \
  (x) = static_cast<unsigned char>(val); \
}

// pad an integer to a multiply of four, for OpenGL
inline int vtkPadToFour(int n)
{
  return ((n+3)/4)*4;
}

//---------------------------------------------------------------
// render the image by doing the following:
// 1) apply shift and scale to pixel values
// 2) clamp to [0,255] and convert to unsigned char
// 3) draw using DrawPixels

template <class T>
void vtkOpenGLImageMapperRenderDouble(vtkOpenGLImageMapper *self, vtkImageData *data,
                                      T *dataPtr, double shift, double scale,
                                      vtkViewport *viewport)
{
  vtkOpenGLClearErrorMacro();

  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  vtkIdType* tempIncs = data->GetIncrements();
  vtkIdType inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();
  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

#ifdef GL_UNPACK_ALIGNMENT
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
#else
  assert("width must be a multiple of 4" && width ==4);
#endif

  // reformat data into unsigned char

  T *inPtr = dataPtr;
  T *inPtr1 = inPtr;

  int i;
  int j = height;

  unsigned char *newPtr;
  if (bpp < 4)
    {
    newPtr = new unsigned char[vtkPadToFour(3*width*height)];
    }
  else
    {
    newPtr = new unsigned char[4*width*height];
    }

  unsigned char *ptr = newPtr;
  double val;
  unsigned char tmp;

  while (--j >= 0)
    {
    inPtr = inPtr1;
    i = width;
    switch (bpp)
      {
      case 1:
        while (--i >= 0)
          {
          vtkClampToUnsignedChar(tmp,((*inPtr++ + shift)*scale));
          *ptr++ = tmp;
          *ptr++ = tmp;
          *ptr++ = tmp;
          }
        break;

      case 2:
        while (--i >= 0)
          {
          vtkClampToUnsignedChar(tmp,((*inPtr++ + shift)*scale));
          *ptr++ = tmp;
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          *ptr++ = tmp;
          }
        break;

      case 3:
        while (--i >= 0)
          {
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          }
        break;

      default:
        while (--i >= 0)
          {
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          inPtr += bpp-4;
          }
        break;
      }
    inPtr1 += inInc1;
    }

  self->DrawPixels(viewport, width, height, ((bpp < 4) ? 3 : 4),
                   static_cast<void *>(newPtr));

  delete [] newPtr;

 vtkOpenGLStaticCheckErrorMacro("failed after ImageMapperRenderDouble");
}

//---------------------------------------------------------------
// Same as above, but uses fixed-point math for shift and scale.
// The number of bits used for the fraction is determined from the
// scale.  Enough bits are always left over for the integer that
// overflow cannot occur.

template <class T>
void vtkOpenGLImageMapperRenderShort(vtkOpenGLImageMapper *self, vtkImageData *data,
                                     T *dataPtr, double shift, double scale,
                                     vtkViewport *viewport)
{
  vtkOpenGLClearErrorMacro();

  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  vtkIdType* tempIncs = data->GetIncrements();
  vtkIdType inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

#ifdef GL_UNPACK_ALIGNMENT
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
#else
  assert("width must be a multiple of 4" && width ==4);
#endif

  // Find the number of bits to use for the fraction:
  // continue increasing the bits until there is an overflow
  // in the worst case, then decrease by 1.
  // The "*2.0" and "*1.0" ensure that the comparison is done
  // with double-precision math.
  int bitShift = 0;
  double absScale = ((scale < 0) ? -scale : scale);

  while ((static_cast<long>(1 << bitShift)*absScale)*2.0*USHRT_MAX < INT_MAX*1.0)
    {
    bitShift++;
    }
  bitShift--;

  long sscale = static_cast<long>(scale*(1 << bitShift));
  long sshift = static_cast<long>(sscale*shift);
  /* should do proper rounding, as follows:
  long sscale = (long) floor(scale*(1 << bitShift) + 0.5);
  long sshift = (long) floor((scale*shift + 0.5)*(1 << bitShift));
  */
  long val;
  unsigned char tmp;

  T *inPtr = dataPtr;
  T *inPtr1 = inPtr;

  int i;
  int j = height;

  unsigned char *newPtr;
  if (bpp < 4)
    {
    newPtr = new unsigned char[vtkPadToFour(3*width*height)];
    }
  else
    {
    newPtr = new unsigned char[4*width*height];
    }

  unsigned char *ptr = newPtr;

  while (--j >= 0)
    {
    inPtr = inPtr1;
    i = width;

    switch (bpp)
      {
      case 1:
        while (--i >= 0)
          {
          vtkClampIntToUnsignedChar(tmp,(*inPtr++*sscale+sshift),bitShift);
          *ptr++ = tmp;
          *ptr++ = tmp;
          *ptr++ = tmp;
          }
        break;

      case 2:
        while (--i >= 0)
          {
          vtkClampIntToUnsignedChar(tmp,(*inPtr++*sscale+sshift),bitShift);
          *ptr++ = tmp;
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          *ptr++ = tmp;
          }
        break;

      case 3:
        while (--i >= 0)
          {
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          }
        break;

      default:
        while (--i >= 0)
          {
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          inPtr += bpp-4;
          }
        break;
      }
    inPtr1 += inInc1;
    }

  self->DrawPixels(viewport, width, height, ((bpp < 4) ? 3 : 4),
                   static_cast<void *>(newPtr));

  delete [] newPtr;

  vtkOpenGLStaticCheckErrorMacro("failed after ImageMapperRenderShort");
}

//---------------------------------------------------------------
// render unsigned char data without any shift/scale

template <class T>
void vtkOpenGLImageMapperRenderChar(vtkOpenGLImageMapper *self, vtkImageData *data,
                                    T *dataPtr, vtkViewport *viewport)
{
  vtkOpenGLClearErrorMacro();

  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  vtkIdType* tempIncs = data->GetIncrements();
  vtkIdType inInc1 = tempIncs[1];

  int bpp = data->GetPointData()->GetScalars()->GetNumberOfComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

#ifdef GL_UNPACK_ALIGNMENT
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
#else
  assert("width must be a multiple of 4" && width ==4);
#endif

  //
#ifdef GL_UNPACK_ROW_LENGTH
  if (bpp == 3)
    { // feed through RGB bytes without reformatting
    if (inInc1 != width*bpp)
      {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
      }
    self->DrawPixels(viewport, width, height, 3,
                 static_cast<void *>(dataPtr));
    }
  else if (bpp == 4)
    { // feed through RGBA bytes without reformatting
    if (inInc1 != width*bpp)
      {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
      }
    self->DrawPixels(viewport, width, height, 4,
                     static_cast<void *>(dataPtr));
    }
  else
#endif
    { // feed through other bytes without reformatting
    T *inPtr = dataPtr;
    T *inPtr1 = inPtr;
    unsigned char tmp;

    int i;
    int j = height;

    unsigned char *newPtr;
    if (bpp < 4)
      {
      newPtr = new unsigned char[vtkPadToFour(3*width*height)];
      }
    else
      {
      newPtr = new unsigned char[4*width*height];
      }

    unsigned char *ptr = newPtr;

    while (--j >= 0)
      {
      inPtr = inPtr1;
      i = width;

      switch (bpp)
        {
        case 1:
          while (--i >= 0)
            {
            *ptr++ = tmp = *inPtr++;
            *ptr++ = tmp;
            *ptr++ = tmp;
            }
          break;

        case 2:
          while (--i >= 0)
            {
            *ptr++ = tmp = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = tmp;
            }
          break;

        case 3:
          while (--i >= 0)
            {
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            }
          break;

        default:
          while (--i >= 0)
            {
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            inPtr += bpp-4;
            }
          break;
        }
      inPtr1 += inInc1;
      }

    self->DrawPixels(viewport, width, height, ((bpp < 4) ? 3 : 4), static_cast<void *>(newPtr));

    delete [] newPtr;
    }

#ifdef GL_UNPACK_ROW_LENGTH
  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
#endif

  vtkOpenGLStaticCheckErrorMacro("failed after ImageMapperRenderChar");
}

//----------------------------------------------------------------------------
// Define overloads to help the template macro below dispatch to a
// suitable implementation for each type.  The last argument is of
// type "long" for the template and of type "int" for the
// non-templates.  The template macro's call to this function always
// passes a literal "1" as the last argument, which requires a
// conversion to produce a long.  This helps broken compilers select
// the non-template even when the template is otherwise an equal
// match.
template <class T>
void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                T *dataPtr, double shift, double scale,
                                vtkViewport *viewport)
{
  vtkOpenGLImageMapperRenderDouble(self, data, dataPtr, shift, scale, viewport);
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       char* dataPtr, double shift, double scale,
                                       vtkViewport *viewport)
{
  if(shift == 0.0 && scale == 1.0)
    {
    vtkOpenGLImageMapperRenderChar(self, data, dataPtr, viewport);
    }
  else
    {
    vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale, viewport);
    }
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       unsigned char* dataPtr, double shift, double scale,
                                       vtkViewport *viewport)
{
  if(shift == 0.0 && scale == 1.0)
    {
    vtkOpenGLImageMapperRenderChar(self, data, dataPtr, viewport);
    }
  else
    {
    vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale, viewport);
    }
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       signed char* dataPtr, double shift, double scale,
                                       vtkViewport *viewport)
{
  if(shift == 0.0 && scale == 1.0)
    {
    vtkOpenGLImageMapperRenderChar(self, data, dataPtr, viewport);
    }
  else
    {
    vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale, viewport);
    }
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       short* dataPtr, double shift, double scale,
                                       vtkViewport *viewport)
{
  vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale, viewport);
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       unsigned short* dataPtr, double shift, double scale,
                                       vtkViewport *viewport)
{
  vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale, viewport);
}

//----------------------------------------------------------------------------
// Expects data to be X, Y, components

void vtkOpenGLImageMapper::RenderData(vtkViewport* viewport,
                                      vtkImageData *data, vtkActor2D *actor)
{
  void *ptr0;
  double shift, scale;

  vtkWindow* window = static_cast<vtkWindow *>(viewport->GetVTKWindow());
  if (!window)
    {
    vtkErrorMacro (<<"vtkOpenGLImageMapper::RenderData - no window set for viewport");
    return;
    }

  // Make this window current. May have become not current due to
  // data updates since the render started.
  window->MakeCurrent();

  vtkOpenGLClearErrorMacro();

  shift = this->GetColorShift();
  scale = this->GetColorScale();

  ptr0 = data->GetScalarPointer(this->DisplayExtent[0],
                                this->DisplayExtent[2],
                                this->DisplayExtent[4]);

  // Get the position of the image actor
  int* actorPos =
    actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0];
  actorPos[1] += this->PositionAdjustment[1];

  this->Actor->SetPosition(actorPos[0], actorPos[1]);
  this->Actor->SetPosition2(actor->GetPosition2());

  switch (data->GetPointData()->GetScalars()->GetDataType())
    {
    vtkTemplateMacro(
      vtkOpenGLImageMapperRender(this, data, static_cast<VTK_TT*>(ptr0),
                                 shift, scale, viewport)
      );
    default:
      vtkErrorMacro ( << "Unsupported image type: " << data->GetScalarType());
    }

  vtkOpenGLCheckErrorMacro("failed after RenderData");
}



void vtkOpenGLImageMapper::DrawPixels(vtkViewport *viewport, int width, int height, int numComponents, void *data)
{
  int* actorPos =
    this->Actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);
  int* actorPos2 =
    this->Actor->GetActualPosition2Coordinate()->GetComputedViewportValue(viewport);

  float xscale = 1.0;
  float yscale = 1.0;
  if (this->GetRenderToRectangle())
    {
    int rectwidth  = (actorPos2[0] - actorPos[0]) + 1;
    int rectheight = (actorPos2[1] - actorPos[1]) + 1;
    xscale = static_cast<float>(rectwidth)/width;
    yscale = static_cast<float>(rectheight)/height;
    }

  vtkPolyData *pd = vtkPolyDataMapper2D::SafeDownCast(this->Actor->GetMapper())->GetInput();
  vtkPoints *points = pd->GetPoints();
  points->SetPoint(0, 0.0, 0.0, 0);
  points->SetPoint(1, width*xscale, 0.0, 0);
  points->SetPoint(2, width*xscale, height*yscale, 0);
  points->SetPoint(3, 0.0, height*yscale, 0);

  vtkDataArray *tcoords = pd->GetPointData()->GetTCoords();
  float tmp[2];
  tmp[0] = 0;
  tmp[1] = 0;
  tcoords->SetTuple(0,tmp);
  tmp[0] = 1.0;
  tcoords->SetTuple(1,tmp);
  tmp[1] = 1.0;
  tcoords->SetTuple(2,tmp);
  tmp[0] = 0.0;
  tcoords->SetTuple(3,tmp);

  vtkImageData *id = vtkImageData::New();
  id->SetExtent(0,width-1, 0,height-1, 0,0);
  vtkUnsignedCharArray *uca = vtkUnsignedCharArray::New();
  uca->SetNumberOfComponents(numComponents);
  uca->SetArray((unsigned char *)data,width*height*numComponents,true);
  id->GetPointData()->SetScalars(uca);
  uca->Delete();

  this->Actor->GetTexture()->SetInputData(id);

  glDisable(GL_DEPTH_TEST);
  this->Actor->RenderOverlay(viewport);
  glEnable(GL_DEPTH_TEST);
  id->Delete();
}


void vtkOpenGLImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
