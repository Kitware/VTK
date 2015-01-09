/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataMapper2D.h"

#include "vtkAbstractArray.h"
#include "vtkColorSeries.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkExecutive.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkPolyData.h"
#include "vtkVariantArray.h"

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkPolyDataMapper2D)
//----------------------------------------------------------------------------

vtkCxxSetObjectMacro(vtkPolyDataMapper2D, TransformCoordinate, vtkCoordinate);

vtkPolyDataMapper2D::vtkPolyDataMapper2D()
{
  this->Colors = NULL;
  this->LookupTable = NULL;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->UseLookupTableScalarRange = 0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;

  this->TransformCoordinate = NULL;
  this->TransformCoordinateUseDouble = false;

  strcpy(this->ArrayName, "");
  this->ArrayId = -1;
  this->ArrayComponent = 0;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkPolyDataMapper2D *m = vtkPolyDataMapper2D::SafeDownCast(mapper);
  if (m != NULL)
    {
    this->SetLookupTable(m->GetLookupTable());
    this->SetScalarVisibility(m->GetScalarVisibility());
    this->SetScalarRange(m->GetScalarRange());
    this->SetColorMode(m->GetColorMode());
    this->SetScalarMode(m->GetScalarMode());
    this->SetUseLookupTableScalarRange(m->GetUseLookupTableScalarRange());
    this->ColorByArrayComponent(m->GetArrayName(),m->GetArrayComponent());
    this->ColorByArrayComponent(m->GetArrayId(),m->GetArrayComponent());
    this->SetTransformCoordinate(m->GetTransformCoordinate());
    }

  // Now do superclass
  this->vtkMapper2D::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
vtkPolyDataMapper2D::~vtkPolyDataMapper2D()
{
  if (this->TransformCoordinate)
    {
    this->TransformCoordinate->UnRegister(this);
    }
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if (this->Colors != NULL)
    {
    this->Colors->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::SetInputData(vtkPolyData *input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPolyData *vtkPolyDataMapper2D::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If lookup table is modified,
// then this object is modified as well.
unsigned long vtkPolyDataMapper2D::GetMTime()
{
  unsigned long mTime = this->MTime;
  unsigned long lutMTime;

  if (this->LookupTable != NULL)
    {
    lutMTime = this->LookupTable->GetMTime();
    mTime = ( lutMTime > mTime ? lutMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
// A side effect of this is that this->Colors is also set
// to the return value
vtkUnsignedCharArray *vtkPolyDataMapper2D::MapScalars(double alpha)
{
  // Since we are not supporting the texture color option in this mapper,
  // the cell flag does nothing.
  int cellFlag = 0;

  // Get rid of old colors
  if (this->Colors)
    {
    this->Colors->UnRegister(this);
    this->Colors = NULL;
    }

  // map scalars if necessary
  if (this->ScalarVisibility)
    {
    vtkAbstractArray *scalars = vtkAbstractMapper::
      GetAbstractScalars(this->GetInput(), this->ScalarMode, this->ArrayAccessMode,
                         this->ArrayId, this->ArrayName, cellFlag);
    // This is for a legacy feature: selection of the array component to color by
    // from the mapper.  It is now in the lookuptable.  When this feature
    // is removed, we can remove this condition.
    if (scalars == NULL || scalars->GetNumberOfComponents() <= this->ArrayComponent)
      {
      this->ArrayComponent = 0;
      }

    if (scalars)
      {
      vtkDataArray *dataArray = vtkDataArray::SafeDownCast(scalars);
      if (dataArray && dataArray->GetLookupTable())
        {
        this->SetLookupTable(dataArray->GetLookupTable());
        }
      else
        {
        // make sure we have a lookup table
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
      this->LookupTable->SetAlpha(alpha);
      // Map Scalar constructs a array and returns it.
      // Not having "New" or "Make" in the name breaks VTK conventions but, ...
      this->Colors = this->LookupTable->
        MapScalars(scalars, this->ColorMode, this->ArrayComponent);
      this->Colors->Register(this);
      this->Colors->Delete();
      }
    }

  return this->Colors;
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::ColorByArrayComponent(int arrayNum, int component)
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

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::ColorByArrayComponent(char* arrayName, int component)
{
  if (strcmp(this->ArrayName, arrayName) == 0 &&
      component == this->ArrayComponent &&
      this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
    return;
    }
  this->Modified();

  strcpy(this->ArrayName, arrayName);
  this->ArrayComponent = component;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_NAME;
}

//----------------------------------------------------------------------------
// Specify a lookup table for the mapper to use.
void vtkPolyDataMapper2D::SetLookupTable(vtkScalarsToColors *lut)
{
  if (this->LookupTable != lut)
    {
    if (lut)
      {
      lut->Register(this);
      }
    if (this->LookupTable)
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkScalarsToColors* vtkPolyDataMapper2D::GetLookupTable()
{
  if (this->LookupTable == NULL)
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::CreateDefaultLookupTable()
{
  if (this->LookupTable)
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

//----------------------------------------------------------------------------
// Return the method of coloring scalar data.
const char *vtkPolyDataMapper2D::GetColorModeAsString(void)
{
  return (this->ColorMode == VTK_COLOR_MODE_MAP_SCALARS) ? "MapScalars" : "Default";
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->LookupTable)
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }

  os << indent << "Scalar Visibility: "
    << (this->ScalarVisibility ? "On\n" : "Off\n");

  os << indent << "Scalar Mode: ";
  switch ( this->ScalarMode )
    {
    case VTK_SCALAR_MODE_DEFAULT:
      os << "Default" << endl;
      break;
    case VTK_SCALAR_MODE_USE_POINT_DATA:
      os << "Use point data" << endl;
      break;
    case VTK_SCALAR_MODE_USE_CELL_DATA:
      os << "Use cell data" << endl;
      break;
    case VTK_SCALAR_MODE_USE_POINT_FIELD_DATA:
      os << "Use point field data" << endl;
      break;
    case VTK_SCALAR_MODE_USE_CELL_FIELD_DATA:
      os << "Use cell field data" << endl;
      break;
    }

  double *range = this->GetScalarRange();
  os << indent << "Scalar Range: ("
    << range[0] << ", "
    << range[1] << ")\n";
  os << indent << "UseLookupTableScalarRange: " <<
    this->UseLookupTableScalarRange << "\n";

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

  if (this->TransformCoordinate)
    {
    os << indent << "Transform Coordinate: "
       << this->TransformCoordinate << "\n";
    this->TransformCoordinate->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "No Transform Coordinate\n";
    }
  os << indent << "Transform Coordinate use double: "
     << (this->TransformCoordinateUseDouble  ? "True\n" : "False\n") << "\n";
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::SetColorModeToDefault()
{
  this->SetColorMode(VTK_COLOR_MODE_DEFAULT);
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::SetColorModeToMapScalars()
{
  this->SetColorMode(VTK_COLOR_MODE_MAP_SCALARS);
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper2D::SetColorModeToDirectScalars()
{
  this->SetColorMode(VTK_COLOR_MODE_DIRECT_SCALARS);
}


//----------------------------------------------------------------------------
int vtkPolyDataMapper2D::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
