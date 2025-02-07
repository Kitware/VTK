// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMatplotlibMathTextUtilities.h"
#include "vtkPython.h" // must be the first thing that's included.
#include "vtkPythonCompatibility.h"

#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkImageReslice.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPoints.h"
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif
#include "vtkSmartPyObject.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

#include <sstream>

#include <vtksys/SystemTools.hxx>

#include <numeric>
#include <vector>

// We need to define Py_ssize_t for older python API version
#if PYTHON_API_VERSION < 1013
// Taken from pyport.h
#ifdef HAVE_SSIZE_T
typedef ssize_t Py_ssize_t;
#elif SIZEOF_VOID_P == SIZEOF_SIZE_T
typedef Py_intptr_t Py_ssize_t;
#else
#error "Python needs a typedef for Py_ssize_t."
#endif
#endif

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkMatplotlibMathTextUtilities::Availability vtkMatplotlibMathTextUtilities::MPLMathTextAvailable =
  vtkMatplotlibMathTextUtilities::NOT_TESTED;

// A macro that is used in New() to print warnings if VTK_MATPLOTLIB_DEBUG
// is defined in the environment. Use vtkGenericWarningMacro to allow this to
// work in release mode builds.
#define vtkMplStartUpDebugMacro(x)                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (debug)                                                                                     \
    {                                                                                              \
      vtkGenericWarningMacro(x);                                                                   \
    }                                                                                              \
  } while (false)

vtkObjectFactoryNewMacro(vtkMatplotlibMathTextUtilities);

//------------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::Availability vtkMatplotlibMathTextUtilities::CheckMPLAvailability()
{
  if (vtkMatplotlibMathTextUtilities::MPLMathTextAvailable != NOT_TESTED)
  {
    // Already tested. Nothing to do now.
    return vtkMatplotlibMathTextUtilities::MPLMathTextAvailable;
  }

  // Enable startup debugging output. This will be set to true when
  // VTK_MATPLOTLIB_DEBUG is defined in the process environment.
  bool debug = (vtksys::SystemTools::GetEnv("VTK_MATPLOTLIB_DEBUG") != nullptr);

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (!Py_IsInitialized())
  {
    // Initialize the python interpreter if needed
    vtkMplStartUpDebugMacro("Initializing Python, if not already.");
    vtkPythonInterpreter::Initialize();
  }
#endif
  if (!Py_IsInitialized())
  {
    // Don't store the result; it might be available if Python is initialized
    // elsewhere later.
    vtkMplStartUpDebugMacro("Python is not available.");
    return UNAVAILABLE;
  }

  vtkMplStartUpDebugMacro("Attempting to import matplotlib.");

  vtkPythonScopeGilEnsurer gilEnsurer;
  if (PyErr_Occurred() || !PyImport_ImportModule("matplotlib") || PyErr_Occurred())
  {
    // FIXME: Check if we need this. Wouldn't pipe-ing the stdout/stderr make
    // this unnecessary?

    // Fetch the exception info. Note that value and traceback may still be
    // nullptr after the call to PyErr_Fetch().
    PyObject* type = nullptr;
    PyObject* value = nullptr;
    PyObject* traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    vtkSmartPyObject tracebackStr;
    if (traceback)
    {
      vtkSmartPyObject tb_module = PyImport_ImportModule("traceback");
      if (tb_module)
      {
        vtkSmartPyObject format_tb = PyObject_GetAttrString(tb_module, "format_tb");
        if (format_tb)
        {
          vtkSmartPyObject tracebacklist =
            PyObject_CallFunction(format_tb, const_cast<char*>("O"), traceback);
          Py_ssize_t tbsz = PySequence_Length(tracebacklist);
          tracebackStr = PyUnicode_FromString("");
          for (Py_ssize_t i = 0; i < tbsz; ++i)
          {
            vtkSmartPyObject item = PySequence_GetItem(tracebacklist, i);
            if (!item)
            {
              continue;
            }
            tracebackStr = PyUnicode_Concat(tracebackStr, item);
            if (!tracebackStr)
            {
              break;
            }
          }
        }
      }
    }
    if (!tracebackStr)
    {
      tracebackStr = PyObject_Str(traceback);
    }
    vtkSmartPyObject typeStr(PyObject_Str(type));
    vtkSmartPyObject valueStr(PyObject_Str(value));
    vtkMplStartUpDebugMacro("Error during matplotlib import:\n"
      << "\nStack:\n"
      << (tracebackStr ? const_cast<char*>(PyUnicode_AsUTF8(tracebackStr)) : "(none)")
      << "\nValue:\n"
      << (valueStr ? const_cast<char*>(PyUnicode_AsUTF8(valueStr)) : "(none)") << "\nType:\n"
      << (typeStr ? const_cast<char*>(PyUnicode_AsUTF8(typeStr)) : "(none)"));
    PyErr_Clear();
    vtkMatplotlibMathTextUtilities::MPLMathTextAvailable = UNAVAILABLE;
  }
  else
  {
    vtkMplStartUpDebugMacro("Successfully imported matplotlib.");
    vtkMatplotlibMathTextUtilities::MPLMathTextAvailable = AVAILABLE;
  }

  return vtkMatplotlibMathTextUtilities::MPLMathTextAvailable;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::IsAvailable()
{
  return this->CheckMPLAvailability() == AVAILABLE;
}

//------------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::vtkMatplotlibMathTextUtilities()
  : MaskParser(nullptr)
  , PathParser(nullptr)
  , FontPropertiesClass(nullptr)
  , ScaleToPowerOfTwo(true)
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  this->Interpreter = vtkPythonInterpreter::New();
  this->Interpreter->AddObserver(
    vtkCommand::ExitEvent, this, &vtkMatplotlibMathTextUtilities::CleanupPythonObjects);
#else
  this->Interpreter = nullptr;
#endif
}

//------------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::~vtkMatplotlibMathTextUtilities()
{
  this->CleanupPythonObjects();
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (this->Interpreter)
  {
    this->Interpreter->Delete();
  }
#endif
}

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::CleanupPythonObjects()
{
  if (Py_IsInitialized())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->MaskParser);
    Py_XDECREF(this->PathParser);
    Py_XDECREF(this->FontPropertiesClass);

    this->MaskParser = nullptr;
    this->PathParser = nullptr;
    this->FontPropertiesClass = nullptr;
  }
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializeMaskParser()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (!Py_IsInitialized())
  {
    // ensure that Python is initialized.
    vtkPythonInterpreter::Initialize();
  }
#endif
  if (!Py_IsInitialized())
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplMathTextLib(PyImport_ImportModule("matplotlib.mathtext"));
  if (this->CheckForError(mplMathTextLib))
  {
    return false;
  }

  vtkSmartPyObject mathTextParserClass(PyObject_GetAttrString(mplMathTextLib, "MathTextParser"));
  if (this->CheckForError(mathTextParserClass))
  {
    return false;
  }

  this->MaskParser = PyObject_CallFunction(mathTextParserClass, const_cast<char*>("s"), "agg");
  if (this->CheckForError(this->MaskParser))
  {
    Py_CLEAR(this->MaskParser);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializePathParser()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (!Py_IsInitialized())
  {
    // ensure that Python is initialized.
    vtkPythonInterpreter::Initialize();
  }
#endif
  if (!Py_IsInitialized())
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplTextPathLib(PyImport_ImportModule("matplotlib.textpath"));
  if (this->CheckForError(mplTextPathLib))
  {
    return false;
  }

  vtkSmartPyObject textToPathClass(PyObject_GetAttrString(mplTextPathLib, "TextToPath"));
  if (this->CheckForError(textToPathClass))
  {
    return false;
  }

  this->PathParser = PyObject_CallFunction(textToPathClass, nullptr);
  if (this->CheckForError(this->PathParser))
  {
    Py_CLEAR(this->PathParser);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializeFontPropertiesClass()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (!Py_IsInitialized())
  {
    // ensure that Python is initialized.
    vtkPythonInterpreter::Initialize();
  }
#endif
  if (!Py_IsInitialized())
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplFontManagerLib(PyImport_ImportModule("matplotlib.font_manager"));
  if (this->CheckForError(mplFontManagerLib))
  {
    return false;
  }

  this->FontPropertiesClass = PyObject_GetAttrString(mplFontManagerLib, "FontProperties");
  if (this->CheckForError(this->FontPropertiesClass))
  {
    Py_CLEAR(this->FontPropertiesClass);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::CheckForError()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  PyObject* exception = PyErr_Occurred();
  if (exception)
  {
    if (this->Debug)
    {
      // Fetch the exception info. Note that value and traceback may still be
      // nullptr after the call to PyErr_Fetch().
      PyObject* type = nullptr;
      PyObject* value = nullptr;
      PyObject* traceback = nullptr;
      PyErr_Fetch(&type, &value, &traceback);
      vtkSmartPyObject tracebackStr;
      if (traceback)
      {
        vtkSmartPyObject tb_module = PyImport_ImportModule("traceback");
        if (tb_module)
        {
          vtkSmartPyObject format_tb = PyObject_GetAttrString(tb_module, "format_tb");
          if (format_tb)
          {
            vtkSmartPyObject tracebacklist =
              PyObject_CallFunction(format_tb, const_cast<char*>("O"), traceback);
            Py_ssize_t tbsz = PySequence_Length(tracebacklist);
            tracebackStr = PyUnicode_FromString("");
            for (Py_ssize_t i = 0; i < tbsz; ++i)
            {
              vtkSmartPyObject item = PySequence_GetItem(tracebacklist, i);
              if (!item)
              {
                continue;
              }
              tracebackStr = PyUnicode_Concat(tracebackStr, item);
              if (!tracebackStr)
              {
                break;
              }
            }
          }
        }
      }
      if (!tracebackStr)
      {
        tracebackStr = PyObject_Str(traceback);
      }
      vtkSmartPyObject typeStr(PyObject_Str(type));
      vtkSmartPyObject valueStr(PyObject_Str(value));
      vtkWarningMacro(<< "Python exception raised:\n"
                      << "\nStack:\n"
                      << (tracebackStr ? const_cast<char*>(PyUnicode_AsUTF8(tracebackStr))
                                       : "(none)")
                      << "\nValue:\n"
                      << (valueStr ? const_cast<char*>(PyUnicode_AsUTF8(valueStr)) : "(none)")
                      << "\nType:\n"
                      << (typeStr ? const_cast<char*>(PyUnicode_AsUTF8(typeStr)) : "(none)"));
    }
    PyErr_Clear();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::CheckForError(PyObject* object)
{
  // Print any exceptions
  bool result = this->CheckForError();

  if (object == nullptr)
  {
    vtkDebugMacro(<< "Object is nullptr!");
    return true;
  }
  return result;
}

//------------------------------------------------------------------------------
PyObject* vtkMatplotlibMathTextUtilities::GetFontProperties(vtkTextProperty* tprop)
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<< "Matplotlib rendering is unavailable.");
    return nullptr;
  }

  if (!this->FontPropertiesClass)
  {
    if (!this->InitializeFontPropertiesClass())
    {
      vtkErrorMacro(<< "FontPropertiesClass is not initialized!");
      return nullptr;
    }
  }

  char tpropFamily[16];
  char tpropStyle[16];
  char tpropWeight[16];
  long tpropFontSize;

  switch (tprop->GetFontFamily())
  {
    default:
    case VTK_ARIAL:
      strcpy(tpropFamily, "sans-serif");
      break;
    case VTK_COURIER:
      strcpy(tpropFamily, "monospace");
      break;
    case VTK_TIMES:
      strcpy(tpropFamily, "serif");
      break;
  }

  if (tprop->GetItalic())
  {
    strcpy(tpropStyle, "italic");
  }
  else
  {
    strcpy(tpropStyle, "normal");
  }

  if (tprop->GetBold())
  {
    strcpy(tpropWeight, "bold");
  }
  else
  {
    strcpy(tpropWeight, "normal");
  }

  tpropFontSize = tprop->GetFontSize();

  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject kwargs = PyDict_New();
#define kwargs_insert(dict, key, value)                                                            \
  do                                                                                               \
  {                                                                                                \
    if (PyDict_SetItemString(dict, key, value))                                                    \
    {                                                                                              \
      vtkErrorMacro(<< "Failed to set the \"" key "\" argument to `FontProperties`");              \
      return nullptr;                                                                              \
    }                                                                                              \
  } while (false)
#define kwarg_ctor_check(obj)                                                                      \
  do                                                                                               \
  {                                                                                                \
    if (!obj)                                                                                      \
    {                                                                                              \
      vtkErrorMacro(<< "Failed to construct the \"" #obj "\" argument");                           \
      return nullptr;                                                                              \
    }                                                                                              \
  } while (false)

  vtkSmartPyObject kwFamily = PyUnicode_FromString(tpropFamily);
  kwarg_ctor_check(kwFamily);
  kwargs_insert(kwargs, "family", kwFamily);
  vtkSmartPyObject kwStyle = PyUnicode_FromString(tpropStyle);
  kwarg_ctor_check(kwStyle);
  kwargs_insert(kwargs, "style", kwStyle);
  vtkSmartPyObject kwVariant = PyUnicode_FromString("normal");
  kwarg_ctor_check(kwVariant);
  kwargs_insert(kwargs, "variant", kwVariant);
  vtkSmartPyObject kwWeight = PyUnicode_FromString(tpropWeight);
  kwarg_ctor_check(kwWeight);
  kwargs_insert(kwargs, "weight", kwWeight);
  vtkSmartPyObject kwStretch = PyUnicode_FromString("normal");
  kwarg_ctor_check(kwStretch);
  kwargs_insert(kwargs, "stretch", kwStretch);
  vtkSmartPyObject kwFontSize = PyLong_FromLong(tpropFontSize);
  kwarg_ctor_check(kwFontSize);
  kwargs_insert(kwargs, "size", kwFontSize);

  vtkSmartPyObject args = PyTuple_New(0);
  kwarg_ctor_check(args);
#undef kwarg_ctor_check
#undef kwargs_insert

  return PyObject_Call(this->FontPropertiesClass, args, kwargs);
}

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::GetJustifiedBBox(
  int rows, int cols, vtkTextProperty* tprop, int bbox[4])
{
  bbox[0] = 0;
  bbox[1] = cols - 1;
  bbox[2] = 0;
  bbox[3] = rows - 1;

  int justifyOffset[2];
  switch (tprop->GetJustification())
  {
    default:
    case VTK_TEXT_LEFT:
      justifyOffset[0] = 0;
      break;
    case VTK_TEXT_CENTERED:
      justifyOffset[0] = bbox[1] / 2;
      break;
    case VTK_TEXT_RIGHT:
      justifyOffset[0] = bbox[1];
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    default:
    case VTK_TEXT_BOTTOM:
      justifyOffset[1] = 0;
      break;
    case VTK_TEXT_CENTERED:
      justifyOffset[1] = bbox[3] / 2;
      break;
    case VTK_TEXT_TOP:
      justifyOffset[1] = bbox[3];
      break;
  }

  bbox[0] -= justifyOffset[0];
  bbox[1] -= justifyOffset[0];
  bbox[2] -= justifyOffset[1];
  bbox[3] -= justifyOffset[1];
}

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::RotateCorners(
  double angleDeg, double corners[4][2], double bbox[4])
{
  double angleRad = vtkMath::RadiansFromDegrees(angleDeg);
  double c = cos(angleRad);
  double s = sin(angleRad);
  // Rotate corners
  for (int i = 0; i < 4; ++i)
  {
    int newpt[2];
    newpt[0] = c * corners[i][0] - s * corners[i][1];
    newpt[1] = s * corners[i][0] + c * corners[i][1];
    corners[i][0] = newpt[0];
    corners[i][1] = newpt[1];
  }
  // Find new bounds
  bbox[0] = VTK_INT_MAX;
  bbox[1] = VTK_INT_MIN;
  bbox[2] = VTK_INT_MAX;
  bbox[3] = VTK_INT_MIN;
  for (int i = 0; i < 4; ++i)
  {
    if (corners[i][0] < bbox[0])
    {
      bbox[0] = corners[i][0];
    }
    if (corners[i][0] > bbox[1])
    {
      bbox[1] = corners[i][0];
    }
    if (corners[i][1] < bbox[2])
    {
      bbox[2] = corners[i][1];
    }
    if (corners[i][1] > bbox[3])
    {
      bbox[3] = corners[i][1];
    }
  }
}

//------------------------------------------------------------------------------
// This is more or less ported from vtkFreeTypeTools.
bool vtkMatplotlibMathTextUtilities::PrepareImageData(vtkImageData* data, int textBbox[4])
{
  // Calculate the bbox's dimensions
  int textDims[2];
  textDims[0] = (textBbox[1] - textBbox[0] + 1);
  textDims[1] = (textBbox[3] - textBbox[2] + 1);

  // Calculate the size the image needs to be.
  int targetDims[3];
  targetDims[0] = textDims[0];
  targetDims[1] = textDims[1];
  targetDims[2] = 1;
  // Scale to the next highest power of 2 if required.
  if (this->ScaleToPowerOfTwo)
  {
    targetDims[0] = vtkMath::NearestPowerOfTwo(targetDims[0]);
    targetDims[1] = vtkMath::NearestPowerOfTwo(targetDims[1]);
  }

  // Calculate the target extent of the image.
  int targetExtent[6];
  targetExtent[0] = textBbox[0];
  targetExtent[1] = textBbox[0] + targetDims[0] - 1;
  targetExtent[2] = textBbox[2];
  targetExtent[3] = textBbox[2] + targetDims[1] - 1;
  targetExtent[4] = 0;
  targetExtent[5] = 0;

  // Get the actual image extents and increments
  int imageExtent[6];
  double imageSpacing[3];
  data->GetExtent(imageExtent);
  data->GetSpacing(imageSpacing);

  // Do we need to reallocate the image memory?
  if (data->GetScalarType() != VTK_UNSIGNED_CHAR || data->GetNumberOfScalarComponents() != 4 ||
    imageExtent[0] != targetExtent[0] || imageExtent[1] != targetExtent[1] ||
    imageExtent[2] != targetExtent[2] || imageExtent[3] != targetExtent[3] ||
    imageExtent[4] != targetExtent[4] || imageExtent[5] != targetExtent[5] ||
    fabs(imageSpacing[0] - 1.0) > 1e-10 || fabs(imageSpacing[1] - 1.0) > 1e-10 ||
    fabs(imageSpacing[2] - 1.0) > 1e-10)
  {
    data->SetSpacing(1.0, 1.0, 1.0);
    data->SetExtent(targetExtent);
    data->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  }

  // Clear the image
  memset(
    data->GetScalarPointer(), 0, (data->GetNumberOfPoints() * data->GetNumberOfScalarComponents()));

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetBoundingBox(
  vtkTextProperty* tprop, const char* str, int dpi, int bbox[4])
{
  vtkTextRenderer::Metrics metrics;
  if (!this->GetMetrics(tprop, str, dpi, metrics))
  {
    return false;
  }
  std::copy(metrics.BoundingBox.GetData(), metrics.BoundingBox.GetData() + 4, bbox);
  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetMetrics(
  vtkTextProperty* tprop, const char* str, int dpi, vtkTextRenderer::Metrics& metrics)
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<< "Matplotlib rendering is unavailable.");
    return false;
  }

  if (!this->MaskParser)
  {
    if (!this->InitializeMaskParser())
    {
      vtkErrorMacro(<< "MaskParser is not initialized!");
      return false;
    }
  }

  // Configure math text font
  if (!this->SetMathTextFont(tprop))
  {
    return false;
  }

  // Get the font property used for all non math text.
  vtkSmartPyObject pyFontProp(this->GetFontProperties(tprop));
  if (this->CheckForError(pyFontProp))
  {
    return false;
  }

  // First, parse the string
  GridOfStrings strGrid;
  std::size_t maxNumberOfCells;
  if (!this->ParseString(str, strGrid, maxNumberOfCells))
  {
    vtkWarningMacro(<< "Failed to parse string.");
    return false;
  }

  std::uint64_t rows = 0;
  std::uint64_t cols = 0;
  if (!this->ComputeRowsAndCols(
        strGrid, maxNumberOfCells, tprop, pyFontProp.GetPointer(), dpi, rows, cols))
  {
    vtkWarningMacro(<< "Failed to compute rows and cols.");
    return false;
  }

  vtkDebugMacro(<< "Calculating metrics for '" << str << "'");

  int extent[4];
  this->GetJustifiedBBox(rows, cols, tprop, extent);

  // Determine the dimensions of the rotated image
  double angleDeg = tprop->GetOrientation();
  // Corners of original image
  double corners[4][2] = { { static_cast<double>(extent[0]), static_cast<double>(extent[2]) }, // TL
    { static_cast<double>(extent[1]), static_cast<double>(extent[2]) },                        // TR
    { static_cast<double>(extent[0]), static_cast<double>(extent[3]) },                        // BL
    { static_cast<double>(extent[1]), static_cast<double>(extent[3]) } };                      // BR

  double bboxd[4];
  this->RotateCorners(angleDeg, corners, bboxd);

  metrics.BoundingBox[0] = vtkMath::Floor(bboxd[0]);
  metrics.BoundingBox[1] = vtkMath::Ceil(bboxd[1]);
  metrics.BoundingBox[2] = vtkMath::Floor(bboxd[2]);
  metrics.BoundingBox[3] = vtkMath::Ceil(bboxd[3]);

  metrics.TopLeft[0] = static_cast<int>(std::round(corners[0][0]));
  metrics.TopLeft[1] = static_cast<int>(std::round(corners[0][1]));
  metrics.TopRight[0] = static_cast<int>(std::round(corners[1][0]));
  metrics.TopRight[1] = static_cast<int>(std::round(corners[1][1]));
  metrics.BottomLeft[0] = static_cast<int>(std::round(corners[2][0]));
  metrics.BottomLeft[1] = static_cast<int>(std::round(corners[2][1]));
  metrics.BottomRight[0] = static_cast<int>(std::round(corners[3][0]));
  metrics.BottomRight[1] = static_cast<int>(std::round(corners[3][1]));

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::ComputeRowsAndCols(const GridOfStrings& strGrid,
  const std::size_t& maxNumberOfCells, vtkTextProperty* tprop, PyObject* pyFontProp, int dpi,
  std::uint64_t& rows, std::uint64_t& cols)
{
  // All columns must have the same width
  // so store the maximum number of cols
  // for each column
  std::vector<std::uint64_t> vecColumnWidth;
  vecColumnWidth.resize(maxNumberOfCells);
  std::fill(vecColumnWidth.begin(), vecColumnWidth.end(), 0);

  // For each line
  for (std::size_t i = 0; i < strGrid.size(); ++i)
  {
    const std::size_t lineNumberOfCells = strGrid[i].size();

    // Number of rows of this line. This
    // is the maximum number of rows of
    // all cells of the line
    std::uint64_t lineRows = 0;

    // For each cells
    for (std::size_t j = 0; j < lineNumberOfCells; ++j)
    {
      const std::string& cell = strGrid[i][j];

      std::uint64_t cellPythonRows = 0;
      std::uint64_t cellPythonCols = 0;
      if (!this->ComputeCellRowsAndCols(
            cell.c_str(), pyFontProp, dpi, cellPythonRows, cellPythonCols, nullptr))
      {
        vtkWarningMacro(<< "Failed to compute rows and cols for cell : " << cell);
        return false;
      }

      lineRows = std::max(lineRows, cellPythonRows);

      // Store the maximum number of cols for each column
      vecColumnWidth[j] = std::max(vecColumnWidth[j], cellPythonCols);
    }

    lineRows *= (tprop->GetLineSpacing() < 1.0) ? 1.0 : tprop->GetLineSpacing();
    lineRows += tprop->GetLineOffset();

    rows += lineRows;

    // Store cell height
    if (i < (strGrid.size() - 1))
    {
      this->HorizontalLinesPosition[i] = lineRows;
    }
  }

  // The total number of cols is the sum of the maximum number of cols
  // of cells for each column
  cols = std::accumulate(vecColumnWidth.begin(), vecColumnWidth.end(), 0);

  // Handle horizontal offset between cells
  cols += tprop->GetCellOffset() * maxNumberOfCells;

  // Store cell widths
  for (std::size_t lineIdx = 0; lineIdx < (maxNumberOfCells - 1); ++lineIdx)
  {
    this->VerticalLinesPosition[lineIdx] = vecColumnWidth[lineIdx] + tprop->GetCellOffset();
  }

  // Compute positions of horizontal lines based on cell heights
  for (std::size_t lineIdx = 1; lineIdx < this->HorizontalLinesPosition.size(); ++lineIdx)
  {
    this->HorizontalLinesPosition[lineIdx] += this->HorizontalLinesPosition[lineIdx - 1];
  }

  // Compute positions of vertical lines based on cell widths
  for (std::size_t lineIdx = 1; lineIdx < this->VerticalLinesPosition.size(); ++lineIdx)
  {
    this->VerticalLinesPosition[lineIdx] += this->VerticalLinesPosition[lineIdx - 1];
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::ComputeCellRowsAndCols(const char* str, PyObject* pyFontProp,
  int dpi, std::uint64_t& rows, std::uint64_t& cols, vtkSmartPyObject* list)
{
  vtkPythonScopeGilEnsurer gilEnsurer;

  // Call the parse method
  // ftimage, depth = parse(str, dpi, fontProp)
  vtkSmartPyObject parse(PyUnicode_FromString("parse"));
  vtkSmartPyObject pyStr(PyUnicode_FromString(str));
  vtkSmartPyObject pyDpi(PyLong_FromLong(dpi));
  vtkSmartPyObject resTupleParse(PyObject_CallMethodObjArgs(this->MaskParser, parse.GetPointer(),
    pyStr.GetPointer(), pyDpi.GetPointer(), pyFontProp, nullptr));
  if (this->CheckForError(resTupleParse))
  {
    return false;
  }

  // Get ftimage
  PyObject* ftImage = PyTuple_GetItem(resTupleParse, 5);
  if (this->CheckForError(ftImage))
  {
    return false;
  }

  // Convert ftimage into a numpy array
  vtkSmartPyObject numpy(PyImport_ImportModule("numpy"));
  if (this->CheckForError(numpy))
  {
    return false;
  }

  vtkSmartPyObject asarray(PyUnicode_FromString("asarray"));
  vtkSmartPyObject numpyArray(
    PyObject_CallMethodObjArgs(numpy.GetPointer(), asarray.GetPointer(), ftImage, nullptr));
  if (this->CheckForError(numpyArray))
  {
    return false;
  }

  vtkSmartPyObject dimTuple(PyObject_GetAttrString(numpyArray, "shape"));
  if (this->CheckForError(dimTuple))
  {
    return false;
  }

  PyArg_ParseTuple(dimTuple, "ii", &rows, &cols);
  if (this->CheckForError())
  {
    return false;
  }

  // Store python data if needed
  if (list != nullptr)
  {
    vtkSmartPyObject flatArray(
      PyObject_CallMethod(numpyArray, const_cast<char*>("flatten"), const_cast<char*>("")));
    if (this->CheckForError(flatArray))
    {
      return false;
    }

    list->TakeReference(
      PyObject_CallMethod(flatArray, const_cast<char*>("tolist"), const_cast<char*>("")));
    if (this->CheckForError(*list))
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::FindAndReplaceInString(
  std::string& str, const std::string& strToFind, const std::string& replacementStr)
{
  size_t start_pos = 0;
  while ((start_pos = str.find(strToFind, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, strToFind.length(), replacementStr);
    start_pos += replacementStr.length();
  }
}

//------------------------------------------------------------------------------
struct TextColors
{
  unsigned char fgR, fgG, fgB;
  double fgA;
  unsigned char bgR, bgG, bgB;
  double bgA;
  bool hasBackground;
  unsigned char frR, frG, frB;
  bool hasFrame;
  int frW;
};

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::ComputeTextColors(vtkTextProperty* tprop, TextColors& tcolors)
{
  double* fgColor = tprop->GetColor();
  tcolors.fgR = static_cast<unsigned char>(fgColor[0] * 255);
  tcolors.fgG = static_cast<unsigned char>(fgColor[1] * 255);
  tcolors.fgB = static_cast<unsigned char>(fgColor[2] * 255);
  tcolors.fgA = tprop->GetOpacity();

  double* bgColor = tprop->GetBackgroundColor();
  tcolors.bgR = static_cast<unsigned char>(bgColor[0] * 255);
  tcolors.bgG = static_cast<unsigned char>(bgColor[1] * 255);
  tcolors.bgB = static_cast<unsigned char>(bgColor[2] * 255);
  tcolors.bgA = tprop->GetBackgroundOpacity();
  tcolors.hasBackground = (static_cast<unsigned char>(tcolors.bgA * 255) != 0);

  double* frameColor = tprop->GetFrameColor();
  tcolors.frR = static_cast<unsigned char>(frameColor[0] * 255);
  tcolors.frG = static_cast<unsigned char>(frameColor[1] * 255);
  tcolors.frB = static_cast<unsigned char>(frameColor[2] * 255);
  tcolors.hasFrame = tprop->GetFrame() && tprop->GetFrameWidth() > 0;
  tcolors.frW = tprop->GetFrameWidth();
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::SetMathTextFont(vtkTextProperty* tprop)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplBase(PyImport_ImportModule("matplotlib"));
  if (this->CheckForError(mplBase))
  {
    return false;
  }
  vtkSmartPyObject rcParams(PyObject_GetAttrString(mplBase, "rcParams"));

  // See https://matplotlib.org/stable/tutorials/text/mathtext.html for available fonts
  // Default is dejavusans
  switch (tprop->GetFontFamily())
  {
    case VTK_TIMES:
      // stix is designed to work well with Times New Roman
      PyDict_SetItemString(rcParams, "mathtext.fontset", PyUnicode_FromString("stix"));
      break;
    default:
      PyDict_SetItemString(rcParams, "mathtext.fontset", PyUnicode_FromString("dejavusans"));
      break;
  }

  if (tprop->GetShadow())
  {
    vtkWarningMacro(<< "Text shadow is not supported with math text.");
    tprop->ShadowOff();
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::RenderOneCell(vtkImageData* image, int bbox[4],
  std::int64_t rowStart, std::int64_t colStart, vtkSmartPyObject& pythonData,
  std::uint64_t pythonRows, std::uint64_t pythonCols, std::uint64_t cellRows,
  std::uint64_t cellCols, vtkTextProperty* tprop, const TextColors& tcolors)
{
  vtkDebugMacro("RenderOneCell start = ("
    << rowStart << "," << colStart << "). Drawing python data of size (" << pythonRows << ","
    << pythonCols << ") inside a cell of size (" << cellRows << "," << cellCols << ").");

  assert(cellCols >= pythonCols);
  assert(cellRows >= pythonRows);

  const std::int64_t rowEnd = rowStart - cellRows + 1;
  const std::int64_t colEnd = colStart + cellCols - 1;

  // Handle cell horizontal justification
  std::uint64_t colOffset;
  switch (tprop->GetJustification())
  {
    default:
    case VTK_TEXT_LEFT:
      colOffset = 0;
      break;
    case VTK_TEXT_CENTERED:
      colOffset = (cellCols - pythonCols) / 2;
      break;
    case VTK_TEXT_RIGHT:
      colOffset = (cellCols - pythonCols);
      break;
  }
  const std::int64_t pythonColStart = colStart + colOffset;
  const std::int64_t pythonColEnd = pythonColStart + pythonCols;

  // Handle cell vertical justification
  std::uint64_t rowOffset;
  switch (tprop->GetVerticalJustification())
  {
    default:
    case VTK_TEXT_BOTTOM:
      rowOffset = (cellRows - pythonRows);
      break;
    case VTK_TEXT_CENTERED:
      rowOffset = (cellRows - pythonRows) / 2;
      break;
    case VTK_TEXT_TOP:
      rowOffset = 0;
      break;
  }
  const std::int64_t pythonRowStart = rowStart - rowOffset;
  const std::int64_t pythonRowEnd = pythonRowStart - pythonRows;

  std::uint64_t ind = 0;
  for (std::int64_t row = rowStart; row >= rowEnd; --row)
  {
    for (std::int64_t col = colStart; col <= colEnd; ++col)
    {
      unsigned char* ptr = static_cast<unsigned char*>(image->GetScalarPointer(col, row, 0));

      // Background, do not load python data
      if (row > pythonRowStart || col < pythonColStart || row <= pythonRowEnd ||
        col >= pythonColEnd)
      {
        if (tcolors.hasFrame &&
          (col < (bbox[0] + tcolors.frW) || col > (bbox[1] - tcolors.frW) ||
            row > (bbox[3] - tcolors.frW) || row < (bbox[2] + tcolors.frW)))
        {
          ptr[0] = static_cast<unsigned char>(tcolors.frR);
          ptr[1] = static_cast<unsigned char>(tcolors.frG);
          ptr[2] = static_cast<unsigned char>(tcolors.frB);
          ptr[3] = 255;
        }
        else if (tcolors.hasBackground)
        {
          ptr[0] = static_cast<unsigned char>(tcolors.bgR);
          ptr[1] = static_cast<unsigned char>(tcolors.bgG);
          ptr[2] = static_cast<unsigned char>(tcolors.bgB);
          ptr[3] = static_cast<unsigned char>(255 * tcolors.bgA);
        }
        else
        {
          ptr[0] = tcolors.fgR;
          ptr[1] = tcolors.fgG;
          ptr[2] = tcolors.fgB;
          ptr[3] = 0;
        }
      } // end background
      else
      {
        // item is borrowed, no need for a smart wrapper
        vtkPythonScopeGilEnsurer gilEnsurer;
        PyObject* item = PyList_GetItem(pythonData, ind++);
        if (this->CheckForError(item))
        {
          return false;
        }
        const unsigned char val = static_cast<unsigned char>(PyLong_AsLong(item));
        if (this->CheckForError())
        {
          return false;
        }

        if (tcolors.hasFrame &&
          (col < (bbox[0] + tcolors.frW) || col > (bbox[1] - tcolors.frW) ||
            row > (bbox[3] - tcolors.frW) || row < (bbox[2] + tcolors.frW)))
        {
          const float fg_blend = tcolors.fgA * (val / 255.f);
          const float fr_blend = 1.f - fg_blend;

          ptr[0] = static_cast<unsigned char>(fr_blend * tcolors.frR + fg_blend * tcolors.fgR);
          ptr[1] = static_cast<unsigned char>(fr_blend * tcolors.frG + fg_blend * tcolors.fgG);
          ptr[2] = static_cast<unsigned char>(fr_blend * tcolors.frB + fg_blend * tcolors.fgB);
          ptr[3] = 255;
        }
        else if (tcolors.hasBackground)
        {
          const float fg_blend = tcolors.fgA * (val / 255.f);
          const float bg_blend = 1.f - fg_blend;

          ptr[0] = static_cast<unsigned char>(bg_blend * tcolors.bgR + fg_blend * tcolors.fgR);
          ptr[1] = static_cast<unsigned char>(bg_blend * tcolors.bgG + fg_blend * tcolors.fgG);
          ptr[2] = static_cast<unsigned char>(bg_blend * tcolors.bgB + fg_blend * tcolors.fgB);
          ptr[3] = static_cast<unsigned char>(255 * (fg_blend + tcolors.bgA * bg_blend));
        }
        else
        {
          ptr[0] = tcolors.fgR;
          ptr[1] = tcolors.fgG;
          ptr[2] = tcolors.fgB;
          ptr[3] = static_cast<unsigned char>(val * tcolors.fgA);
        }
      } // end text
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::DrawInteriorLines(
  vtkImageData* image, int bbox[4], vtkTextProperty* tprop)
{
  if (!image)
  {
    vtkErrorMacro("Invalid image.");
    return false;
  }

  // Define line offsets to take line width into account
  int extraLinesMin = 0;
  int extraLinesMax = 0;
  int width = tprop->GetInteriorLinesWidth();
  double* doubleColor = tprop->GetInteriorLinesColor();
  unsigned char color[3] = { static_cast<unsigned char>(doubleColor[0] * 255),
    static_cast<unsigned char>(doubleColor[1] * 255),
    static_cast<unsigned char>(doubleColor[2] * 255) };

  // Draw horizontal lines
  for (std::size_t lineIdx = 0; lineIdx < this->HorizontalLinesPosition.size(); ++lineIdx)
  {
    // Clamp line width to remain in bounds
    extraLinesMin = ((this->HorizontalLinesPosition[lineIdx] - (width / 2)) >= 0)
      ? (-width / 2)
      : (-this->HorizontalLinesPosition[lineIdx]);
    extraLinesMax = ((this->HorizontalLinesPosition[lineIdx] + (width / 2)) <= (bbox[3] - bbox[2]))
      ? ((width + 1) / 2)
      : ((bbox[3] - bbox[2]) - this->HorizontalLinesPosition[lineIdx]);

    for (int extraLineIdx = extraLinesMin; extraLineIdx < extraLinesMax; ++extraLineIdx)
    {
      for (int colIdx = bbox[0]; colIdx <= bbox[1]; ++colIdx)
      {
        unsigned char* ptr = static_cast<unsigned char*>(image->GetScalarPointer(
          colIdx, bbox[2] + this->HorizontalLinesPosition[lineIdx] + extraLineIdx, 0));
        ptr[0] = color[0];
        ptr[1] = color[1];
        ptr[2] = color[2];
        ptr[3] = 255;
      }
    }
  }

  // Draw vertical lines
  for (std::size_t lineIdx = 0; lineIdx < this->VerticalLinesPosition.size(); ++lineIdx)
  {
    // Clamp line width to remain in bounds
    extraLinesMin = ((this->VerticalLinesPosition[lineIdx] - (width / 2)) >= 0)
      ? (-width / 2)
      : (-this->VerticalLinesPosition[lineIdx]);
    extraLinesMax = ((this->VerticalLinesPosition[lineIdx] + (width / 2)) <= (bbox[1] - bbox[0]))
      ? ((width + 1) / 2)
      : ((bbox[1] - bbox[0]) - this->VerticalLinesPosition[lineIdx]);

    for (int extraLineIdx = extraLinesMin; extraLineIdx < extraLinesMax; ++extraLineIdx)
    {
      for (int rowIdx = bbox[2]; rowIdx <= bbox[3]; ++rowIdx)
      {
        unsigned char* ptr = static_cast<unsigned char*>(image->GetScalarPointer(
          bbox[0] + this->VerticalLinesPosition[lineIdx] + extraLineIdx, rowIdx, 0));
        ptr[0] = color[0];
        ptr[1] = color[1];
        ptr[2] = color[2];
        ptr[3] = 255;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::ParseString(
  const char* str, GridOfStrings& strGrid, std::size_t& maxNumberOfCells)
{
  // First, change all occurrence of escaped pipe ("\|")
  // Into a special character and recover them after splitting
  std::string stdStr = std::string(str);
  this->FindAndReplaceInString(stdStr, "\\|", vtkMatplotlibMathTextUtilities::PipeProtectString);

  maxNumberOfCells = 0;
  std::stringstream ss(stdStr);
  std::string line;
  strGrid.clear();

  // Split lines
  while (std::getline(ss, line))
  {
    std::size_t numberOfCells = 0;
    std::vector<std::string> lineStrVec;

    // A cell is defined by a pipe '|'
    std::stringstream ssCell(line);
    std::string cell;
    while (std::getline(ssCell, cell, '|'))
    {
      // Recover the escaped pipe
      this->FindAndReplaceInString(cell, vtkMatplotlibMathTextUtilities::PipeProtectString, "\\|");
      lineStrVec.push_back(std::move(cell));
      numberOfCells++;
    }
    strGrid.push_back(lineStrVec);
    maxNumberOfCells = std::max(maxNumberOfCells, numberOfCells);
  }

  // Initialize number of interior borders
  this->VerticalLinesPosition.resize(maxNumberOfCells - 1);
  this->HorizontalLinesPosition.resize(strGrid.size() - 1);

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::RenderString(
  const char* str, vtkImageData* image, vtkTextProperty* tprop, int dpi, int textDims[2])
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<< "Matplotlib rendering is unavailable.");
    return false;
  }

  if (!this->MaskParser)
  {
    if (!this->InitializeMaskParser())
    {
      vtkErrorMacro(<< "MaskParser is not initialized!");
      return false;
    }
  }

  TextColors tcolors;
  this->ComputeTextColors(tprop, tcolors);

  // To customize math text font, we must use rcParams
  // (see https://matplotlib.org/stable/tutorials/introductory/customizing.html)
  if (!this->SetMathTextFont(tprop))
  {
    return false;
  }

  // Create the font property used for all non math text
  vtkSmartPyObject pyFontProp(this->GetFontProperties(tprop));
  if (this->CheckForError(pyFontProp))
  {
    return false;
  }

  // Parse the string by lines and columns and store
  // each cell string in the string grid
  GridOfStrings strGrid;
  std::size_t maxNumberOfCells;
  if (!this->ParseString(str, strGrid, maxNumberOfCells))
  {
    vtkWarningMacro(<< "Failed to parse string.");
    return false;
  }

  // For each line, render all the cells
  // And store each cell python representation
  // and the associated rows and cols
  std::vector<std::vector<vtkSmartPyObject>> gridPythonData;
  std::vector<std::vector<std::uint64_t>> gridRowsAndCols;

  // All columns must have the same width
  // so store the maximum number of cols
  // for each column
  std::vector<std::uint64_t> vecColumnWidth;
  vecColumnWidth.resize(maxNumberOfCells);
  std::fill(vecColumnWidth.begin(), vecColumnWidth.end(), 0);

  // Store the number of rows of each line
  std::vector<std::uint64_t> vecLineRows;

  // The total number of rows is the sum
  // of all rows of each line
  std::uint64_t totalRows = 0;

  // For each line
  for (std::size_t i = 0; i < strGrid.size(); ++i)
  {
    const std::size_t lineNumberOfCells = strGrid[i].size();

    // Number of rows of this line. This
    // is the maximum number of rows of
    // all cells of the line
    std::uint64_t lineRows = 0;

    // store each cell matplotlib representation
    std::vector<vtkSmartPyObject> cellsPythonData;
    // And its number of python rows and cols
    std::vector<std::uint64_t> cellsPythonRowsAndCols;

    // For each cells
    for (std::size_t j = 0; j < lineNumberOfCells; ++j)
    {
      std::string& cell = strGrid[i][j];

      std::uint64_t cellPythonRows = 0;
      std::uint64_t cellPythonCols = 0;
      vtkSmartPyObject cellPythonData;

      if (!this->ComputeCellRowsAndCols(cell.c_str(), pyFontProp.GetPointer(), dpi, cellPythonRows,
            cellPythonCols, &cellPythonData))
      {
        vtkWarningMacro(<< "Failed to compute rows and cols for cell : " << cell);
        return false;
      }

      lineRows = std::max(lineRows, cellPythonRows);

      cellsPythonData.push_back(cellPythonData);
      cellsPythonRowsAndCols.push_back(cellPythonRows);
      cellsPythonRowsAndCols.push_back(cellPythonCols);

      // Store the maximum number of cols for each column
      vecColumnWidth[j] = std::max(vecColumnWidth[j], cellPythonCols);
    }

    // Missing cells are replaced by empty data
    for (std::size_t j = lineNumberOfCells; j < maxNumberOfCells; ++j)
    {
      cellsPythonData.emplace_back();
      cellsPythonRowsAndCols.push_back(0);
      cellsPythonRowsAndCols.push_back(0);
    }

    lineRows *= (tprop->GetLineSpacing() < 1.0) ? 1.0 : tprop->GetLineSpacing();
    lineRows += tprop->GetLineOffset();

    vecLineRows.push_back(lineRows);

    totalRows += lineRows;

    gridPythonData.push_back(cellsPythonData);
    gridRowsAndCols.push_back(cellsPythonRowsAndCols);
  }

  // The total number of cols is the sum of the maximum number of cols
  // of cells for each column
  std::uint64_t totalCols = std::accumulate(vecColumnWidth.begin(), vecColumnWidth.end(), 0);

  // Handle horizontal offset between cells
  totalCols += tprop->GetCellOffset() * maxNumberOfCells;

  // Create justified bounding box.
  int bbox[4];
  this->GetJustifiedBBox(totalRows, totalCols, tprop, bbox);
  this->PrepareImageData(image, bbox);

  const std::size_t numberOfLines = gridPythonData.size();
  const std::size_t numberOfCells = gridPythonData[0].size();

  vtkDebugMacro("RenderString of size ("
    << totalRows << "," << totalCols << "), starting at (" << bbox[3] << "," << bbox[0]
    << "), ending at (" << bbox[2] << "," << bbox[1] << "), with " << numberOfLines << " lines and "
    << numberOfCells << " cells per line");

  std::int64_t rowStart = bbox[3];

  for (std::size_t i = 0; i < numberOfLines; ++i)
  {
    std::int64_t colStart = bbox[0];

    std::vector<vtkSmartPyObject>& cellsPythonData = gridPythonData[i];
    std::vector<std::uint64_t>& cellsPythonRowsAndCols = gridRowsAndCols[i];

    // The number of rows of this line
    const std::uint64_t lineRows = vecLineRows[i];

    for (std::size_t j = 0; j < numberOfCells; ++j)
    {
      vtkSmartPyObject& cellPythonData = cellsPythonData[j];
      const std::uint64_t pythonRows = cellsPythonRowsAndCols[2 * j];
      const std::uint64_t pythonCols = cellsPythonRowsAndCols[2 * j + 1];

      // Get the width of the cell and don't forget offset between cells
      std::uint64_t cellCols = vecColumnWidth[j] + tprop->GetCellOffset();

      // The cell number of rows is the number of rows of the line
      if (!this->RenderOneCell(image, bbox, rowStart, colStart, cellPythonData, pythonRows,
            pythonCols, lineRows, cellCols, tprop, tcolors))
      {
        vtkErrorMacro(<< "Failed to render cell number " << j);
        return false;
      }

      colStart += cellCols;
    }

    rowStart -= lineRows;
  }

  if (textDims)
  {
    textDims[0] = totalCols;
    textDims[1] = totalRows;
  }

  // Draw interior lines between cells
  if (tprop->GetInteriorLinesVisibility())
  {
    if (!this->DrawInteriorLines(image, bbox, tprop))
    {
      vtkErrorMacro("Failed to draw interior lines.");
      return false;
    }
  }

  // Mark the image data as modified, as it is possible that only
  // vtkImageData::Get*Pointer methods will be called, which do not update the
  // MTime.
  image->Modified();

  // Determine the dimensions of the rotated image
  double angleDeg = tprop->GetOrientation();
  // Save some time if no rotation needed
  if (fabs(angleDeg) < 0.01)
  {
    return true;
  }

  // Corners of original image
  double corners[4][2] = { { static_cast<double>(bbox[0]), static_cast<double>(bbox[2]) },
    { static_cast<double>(bbox[1]), static_cast<double>(bbox[2]) },
    { static_cast<double>(bbox[0]), static_cast<double>(bbox[3]) },
    { static_cast<double>(bbox[1]), static_cast<double>(bbox[3]) } };
  double bboxd[4];

  // Rotate the corners of the image and determine the bounding box
  this->RotateCorners(angleDeg, corners, bboxd);

  // Also rotate the text dimensions.
  if (textDims)
  {
    double text_bbox[4];
    corners[0][0] = static_cast<double>(0);
    corners[0][1] = static_cast<double>(0);
    corners[1][0] = static_cast<double>(0);
    corners[1][1] = static_cast<double>(textDims[1]);
    corners[2][0] = static_cast<double>(textDims[0]);
    corners[2][1] = static_cast<double>(textDims[1]);
    corners[3][0] = static_cast<double>(textDims[0]);
    corners[3][1] = static_cast<double>(0);
    this->RotateCorners(angleDeg, corners, text_bbox);
    textDims[0] = std::ceil(text_bbox[1] - text_bbox[0]);
    textDims[1] = std::ceil(text_bbox[3] - text_bbox[2]);
  }

  bbox[0] = vtkMath::Floor(bboxd[0]);
  bbox[1] = vtkMath::Ceil(bboxd[1]);
  bbox[2] = vtkMath::Floor(bboxd[2]);
  bbox[3] = vtkMath::Ceil(bboxd[3]);

  // Rotate the temporary image into the returned image:
  vtkNew<vtkTransform> rotation;
  rotation->RotateWXYZ(-angleDeg, 0, 0, 1);
  // Dummy image with the output dimensions
  vtkNew<vtkImageData> dummyImage;
  this->PrepareImageData(dummyImage, bbox);
  vtkNew<vtkImageReslice> rotator;
  rotator->SetInputData(image);
  rotator->SetInformationInput(dummyImage);
  rotator->SetResliceTransform(rotation);
  rotator->SetInterpolationModeToLinear();
  rotator->Update();
  image->ShallowCopy(rotator->GetOutput());

  return true;
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::StringToPath(
  const char* str, vtkPath* path, vtkTextProperty* tprop, int dpi)
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<< "Matplotlib rendering is unavailable.");
    return false;
  }

  if (!this->PathParser)
  {
    if (!this->InitializePathParser())
    {
      vtkErrorMacro(<< "PathParser is not initialized!");
      return false;
    }
  }

  vtkDebugMacro(<< "Converting '" << str << "' into a vtkPath...");

  // Matplotlib path codes:
  const int pathStop = 0;
  const int pathMoveTo = 1;
  const int pathLineTo = 2;
  const int pathCurve3 = 3;
  const int pathCurve4 = 4;
  const int pathClosePoly = 0x4f;

  // List sizes:
  Py_ssize_t numCodes;
  Py_ssize_t numVerts;

  // Temp vars:
  float origin[2] = { 0.0, 0.0 };
  float vert[2];
  float delta[2] = { 0.0, 0.0 };
  int code;
  bool hasOrigin = false;

  // Bounding box for all control points, used for justification
  float cbox[4] = { VTK_FLOAT_MAX, VTK_FLOAT_MAX, VTK_FLOAT_MIN, VTK_FLOAT_MIN };

  // The path is always generated using a 100pt font @72 dpi. Use this factor to
  // recover the font.
  const float fontScale = (tprop->GetFontSize() / 100.f) * (dpi / 72.f);

  path->Reset();

  // Create the font property
  vtkSmartPyObject pyFontProp(this->GetFontProperties(tprop));
  if (this->CheckForError(pyFontProp))
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject pyResultTuple(PyObject_CallMethod(this->PathParser,
    const_cast<char*>("get_text_path"), const_cast<char*>("Osi"),
    pyFontProp.GetPointer(), // prop
    const_cast<char*>(str),  // texstring
    1,                       // boolean, ismath
    0));                     // boolean, usetex
  if (this->CheckForError(pyResultTuple))
  {
    return false;
  }

  // pyVerts and pyCodes are borrowed references -- no need for smart wrappers
  PyObject* pyVerts = PyTuple_GetItem(pyResultTuple, 0);
  PyObject* pyCodes = PyTuple_GetItem(pyResultTuple, 1);
  if (this->CheckForError(pyVerts) || this->CheckForError(pyCodes))
  {
    return false;
  }

  // Both verts and codes are lists?
  if (!PySequence_Check(pyVerts) || !PySequence_Check(pyCodes))
  {
    return false;
  }

  numVerts = PySequence_Size(pyVerts);
  numCodes = PySequence_Size(pyCodes);
  if (numVerts != numCodes)
  {
    return false;
  }

  path->Allocate(numVerts);

  for (Py_ssize_t i = 0; i < numVerts; ++i)
  {
    vtkSmartPyObject pyVert(PySequence_GetItem(pyVerts, i));
    vtkSmartPyObject pyCode(PySequence_GetItem(pyCodes, i));
    if (this->CheckForError(pyVert) || this->CheckForError(pyCode))
    {
      return false;
    }

    // pyVert is sometimes a numpy array, sometimes it's a tuple.
    // Initialize the following objects in the following conditional, then
    // convert to smart pointers afterwards.
    PyObject* pyVertXObj = nullptr;
    PyObject* pyVertYObj = nullptr;
    if (pyVert->ob_type == &PyTuple_Type)
    {
      pyVertXObj = PyTuple_GetItem(pyVert, 0);
      pyVertYObj = PyTuple_GetItem(pyVert, 1);
      // Increase reference count -- the other branch returns a new reference,
      // this keeps cleanup consistent
      if (pyVertXObj)
      {
        Py_INCREF(pyVertXObj);
      }
      if (pyVertYObj)
      {
        Py_INCREF(pyVertYObj);
      }
    }
    else // Assume numpy array. Convert to list and extract elements.
    {
      vtkSmartPyObject pyVertList(
        PyObject_CallMethod(pyVert, const_cast<char*>("tolist"), nullptr));
      if (this->CheckForError(pyVertList) || PySequence_Size(pyVertList) < 2)
      {
        return false;
      }

      pyVertXObj = PySequence_GetItem(pyVertList, 0);
      pyVertYObj = PySequence_GetItem(pyVertList, 1);
    }

    vtkSmartPyObject pyVertX(pyVertXObj);
    vtkSmartPyObject pyVertY(pyVertYObj);
    if (this->CheckForError(pyVertX) || this->CheckForError(pyVertY))
    {
      return false;
    }

    vert[0] = PyFloat_AsDouble(pyVertX) * fontScale;
    vert[1] = PyFloat_AsDouble(pyVertY) * fontScale;
    if (this->CheckForError())
    {
      return false;
    }

    if (vert[0] < cbox[0])
    {
      cbox[0] = vert[0];
    }
    if (vert[1] < cbox[1])
    {
      cbox[1] = vert[1];
    }
    if (vert[0] > cbox[2])
    {
      cbox[2] = vert[0];
    }
    if (vert[1] > cbox[3])
    {
      cbox[3] = vert[1];
    }

    code = PyLong_AsLong(pyCode);
    if (this->CheckForError())
    {
      return false;
    }

    switch (code)
    {
      case pathStop:
        hasOrigin = false;
        break;
      case pathMoveTo:
        path->InsertNextPoint(vert[0], vert[1], 0, vtkPath::MOVE_TO);
        origin[0] = vert[0];
        origin[1] = vert[1];
        hasOrigin = true;
        break;
      case pathLineTo:
        path->InsertNextPoint(vert[0], vert[1], 0, vtkPath::LINE_TO);
        break;
      case pathCurve3:
        path->InsertNextPoint(vert[0], vert[1], 0, vtkPath::CONIC_CURVE);
        break;
      case pathCurve4:
        path->InsertNextPoint(vert[0], vert[1], 0, vtkPath::CUBIC_CURVE);
        break;
      case pathClosePoly:
        if (hasOrigin)
          path->InsertNextPoint(origin[0], origin[1], 0, vtkPath::LINE_TO);
        hasOrigin = false;
        break;
      default:
        vtkWarningMacro(<< "Unrecognized code: " << code);
        break;
    }
  }

  // Apply justification:
  switch (tprop->GetJustification())
  {
    default:
    case VTK_TEXT_LEFT:
      delta[0] = -cbox[0];
      break;
    case VTK_TEXT_CENTERED:
      delta[0] = -(cbox[2] - cbox[0]) * 0.5;
      break;
    case VTK_TEXT_RIGHT:
      delta[0] = -cbox[2];
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    default:
    case VTK_TEXT_BOTTOM:
      delta[1] = -cbox[1];
      break;
    case VTK_TEXT_CENTERED:
      delta[1] = -(cbox[3] - cbox[1]) * 0.5;
      break;
    case VTK_TEXT_TOP:
      delta[1] = -cbox[3];
  }

  const double theta = vtkMath::RadiansFromDegrees(tprop->GetOrientation());
  const double sinTheta = sin(theta);
  const double cosTheta = cos(theta);
  vtkPoints* points = path->GetPoints();
  double point[3];
  double newPoint[3];
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
  {
    points->GetPoint(i, point);
    point[0] += delta[0];
    point[1] += delta[1];
    newPoint[0] = (point[0] * cosTheta) - (point[1] * sinTheta);
    newPoint[1] = (point[0] * sinTheta) + (point[1] * cosTheta);
    newPoint[2] = point[2];
    points->SetPoint(i, newPoint);
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::SetScaleToPowerOfTwo(bool val)
{
  if (this->ScaleToPowerOfTwo != val)
  {
    this->ScaleToPowerOfTwo = val;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetScaleToPowerOfTwo()
{
  return this->ScaleToPowerOfTwo;
}

//------------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MPLMathTextAvailable: ";
  switch (vtkMatplotlibMathTextUtilities::MPLMathTextAvailable)
  {
    case vtkMatplotlibMathTextUtilities::NOT_TESTED:
      os << "Not tested\n";
      break;
    case vtkMatplotlibMathTextUtilities::AVAILABLE:
      os << "Available\n";
      break;
    default:
    case vtkMatplotlibMathTextUtilities::UNAVAILABLE:
      os << "Unavailable\n";
      break;
  }

  os << indent << "MaskParser: " << this->MaskParser << endl;
  os << indent << "PathParser: " << this->PathParser << endl;
  os << indent << "FontPropertiesClass: " << this->FontPropertiesClass << endl;
}
VTK_ABI_NAMESPACE_END
