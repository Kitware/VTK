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
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h" //for VTK_MATERIALMODE_*
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkCompositeDataIterator.h"

#define COLOR_TEXTURE_MAP_SIZE 256

//-----------------------------------------------------------------------------
static inline void vtkMultiplyColorsWithAlpha(vtkDataArray* array)
{
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(array);
  if (!colors || colors->GetNumberOfComponents() != 4)
    {
    return;
    }
  unsigned char* ptr = colors->GetPointer(0);
  vtkIdType numValues = colors->GetNumberOfTuples() *
    colors->GetNumberOfComponents();
  if (numValues <= 4)
    {
    return;
    }
  for (vtkIdType cc=0; cc < numValues; cc+=4, ptr+=4)
    {
    double alpha = (0x0ff & ptr[3])/255.0;
    ptr[0] = static_cast<unsigned char>(0x0ff & static_cast<int>((0x0ff&ptr[0])*alpha));
    ptr[1] = static_cast<unsigned char>(0x0ff & static_cast<int>((0x0ff&ptr[1])*alpha));
    ptr[2] = static_cast<unsigned char>(0x0ff & static_cast<int>((0x0ff&ptr[2])*alpha));
    }
}

// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkScalarsToColorsPainter);
vtkCxxSetObjectMacro(vtkScalarsToColorsPainter, LookupTable, vtkScalarsToColors);
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
  
  this->OutputData = 0;
  
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  this->UseLookupTableScalarRange = 1; 
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->ScalarMaterialMode = VTK_MATERIALMODE_DEFAULT;

  this->UsingScalarColoring = 0;
  this->ScalarVisibility = 1;

  this->LastUsedAlpha = -1.0;
  this->LastUsedMultiplyWithAlpha = -1;
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
  this->ColorTextureMap = 0;
  this->SetArrayName(0);
}

//-----------------------------------------------------------------------------
vtkScalarsToColorsPainter* vtkScalarsToColorsPainter::New()
{
  vtkObject* o = vtkGraphicsFactory::CreateInstance("vtkScalarsToColorsPainter");
  return static_cast<vtkScalarsToColorsPainter *>(o);
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
int vtkScalarsToColorsPainter::GetPremultiplyColorsWithAlpha(vtkActor* actor)
{
  if (actor && (actor->GetTexture() || 
      actor->GetProperty()->GetNumberOfTextures() > 0))
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkScalarsToColorsPainter::NewClone(vtkDataObject* data)
{
  if (data->IsA("vtkDataSet"))
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(data);
    vtkDataSet* clone = ds->NewInstance();
    clone->ShallowCopy(ds);
    // scalars passed thru this filter are colors, which will be buit in
    // the pre-rendering stage.
    clone->GetCellData()->SetScalars(0);
    clone->GetPointData()->SetScalars(0);
    // field data is only passed when coloring
    // TriangleStrips with colors for each triangle.
    clone->GetFieldData()->Initialize();
    return clone;
    }
  else if (data->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(data);
    vtkCompositeDataSet* clone = cd->NewInstance();
    clone->CopyStructure(cd);
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); 
      !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataObject* leafClone = this->NewClone(iter->GetCurrentDataObject());
      clone->SetDataSet(iter, leafClone);
      leafClone->Delete();
      }
    iter->Delete();
    return clone;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::PrepareForRendering(vtkRenderer* renderer,
  vtkActor* actor)
{
  vtkDataObject* input = this->GetInput();
  if (!input)
    {
    vtkErrorMacro("No input present.");
    return;
    }

  // If the input polydata has changed, the output should also reflect
  if (!this->OutputData ||
    !this->OutputData->IsA(input->GetClassName()) || 
    this->OutputUpdateTime < this->MTime ||
    this->OutputUpdateTime < this->GetInput()->GetMTime())
    {
    if (this->OutputData)
      {
      this->OutputData->Delete();
      this->OutputData = 0;
      }
    // Create a shallow-copied clone with no output scalars.
    this->OutputData = this->NewClone(input);
    this->OutputUpdateTime.Modified();
    } 

  if (!this->ScalarVisibility)
    {
    // Nothing to do here.
    this->ColorTextureMap = 0;
    this->Superclass::PrepareForRendering(renderer, actor);
    return;
    }

  // Build the colors.
  // As per the vtkOpenGLPolyDataMapper's claim, this
  // it not a very expensive task, as the colors are cached
  // and hence we do this always.

  // Determine if we are going to use a texture for coloring or use vertex
  // colors. This need to be determine before we iterate over all the blocks in
  // the composite dataset to ensure that we emply the technique for all the
  // blocks.
  this->ScalarsLookupTable = 0;
  int useTexture = this->CanUseTextureMapForColoring(input);
  if (useTexture)
    {
    // Ensure that the ColorTextureMap has been created and updated correctly.
    // ColorTextureMap depends on the LookupTable. Hence it can be generated
    // independent of the input.
    this->UpdateColorTextureMap(actor->GetProperty()->GetOpacity(),
          this->GetPremultiplyColorsWithAlpha(actor));
    }
  else
    {
    // Remove texture map if present.
    this->ColorTextureMap = 0;
    }

  this->UsingScalarColoring = 0;

  // Now if we have composite data, we need to MapScalars for all leaves.
  if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input);
    vtkCompositeDataSet* cdOutput = 
      vtkCompositeDataSet::SafeDownCast(this->OutputData);
    vtkCompositeDataIterator* iter = cdInput->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
      {
      vtkDataSet* pdInput = vtkDataSet::SafeDownCast(
        iter->GetCurrentDataObject());
      vtkDataSet* pdOutput = vtkDataSet::SafeDownCast(
        cdOutput->GetDataSet(iter));
      if (pdInput && pdOutput)
        {
        this->MapScalars(pdOutput, actor->GetProperty()->GetOpacity(),
          this->GetPremultiplyColorsWithAlpha(actor),
          pdInput);
        }
      }

    iter->Delete();
    }
  else
    {
    this->MapScalars(vtkDataSet::SafeDownCast(this->OutputData),
      actor->GetProperty()->GetOpacity(),
      this->GetPremultiplyColorsWithAlpha(actor),
      vtkDataSet::SafeDownCast(input));
    }
  this->LastUsedAlpha = actor->GetProperty()->GetOpacity();
  this->LastUsedMultiplyWithAlpha = this->GetPremultiplyColorsWithAlpha(actor);
  this->Superclass::PrepareForRendering(renderer, actor);
}

//-----------------------------------------------------------------------------
// Returns if we can use texture maps for scalar coloring. Note this doesn't say
// we "will" use scalar coloring. It says, if we do use scalar coloring, we will
// use a 1D texture.
// When rendering multiblock datasets, if any 2 blocks provide different
// lookup tables for the scalars, then also we cannot use textures. This case can
// be handled if required. 
int vtkScalarsToColorsPainter::CanUseTextureMapForColoring(vtkDataObject* input)
{
  if (!this->InterpolateScalarsBeforeMapping)
    {
    return 0; // user doesn't want us to use texture maps at all.
    }

  if (input->IsA("vtkDataSet"))
    {
    int cellFlag=0;
    vtkDataSet* ds = static_cast<vtkDataSet*>(input);
    vtkDataArray* scalars = vtkAbstractMapper::GetScalars(ds,
      this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
      this->ArrayName, cellFlag);

    if (!scalars)
      {
      // no scalars on  this dataset, we don't care if texture is used at all.
      return 1;
      }

    if (cellFlag)
      {
      return 0; // cell data colors, don't use textures.
      }

    if (this->ColorMode == VTK_COLOR_MODE_DEFAULT && 
      vtkUnsignedCharArray::SafeDownCast(scalars))
      { 
      // Don't use texture is direct coloring using RGB unsigned chars is
      // requested. 
      return 0;
      }

    if (this->ScalarsLookupTable && scalars->GetLookupTable() &&
      (this->ScalarsLookupTable.GetPointer() != scalars->GetLookupTable()))
      {
      // Two datasets are requesting different lookup tables to color with.
      // We don't handle this case right now for composite datasets.
      this->ScalarsLookupTable = 0;
      return 0;
      }
    
    if (scalars->GetLookupTable())
      {
      this->ScalarsLookupTable = scalars->GetLookupTable();
      }
    }
  else if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataIterator* iter = 
      static_cast<vtkCompositeDataSet*>(input)->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
      {
      if (!this->CanUseTextureMapForColoring(iter->GetCurrentDataObject()))
        {
        iter->Delete();
        return 0;
        }
      }
    iter->Delete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
// Should not be called if CanUseTextureMapForColoring() returns 0.
void vtkScalarsToColorsPainter::UpdateColorTextureMap(double alpha,
  int multiply_with_alpha)
{
  if (this->ScalarsLookupTable)
    {
    this->SetLookupTable(this->ScalarsLookupTable);
    }
  else
    {
    // this creates a default one if none present.
    this->GetLookupTable()->Build();
    }

  if (!this->UseLookupTableScalarRange)
    {
    this->LookupTable->SetRange(this->ScalarRange);
    }
  
  double range[2];
  range[0] = this->LookupTable->GetRange()[0];
  range[1] = this->LookupTable->GetRange()[1];

  bool use_log_scale = (this->LookupTable->UsingLogScale() != 0);
  if (use_log_scale)
    {
    // convert range to log.
    vtkLookupTable::GetLogRange(range, range);
    }

  double orig_alpha = this->LookupTable->GetAlpha();

  // If the lookup table has changed, the recreate the color texture map.
  // Set a new lookup table changes this->MTime.
  if (this->ColorTextureMap == 0 || 
    this->GetMTime() > this->ColorTextureMap->GetMTime() ||
    this->LookupTable->GetMTime() > this->ColorTextureMap->GetMTime() ||
    this->LookupTable->GetAlpha() != alpha ||
    this->LastUsedAlpha != alpha ||
    this->LastUsedMultiplyWithAlpha != multiply_with_alpha)
    {
    this->LookupTable->SetAlpha(alpha);
    this->ColorTextureMap = 0;

    // Get the texture map from the lookup table.
    // Create a dummy ramp of scalars.
    // In the future, we could extend vtkScalarsToColors.
    double k = (range[1]-range[0]) / (COLOR_TEXTURE_MAP_SIZE-1);
    vtkFloatArray* tmp = vtkFloatArray::New();
    tmp->SetNumberOfTuples(COLOR_TEXTURE_MAP_SIZE);
    float* ptr = tmp->GetPointer(0);
    for (int i = 0; i < COLOR_TEXTURE_MAP_SIZE; ++i)
      {
      *ptr = range[0] + i * k;
      if (use_log_scale)
        {
        *ptr = pow(static_cast<float>(10.0), *ptr);
        }
      ++ptr;
      }
    this->ColorTextureMap = vtkSmartPointer<vtkImageData>::New();
    this->ColorTextureMap->SetExtent(0,COLOR_TEXTURE_MAP_SIZE-1, 
      0,0, 0,0);
    this->ColorTextureMap->SetNumberOfScalarComponents(4);
    this->ColorTextureMap->SetScalarTypeToUnsignedChar();
    vtkDataArray* colors = 
      this->LookupTable->MapScalars(tmp, this->ColorMode, 0);
    if (multiply_with_alpha)
      {
      vtkMultiplyColorsWithAlpha(colors);
      }

    this->ColorTextureMap->GetPointData()->SetScalars(colors);
    this->LookupTable->SetAlpha(orig_alpha);
    colors->Delete();
    tmp->Delete();
    }
}

//-----------------------------------------------------------------------------
// This method has the same functionality as the old vtkMapper::MapScalars.
void vtkScalarsToColorsPainter::MapScalars(vtkDataSet* output,
  double alpha, int multiply_with_alpha,
  vtkDataSet* input)
{
  int cellFlag;
  double orig_alpha;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(input,
    this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
    this->ArrayName, cellFlag);

  vtkPointData* oppd = output->GetPointData();
  vtkCellData* opcd  = output->GetCellData();
  vtkFieldData* opfd = output->GetFieldData();

  int arraycomponent = this->ArrayComponent;
  // This is for a legacy feature: selection of the array component to color by
  // from the mapper.  It is now in the lookuptable.  When this feature
  // is removed, we can remove this condition.
  if (scalars == 0 || scalars->GetNumberOfComponents() <= this->ArrayComponent)
    {
    arraycomponent = 0;
    }
  
  if (!this->ScalarVisibility || scalars == 0 || input == 0)
    {
    return;
    }

  // Let subclasses know that scalar coloring was employed in the current pass.
  this->UsingScalarColoring = 1;
  if (this->ColorTextureMap)
    {
    // Implies that we have verified that we must use texture map for scalar
    // coloring. Just create texture coordinates for the input dataset.
    this->MapScalarsToTexture(output, scalars, input);
    return;
    }
 
  vtkScalarsToColors* lut = 0;
  // Get the lookup table.
  if (scalars->GetLookupTable())
    {
    lut = scalars->GetLookupTable();
    }
  else
    {
    lut = this->GetLookupTable();
    lut->Build();
    }
  
  if (!this->UseLookupTableScalarRange)
    {
    lut->SetRange(this->ScalarRange);
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
 
  // The LastUsedAlpha checks ensures that opacity changes are reflected
  // correctly when this->MapScalars(..) is called when iterating over a
  // composite dataset.
  if (colors && 
    this->LastUsedAlpha == alpha &&
    this->LastUsedMultiplyWithAlpha == multiply_with_alpha)
    {
    if (this->GetMTime() < colors->GetMTime() &&
      input->GetMTime() < colors->GetMTime() &&
      lut->GetMTime() < colors->GetMTime())
      {
      // using old colors.
      return;
      }
    }
 
  // Get rid of old colors.
  colors = 0;
  orig_alpha = lut->GetAlpha();
  lut->SetAlpha(alpha);
  colors = lut->MapScalars(scalars, this->ColorMode, arraycomponent);
  lut->SetAlpha(orig_alpha);
  if (multiply_with_alpha)
    {
    // It is possible that the LUT simply returns the scalars as the
    // colors. In which case, we allocate a new array to ensure
    // that we don't modify the array in the input.
    if (scalars == colors)
      {
      // Since we will be changing the colors array
      // we create a copy.
      colors->Delete();
      colors = scalars->NewInstance();
      colors->DeepCopy(scalars);
      }
    vtkMultiplyColorsWithAlpha(colors);
    }
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
vtkScalarsToColors *vtkScalarsToColorsPainter::GetLookupTable()
{
  if (this->LookupTable == 0)
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::CreateDefaultLookupTable()
{
  vtkLookupTable* lut = vtkLookupTable::New();
  this->SetLookupTable(lut);
  lut->Delete();
}

//-----------------------------------------------------------------------------
template<class T>
void vtkMapperCreateColorTextureCoordinates(T* input, float* output,
                                            vtkIdType num, int numComps, 
                                            int component, double* range,
                                            double* table_range,
                                            bool use_log_scale)
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
        tmp = static_cast<double>(*input);  
        sum += (tmp * tmp);
        ++input;
        }
      double magnitude = sqrt(sum);
      if (use_log_scale)
        {
        magnitude = vtkLookupTable::ApplyLogScale(
          magnitude, table_range, range);
        }
      output[i] = k * (magnitude - range[0]);
      output[i] = output[i] > 1.0f ? 1.0f : 
        (output[i] < 0.0f ? 0.0f : output[i]);
      }
    }  
  else
    {
    input += component;
    for (i = 0; i < num; ++i)
      {
      double input_value = static_cast<double>(*input);
      if (use_log_scale)
        {
        input_value = vtkLookupTable::ApplyLogScale(
          input_value, table_range, range);
        }
      output[i] = k * (input_value - range[0]);
      output[i] = output[i] > 1.0f ? 1.0f : 
        (output[i] < 0.0f ? 0.0f : output[i]);
      input = input + numComps;
      }      
    }
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsPainter::MapScalarsToTexture(
  vtkDataSet* output, vtkDataArray* scalars, vtkDataSet* input)
{
  // Create new coordinates if necessary.
  // Need to compare lookup table incase the range has changed.
  vtkDataArray* tcoords = output->GetPointData()->GetTCoords();

  if (tcoords == 0 ||
    this->GetMTime() > tcoords->GetMTime() ||
    input->GetMTime() > tcoords->GetMTime() ||
    this->LookupTable->GetMTime() > tcoords->GetMTime())
    {
    double range[2];
    range[0] = this->LookupTable->GetRange()[0];
    range[1] = this->LookupTable->GetRange()[1];
    bool use_log_scale = (this->LookupTable->UsingLogScale() != 0);
    if (use_log_scale)
      {
      // convert range to log.
      vtkLookupTable::GetLogRange(range, range);
      }

    // Get rid of old colors
    if ( tcoords )
      {
      output->GetPointData()->SetTCoords(NULL);
      tcoords = 0;
      }

    // Now create the color texture coordinates.
    int numComps = scalars->GetNumberOfComponents();
    void* void_input = scalars->GetVoidPointer(0);
    vtkIdType num = scalars->GetNumberOfTuples();
    vtkFloatArray* dtcoords = vtkFloatArray::New();
    dtcoords->SetNumberOfTuples(num);
    output->GetPointData()->SetTCoords(dtcoords);
    dtcoords->Delete();
    float* tcptr = dtcoords->GetPointer(0);
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
        vtkMapperCreateColorTextureCoordinates(static_cast<VTK_TT*>(void_input),
          tcptr, num, numComps,
          scalarComponent, range,
          this->LookupTable->GetRange(),
          use_log_scale)
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
vtkDataObject *vtkScalarsToColorsPainter::GetOutput()
{
  return this->OutputData;
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
