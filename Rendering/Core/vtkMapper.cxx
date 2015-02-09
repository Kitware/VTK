/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMapper.h"

#include "vtkAbstractArray.h"
#include "vtkColorSeries.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkLookupTable.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkVariantArray.h"


// Initialize static member that controls global immediate mode rendering
static int vtkMapperGlobalImmediateModeRendering = 0;

// Initialize static member that controls global coincidence resolution
static int vtkMapperGlobalResolveCoincidentTopology = VTK_RESOLVE_OFF;
static double vtkMapperGlobalResolveCoincidentTopologyZShift = 0.01;
static double vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor = 1.0;
static double vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits = 1.0;
static int vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFaces = 1;

// Construct with initial range (0,1).
vtkMapper::vtkMapper()
{
  this->Colors = 0;
  this->Static = 0;
  this->LookupTable = 0;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
  this->UseLookupTableScalarRange = 0;

  this->ImmediateModeRendering = 0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  this->ScalarMaterialMode = VTK_MATERIALMODE_DEFAULT;

  vtkMath::UninitializeBounds(this->Bounds);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->RenderTime = 0.0;

  strcpy(this->ArrayName, "");
  this->ArrayId = -1;
  this->ArrayComponent = 0;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;

  this->FieldDataTupleId = -1;

  this->InterpolateScalarsBeforeMapping = 0;
  this->ColorCoordinates = 0;
  this->ColorTextureMap = 0;

  this->ForceCompileOnly=0;
}

vtkMapper::~vtkMapper()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if ( this->Colors != 0 )
    {
    this->Colors->UnRegister(this);
    }
  if ( this->ColorCoordinates != 0 )
    {
    this->ColorCoordinates->UnRegister(this);
    }
  if ( this->ColorTextureMap != 0 )
    {
    this->ColorTextureMap->UnRegister(this);
    }
}

// Get the bounds for the input of this mapper as
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkMapper::GetBounds()
{
  vtkDataSet *input = this->GetInput();
  if ( ! input )
    {
      vtkMath::UninitializeBounds(this->Bounds);
    }
  else
    {
    if (!this->Static)
      {
      this->Update();
      }
    input->GetBounds(this->Bounds);
    }
  return this->Bounds;
}

vtkDataSet *vtkMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

void vtkMapper::SetForceCompileOnly(int value)
{
  if(this->ForceCompileOnly!=value)
    {
      this->ForceCompileOnly=value;
      // make sure we don't call this->Modified();
      //      this->Modified();
    }
}

void vtkMapper::SetGlobalImmediateModeRendering(int val)
{
  if (val == vtkMapperGlobalImmediateModeRendering)
    {
    return;
    }
  vtkMapperGlobalImmediateModeRendering = val;
}

int vtkMapper::GetGlobalImmediateModeRendering()
{
  return vtkMapperGlobalImmediateModeRendering;
}

void vtkMapper::SetResolveCoincidentTopology(int val)
{
  if (val == vtkMapperGlobalResolveCoincidentTopology)
    {
    return;
    }
  vtkMapperGlobalResolveCoincidentTopology = val;
}

int vtkMapper::GetResolveCoincidentTopology()
{
  return vtkMapperGlobalResolveCoincidentTopology;
}

void vtkMapper::SetResolveCoincidentTopologyToDefault()
{
  vtkMapperGlobalResolveCoincidentTopology = VTK_RESOLVE_OFF;
}

void vtkMapper::SetResolveCoincidentTopologyZShift(double val)
{
  if (val == vtkMapperGlobalResolveCoincidentTopologyZShift)
    {
    return;
    }
  vtkMapperGlobalResolveCoincidentTopologyZShift = val;
}

double vtkMapper::GetResolveCoincidentTopologyZShift()
{
  return vtkMapperGlobalResolveCoincidentTopologyZShift;
}

void vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(
                                            double factor, double units)
{
  if (factor == vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor &&
      units == vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits )
    {
    return;
    }
  vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor = factor;
  vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits = units;
}

void vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(
                           double& factor, double& units)
{
  factor = vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor;
  units = vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits;
}

void vtkMapper::SetResolveCoincidentTopologyPolygonOffsetFaces(int faces)
{
  vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFaces = faces;
}

int vtkMapper::GetResolveCoincidentTopologyPolygonOffsetFaces()
{
  return vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFaces;
}

// Overload standard modified time function. If lookup table is modified,
// then this object is modified as well.
unsigned long vtkMapper::GetMTime()
{
  //unsigned long mTime=this->MTime.GetMTime();
  unsigned long mTime=vtkAbstractMapper::GetMTime();
  unsigned long lutMTime;

  if ( this->LookupTable != NULL )
    {
    lutMTime = this->LookupTable->GetMTime();
    mTime = ( lutMTime > mTime ? lutMTime : mTime );
    }

  return mTime;
}

void vtkMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkMapper *m = vtkMapper::SafeDownCast(mapper);
  if ( m != NULL )
    {
    this->SetLookupTable(m->GetLookupTable());
    this->SetScalarVisibility(m->GetScalarVisibility());
    this->SetScalarRange(m->GetScalarRange());
    this->SetColorMode(m->GetColorMode());
    this->SetScalarMode(m->GetScalarMode());
    this->SetScalarMaterialMode(m->GetScalarMaterialMode());
    this->SetImmediateModeRendering(m->GetImmediateModeRendering());
    this->SetUseLookupTableScalarRange(m->GetUseLookupTableScalarRange());
    this->SetInterpolateScalarsBeforeMapping(
      m->GetInterpolateScalarsBeforeMapping());

    if ( m->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID )
      {
      this->ColorByArrayComponent(m->GetArrayId(),m->GetArrayComponent());
      }
    else
      {
      this->ColorByArrayComponent(m->GetArrayName(),m->GetArrayComponent());
      }
    }

  // Now do superclass
  this->vtkAbstractMapper3D::ShallowCopy(mapper);

}

// a side effect of this is that this->Colors is also set
// to the return value
vtkUnsignedCharArray *vtkMapper::MapScalars(double alpha)
{
  vtkDataSet *input = this->GetInput();
  return this->MapScalars(input,alpha);
}

// a side effect of this is that this->Colors is also set
// to the return value
vtkUnsignedCharArray *vtkMapper::MapScalars(vtkDataSet *input,
                                            double alpha)
{
  int cellFlag = 0;

  vtkAbstractArray *scalars = vtkAbstractMapper::
    GetAbstractScalars(input, this->ScalarMode, this->ArrayAccessMode,
                       this->ArrayId, this->ArrayName, cellFlag);

  // This is for a legacy feature: selection of the array component to color by
  // from the mapper.  It is now in the lookuptable.  When this feature
  // is removed, we can remove this condition.
  if (scalars == 0 || scalars->GetNumberOfComponents() <= this->ArrayComponent)
    {
    this->ArrayComponent = 0;
    }

  if ( !this->ScalarVisibility || scalars==0 || input==0)
    { // No scalar colors.
    if ( this->ColorCoordinates )
      {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = 0;
      }
    if ( this->ColorTextureMap )
      {
      this->ColorTextureMap->UnRegister(this);
      this->ColorTextureMap = 0;
      }
    if ( this->Colors )
      {
      this->Colors->UnRegister(this);
      this->Colors = 0;
      }
    return 0;
    }

  // Get the lookup table.
  vtkDataArray *dataArray = vtkDataArray::SafeDownCast(scalars);
  if (dataArray && dataArray->GetLookupTable())
    {
    this->SetLookupTable(dataArray->GetLookupTable());
    }
  else
    {
    // make sure we have a lookup table
    if ( this->LookupTable == 0 )
      {
      this->CreateDefaultLookupTable();
      }
    this->LookupTable->Build();
    }

  if ( !this->UseLookupTableScalarRange )
    {
    this->LookupTable->SetRange(this->ScalarRange);
    }

  // Decide betweeen texture color or vertex color.
  // Cell data always uses vertext color.
  // Only point data can use both texture and vertext coloring.
  if (this->InterpolateScalarsBeforeMapping && ! cellFlag)
    {
    // Only use texture color if we are mapping scalars.
    // Directly coloring with RGB unsigned chars should not use texture.
    if (dataArray && (this->ColorMode != VTK_COLOR_MODE_DEFAULT ||
          (vtkUnsignedCharArray::SafeDownCast(scalars)) == 0) &&
         this->ColorMode != VTK_COLOR_MODE_DIRECT_SCALARS)
      { // Texture color option.
      this->MapScalarsToTexture(dataArray, alpha);
      return 0;
      }
    }

  // Vertex colors are being used.
  // Get rid of texure Color arrays.  Only texture or vertex coloring
  // can be active at one time.  The existence of the array is the
  // signal to use that technique.
  if ( this->ColorCoordinates )
    {
    this->ColorCoordinates->UnRegister(this);
    this->ColorCoordinates = 0;
    }
  if ( this->ColorTextureMap )
    {
    this->ColorTextureMap->UnRegister(this);
    this->ColorTextureMap = 0;
    }

  // Lets try to resuse the old colors.
  if (this->Colors)
    {
    if (this->LookupTable && this->LookupTable->GetAlpha() == alpha)
      {
      if (this->GetMTime() < this->Colors->GetMTime() &&
          input->GetMTime() < this->Colors->GetMTime() &&
          this->LookupTable->GetMTime() < this->Colors->GetMTime())
        {
        return this->Colors;
        }
      }
    }

  // Get rid of old colors
  if ( this->Colors )
    {
    this->Colors->UnRegister(this);
    this->Colors = 0;
    }

  // map scalars
  double orig_alpha = this->LookupTable->GetAlpha();
  this->LookupTable->SetAlpha(alpha);
  this->Colors = this->LookupTable->
    MapScalars(scalars, this->ColorMode, this->ArrayComponent);
  this->LookupTable->SetAlpha(orig_alpha);
  // Consistent register and unregisters
  this->Colors->Register(this);
  this->Colors->Delete();

  return this->Colors;
}


void vtkMapper::SelectColorArray(int arrayNum)
{
  this->ColorByArrayComponent(arrayNum, -1);
}


void vtkMapper::SelectColorArray(const char* arrayName)
{
  this->ColorByArrayComponent(arrayName, -1);
}


void vtkMapper::ColorByArrayComponent(int arrayNum, int component)
{
  if (this->ArrayId == arrayNum && component == this->ArrayComponent &&
      this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
    return;
    }
  this->Modified();

  this->ArrayId = arrayNum;
  this->ArrayComponent = component;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

void vtkMapper::ColorByArrayComponent(const char* arrayName, int component)
{
  if (!arrayName ||
      ( strcmp(this->ArrayName, arrayName) == 0 &&
        component == this->ArrayComponent &&
        this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME ))
    {
    return;
    }
  this->Modified();

  strcpy(this->ArrayName, arrayName);
  this->ArrayComponent = component;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_NAME;
}

// Specify a lookup table for the mapper to use.
void vtkMapper::SetLookupTable(vtkScalarsToColors *lut)
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

vtkScalarsToColors *vtkMapper::GetLookupTable()
{
  if ( this->LookupTable == 0 )
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

void vtkMapper::CreateDefaultLookupTable()
{
  if ( this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  vtkLookupTable* table = vtkLookupTable::New();
  this->LookupTable = table;
  this->LookupTable->Register(this);
  this->LookupTable->Delete();

  int cellFlag = 0; // not used
  vtkAbstractArray* abstractArray = vtkAbstractMapper::
    GetAbstractScalars(this->GetInput(), this->ScalarMode, this->ArrayAccessMode,
                       this->ArrayId, this->ArrayName, cellFlag);

  vtkDataArray *dataArray = vtkDataArray::SafeDownCast(abstractArray);
  if (abstractArray && !dataArray)
    {
    // Use indexed lookup for non-numeric arrays
    this->LookupTable->IndexedLookupOn();

    // Get prominent values from array and set them up as annotations in the color map.
    vtkVariantArray* prominentValues = vtkVariantArray::New();
    abstractArray->GetProminentComponentValues(0, prominentValues);
    vtkIdType numProminentValues = prominentValues->GetNumberOfValues();
    table->SetNumberOfTableValues(numProminentValues);
    for (vtkIdType i = 0; i < numProminentValues; ++i)
      {
      vtkVariant & variant = prominentValues->GetValue(i);
      this->LookupTable->SetAnnotation(variant, variant.ToString());
      }
    prominentValues->Delete();

    // Set colors for annotations
    vtkColorSeries* colorSeries = vtkColorSeries::New();
    colorSeries->SetColorScheme(vtkColorSeries::BREWER_QUALITATIVE_PAIRED);
    colorSeries->BuildLookupTable(table, vtkColorSeries::CATEGORICAL);
    colorSeries->Delete();
    }
}

// Return the method of coloring scalar data.
const char *vtkMapper::GetColorModeAsString(void)
{
  if ( this->ColorMode == VTK_COLOR_MODE_MAP_SCALARS )
    {
    return "MapScalars";
    }
  else
    {
    return "Default";
    }
}

// Return the method for obtaining scalar data.
const char *vtkMapper::GetScalarModeAsString(void)
{
  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    return "UseCellData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
    {
    return "UsePointFieldData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    return "UseCellFieldData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA )
    {
    return "UseFieldData";
    }
  else
    {
    return "Default";
    }
}

const char *vtkMapper::GetScalarMaterialModeAsString(void)
{
  if ( this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT )
    {
    return "Ambient";
    }
  else if ( this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE )
    {
    return "Diffuse";
    }
  else if ( this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE )
    {
    return "Ambient and Diffuse";
    }
  else
    {
    return "Default";
    }
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
    }

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

//-----------------------------------------------------------------------------
template<class T>
void CreateColorTextureCoordinates(T* input, float* output,
                                   vtkIdType numScalars, int numComps,
                                   int component, double* range,
                                   const double* table_range,
                                   bool use_log_scale)
{
  double inv_range_width = 1.0 / (range[1]-range[0]);

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
      ScalarToTextureCoordinate(magnitude, range[0], inv_range_width,
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
      ScalarToTextureCoordinate(input_value, range[0], inv_range_width,
                                output[0], output[1]);
      output += 2;
      input = input + numComps;
      }
    }
}

} // end anonymous namespace

#define ColorTextureMapSize 256
// a side effect of this is that this->ColorCoordinates and
// this->ColorTexture are set.
void vtkMapper::MapScalarsToTexture(vtkDataArray* scalars, double alpha)
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

  double orig_alpha = this->LookupTable->GetAlpha();

  // Get rid of vertex color array.  Only texture or vertex coloring
  // can be active at one time.  The existence of the array is the
  // signal to use that technique.
  if ( this->Colors )
    {
    this->Colors->UnRegister(this);
    this->Colors = 0;
    }

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
    double k = (range[1]-range[0]) / (ColorTextureMapSize-1);
    vtkFloatArray* tmp = vtkFloatArray::New();
    tmp->SetNumberOfTuples(ColorTextureMapSize*2);
    float* ptr = tmp->GetPointer(0);
    for (int i = 0; i < ColorTextureMapSize; ++i)
      {
      *ptr = range[0] + i * k;
      ++ptr;
      }
    // Dimension on NaN.
    double nan = vtkMath::Nan();
    for (int i = 0; i < ColorTextureMapSize; ++i)
      {
      *ptr = nan;
      ++ptr;
      }
    this->ColorTextureMap = vtkImageData::New();
    this->ColorTextureMap->SetExtent(0,ColorTextureMapSize-1,
                                     0,1, 0,0);
    this->ColorTextureMap->GetPointData()->SetScalars(
         this->LookupTable->MapScalars(tmp, this->ColorMode, 0));
    this->LookupTable->SetAlpha(orig_alpha);
    // Do we need to delete the scalars?
    this->ColorTextureMap->GetPointData()->GetScalars()->Delete();
    // Consistent register and unregisters
    this->ColorTextureMap->Register(this);
    this->ColorTextureMap->Delete();
    tmp->Delete();
    }

  // Create new coordinates if necessary.
  // Need to compare lookup table incase the range has changed.
  if (this->ColorCoordinates == 0 ||
      this->GetMTime() > this->ColorCoordinates->GetMTime() ||
      this->GetExecutive()->GetInputData(0, 0)->GetMTime() >
      this->ColorCoordinates->GetMTime() ||
      this->LookupTable->GetMTime() > this->ColorCoordinates->GetMTime())
    {
    // Get rid of old colors
    if ( this->ColorCoordinates )
      {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = 0;
      }

    // Now create the color texture coordinates.
    int numComps = scalars->GetNumberOfComponents();
    void* input = scalars->GetVoidPointer(0);
    vtkIdType num = scalars->GetNumberOfTuples();
    this->ColorCoordinates = vtkFloatArray::New();
    this->ColorCoordinates->SetNumberOfComponents(2);
    this->ColorCoordinates->SetNumberOfTuples(num);
    float* output = this->ColorCoordinates->GetPointer(0);
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
        CreateColorTextureCoordinates(static_cast<VTK_TT*>(input),
                                      output, num, numComps,
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

void vtkMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }

  os << indent << "Immediate Mode Rendering: "
    << (this->ImmediateModeRendering ? "On\n" : "Off\n");

   os << indent << "Force compile only for display lists: "
    << (this->ForceCompileOnly ? "On\n" : "Off\n");

  os << indent << "Global Immediate Mode Rendering: " <<
    (vtkMapperGlobalImmediateModeRendering ? "On\n" : "Off\n");

  os << indent << "Scalar Visibility: "
    << (this->ScalarVisibility ? "On\n" : "Off\n");

  os << indent << "Static: "
    << (this->Static ? "On\n" : "Off\n");

  double *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "UseLookupTableScalarRange: "
     << this->UseLookupTableScalarRange << "\n";

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;
  os << indent << "InterpolateScalarsBeforeMapping: "
     << (this->InterpolateScalarsBeforeMapping ? "On\n" : "Off\n");

  os << indent << "Scalar Mode: " << this->GetScalarModeAsString() << endl;

  os << indent << "LM Color Mode: "
     << this->GetScalarMaterialModeAsString() << endl;

  os << indent << "RenderTime: " << this->RenderTime << endl;

  os << indent << "Resolve Coincident Topology: ";
  if ( vtkMapperGlobalResolveCoincidentTopology == VTK_RESOLVE_OFF )
    {
    os << "Off" << endl;
    }
  else if ( vtkMapperGlobalResolveCoincidentTopology == VTK_RESOLVE_POLYGON_OFFSET )
    {
    os << "Polygon Offset" << endl;
    }
  else
    {
    os << "Shift Z-Buffer" << endl;
    }
}
