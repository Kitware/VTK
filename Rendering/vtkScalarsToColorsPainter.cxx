/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColorsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkScalarsToColorsPainter.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h" //for VTK_MATERIALMODE_*
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"

// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkScalarsToColorsPainter);
vtkCxxRevisionMacro(vtkScalarsToColorsPainter, "1.1.2.1");
vtkInformationKeyMacro(vtkScalarsToColorsPainter, USE_LOOKUP_TABLE_SCALAR_RANGE, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, SCALAR_RANGE, DoubleVector);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, SCALAR_MODE, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, COLOR_MODE, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, INTERPOLATE_SCALARS_BEFORE_MAPPING, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, LOOKUP_TABLE, ObjectBase);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, SCALAR_VISIBILITY, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, ARRAY_ACCESS_MODE, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, ARRAY_ID, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, ARRAY_NAME, String);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, ARRAY_COMPONENT, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, SCALAR_MATERIAL_MODE, Integer);

//-----------------------------------------------------------------------------
vtkScalarsToColorsPainter::vtkScalarsToColorsPainter()
{
  this->ArrayName = NULL;
  this->ArrayId = -1;
  this->ArrayComponent = 0;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
  
  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->InterpolateScalarsBeforeMapping = 0;
  this->LookupTable = NULL;
  
  this->OutputData = vtkPolyData::New();
  
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  this->UseLookupTableScalarRange = 1; 
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->ScalarMaterialMode = VTK_MATERIALMODE_DEFAULT;

  this->ScalarVisibility = 1;

  this->ColorTextureMap = 0;

}

//-----------------------------------------------------------------------------
vtkScalarsToColorsPainter::~vtkScalarsToColorsPainter()
{
  if (this->OutputData)
    {
    this->OutputData->Delete();
    this->OutputData = 0;
    }
  this->SetLookupTable(NULL);

  if (this->ColorTextureMap)
    {
    this->ColorTextureMap->UnRegister(this);
    this->ColorTextureMap = NULL;
    }
}

//-----------------------------------------------------------------------------
vtkScalarsToColorsPainter* vtkScalarsToColorsPainter::New()
{
  vtkObject* o = vtkGraphicsFactory::CreateInstance("vtkScalarsToColorsPainter");
  return (vtkScalarsToColorsPainter*)o;
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::PrepareForRendering(vtkRenderer* renderer,
  vtkActor* actor)
{
  // If the input polydata has changed, the output should also reflect
  if (this->OutputUpdateTime < this->MTime ||
    this->OutputUpdateTime < this->PolyData->GetMTime())
    {
    this->OutputData->ShallowCopy(this->PolyData);

    // scalars passed thru this filter are colors, which will be buit in
    // the pre-rendering stage.
    this->OutputData->GetPointData()->SetScalars(NULL);
    this->OutputData->GetCellData()->SetScalars(NULL);

    // field data is only passed when coloring
    // TriangleStrips with colors for each triangle.
    this->OutputData->GetFieldData()->Initialize();
    this->OutputUpdateTime.Modified();
    } 

  // Build the colors.
  // As per the vtkOpenGLPolyDataMapper's claim, this
  // it not a very expensive task, as the colors are cached
  // and hence we do this always.
  this->MapScalars(actor->GetProperty()->GetOpacity()); 
  this->Superclass::PrepareForRendering(renderer, actor);
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::ProcessInformation(vtkInformation* info)
{
  if (info->Has(USE_LOOKUP_TABLE_SCALAR_RANGE()))
    {
    this->SetUseLookupTableScalarRange(info->Get(USE_LOOKUP_TABLE_SCALAR_RANGE()));
    }

  if (info->Has(SCALAR_RANGE()))
    {
    this->SetScalarRange(info->Get(SCALAR_RANGE()));
    }

  if (info->Has(SCALAR_MODE()))
    {
    this->SetScalarMode(info->Get(SCALAR_MODE()));
    }

  if (info->Has(COLOR_MODE()))
    {
    this->SetColorMode(info->Get(COLOR_MODE()));
    }

  if (info->Has(INTERPOLATE_SCALARS_BEFORE_MAPPING()))
    {
    this->SetInterpolateScalarsBeforeMapping(info->Get(
        INTERPOLATE_SCALARS_BEFORE_MAPPING()));
    }

  if (info->Has(LOOKUP_TABLE()))
    {
    vtkScalarsToColors* lut = vtkScalarsToColors::SafeDownCast(
      info->Get(LOOKUP_TABLE()));
    if (lut)
      {
      this->SetLookupTable(lut);
      } 
    }

  if (info->Has(SCALAR_VISIBILITY()))
    {
    this->SetScalarVisibility(info->Get(SCALAR_VISIBILITY()));
    }

  if (info->Has(ARRAY_ACCESS_MODE()))
    {
    this->SetArrayAccessMode(info->Get(ARRAY_ACCESS_MODE()));
    }

  if (info->Has(ARRAY_ID()))
    {
    this->SetArrayId(info->Get(ARRAY_ID()));
    }

  if (info->Has(ARRAY_NAME()))
    {
    this->SetArrayName(info->Get(ARRAY_NAME()));
    }

  if (info->Has(ARRAY_COMPONENT()))
    {
    this->SetArrayComponent(info->Get(ARRAY_COMPONENT()));
    }
  
  // when the iVars will be set, this->MTime will get updated.
  // This will eventually get caught by PrepareForRendering()
  // which will update the output. We need to discard old colors, 
  // since some iVar that affects the color might have changed.
}

//-----------------------------------------------------------------------------
// This method has the same functionality as the old vtkMapper::MapScalars.
void vtkScalarsToColorsPainter::MapScalars(double alpha)
{
  int cellFlag;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(this->GetPolyData(),
    this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
    this->ArrayName, cellFlag);

  vtkPointData* oppd = this->OutputData->GetPointData();
  vtkCellData* opcd = this->OutputData->GetCellData();
  vtkFieldData* opfd = this->OutputData->GetFieldData();

  // This is for a legacy feature: selection of the array component to color by
  // from the mapper.  It is now in the lookuptable.  When this feature
  // is removed, we can remove this condition.
  if (scalars == 0 || scalars->GetNumberOfComponents() <= this->ArrayComponent)
    {
    this->ArrayComponent = 0;
    }
  
  if (!this->ScalarVisibility || scalars == 0 || this->GetPolyData() == 0)
    {
    // no color info to pass thru.
    // Earlier, I was cleaning up the output scalars here.
    // However, if any entity affecting colors was changed, this->SetupPipeline()
    // would have caught it before this method call and cleaned up the
    // scalars, so no need to check/clean then again.
    if ( this->ColorTextureMap )
      {
      this->ColorTextureMap->UnRegister(this);
      this->ColorTextureMap = 0;
      }
    return;
    }
  
  // Get the lookup table.
  if (scalars->GetLookupTable())
    {
    this->SetLookupTable(scalars->GetLookupTable());
    }
  else
    {
    if (this->LookupTable == NULL)
      {
      this->CreateDefaultLookupTable();
      }
    this->LookupTable->Build();
    }
  
  if (!this->UseLookupTableScalarRange)
    {
    this->LookupTable->SetRange(this->ScalarRange);
    }

  // Decide betweeen texture color or vertex color.
  // Cell data always uses vertext color.
  // Only point data can use both texture and vertex coloring.
  if (this->InterpolateScalarsBeforeMapping && cellFlag==0)
    {
    // Only use texture color if we are mapping scalars.
    // Directly coloring with RGB unsigned chars should not use texture.
    if ( this->ColorMode != VTK_COLOR_MODE_DEFAULT || 
         (vtkUnsignedCharArray::SafeDownCast(scalars)) == 0 )
      { // Texture color option.
      this->MapScalarsToTexture(scalars, alpha);
      return;
      }
    }

  // Vertex colors are being used.
  // Get rid of texure Color arrays.  Only texture or vertex coloring 
  // can be active at one time.  The existence of the array is the 
  // signal to use that technique.
  if ( this->ColorTextureMap )
    {
    this->ColorTextureMap->UnRegister(this);
    this->ColorTextureMap = 0;
    }

  // Try to reuse the old colors.
  vtkDataArray* colors;
  if (cellFlag == 0)
    {
    colors = oppd->GetScalars();
    }
  else if (cellFlag == 1)
    {
    colors = opcd->GetScalars();
    }
  else
    {
    colors = opfd->GetArray("Color");
    }
  
  if (colors && this->LookupTable->GetAlpha() == alpha)
    {
    // (this->GetMTime() < colors->GetMTime() &&
    //  this->GetPolyData()->GetMTime() < colors->GetMTime())
    // checks are redundant, since if the PolyData or this->MTime changed,
    // this->SetupPipeline would have caught it and got rid of all the
    // scalars in the output, hence, the control would never reach here.
    if (this->GetMTime() < colors->GetMTime() &&
      this->GetPolyData()->GetMTime() < colors->GetMTime() &&
      this->LookupTable->GetMTime() < colors->GetMTime())
      {
      // using old colors.
      return;
      }
    }
 
  // Get rid of old colors.
  colors = 0;
  this->LookupTable->SetAlpha(alpha);
  colors = this->LookupTable->
    MapScalars(scalars, this->ColorMode, this->ArrayComponent);
  if (cellFlag == 0)
    {
    oppd->SetScalars(colors);
    }
  else if (cellFlag == 1)
    {
    opcd->SetScalars(colors);
    }
  else
    {
    // Typically, when a name is assigned of the scalars array in PointData or CellData
    // it implies 3 component colors. This implication does not hold for FieldData.
    // For colors in field data, we use the component count of the color array
    // to decide if the colors are opaque colors. 
    // These colors are nothing but cell colors, 
    // except when rendering TStrips, in which case they represent
    // the triange colors.
    colors->SetName("Color");
    opfd->AddArray(colors);
    }
  colors->Delete(); 
}

//-----------------------------------------------------------------------------
// Specify a lookup table for the mapper to use.
void vtkScalarsToColorsPainter::SetLookupTable(vtkScalarsToColors *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->LookupTable) 
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    if (lut)
      {
      lut->Register(this);
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkScalarsToColors *vtkScalarsToColorsPainter::GetLookupTable()
{
  if ( this->LookupTable == 0 )
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::CreateDefaultLookupTable()
{
  if ( this->LookupTable) 
    {
    this->LookupTable->UnRegister(this);
    }
  this->LookupTable = vtkLookupTable::New();
  // Consistent Register/UnRegisters.
  this->LookupTable->Register(this);
  this->LookupTable->Delete();
}

//-----------------------------------------------------------------------------
template<class T>
void vtkMapperCreateColorTextureCoordinates(T* input, float* output,
                                            vtkIdType num, int numComps, 
                                            int component, double* range)
{
  double tmp, sum;
  double k = 1.0 / (range[1]-range[0]);
  vtkIdType i;
  int j;

  if (component < 0 || component >= numComps)
    {
    for (i = 0; i < num; ++i)
      {
      sum = 0;
      for (j = 0; j < numComps; ++j)
        {
        tmp = (double)(*input);  
        sum += (tmp * tmp);
        ++input;
        }
      output[i] = k * (sqrt(sum) - range[0]);
      if (output[i] > 1.0)
        {
        output[i] = 1.0;
        }
      if (output[i] < 0.0)
        {
        output[i] = 0.0;
        }
      }
    }  
  else
    {
    input += component;
    for (i = 0; i < num; ++i)
      {
      output[i] = k * ((float)(*input) - range[0]);
      if (output[i] > 1.0)
        {
        output[i] = 1.0;
        }
      if (output[i] < 0.0)
        {
        output[i] = 0.0;
        }
      input = input + numComps;
      }      
    }
}

//-----------------------------------------------------------------------------
#define ColorTextureMapSize 256

void vtkScalarsToColorsPainter::MapScalarsToTexture(vtkDataArray* scalars,
  double alpha)
{
  vtkPolyData* input = this->GetPolyData();
  
  // this->SetupPipeline() assures that output has no scalars
  // of any kind, so no need to clean up the output scalars again. 

  double* range = this->LookupTable->GetRange();

  // If the lookup table has changed, the recreate the color texture map.
  // Set a new lookup table changes this->MTime.
  if (this->ColorTextureMap == 0 || 
    this->GetMTime() > this->ColorTextureMap->GetMTime() ||
    this->LookupTable->GetMTime() > this->ColorTextureMap->GetMTime() ||
    this->LookupTable->GetAlpha() != alpha)
    {
    this->LookupTable->SetAlpha(alpha);
    if ( this->ColorTextureMap )
      {
      this->ColorTextureMap->UnRegister(this);
      this->ColorTextureMap = 0;
      }
    // Get the texture map from the lookup table.
    // Create a dummy ramp of scalars.
    // In the future, we could extend vtkScalarsToColors.
    double k = (range[1]-range[0]) / (double)(ColorTextureMapSize-1);
    vtkFloatArray* tmp = vtkFloatArray::New();
    tmp->SetNumberOfTuples(ColorTextureMapSize);
    float* ptr = tmp->GetPointer(0);
    for (int i = 0; i < ColorTextureMapSize; ++i)
      {
      *ptr = range[0] + ((float)(i)) * k;
      ++ptr;
      }
    this->ColorTextureMap = vtkImageData::New();
    this->ColorTextureMap->SetExtent(0,ColorTextureMapSize-1, 
      0,0, 0,0);
    this->ColorTextureMap->SetNumberOfScalarComponents(4);
    this->ColorTextureMap->SetScalarTypeToUnsignedChar();
    this->ColorTextureMap->GetPointData()->SetScalars(
      this->LookupTable->MapScalars(tmp, this->ColorMode, 0));
    // Do we need to delete the scalars?
    this->ColorTextureMap->GetPointData()->GetScalars()->Delete();
    // Consistent register and unregisters
    this->ColorTextureMap->Register(this);
    this->ColorTextureMap->Delete();
    tmp->Delete();
    }

  // Create new coordinates if necessary.
  // Need to compare lookup table incase the range has changed.
  vtkDataArray* tcoords = this->OutputData->GetPointData()->GetTCoords();

  if (tcoords == 0 ||
    this->GetMTime() > tcoords->GetMTime() ||
    input->GetMTime() > tcoords->GetMTime() ||
    this->LookupTable->GetMTime() > tcoords->GetMTime())
    {
    // Get rid of old colors
    if ( tcoords )
      {
      this->OutputData->GetPointData()->SetTCoords(NULL);
      tcoords = 0;
      }

    // Now create the color texture coordinates.
    int numComps = scalars->GetNumberOfComponents();
    void* input = scalars->GetVoidPointer(0);
    vtkIdType num = scalars->GetNumberOfTuples();
    vtkFloatArray* dtcoords = vtkFloatArray::New();
    dtcoords->SetNumberOfTuples(num);
    this->OutputData->GetPointData()->SetTCoords(dtcoords);
    dtcoords->Delete();
    float* output = dtcoords->GetPointer(0);
    int scalarComponent;
    // Although I like the feature of applying magnitude to single component
    // scalars, it is not how the old MapScalars for vertex coloring works.
    if (this->LookupTable->GetVectorMode() == vtkScalarsToColors::MAGNITUDE &&
      scalars->GetNumberOfComponents() > 1)
      {
      scalarComponent = -1;
      }
    else
      {
      scalarComponent = this->LookupTable->GetVectorComponent();
      }
    switch (scalars->GetDataType())
      {
      vtkTemplateMacro(
        vtkMapperCreateColorTextureCoordinates(static_cast<VTK_TT*>(input),
          output, num, numComps,
          scalarComponent, range)
      );
    case VTK_BIT:
      vtkErrorMacro("Cannot color by bit array.");
      break;
    default:
      vtkErrorMacro(<< "Unknown input ScalarType");
      return;
      }
    }
}
//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->OutputData, "Output PolyData");
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }
}
