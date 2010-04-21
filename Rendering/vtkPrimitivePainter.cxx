/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrimitivePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPrimitivePainter.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericVertexAttributeMapping.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShaderDeviceAdapter.h"
#include "vtkShaderProgram.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkGLSLShaderDeviceAdapter2.h"
#include "vtkOpenGLProperty.h"

//---------------------------------------------------------------------------
vtkPrimitivePainter::vtkPrimitivePainter()
{
  this->SupportedPrimitive = 0x0; // must be set by subclasses. No primitive
                                  // supported by default.
  this->DisableScalarColor = 0;
  this->OutputData = vtkPolyData::New();
  this->GenericVertexAttributes = false;
  this->MultiTextureAttributes = false;
}

//---------------------------------------------------------------------------
vtkPrimitivePainter::~vtkPrimitivePainter()
{
  if( this->OutputData )
    {
    this->OutputData->Delete();
    this->OutputData = 0;
    }
}

//---------------------------------------------------------------------------
void vtkPrimitivePainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->OutputData, "Output Data");
}

//---------------------------------------------------------------------------
vtkDataObject* vtkPrimitivePainter::GetOutput()
{
  return this->OutputData;
}

//---------------------------------------------------------------------------
void vtkPrimitivePainter::ProcessInformation(vtkInformation *info)
{
  this->GenericVertexAttributes = false;
  if (info->Has(DATA_ARRAY_TO_VERTEX_ATTRIBUTE()))
    {
    vtkGenericVertexAttributeMapping *mappings = 
      vtkGenericVertexAttributeMapping::SafeDownCast(
      info->Get(DATA_ARRAY_TO_VERTEX_ATTRIBUTE()));
    this->GenericVertexAttributes = mappings && 
      (mappings->GetNumberOfMappings() > 0);
    this->MultiTextureAttributes = false;
    if (mappings)
      {
      for (unsigned int i = 0; i < mappings->GetNumberOfMappings(); ++i)
        {
        if (mappings->GetTextureUnit(i) >= 0)
          {
          this->MultiTextureAttributes = true;
          break;
          }
        }
      }
    }

  if (info->Has(DISABLE_SCALAR_COLOR()) &&
    info->Get(DISABLE_SCALAR_COLOR()) == 1)
    {
    this->DisableScalarColor = 1;
    }
  else
    {
    this->DisableScalarColor = 0;
    }
}

//---------------------------------------------------------------------------
void vtkPrimitivePainter::PrepareForRendering(vtkRenderer* renderer,
  vtkActor* actor)
{
  // Here, we don't use the this->StaticData flag to mean that the input
  // can never change, since the input may be the output of
  // some filtering painter that filter on actor/renderer properties
  // and not on the input polydata. Hence the input polydata
  // may get modified even if the input to the PolyDataMapper is
  // immutable.

  // If the input has changed update the output.
  if (this->OutputUpdateTime < this->MTime ||
    this->OutputUpdateTime < this->GetInput()->GetMTime())
    {
    this->OutputData->ShallowCopy(this->GetInputAsPolyData());
    this->OutputUpdateTime.Modified();
    }

  this->Superclass::PrepareForRendering(renderer, actor);
}

//---------------------------------------------------------------------------
void vtkPrimitivePainter::RenderInternal(vtkRenderer* renderer,
                                         vtkActor* act,
                                         unsigned long typeflags,
                                         bool forceCompileOnly)
{
  unsigned long supported_typeflags = this->SupportedPrimitive & typeflags;
  if (!supported_typeflags)
    {
    // no supported primitive requested to be rendered.
      this->Superclass::RenderInternal(renderer, act, typeflags,
                                       forceCompileOnly);
    return;
    }

  if (!renderer->GetRenderWindow()->GetPainterDeviceAdapter())
    {
    vtkErrorMacro("Painter Device Adapter is missing!");
    return;
    }

  this->Timer->StartTimer();

  int interpolation;
  float tran;
  vtkProperty *prop;
  vtkUnsignedCharArray *c=NULL;
  vtkDataArray *n;
  vtkDataArray *t;
  vtkDataArray *ef;
  int tDim;
  vtkPolyData *input = this->GetInputAsPolyData();
  int cellNormals;
  int cellScalars = 0;
  int fieldScalars = 0;

  // get the property
  prop = act->GetProperty();

  // get the transparency
  tran = prop->GetOpacity();

  // if the primitives are invisible then get out of here
  if (tran <= 0.0)
    {
    return;
    }

  // get the shading interpolation
  interpolation = prop->GetInterpolation();

  if (!this->DisableScalarColor)
    {
    // are they cell or point scalars
    c = vtkUnsignedCharArray::SafeDownCast(input->GetPointData()->GetScalars());
    if (!c)
      {
      c = vtkUnsignedCharArray::SafeDownCast(input->GetCellData()->GetScalars());
      cellScalars = 1;
      }

    if (!c)
      {
      c = vtkUnsignedCharArray::SafeDownCast(input->GetFieldData()->
        GetArray("Color"));
      fieldScalars = 1; // note when fieldScalars == 1, also cellScalars == 1.
      // this ensures that primitive painters that do not distinguish between
      // fieldScalars and cellScalars (eg. Verts/Lines/Polys painters) can ignore 
      // fieldScalars flag.
      }
    }

  n = input->GetPointData()->GetNormals();
  if (interpolation == VTK_FLAT)
    {
    // shunt point normals.
    n = 0;
    if (this->OutputData->GetPointData()->GetNormals())
      {
      this->OutputData->GetPointData()->SetNormals(0);
      }
    }

  cellNormals = 0;
  if (n == 0 && input->GetCellData()->GetNormals())
    {
    cellNormals = 1;
    n = input->GetCellData()->GetNormals();
    }

  unsigned long idx = 0;
  if (n && !cellNormals)
    {
    idx |= VTK_PDM_NORMALS;
    }
  if (c)
    {
    idx |= VTK_PDM_COLORS;
    if (!fieldScalars && c->GetName())
      {
      // In the future, I will look at the number of components.
      // All paths will have to handle 3 component colors.
      // When using field colors, the c->GetName() condition is not valid,
      // since field data arrays always have names. In that case we
      // forfeit the speed improvement gained by using RGB colors instead
      // or RGBA.
      idx |= VTK_PDM_OPAQUE_COLORS;
      }
    if (cellScalars)
      {
      idx |= VTK_PDM_CELL_COLORS;
      }
    if (fieldScalars)
      {
      idx |= VTK_PDM_FIELD_COLORS;
      }
    }
  if (cellNormals)
    {
    idx |= VTK_PDM_CELL_NORMALS;
    }

  // Texture and color by texture
  t = input->GetPointData()->GetTCoords();
  if ( t )
    {
    tDim = t->GetNumberOfComponents();
    if (tDim > 3)
      {
      vtkDebugMacro(<< "Currently only 1d, 2d and 3d texture coordinates are supported.\n");
      t = NULL;
      }
    }

  // Set the flags
  if (t)
    {
    idx |= VTK_PDM_TCOORDS;
    }

  // Edge flag
  ef = input->GetPointData()->GetAttribute(vtkDataSetAttributes::EDGEFLAG);
  if (ef)
    {
    if (ef->GetNumberOfComponents() != 1)
      {
      vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
      ef = NULL;
      }
    if (!ef->IsA("vtkUnsignedCharArray"))
      {
      vtkDebugMacro(<< "Currently only unsigned char edge flags are suported.");
      ef = NULL;
      }
    }

  // Set the flags
  if (ef)
    {
    idx |= VTK_PDM_EDGEFLAGS;
    }

  if (!act)
    {
    vtkErrorMacro("No actor");
    }

  vtkShaderDeviceAdapter *shaderDevice = NULL;
  vtkGLSLShaderDeviceAdapter2 *shaderDevice2 = NULL;

  if (prop->GetShading())
    {
    if ( prop->GetShaderProgram())
      {
      shaderDevice = prop->GetShaderProgram()->GetShaderDeviceAdapter();
      }
    vtkOpenGLProperty *oglProp=vtkOpenGLProperty::SafeDownCast(prop);
    if (oglProp->GetCurrentShaderProgram2()!=0)
      {
      shaderDevice2=oglProp->GetShaderDeviceAdapter2();
      }
    }

  if (shaderDevice && this->GenericVertexAttributes)
    {
    idx |= VTK_PDM_GENERIC_VERTEX_ATTRIBUTES;
    }
  
  if (shaderDevice2 && this->GenericVertexAttributes)
    {
    idx |= VTK_PDM_GENERIC_VERTEX_ATTRIBUTES;
    }

  if (this->MultiTextureAttributes)
    {
    idx |= VTK_PDM_GENERIC_VERTEX_ATTRIBUTES;
    }

  if (this->RenderPrimitive(idx, n, c, t, renderer))
    {
    // subclass rendered the supported primitive successfully.
    // The delegate need not render it.
    typeflags &= (~this->SupportedPrimitive);
    }

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();

  this->Superclass::RenderInternal(renderer, act, typeflags,forceCompileOnly);
}

//---------------------------------------------------------------------------
void vtkPrimitivePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SupportedPrimitive: " << this->SupportedPrimitive << endl;
}
