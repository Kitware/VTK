// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLabeledDataMapper.h"
#include "Private/vtkLabeledFormatter.h"

#include "vtkActor2D.h"
#include "vtkArrayDispatch.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN
class vtkLabeledDataMapper::Internals
{
public:
  std::map<int, vtkSmartPointer<vtkTextProperty>> TextProperties;
};

vtkStandardNewMacro(vtkLabeledDataMapper);

vtkCxxSetObjectMacro(vtkLabeledDataMapper, Transform, vtkTransform);

//------------------------------------------------------------------------------
// Creates a new label mapper

vtkLabeledDataMapper::vtkLabeledDataMapper()
  : Implementation(new Internals)
{
  this->Input = nullptr;
  this->LabelMode = VTK_LABEL_IDS;

  this->LabelFormat = nullptr;

  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
  this->FieldDataName = nullptr;

  this->NumberOfLabels = 0;
  this->NumberOfLabelsAllocated = 0;

  this->AllocateLabels(50);

  this->ComponentSeparator = ' ';

  auto prop = vtkSmartPointer<vtkTextProperty>::New();
  prop->SetFontSize(12);
  prop->SetBold(1);
  prop->SetItalic(1);
  prop->SetShadow(1);
  prop->SetFontFamilyToArial();
  this->Implementation->TextProperties[0] = prop;

  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "type");

  this->Transform = nullptr;
  this->CoordinateSystem = vtkLabeledDataMapper::WORLD;
}

//------------------------------------------------------------------------------
vtkLabeledDataMapper::~vtkLabeledDataMapper()
{
  this->SetLabelFormat(nullptr);
  this->LabelPositions.clear();
  this->TextMappers.clear();
  this->SetFieldDataName(nullptr);
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::AllocateLabels(int numLabels)
{
  if (numLabels > this->NumberOfLabelsAllocated)
  {
    // delete old stuff
    this->LabelPositions.clear();
    this->TextMappers.clear();

    this->NumberOfLabelsAllocated = numLabels;

    // Allocate and initialize new stuff
    this->LabelPositions.resize(this->NumberOfLabelsAllocated * 3, 0.0);
    this->TextMappers.resize(this->NumberOfLabelsAllocated, nullptr);
    for (int i = 0; i < this->NumberOfLabelsAllocated; i++)
    {
      this->TextMappers[i] = vtkSmartPointer<vtkTextMapper>::New();
    }
  }
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::SetLabelTextProperty(vtkTextProperty* prop, int type)
{
  this->Implementation->TextProperties[type] = prop;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkLabeledDataMapper::GetLabelTextProperty(int type)
{
  if (this->Implementation->TextProperties.find(type) != this->Implementation->TextProperties.end())
  {
    return this->Implementation->TextProperties[type];
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::SetInputData(vtkDataObject* input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet* vtkLabeledDataMapper::GetInput()
{
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this mapper.
void vtkLabeledDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->TextMappers.empty())
  {
    for (int i = 0; i < this->NumberOfLabelsAllocated; i++)
    {
      if (this->TextMappers[i])
      {
        this->TextMappers[i]->ReleaseGraphicsResources(win);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
{
  for (int i = 0; i < this->NumberOfLabels; i++)
  {
    double x[3];
    x[0] = this->LabelPositions[3 * i];
    x[1] = this->LabelPositions[3 * i + 1];
    x[2] = this->LabelPositions[3 * i + 2];

    double* pos = x;
    if (this->Transform)
    {
      pos = this->Transform->TransformDoublePoint(x);
    }

    if (this->CoordinateSystem == vtkLabeledDataMapper::WORLD)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(pos);
    }
    else if (this->CoordinateSystem == vtkLabeledDataMapper::DISPLAY)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      actor->GetPositionCoordinate()->SetValue(pos);
    }

    bool show = true;
    if (this->ClippingPlanes)
    {
      for (int p = 0; p < this->GetNumberOfClippingPlanes(); ++p)
      {
        if (this->ClippingPlanes->GetItem(p)->FunctionValue(pos) < 0.0)
        {
          show = false;
        }
      }
    }
    if (show)
    {
      this->TextMappers[i]->RenderOverlay(viewport, actor);
    }
  }
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor)
{
  vtkTextProperty* tprop = this->Implementation->TextProperties[0];
  if (!tprop)
  {
    vtkErrorMacro(<< "Need default text property to render labels");
    return;
  }

  // Updates the input pipeline if needed.
  this->Update();

  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  if (!inputDO)
  {
    this->NumberOfLabels = 0;
    vtkErrorMacro(<< "Need input data to render labels (2)");
    return;
  }

  // Check for property updates.
  vtkMTimeType propMTime = 0;
  std::map<int, vtkSmartPointer<vtkTextProperty>>::iterator it, itEnd;
  it = this->Implementation->TextProperties.begin();
  itEnd = this->Implementation->TextProperties.end();
  for (; it != itEnd; ++it)
  {
    vtkTextProperty* prop = it->second;
    if (prop && prop->GetMTime() > propMTime)
    {
      propMTime = prop->GetMTime();
    }
  }

  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime || inputDO->GetMTime() > this->BuildTime ||
    propMTime > this->BuildTime)
  {
    this->BuildLabels();
  }

  for (int i = 0; i < this->NumberOfLabels; i++)
  {
    double* pos = &this->LabelPositions[3 * i];
    if (this->Transform)
    {
      pos = this->Transform->TransformDoublePoint(pos);
    }

    if (this->CoordinateSystem == vtkLabeledDataMapper::WORLD)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(pos);
    }
    else if (this->CoordinateSystem == vtkLabeledDataMapper::DISPLAY)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      actor->GetPositionCoordinate()->SetValue(pos);
    }
    bool show = true;
    if (this->ClippingPlanes)
    {
      for (int p = 0; p < this->GetNumberOfClippingPlanes(); ++p)
      {
        if (this->ClippingPlanes->GetItem(p)->FunctionValue(pos) < 0.0)
        {
          show = false;
        }
      }
    }
    if (show)
    {
      this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
    }
  }
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::BuildLabels()
{
  vtkDebugMacro(<< "Rebuilding labels");
  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(inputDO);
  if (ds)
  {
    this->AllocateLabels(ds->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    this->BuildLabelsInternal(ds);
  }
  else if (cd)
  {
    this->AllocateLabels(cd->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        this->BuildLabelsInternal(ds);
      }
    }
    iter->Delete();
  }
  else
  {
    vtkErrorMacro("Unsupported data type: " << inputDO->GetClassName());
  }

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
struct vtkLabeledDataMapper::vtkLabeledDataMapperFormatter : vtkLabeledFormatterInterface
{
  vtkDataSet* Input;

  vtkLabeledDataMapperFormatter(
    vtkLabeledDataMapper* self, vtkIntArray* typeArr, vtkDataSet* input, int numCurLabels)
    : vtkLabeledFormatterInterface(self, typeArr, numCurLabels)
    , Input(input)
  {
  }

  void SetFormattedString(int i, const char* resultString) override
  {
    this->Self->TextMappers[i + this->Self->NumberOfLabels]->SetInput(resultString);

    // Find the correct property type
    int type = 0;
    if (this->TypeArr)
    {
      type = this->TypeArr->GetValue(i);
    }
    vtkTextProperty* prop = this->Self->Implementation->TextProperties[type];
    if (!prop)
    {
      prop = this->Self->Implementation->TextProperties[0];
    }
    this->Self->TextMappers[i + this->Self->NumberOfLabels]->SetTextProperty(prop);

    double x[3];
    this->Input->GetPoint(i, x);
    this->Self->LabelPositions[3 * (i + this->Self->NumberOfLabels)] = x[0];
    this->Self->LabelPositions[3 * (i + this->Self->NumberOfLabels) + 1] = x[1];
    this->Self->LabelPositions[3 * (i + this->Self->NumberOfLabels) + 2] = x[2];
  }
};

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::BuildLabelsInternal(vtkDataSet* input)
{
  auto formatterInput =
    this->ResolveLabeledFormatterInput(input->GetPointData(), input->GetNumberOfPoints(), input);
  if (!formatterInput.Valid)
  {
    return;
  }
  if (this->NumberOfLabelsAllocated < (this->NumberOfLabels + formatterInput.NumCurLabels))
  {
    vtkErrorMacro("Number of labels must be allocated before this method is called.");
    return;
  }

  vtkLabeledDataMapperFormatter formatter(
    this, formatterInput.TypeArr, input, formatterInput.NumCurLabels);
  formatter.Dispatch(formatterInput);
  this->NumberOfLabels += formatterInput.NumCurLabels;
}

//------------------------------------------------------------------------------
vtkLabeledFormatterInput vtkLabeledDataMapper::ResolveLabeledFormatterInput(
  vtkDataSetAttributes* pd, int numItems, vtkDataObject* inputObj)
{
  vtkLabeledFormatterInput setup;

  if (numItems == 0)
  {
    setup.Valid = false;
    return setup;
  }

  setup.TypeArr = vtkArrayDownCast<vtkIntArray>(this->GetInputAbstractArrayToProcess(0, inputObj));
  setup.NumCurLabels = numItems;

  vtkAbstractArray* abstractData = nullptr;
  switch (this->LabelMode)
  {
    case VTK_LABEL_IDS:
      setup.PointIdLabels = true;
      break;
    case VTK_LABEL_SCALARS:
      if (pd->GetScalars())
      {
        setup.NumericData = pd->GetScalars();
      }
      break;
    case VTK_LABEL_VECTORS:
      if (pd->GetVectors())
      {
        setup.NumericData = pd->GetVectors();
      }
      break;
    case VTK_LABEL_NORMALS:
      if (pd->GetNormals())
      {
        setup.NumericData = pd->GetNormals();
      }
      break;
    case VTK_LABEL_TCOORDS:
      if (pd->GetTCoords())
      {
        setup.NumericData = pd->GetTCoords();
      }
      break;
    case VTK_LABEL_TENSORS:
      if (pd->GetTensors())
      {
        setup.NumericData = pd->GetTensors();
      }
      break;
    case VTK_LABEL_FIELD_DATA:
    {
      int arrayNum;
      if (this->FieldDataName != nullptr)
      {
        vtkDebugMacro(<< "Labeling field data array " << this->FieldDataName);
        abstractData = pd->GetAbstractArray(this->FieldDataName, arrayNum);
      }
      else
      {
        arrayNum = (this->FieldDataArray < pd->GetNumberOfArrays() ? this->FieldDataArray
                                                                   : pd->GetNumberOfArrays() - 1);
        abstractData = pd->GetAbstractArray(arrayNum);
      }
      setup.NumericData = vtkArrayDownCast<vtkDataArray>(abstractData);
      setup.StringData = vtkArrayDownCast<vtkStringArray>(abstractData);
    }
    break;
  }

  if (setup.PointIdLabels)
  {
    setup.NumComp = 1;
  }
  else if (setup.NumericData)
  {
    setup.NumComp = setup.NumericData->GetNumberOfComponents();
    if (this->LabeledComponent >= 0)
    {
      setup.ActiveComp =
        (this->LabeledComponent < setup.NumComp ? this->LabeledComponent : setup.NumComp - 1);
      setup.NumComp = 1;
    }
  }
  else
  {
    if (setup.StringData)
    {
      setup.NumComp = setup.StringData->GetNumberOfComponents();
    }
    else
    {
      if (this->FieldDataName)
      {
        vtkWarningMacro(<< "Could not find label array (" << this->FieldDataName << ") "
                        << "in input.");
      }
      else
      {
        vtkWarningMacro(<< "Could not find label array ("
                        << "index " << this->FieldDataArray << ") "
                        << "in input.");
      }
      setup.Valid = false;
      return setup;
    }
  }

  if (this->LabelFormat && !std::string_view(this->LabelFormat).empty())
  {
    vtkDebugMacro(<< "Using user-specified format string " << this->LabelFormat);
    setup.FormatString = vtk::to_std_format(this->LabelFormat);
  }
  else
  {
    if (setup.PointIdLabels)
    {
      setup.FormatString = "{:d}";
    }
    else if (setup.NumericData)
    {
      switch (setup.NumericData->GetDataType())
      {
        case VTK_VOID:
          setup.FormatString = "0x{:x}";
          break;
        case VTK_BIT:
        case VTK_SHORT:
        case VTK_UNSIGNED_SHORT:
        case VTK_INT:
        case VTK_UNSIGNED_INT:
          setup.FormatString = "{:d}";
          break;
        case VTK_CHAR:
        case VTK_SIGNED_CHAR:
        case VTK_UNSIGNED_CHAR:
          setup.FormatString = "{:c}";
          break;
        case VTK_LONG:
        case VTK_UNSIGNED_LONG:
        case VTK_ID_TYPE:
        case VTK_LONG_LONG:
        case VTK_UNSIGNED_LONG_LONG:
          setup.FormatString = "{:d}";
          break;
        case VTK_FLOAT:
        case VTK_DOUBLE:
          setup.FormatString = "{:f}";
          break;
        default:
          setup.FormatString = "BUG - UNKNOWN DATA FORMAT";
          break;
      }
    }
    else if (setup.StringData)
    {
      setup.FormatString = "";
    }
    else
    {
      setup.FormatString = "BUG - COULDN'T DETECT DATA TYPE";
    }
    vtkDebugMacro(<< "Using default format string " << setup.FormatString);
  }

  return setup;
}

//------------------------------------------------------------------------------
int vtkLabeledDataMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // Can handle composite datasets.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
void vtkLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input: (" << this->Input << ")\n";
  }
  else
  {
    os << indent << "Input: (none)\n";
  }

  std::map<int, vtkSmartPointer<vtkTextProperty>>::iterator it, itEnd;
  it = this->Implementation->TextProperties.begin();
  itEnd = this->Implementation->TextProperties.end();
  for (; it != itEnd; ++it)
  {
    vtkTextProperty* prop = it->second;
    if (prop)
    {
      os << indent << "LabelTextProperty " << it->first << ":\n";
      prop->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << indent << "LabelTextProperty " << it->first << ": (none)\n";
    }
  }

  os << indent << "Label Mode: ";
  if (this->LabelMode == VTK_LABEL_IDS)
  {
    os << "Label Ids\n";
  }
  else if (this->LabelMode == VTK_LABEL_SCALARS)
  {
    os << "Label Scalars\n";
  }
  else if (this->LabelMode == VTK_LABEL_VECTORS)
  {
    os << "Label Vectors\n";
  }
  else if (this->LabelMode == VTK_LABEL_NORMALS)
  {
    os << "Label Normals\n";
  }
  else if (this->LabelMode == VTK_LABEL_TCOORDS)
  {
    os << "Label TCoords\n";
  }
  else if (this->LabelMode == VTK_LABEL_TENSORS)
  {
    os << "Label Tensors\n";
  }
  else
  {
    os << "Label Field Data\n";
  }

  os << indent << "Label Format: " << (this->LabelFormat ? this->LabelFormat : "Null") << "\n";

  os << indent << "Labeled Component: ";
  if (this->LabeledComponent < 0)
  {
    os << "(All Components)\n";
  }
  else
  {
    os << this->LabeledComponent << "\n";
  }

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";
  os << indent << "Field Data Name: " << (this->FieldDataName ? this->FieldDataName : "Null")
     << "\n";

  os << indent << "Transform: " << (this->Transform ? "" : "(none)") << endl;
  if (this->Transform)
  {
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "CoordinateSystem: " << this->CoordinateSystem << endl;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkLabeledDataMapper::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  std::map<int, vtkSmartPointer<vtkTextProperty>>::iterator it, itEnd;
  it = this->Implementation->TextProperties.begin();
  itEnd = this->Implementation->TextProperties.end();
  for (; it != itEnd; ++it)
  {
    vtkTextProperty* p = it->second;
    vtkMTimeType curMTime = p->GetMTime();
    mtime = std::max(curMTime, mtime);
  }
  return mtime;
}

//------------------------------------------------------------------------------
const char* vtkLabeledDataMapper::GetLabelText(int label)
{
  assert("label index range" && label >= 0 && label < this->NumberOfLabels);
  if (!this->TextMappers.empty())
  {
    return this->TextMappers[label]->GetInput();
  }
  return nullptr;
}
VTK_ABI_NAMESPACE_END
