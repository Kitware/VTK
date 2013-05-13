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

// Smart pointer for PyObjects. Calls Py_XDECREF when scope ends.
class SmartPyObject
{
  PyObject *Object;

public:
  SmartPyObject(PyObject *obj = NULL)
    : Object(obj)
  {
  }

  ~SmartPyObject()
  {
    Py_XDECREF(this->Object);
  }

  PyObject *operator->() const
  {
    return this->Object;
  }

  PyObject *GetPointer() const
  {
    return this->Object;
  }
};

//----------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities::Availablity
vtkMatplotlibMathTextUtilities::MPLMathTextAvailable =
vtkMatplotlibMathTextUtilities::NOT_TESTED;

// A macro that is used in New() to print warnings if VTK_MATPLOTLIB_DEBUG
// is defined in the environment. Use vtkGenericWarningMacro to allow this to
// work in release mode builds.
#define vtkMplStartUpDebugMacro(x) if(debug){vtkGenericWarningMacro(x);}

namespace {

  //----------------------------------------------------------------------------
  // Used to replace "\ " with " " in paths.
  void UnEscapeSpaces(std::string &str)
    {
    size_t pos = str.rfind("\\ ");
    while (pos != std::string::npos)
      {
      str.erase(pos, 1);
      pos = str.rfind("\\ ", pos);
      }
    }

} // end anon namespace

//----------------------------------------------------------------------------
void vtkMatplotlibMathTextUtilities::CheckMPLAvailability()
{
  if (vtkMatplotlibMathTextUtilities::MPLMathTextAvailable != NOT_TESTED)
    {
    // Already tested. Nothing to do now.
    return;
    }

  // Enable startup debugging output. This will be set to true when
  // VTK_MATPLOTLIB_DEBUG is defined in the process environment.
  bool debug = (vtksys::SystemTools::GetEnv("VTK_MATPLOTLIB_DEBUG") != NULL);

  // Initialize the python interpretor if needed
  vtkMplStartUpDebugMacro("Initializing Python, if not already.");
  vtkPythonInterpreter::Initialize();
  vtkMplStartUpDebugMacro("Attempting to import matplotlib.");
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
    SmartPyObject typeStr(PyObject_Str(type));
    SmartPyObject valueStr(PyObject_Str(value));
    SmartPyObject tracebackStr(PyObject_Str(traceback));
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
}

//----------------------------------------------------------------------------
vtkMatplotlibMathTextUtilities* vtkMatplotlibMathTextUtilities::New()
{
  vtkMatplotlibMathTextUtilities::CheckMPLAvailability();

  // Attempt to import matplotlib to check for availability
  switch (vtkMatplotlibMathTextUtilities::MPLMathTextAvailable)
    {
  case vtkMatplotlibMathTextUtilities::AVAILABLE:
    break;

  case vtkMatplotlibMathTextUtilities::NOT_TESTED:
  case vtkMatplotlibMathTextUtilities::UNAVAILABLE:
  default:
    return NULL;
    }

  // Adapted from VTK_OBJECT_FACTORY_NEW_BODY to enable debugging output when
  // requested.
  vtkObject* ret =
    vtkObjectFactory::CreateInstance("vtkMatplotlibMathTextUtilities");
  if (ret)
    {
    return static_cast<vtkMatplotlibMathTextUtilities*>(ret);
    }

  return new vtkMatplotlibMathTextUtilities;
}

vtkInstantiatorNewMacro(vtkMatplotlibMathTextUtilities)
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
  Py_XDECREF(this->MaskParser);
  Py_XDECREF(this->PathParser);
  Py_XDECREF(this->FontPropertiesClass);

  this->MaskParser = NULL;
  this->PathParser = NULL;
  this->FontPropertiesClass = NULL;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::InitializeMaskParser()
{
  // ensure that Python is initialized.
  vtkPythonInterpreter::Initialize();

  SmartPyObject mplMathTextLib(PyImport_ImportModule("matplotlib.mathtext"));
  if (this->CheckForError(mplMathTextLib.GetPointer()))
    {
    return false;
    }

  SmartPyObject mathTextParserClass(
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

  SmartPyObject mplTextPathLib(PyImport_ImportModule("matplotlib.textpath"));
  if (this->CheckForError(mplTextPathLib.GetPointer()))
    {
    return false;
    }

  SmartPyObject textToPathClass(
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

  SmartPyObject mplFontManagerLib(
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
      SmartPyObject typeStr(PyObject_Str(type));
      SmartPyObject valueStr(PyObject_Str(value));
      SmartPyObject tracebackStr(PyObject_Str(traceback));
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

  return PyObject_CallFunction(this->FontPropertiesClass,
                               const_cast<char*>("sssssi"), tpropFamily,
                               tpropStyle, tpropVariant, tpropStyle,
                               tpropWeight, tpropFontSize);
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
                                                      int bbox[4])
{
  int width = bbox[1] - bbox[0] + 1;
  int height = bbox[3] - bbox[2] + 1;
  // If the current image data is too small to render the text,
  // or more than twice as big (too hungry), then resize
  int imgDims[3], newImgDims[3];
  data->GetDimensions(imgDims);

  if (data->GetScalarType() != VTK_UNSIGNED_CHAR ||
      data->GetNumberOfScalarComponents() != 4 ||
      imgDims[0] < width || imgDims[1] < height ||
      width * 2 < imgDims[0] || height * 2 < imgDims[1])
    {
    // Scale to the next highest power of 2 if required.
    if (this->ScaleToPowerOfTwo)
      {
      newImgDims[0] = newImgDims[1] = 1;
      while (newImgDims[0] < width)
        {
        newImgDims[0] *= 2;
        }
      while (newImgDims[1] < height)
        {
        newImgDims[1] *= 2;
        }
      }
    else
      {
      newImgDims[0] = width;
      newImgDims[1] = height;
      }
    newImgDims[2] = 1;

    // Allocate the new image if needed
    if (data->GetScalarType() != VTK_UNSIGNED_CHAR ||
        data->GetNumberOfScalarComponents() != 4 ||
        newImgDims[0] != imgDims[0] ||
        newImgDims[1] != imgDims[1] ||
        newImgDims[2] != imgDims[2])
      {
      data->SetExtent(bbox[0], bbox[0] + newImgDims[0] - 1,
                      bbox[2], bbox[2] + newImgDims[1] - 1,
                      0, 0);
      data->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
      }
    }

  // Clear the image
  memset(data->GetScalarPointer(), 0,
         (data->GetNumberOfPoints() * data->GetNumberOfScalarComponents()));

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::GetBoundingBox(
    vtkTextProperty *tprop, const char *str, unsigned int dpi, int bbox[4])
{
  if (!this->MaskParser)
    {
    if (!this->InitializeMaskParser())
      {
      vtkErrorMacro(<<"MaskParser is not initialized!");
      return false;
      }
    }

  vtkDebugMacro(<<"Calculating bbox for '" << str << "'");

  long int rows = 0;
  long int cols = 0;
  // matplotlib.mathtext seems to mishandle the dpi, this conversion makes the
  // text size match the images produced by vtkFreeTypeUtilities, as well as the
  // paths generated by StringToPath
  long int fontSize = tprop->GetFontSize() * 72.0 / static_cast<float>(dpi);

  SmartPyObject resultTuple(PyObject_CallMethod(this->MaskParser,
                                                const_cast<char*>("to_mask"),
                                                const_cast<char*>("sii"),
                                                const_cast<char*>(str),
                                                fontSize, dpi));
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

  SmartPyObject dimTuple(PyObject_GetAttrString(numpyArray,
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


  // Determine the dimensions of the rotated image
  double angleDeg = tprop->GetOrientation();
  // Corners of original image
  double corners[4][2] = { {0, 0},
                           {static_cast<double>(cols), 0},
                           {0, static_cast<double>(rows)},
                           {static_cast<double>(cols), static_cast<double>(rows)} };

  double bboxd[4];
  this->RotateCorners(angleDeg, corners, bboxd);
  bbox[0] = vtkMath::Ceil(bboxd[0]);
  bbox[1] = vtkMath::Ceil(bboxd[1]);
  bbox[2] = vtkMath::Ceil(bboxd[2]);
  bbox[3] = vtkMath::Ceil(bboxd[3]);

  return true;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibMathTextUtilities::RenderString(const char *str,
                                                  vtkImageData *image,
                                                  vtkTextProperty *tprop,
                                                  unsigned int dpi,
                                                  int textDims[2])
{
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
  //long int numPixels = 0;
  // matplotlib.mathtext seems to mishandle the dpi, this conversion makes the
  // text size match the images produced by vtkFreeTypeUtilities, as well as the
  // paths generated by StringToPath
  long int fontSize = tprop->GetFontSize() * 72.0 / static_cast<float>(dpi);
  double *dcolor = tprop->GetColor();
  unsigned char r = static_cast<unsigned char>(dcolor[0] * 255);
  unsigned char g = static_cast<unsigned char>(dcolor[1] * 255);
  unsigned char b = static_cast<unsigned char>(dcolor[2] * 255);
  double alpha = tprop->GetOpacity();

  SmartPyObject resultTuple(PyObject_CallMethod(this->MaskParser,
                                                const_cast<char*>("to_mask"),
                                                const_cast<char*>("sii"),
                                                const_cast<char*>(str),
                                                fontSize, dpi));
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

  SmartPyObject flatArray(PyObject_CallMethod(numpyArray,
                                              const_cast<char*>("flatten"),
                                              const_cast<char*>("")));
  if (this->CheckForError(flatArray.GetPointer()))
    {
    return false;
    }

  SmartPyObject list(PyObject_CallMethod(flatArray.GetPointer(),
                                         const_cast<char*>("tolist"),
                                         const_cast<char*>("")));
  if (this->CheckForError(list.GetPointer()))
    {
    return false;
    }

  SmartPyObject dimTuple(PyObject_GetAttrString(numpyArray,
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

  int bbox[4] = {0, static_cast<int>(cols - 1),
                 0, static_cast<int>(rows - 1)};
  this->PrepareImageData(image, bbox);

  for (long int row = rows-1; row >= 0; --row)
    {
    for (long int col = 0; col < cols; ++col)
      {
      // item is borrowed, no need for a smart wrapper
      PyObject *item = PyList_GetItem(list.GetPointer(), ind++);
      if (this->CheckForError(item))
        {
        return false;
        }
      unsigned char val =
          static_cast<unsigned char>(alpha * PyInt_AsLong(item));
      if (this->CheckForError())
        {
        return false;
        }
      unsigned char *ptr =
          static_cast<unsigned char*>(image->GetScalarPointer(col, row, 0));
      ptr[0] = r;
      ptr[1] = g;
      ptr[2] = b;
      ptr[3] = val;
      }
    }

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
  double bbox2[4];

  // Rotate the corners of the image and determine the bounding box
  this->RotateCorners(angleDeg, corners, bbox2);

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

  bbox[0] = static_cast<int>(bbox2[0]);
  bbox[1] = static_cast<int>(bbox2[1]);
  bbox[2] = static_cast<int>(bbox2[2]);
  bbox[3] = static_cast<int>(bbox2[3]);

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
                                                  vtkTextProperty *tprop)
{
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

  // The path is always generated at a font size of 100. Use this factor to
  // recover the font.
  const float fontScale = ((tprop->GetFontSize()) / 100.);
  path->Reset();

  // Create the font property
  SmartPyObject pyFontProp(this->GetFontProperties(tprop));
  if (this->CheckForError(pyFontProp.GetPointer()))
    {
    return false;
    }

  SmartPyObject pyResultTuple(
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
    SmartPyObject pyVert(PySequence_GetItem(pyVerts, i));
    SmartPyObject pyCode(PySequence_GetItem(pyCodes, i));
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
      SmartPyObject pyVertList(PyObject_CallMethod(pyVert.GetPointer(),
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

    SmartPyObject pyVertX(pyVertXObj);
    SmartPyObject pyVertY(pyVertYObj);
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
void vtkMatplotlibMathTextUtilities::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaskParser: " << this->MaskParser << endl;
  os << indent << "PathParser: " << this->PathParser << endl;
  os << indent << "FontPropertiesClass: " << this->FontPropertiesClass << endl;
}
