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
#include "vtkCellTypes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h" //for VTK_MATERIALMODE_*
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//-----------------------------------------------------------------------------
static inline void vtkMultiplyColorsWithAlpha(vtkDataArray* array)
{
  vtkUnsignedCharArray* colors = vtkArrayDownCast<vtkUnsignedCharArray>(array);
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

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkScalarsToColorsPainter)
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
vtkInformationKeyMacro(vtkScalarsToColorsPainter, FIELD_DATA_TUPLE_ID, Integer);
vtkInformationKeyMacro(vtkScalarsToColorsPainter, SCALAR_MATERIAL_MODE, Integer);

//-----------------------------------------------------------------------------
vtkScalarsToColorsPainter::vtkScalarsToColorsPainter()
{
  this->ArrayName = NULL;
  this->ArrayId = -1;
  this->ArrayComponent = 0;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
  this->FieldDataTupleId = -1;

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

  if (info->Has(FIELD_DATA_TUPLE_ID()))
  {
    this->SetFieldDataTupleId(info->Get(FIELD_DATA_TUPLE_ID()));
  }

  if (info->Has(SCALAR_MATERIAL_MODE()))
  {
    this->SetScalarMaterialMode(info->Get(SCALAR_MATERIAL_MODE()));
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
    clone->GetCellData()->SetActiveAttribute(-1, vtkDataSetAttributes::SCALARS);
    clone->GetPointData()->SetActiveAttribute(-1, vtkDataSetAttributes::SCALARS);
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

    if ((this->ColorMode == VTK_COLOR_MODE_DEFAULT &&
         vtkArrayDownCast<vtkUnsignedCharArray>(scalars)) ||
        this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS)
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

  if (
    (this->ScalarsLookupTable &&
     this->ScalarsLookupTable->GetIndexedLookup()) ||
    (!this->ScalarsLookupTable &&
     this->LookupTable &&
     this->LookupTable->GetIndexedLookup()))
  {
    return 0;
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
    vtkIdType numberOfColors = this->LookupTable->GetNumberOfAvailableColors();
    numberOfColors += 2; // add 2 for below-/above-range colors
    vtkIdType textureSize = this->GetTextureSizeLimit();
    if(numberOfColors > textureSize)
    {
      numberOfColors = textureSize;
    }
    if(numberOfColors <= 1)
    {
      numberOfColors = 2;
    }
    // Subtract 2 from denominator below to remove below-/above-range
    // colors from color count
    double k = (range[1]-range[0]) / (numberOfColors-1-2);
    VTK_CREATE(vtkDoubleArray, scalarTable);
    // Size of lookup is actual 2*numberOfColors because one dimension
    // has actual values, then NaNs.
    scalarTable->SetNumberOfTuples(2*numberOfColors);
    double* scalarTablePtr = scalarTable->GetPointer(0);
    // The actual scalar values.
    for (int i = 0; i < numberOfColors; ++i)
    {
      *scalarTablePtr = range[0] + i * k - k; // minus k to start at below range color
      if (use_log_scale)
      {
        *scalarTablePtr = pow(10.0, *scalarTablePtr);
      }
      ++scalarTablePtr;
    }
    // Dimension on NaN.
    double nan = vtkMath::Nan();
    for (int i = 0; i < numberOfColors; ++i)
    {
      *scalarTablePtr = nan;
      ++scalarTablePtr;
    }
    this->ColorTextureMap = vtkSmartPointer<vtkImageData>::New();
    this->ColorTextureMap->SetExtent(0,numberOfColors-1,
      0,1, 0,0);
    vtkSmartPointer<vtkDataArray> colors;
    colors.TakeReference(this->LookupTable->MapScalars(scalarTable,
                                                       this->ColorMode, 0));
    if (multiply_with_alpha)
    {
      vtkMultiplyColorsWithAlpha(colors);
    }

    this->ColorTextureMap->GetPointData()->SetScalars(colors);
    this->LookupTable->SetAlpha(orig_alpha);
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
  vtkAbstractArray* abstractScalars = vtkAbstractMapper::GetAbstractScalars(input,
    this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
    this->ArrayName, cellFlag);

  vtkPointData* oppd = output->GetPointData();
  vtkCellData* opcd  = output->GetCellData();
  vtkFieldData* opfd = output->GetFieldData();

  int arraycomponent = this->ArrayComponent;
  // This is for a legacy feature: selection of the array component to color by
  // from the mapper.  It is now in the lookuptable.  When this feature
  // is removed, we can remove this condition.
  if (abstractScalars == 0 || abstractScalars->GetNumberOfComponents() <= this->ArrayComponent)
  {
    arraycomponent = 0;
  }

  if (!this->ScalarVisibility || abstractScalars == 0 || input == 0)
  {
    return;
  }

  vtkDataArray* scalars = vtkArrayDownCast<vtkDataArray>(abstractScalars);

  // Let subclasses know that scalar coloring was employed in the current pass.
  this->UsingScalarColoring = 1;
  if (this->ColorTextureMap && scalars)
  {
    // Implies that we have verified that we must use texture map for scalar
    // coloring. Just create texture coordinates for the input dataset.
    this->MapScalarsToTexture(output, scalars, input);
    return;
  }

  vtkScalarsToColors* lut = 0;
  // Get the lookup table.
  if (scalars && scalars->GetLookupTable())
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
  orig_alpha = lut->GetAlpha();
  lut->SetAlpha(alpha);
  colors = lut->MapScalars(abstractScalars, this->ColorMode, arraycomponent);
  lut->SetAlpha(orig_alpha);
  if (multiply_with_alpha)
  {
    // It is possible that the LUT simply returns the scalars as the
    // colors. In which case, we allocate a new array to ensure
    // that we don't modify the array in the input.
    if (abstractScalars == colors)
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
    if (this->FieldDataTupleId <= -1)
    {
      // Treat field data as cell-associated data
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
    else
    {
      vtkUnsignedCharArray* scalarColors =
        lut->MapScalars(abstractScalars, this->ColorMode, arraycomponent);

      if (this->FieldDataTupleId < scalarColors->GetNumberOfTuples())
      {
        // Use only the requested tuple's color
        unsigned char color[4];
        scalarColors->GetTypedTuple(this->FieldDataTupleId, color);

        vtkUnsignedCharArray* newColors = vtkUnsignedCharArray::New();
        newColors->SetNumberOfComponents(4);
        newColors->SetNumberOfTuples(input->GetNumberOfCells());
        newColors->SetName("Color");
        for (vtkIdType i = 0; i < input->GetNumberOfCells(); ++i)
        {
          newColors->SetTypedTuple(i, color);
        }
        opfd->AddArray(newColors);

        if (multiply_with_alpha)
        {
          vtkMultiplyColorsWithAlpha(newColors);
        }
        newColors->Delete();
      }
      else
      {
        vtkErrorMacro(<< "FieldDataTupleId " << this->FieldDataTupleId << " is greater than "
                      << "the number of tuples in the scalarColors array ("
                      << scalarColors->GetNumberOfTuples() << ")");
      }

      if (scalarColors)
      {
        scalarColors->Delete();
      }
    }
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

// anonymous namespace
namespace {

//-----------------------------------------------------------------------------
template<class T>
void ScalarToTextureCoordinate(
                               T scalar_value,         // Input scalar
                               double range_min,       // range[0]
                               double inv_range_width, // 1/(range[1]-range[0])
                               float &tex_coord_s,     // 1st tex coord
                               float &tex_coord_t)     // 2nd tex coord
{
  if (vtkMath::IsNan(scalar_value))
  {
    tex_coord_s = 0.5;  // Scalar value is arbitrary when NaN
    tex_coord_t = 1.0;  // 1.0 in t coordinate means NaN
  }
  else
  {
    // 0.0 in t coordinate means not NaN.  So why am I setting it to 0.49?
    // Because when you are mapping scalars and you have a NaN adjacent to
    // anything else, the interpolation everywhere should be NaN.  Thus, I
    // want the NaN color everywhere except right on the non-NaN neighbors.
    // To simulate this, I set the t coord for the real numbers close to
    // the threshold so that the interpolation almost immediately looks up
    // the NaN value.
    tex_coord_t = 0.49;

    double ranged_scalar = (scalar_value - range_min) * inv_range_width;
    tex_coord_s = static_cast<float>(ranged_scalar);

    // Some implementations apparently don't handle relatively large
    // numbers (compared to the range [0.0, 1.0]) very well. In fact,
    // values above 1122.0f appear to cause texture wrap-around on
    // some systems even when edge clamping is enabled. Why 1122.0f? I
    // don't know. For safety, we'll clamp at +/- 1000. This will
    // result in incorrect images when the texture value should be
    // above or below 1000, but I don't have a better solution.
    if (tex_coord_s > 1000.0f)
    {
      tex_coord_s = 1000.0f;
    }
    else if (tex_coord_s < -1000.0f)
    {
      tex_coord_s = -1000.0f;
    }
  }
}

//-----------------------------------------------------------------------------
template<class T>
void CreateColorTextureCoordinates(T* input, float* output,
                                   vtkIdType numScalars, int numComps,
                                   int component, double* range,
                                   const double* table_range,
                                   int tableNumberOfColors,
                                   bool use_log_scale)
{
  // We have to change the range used for computing texture
  // coordinates slightly to accomodate the special above- and
  // below-range colors that are the first and last texels,
  // respectively.
  double scalar_texel_width = (range[1] - range[0]) / static_cast<double>(tableNumberOfColors);
  double padded_range[2];
  padded_range[0] = range[0] - scalar_texel_width;
  padded_range[1] = range[1] + scalar_texel_width;
  double inv_range_width = 1.0 / (padded_range[1] - padded_range[0]);

  if (component < 0 || component >= numComps)
  {
    for (vtkIdType scalarIdx = 0; scalarIdx < numScalars; ++scalarIdx)
    {
      double sum = 0;
      for (int compIdx = 0; compIdx < numComps; ++compIdx)
      {
        double tmp = static_cast<double>(*input);
        sum += (tmp * tmp);
        ++input;
      }
      double magnitude = sqrt(sum);
      if (use_log_scale)
      {
        magnitude = vtkLookupTable::ApplyLogScale(
          magnitude, table_range, range);
      }
      ScalarToTextureCoordinate(magnitude, padded_range[0], inv_range_width,
                                output[0], output[1]);
      output += 2;
    }
  }
  else
  {
    input += component;
    for (vtkIdType scalarIdx = 0; scalarIdx < numScalars; ++scalarIdx)
    {
      double input_value = static_cast<double>(*input);
      if (use_log_scale)
      {
        input_value = vtkLookupTable::ApplyLogScale(
          input_value, table_range, range);
      }
      ScalarToTextureCoordinate(input_value, padded_range[0], inv_range_width,
                                output[0], output[1]);
      output += 2;
      input = input + numComps;
    }
  }
}

} // end anonymous namespace

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
    dtcoords->SetNumberOfComponents(2);
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
        CreateColorTextureCoordinates(static_cast<VTK_TT*>(void_input),
                                      tcptr, num, numComps,
                                      scalarComponent, range,
                                      this->LookupTable->GetRange(),
                                      this->LookupTable->GetNumberOfAvailableColors(),
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
vtkIdType vtkScalarsToColorsPainter::GetTextureSizeLimit()
{
  return 1024;
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
