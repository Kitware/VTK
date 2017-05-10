/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatplotlibMathTextUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be the first thing that's included.
#include "vtkPythonCompatibility.h"
#include "vtkMatplotlibMathTextUtilities.h"

#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkImageReslice.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPoints.h"
#include "vtkPythonInterpreter.h"
#include "vtkStdString.h"
#include "vtkSmartPyObject.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

#include <vtksys/SystemTools.hxx>

#include <vector>

// We need to define Py_ssize_t for older python API version
#if PYTHON_API_VERSION < 1013
// Taken from pyport.h
#  ifdef HAVE_SSIZE_T
typedef ssize_t         Py_ssize_t;
#  elif SIZEOF_VOID_P == SIZEOF_SIZE_T
typedef Py_intptr_t     Py_ssize_t;
#  else
#    error "Python needs a typedef for Py_ssize_t."
#  endif
#endif

//----------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::Availability
vtkMatplotlibMathTextUtilities::MPLMathTextAvailable =
vtkMatplotlibMathTextUtilities::NOT_TESTED;

// A macro that is used in New() to print warnings if VTK_MATPLOTLIB_DEBUG
// is defined in the environment. Use vtkGenericWarningMacro to allow this to
// work in release mode builds.
#define vtkMplStartUpDebugMacro(x) if(debug){vtkGenericWarningMacro(x);}

vtkObjectFactoryNewMacro(vtkMatplotlibMathTextUtilities)

//----------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::Availability
vtkMatplotlibMathTextUtilities::CheckMPLAvailability()
{
  if (vtkMatplotlibMathTextUtilities::MPLMathTextAvailable != NOT_TESTED)
  {
    // Already tested. Nothing to do now.
    return vtkMatplotlibMathTextUtilities::MPLMathTextAvailable;
  }

  // Enable startup debugging output. This will be set to true when
  // VTK_MATPLOTLIB_DEBUG is defined in the process environment.
  bool debug = (vtksys::SystemTools::GetEnv("VTK_MATPLOTLIB_DEBUG") != NULL);

  // Initialize the python interpretor if needed
  vtkMplStartUpDebugMacro("Initializing Python, if not already.");
  vtkPythonInterpreter::Initialize();
  vtkMplStartUpDebugMacro("Attempting to import matplotlib.");
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (PyErr_Occurred() || !PyImport_ImportModule("matplotlib") || PyErr_Occurred())
  {
    // FIXME: Check if we need this. Wouldn't pipe-ing the stdout/stderr make
    // this unnecessary?

    // Fetch the exception info. Note that value and traceback may still be
    // NULL after the call to PyErr_Fetch().
    PyObject *type = NULL;
    PyObject *value = NULL;
    PyObject *traceback = NULL;
    PyErr_Fetch(&type, &value, &traceback);
    vtkSmartPyObject typeStr(PyObject_Str(type));
    vtkSmartPyObject valueStr(PyObject_Str(value));
    vtkSmartPyObject tracebackStr(PyObject_Str(traceback));
    vtkMplStartUpDebugMacro(
      "Error during matplotlib import:\n"
      << "\nStack:\n"
      << (tracebackStr.GetPointer() == NULL
        ? "(none)"
        : const_cast<char*>(
          PyString_AsString(tracebackStr.GetPointer())))
      << "\nValue:\n"
      << (valueStr.GetPointer() == NULL
        ? "(none)"
        : const_cast<char*>(
          PyString_AsString(valueStr.GetPointer())))
      << "\nType:\n"
      << (typeStr.GetPointer() == NULL
        ? "(none)"
        : const_cast<char*>(
          PyString_AsString(typeStr.GetPointer()))));
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

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::IsAvailable()
{
  return this->CheckMPLAvailability() == AVAILABLE;
}

//----------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::vtkMatplotlibMathTextUtilities()
  : Superclass(), MaskParser(NULL), PathParser(NULL), FontPropertiesClass(NULL),
    ScaleToPowerOfTwo(true)
{
  this->Interpreter = vtkPythonInterpreter::New();
  this->Interpreter->AddObserver(vtkCommand::ExitEvent,
    this, &vtkMatplotlibMathTextUtilities::CleanupPythonObjects);
}

//----------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::~vtkMatplotlibMathTextUtilities()
{
  this->CleanupPythonObjects();
  this->Interpreter->Delete();
}

//----------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::CleanupPythonObjects()
{
  if (Py_IsInitialized())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->MaskParser);
    Py_XDECREF(this->PathParser);
    Py_XDECREF(this->FontPropertiesClass);

    this->MaskParser = NULL;
    this->PathParser = NULL;
    this->FontPropertiesClass = NULL;
  }
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializeMaskParser()
{
  // ensure that Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplMathTextLib(PyImport_ImportModule("matplotlib.mathtext"));
  if (this->CheckForError(mplMathTextLib.GetPointer()))
  {
    return false;
  }

  vtkSmartPyObject mathTextParserClass(
        PyObject_GetAttrString(mplMathTextLib.GetPointer(), "MathTextParser"));
  if (this->CheckForError(mathTextParserClass.GetPointer()))
  {
    return false;
  }

  this->MaskParser =
      PyObject_CallFunction(mathTextParserClass.GetPointer(),
                            const_cast<char*>("s"), "bitmap");
  if (this->CheckForError(this->MaskParser))
  {
    Py_CLEAR(this->MaskParser);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializePathParser()
{
  // ensure that Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplTextPathLib(PyImport_ImportModule("matplotlib.textpath"));
  if (this->CheckForError(mplTextPathLib.GetPointer()))
  {
    return false;
  }

  vtkSmartPyObject textToPathClass(
        PyObject_GetAttrString(mplTextPathLib.GetPointer(), "TextToPath"));
  if (this->CheckForError(textToPathClass.GetPointer()))
  {
    return false;
  }

  this->PathParser = PyObject_CallFunction(textToPathClass.GetPointer(), NULL);
  if (this->CheckForError(this->PathParser))
  {
    Py_CLEAR(this->PathParser);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializeFontPropertiesClass()
{
  // ensure that Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mplFontManagerLib(
        PyImport_ImportModule("matplotlib.font_manager"));
  if (this->CheckForError(mplFontManagerLib.GetPointer()))
  {
    return false;
  }

  this->FontPropertiesClass = PyObject_GetAttrString(
        mplFontManagerLib.GetPointer(), "FontProperties");
  if (this->CheckForError(this->FontPropertiesClass))
  {
    Py_CLEAR(this->FontPropertiesClass);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::CheckForError()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  PyObject *exception = PyErr_Occurred();
  if (exception)
  {
    if (this->Debug)
    {
      // Fetch the exception info. Note that value and traceback may still be
      // NULL after the call to PyErr_Fetch().
      PyObject *type = NULL;
      PyObject *value = NULL;
      PyObject *traceback = NULL;
      PyErr_Fetch(&type, &value, &traceback);
      vtkSmartPyObject typeStr(PyObject_Str(type));
      vtkSmartPyObject valueStr(PyObject_Str(value));
      vtkSmartPyObject tracebackStr(PyObject_Str(traceback));
      vtkWarningMacro(<< "Python exception raised:\n"
                      << "\nStack:\n"
                      << (tracebackStr.GetPointer() == NULL
                          ? "(none)"
                          : const_cast<char*>(
                            PyString_AsString(tracebackStr.GetPointer())))
                      << "\nValue:\n"
                      << (valueStr.GetPointer() == NULL
                          ? "(none)"
                          : const_cast<char*>(
                            PyString_AsString(valueStr.GetPointer())))
                      << "\nType:\n"
                      << (typeStr.GetPointer() == NULL
                          ? "(none)"
                          : const_cast<char*>(
                            PyString_AsString(typeStr.GetPointer()))));
    }
    PyErr_Clear();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::CheckForError(PyObject *object)
{
  // Print any exceptions
  bool result = this->CheckForError();

  if (object == NULL)
  {
    vtkDebugMacro(<< "Object is NULL!");
    return true;
  }
  return result;
}

//----------------------------------------------------------------------------
PyObject *
vtkMatplotlibMathTextUtilities::GetFontProperties(vtkTextProperty *tprop)
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<<"Matplotlib rendering is unavailable.");
    return NULL;
  }

  if (!this->FontPropertiesClass)
  {
    if (!this->InitializeFontPropertiesClass())
    {
      vtkErrorMacro(<<"FontPropertiesClass is not initialized!");
      return NULL;
    }
  }

  char tpropFamily[16];
  char tpropStyle[16];
  char tpropVariant[16] = "normal";
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
  return PyObject_CallFunction(this->FontPropertiesClass,
                               const_cast<char*>("sssssi"), tpropFamily,
                               tpropStyle, tpropVariant, tpropStyle,
                               tpropWeight, tpropFontSize);
}

//----------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::GetJustifiedBBox(int rows, int cols,
                                                      vtkTextProperty *tprop,
                                                      int bbox[])
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

//----------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::RotateCorners(double angleDeg,
                                                   double corners[4][2],
                                                   double bbox[4])
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

//----------------------------------------------------------------------------
// This is more or less ported from vtkFreeTypeTools.
bool vtkMatplotlibMathTextUtilities::PrepareImageData(vtkImageData *data,
                                                      int textBbox[4])
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
  if (data->GetScalarType() != VTK_UNSIGNED_CHAR ||
      data->GetNumberOfScalarComponents() != 4 ||
      imageExtent[0] != targetExtent[0] ||
      imageExtent[1] != targetExtent[1] ||
      imageExtent[2] != targetExtent[2] ||
      imageExtent[3] != targetExtent[3] ||
      imageExtent[4] != targetExtent[4] ||
      imageExtent[5] != targetExtent[5] ||
      fabs(imageSpacing[0] - 1.0) > 1e-10 ||
      fabs(imageSpacing[1] - 1.0) > 1e-10 ||
      fabs(imageSpacing[2] - 1.0) > 1e-10 )
  {
    data->SetSpacing(1.0, 1.0, 1.0);
    data->SetExtent(targetExtent);
    data->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  }

  // Clear the image
  memset(data->GetScalarPointer(), 0,
         (data->GetNumberOfPoints() * data->GetNumberOfScalarComponents()));

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetBoundingBox(
    vtkTextProperty *tprop, const char *str, int dpi, int bbox[4])
{
  vtkTextRenderer::Metrics metrics;
  if (!this->GetMetrics(tprop, str, dpi, metrics))
  {
    return false;
  }
  std::copy(metrics.BoundingBox.GetData(), metrics.BoundingBox.GetData() + 4,
            bbox);
  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetMetrics(
    vtkTextProperty *tprop, const char *str, int dpi,
    vtkTextRenderer::Metrics &metrics)
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<<"Matplotlib rendering is unavailable.");
    return false;
  }

  if (!this->MaskParser)
  {
    if (!this->InitializeMaskParser())
    {
      vtkErrorMacro(<<"MaskParser is not initialized!");
      return false;
    }
  }

  vtkDebugMacro(<<"Calculating metrics for '" << str << "'");

  long int rows = 0;
  long int cols = 0;

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject resultTuple(PyObject_CallMethod(this->MaskParser,
                                                   const_cast<char*>("to_mask"),
                                                   const_cast<char*>("sii"),
                                                   const_cast<char*>(str),
                                                   tprop->GetFontSize(),
                                                   dpi));
  if (this->CheckForError(resultTuple.GetPointer()))
  {
    return false;
  }

  // numpyArray is a borrowed reference, no smart wrapper needed:
  PyObject *numpyArray = PyTuple_GetItem(resultTuple.GetPointer(), 0);
  if (this->CheckForError(numpyArray))
  {
    return false;
  }

  vtkSmartPyObject dimTuple(PyObject_GetAttrString(numpyArray,
                                                const_cast<char*>("shape")));
  if (this->CheckForError(dimTuple.GetPointer()))
  {
    return false;
  }

  PyArg_ParseTuple(dimTuple.GetPointer(), "ii", &rows, &cols);
  if (this->CheckForError())
  {
    return false;
  }

  int extent[4];
  this->GetJustifiedBBox(rows, cols, tprop, extent);

  // Determine the dimensions of the rotated image
  double angleDeg = tprop->GetOrientation();
  // Corners of original image
  double corners[4][2] = {
    {static_cast<double>(extent[0]), static_cast<double>(extent[2])},   // TL
    {static_cast<double>(extent[1]), static_cast<double>(extent[2])},   // TR
    {static_cast<double>(extent[0]), static_cast<double>(extent[3])},   // BL
    {static_cast<double>(extent[1]), static_cast<double>(extent[3])} }; // BR

  double bboxd[4];
  this->RotateCorners(angleDeg, corners, bboxd);

  metrics.BoundingBox[0] = vtkMath::Floor(bboxd[0]);
  metrics.BoundingBox[1] = vtkMath::Ceil(bboxd[1]);
  metrics.BoundingBox[2] = vtkMath::Floor(bboxd[2]);
  metrics.BoundingBox[3] = vtkMath::Ceil(bboxd[3]);

  metrics.TopLeft[0]     = vtkMath::Round(corners[0][0]);
  metrics.TopLeft[1]     = vtkMath::Round(corners[0][1]);
  metrics.TopRight[0]    = vtkMath::Round(corners[1][0]);
  metrics.TopRight[1]    = vtkMath::Round(corners[1][1]);
  metrics.BottomLeft[0]  = vtkMath::Round(corners[2][0]);
  metrics.BottomLeft[1]  = vtkMath::Round(corners[2][1]);
  metrics.BottomRight[0] = vtkMath::Round(corners[3][0]);
  metrics.BottomRight[1] = vtkMath::Round(corners[3][1]);

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::RenderString(const char *str,
                                                  vtkImageData *image,
                                                  vtkTextProperty *tprop,
                                                  int dpi,
                                                  int textDims[2])
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<<"Matplotlib rendering is unavailable.");
    return false;
  }

  if (!this->MaskParser)
  {
    if (!this->InitializeMaskParser())
    {
      vtkErrorMacro(<<"MaskParser is not initialized!");
      return false;
    }
  }

  vtkDebugMacro(<<"Converting '" << str << "' into MathText image...");

  long int rows = 0;
  long int cols = 0;
  long int ind = 0;

  double *fgColor = tprop->GetColor();
  unsigned char fgR = static_cast<unsigned char>(fgColor[0] * 255);
  unsigned char fgG = static_cast<unsigned char>(fgColor[1] * 255);
  unsigned char fgB = static_cast<unsigned char>(fgColor[2] * 255);
  double fgA = tprop->GetOpacity();

  double *bgColor = tprop->GetBackgroundColor();
  unsigned char bgR = static_cast<unsigned char>(bgColor[0] * 255);
  unsigned char bgG = static_cast<unsigned char>(bgColor[1] * 255);
  unsigned char bgB = static_cast<unsigned char>(bgColor[2] * 255);
  double bgA = tprop->GetBackgroundOpacity();
  bool hasBackground = (static_cast<unsigned char>(bgA * 255) != 0);

  double *frameColor = tprop->GetFrameColor();
  unsigned char frR = static_cast<unsigned char>(frameColor[0] * 255);
  unsigned char frG = static_cast<unsigned char>(frameColor[1] * 255);
  unsigned char frB = static_cast<unsigned char>(frameColor[2] * 255);
  bool hasFrame = tprop->GetFrame() && tprop->GetFrameWidth() > 0;
  int frW = tprop->GetFrameWidth();

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject resultTuple(PyObject_CallMethod(this->MaskParser,
                                                   const_cast<char*>("to_mask"),
                                                   const_cast<char*>("sii"),
                                                   const_cast<char*>(str),
                                                   tprop->GetFontSize(), dpi));
  if (this->CheckForError(resultTuple.GetPointer()))
  {
    return false;
  }

  // numpyArray is a borrowed reference, no smart wrapper needed:
  PyObject *numpyArray = PyTuple_GetItem(resultTuple.GetPointer(), 0);
  if (this->CheckForError(numpyArray))
  {
    return false;
  }

  vtkSmartPyObject flatArray(PyObject_CallMethod(numpyArray,
                                              const_cast<char*>("flatten"),
                                              const_cast<char*>("")));
  if (this->CheckForError(flatArray.GetPointer()))
  {
    return false;
  }

  vtkSmartPyObject list(PyObject_CallMethod(flatArray.GetPointer(),
                                         const_cast<char*>("tolist"),
                                         const_cast<char*>("")));
  if (this->CheckForError(list.GetPointer()))
  {
    return false;
  }

  vtkSmartPyObject dimTuple(PyObject_GetAttrString(numpyArray,
                                                const_cast<char*>("shape")));
  if (this->CheckForError(dimTuple.GetPointer()))
  {
    return false;
  }

  PyArg_ParseTuple(dimTuple.GetPointer(), "ii", &rows, &cols);
  if (this->CheckForError())
  {
    return false;
  }

  //numPixels = PyObject_Length(list.GetPointer());
  if (this->CheckForError())
  {
    return false;
  }

  if (textDims)
  {
    textDims[0] = cols;
    textDims[1] = rows;
  }

  // Create justified bounding box.
  int bbox[4];
  this->GetJustifiedBBox(rows, cols, tprop, bbox);

  this->PrepareImageData(image, bbox);

  for (long int row = bbox[3]; row >= bbox[2]; --row)
  {
    for (long int col = bbox[0]; col <= bbox[1]; ++col)
    {
      // item is borrowed, no need for a smart wrapper
      PyObject *item = PyList_GetItem(list.GetPointer(), ind++);
      if (this->CheckForError(item))
      {
        return false;
      }
      const unsigned char val = static_cast<unsigned char>(PyInt_AsLong(item));
      if (this->CheckForError())
      {
        return false;
      }
      unsigned char *ptr =
          static_cast<unsigned char*>(image->GetScalarPointer(col, row, 0));

      if (hasFrame && (col < (bbox[0] + frW) || col > (bbox[1] - frW)
        || row > (bbox[3] - frW) || row < (bbox[2] + frW)))
      {
        const float fg_blend = fgA * (val / 255.f);
        const float fr_blend = 1.f - fg_blend;

        ptr[0] = static_cast<unsigned char>(fr_blend * frR + fg_blend * fgR);
        ptr[1] = static_cast<unsigned char>(fr_blend * frG + fg_blend * fgG);
        ptr[2] = static_cast<unsigned char>(fr_blend * frB + fg_blend * fgB);
        ptr[3] = 255;
      }
      else if (hasBackground)
      {
        const float fg_blend = fgA * (val / 255.f);
        const float bg_blend = 1.f - fg_blend;

        ptr[0] = static_cast<unsigned char>(bg_blend * bgR + fg_blend * fgR);
        ptr[1] = static_cast<unsigned char>(bg_blend * bgG + fg_blend * fgG);
        ptr[2] = static_cast<unsigned char>(bg_blend * bgB + fg_blend * fgB);
        ptr[3] = static_cast<unsigned char>(255 * (fg_blend + bgA * bg_blend));
      }
      else
      {
        ptr[0] = fgR;
        ptr[1] = fgG;
        ptr[2] = fgB;
        ptr[3] = static_cast<unsigned char>(val * fgA);
      }
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
  double corners[4][2] = { {static_cast<double>(bbox[0]),
                            static_cast<double>(bbox[2])},
                           {static_cast<double>(bbox[1]),
                            static_cast<double>(bbox[2])},
                           {static_cast<double>(bbox[0]),
                            static_cast<double>(bbox[3])},
                           {static_cast<double>(bbox[1]),
                            static_cast<double>(bbox[3])} };
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
    corners[3][0] = static_cast<double>(textDims[0]) ;
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
  this->PrepareImageData(dummyImage.GetPointer(), bbox);
  vtkNew<vtkImageReslice> rotator;
  rotator->SetInputData(image);
  rotator->SetInformationInput(dummyImage.GetPointer());
  rotator->SetResliceTransform(rotation.GetPointer());
  rotator->SetInterpolationModeToLinear();
  rotator->Update();
  image->ShallowCopy(rotator->GetOutput());

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::StringToPath(const char *str,
                                                  vtkPath *path,
                                                  vtkTextProperty *tprop,
                                                  int dpi)
{
  if (!this->IsAvailable())
  {
    vtkErrorMacro(<<"Matplotlib rendering is unavailable.");
    return false;
  }

  if (!this->PathParser)
  {
    if (!this->InitializePathParser())
    {
      vtkErrorMacro(<<"PathParser is not initialized!");
      return false;
    }
  }

  vtkDebugMacro(<<"Converting '" << str << "' into a vtkPath...");

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
  float origin[2] = {0.0, 0.0};
  float vert[2];
  float delta[2] = {0.0, 0.0};
  int code;
  bool hasOrigin = false;

  // Bounding box for all control points, used for justification
  float cbox[4] = {VTK_FLOAT_MAX, VTK_FLOAT_MAX, VTK_FLOAT_MIN, VTK_FLOAT_MIN};

  // The path is always generated using a 100pt font @72 dpi. Use this factor to
  // recover the font.
  const float fontScale = (tprop->GetFontSize() / 100.f) * (dpi / 72.f) ;

  path->Reset();

  // Create the font property
  vtkSmartPyObject pyFontProp(this->GetFontProperties(tprop));
  if (this->CheckForError(pyFontProp.GetPointer()))
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject pyResultTuple(
        PyObject_CallMethod(this->PathParser,
                            const_cast<char*>("get_text_path"),
                            const_cast<char*>("Osi"),
                            pyFontProp.GetPointer(),// prop
                            const_cast<char*>(str), // texstring
                            1,                      // boolean, ismath
                            0));                    // boolean, usetex
  if (this->CheckForError(pyResultTuple.GetPointer()))
  {
    return false;
  }

  // pyVerts and pyCodes are borrowed references -- no need for smart wrappers
  PyObject *pyVerts = PyTuple_GetItem(pyResultTuple.GetPointer(), 0);
  PyObject *pyCodes = PyTuple_GetItem(pyResultTuple.GetPointer(), 1);
  if (this->CheckForError(pyVerts)  ||
      this->CheckForError(pyCodes))
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
    if (this->CheckForError(pyVert.GetPointer()) ||
        this->CheckForError(pyCode.GetPointer()))
    {
      return false;
    }

    // pyVert is sometimes a numpy array, sometimes it's a tuple.
    // Initialize the following objects in the following conditional, then
    // convert to smart pointers afterwards.
    PyObject *pyVertXObj = NULL;
    PyObject *pyVertYObj = NULL;
    if (pyVert->ob_type == &PyTuple_Type)
    {
      pyVertXObj = PyTuple_GetItem(pyVert.GetPointer(), 0);
      pyVertYObj = PyTuple_GetItem(pyVert.GetPointer(), 1);
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
      vtkSmartPyObject pyVertList(PyObject_CallMethod(pyVert.GetPointer(),
                                                   const_cast<char*>("tolist"),
                                                   NULL));
      if (this->CheckForError(pyVertList.GetPointer()) ||
          PySequence_Size(pyVertList.GetPointer()) < 2)
      {
        return false;
      }

      pyVertXObj = PySequence_GetItem(pyVertList.GetPointer(), 0);
      pyVertYObj = PySequence_GetItem(pyVertList.GetPointer(), 1);
    }

    vtkSmartPyObject pyVertX(pyVertXObj);
    vtkSmartPyObject pyVertY(pyVertYObj);
    if (this->CheckForError(pyVertX.GetPointer()) ||
        this->CheckForError(pyVertY.GetPointer()))
    {
      return false;
    }

    vert[0] = PyFloat_AsDouble(pyVertX.GetPointer()) * fontScale;
    vert[1] = PyFloat_AsDouble(pyVertY.GetPointer()) * fontScale;
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

    code = PyInt_AsLong(pyCode.GetPointer());
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
        vtkWarningMacro(<<"Unrecognized code: " << code);
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
  vtkPoints *points = path->GetPoints();
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

//----------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::SetScaleToPowerOfTwo(bool val)
{
  if (this->ScaleToPowerOfTwo != val)
  {
    this->ScaleToPowerOfTwo = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetScaleToPowerOfTwo()
{
  return this->ScaleToPowerOfTwo;
}

//----------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MPLMathTextAvailable: ";
  switch (this->MPLMathTextAvailable)
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
