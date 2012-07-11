/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextUtilities.h"

#include "vtkPython.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkInstantiatorNewMacro(vtkMathTextUtilities);

//----------------------------------------------------------------------------
// The singleton, and the singleton cleanup
vtkMathTextUtilities* vtkMathTextUtilities::Instance = NULL;
vtkMathTextUtilitiesCleanup vtkMathTextUtilities::Cleanup;

//----------------------------------------------------------------------------
// Create the singleton cleanup
// Register our singleton cleanup callback against the FTLibrary so that
// it might be called before the FTLibrary singleton is destroyed.
vtkMathTextUtilitiesCleanup::vtkMathTextUtilitiesCleanup()
{
}

//----------------------------------------------------------------------------
// Delete the singleton cleanup
vtkMathTextUtilitiesCleanup::~vtkMathTextUtilitiesCleanup()
{
  vtkMathTextUtilities::SetInstance(NULL);
}

//----------------------------------------------------------------------------
vtkMathTextUtilities* vtkMathTextUtilities::GetInstance()
{
  if (!vtkMathTextUtilities::Instance)
    {
    vtkMathTextUtilities::Instance = static_cast<vtkMathTextUtilities *>(
      vtkObjectFactory::CreateInstance("vtkMathTextUtilities"));
    if (!vtkMathTextUtilities::Instance)
      {
      vtkMathTextUtilities::Instance = new vtkMathTextUtilities;
      }
    }

  return vtkMathTextUtilities::Instance;
}

//----------------------------------------------------------------------------
void vtkMathTextUtilities::SetInstance(vtkMathTextUtilities* instance)
{
  if (vtkMathTextUtilities::Instance == instance)
    {
    return;
    }

  if (vtkMathTextUtilities::Instance)
    {
    vtkMathTextUtilities::Instance->Delete();
    }

  vtkMathTextUtilities::Instance = instance;

  // User will call ->Delete() after setting instance
  if (instance)
    {
    instance->Register(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMathTextUtilities* vtkMathTextUtilities::New()
{
  vtkMathTextUtilities* ret = vtkMathTextUtilities::GetInstance();
  ret->Register(NULL);
  return ret;
}

//----------------------------------------------------------------------------
vtkMathTextUtilities::vtkMathTextUtilities()
  : Superclass(), PythonIsInitialized(false), MaskParser(NULL)
{
}

//----------------------------------------------------------------------------
vtkMathTextUtilities::~vtkMathTextUtilities()
{
  Py_XDECREF(this->MaskParser);
  if (this->PythonIsInitialized)
    {
    Py_Finalize();
    }
}

//----------------------------------------------------------------------------
bool vtkMathTextUtilities::InitializePython()
{
  if (this->PythonIsInitialized)
    {
    return true;
    }
  Py_Initialize();
  this->PythonIsInitialized = !this->CheckForError();
  return this->PythonIsInitialized;
}

//----------------------------------------------------------------------------
bool vtkMathTextUtilities::InitializeMaskParser()
{
  if (!this->InitializePython())
    {
    return false;
    }
  PyObject *mplMathTextLib = NULL;
  PyObject *mathTextParserClass = NULL;
  bool result = false;

  mplMathTextLib = PyImport_ImportModule("matplotlib.mathtext");
  if (this->CheckForError(mplMathTextLib))
    {
    goto initializeMaskParserCleanup;
    }

  mathTextParserClass =
      PyObject_GetAttrString(mplMathTextLib, "MathTextParser");
  if (this->CheckForError(mathTextParserClass))
    {
    goto initializeMaskParserCleanup;
    }

  this->MaskParser =
      PyObject_CallFunction(mathTextParserClass, const_cast<char*>("s"),
                            "bitmap");
  if (this->CheckForError(this->MaskParser))
    {
    // in case the error was an exception.
    delete this->MaskParser;
    this->MaskParser = NULL;
    goto initializeMaskParserCleanup;
    }

  result = true;

initializeMaskParserCleanup:
  Py_XDECREF(mplMathTextLib);
  Py_XDECREF(mathTextParserClass);

  return result;
}

//----------------------------------------------------------------------------
bool vtkMathTextUtilities::CheckForError()
{
  PyObject *exception = PyErr_Occurred();
  if (exception)
    {
    vtkDebugMacro(<< "Python exception raised.");
    PyErr_PrintEx(0);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkMathTextUtilities::CheckForError(PyObject *object)
{
  // Print any exceptions
  bool result = this->CheckForError();

  if (object == NULL)
    {
    vtkErrorMacro(<< "Object is NULL!");
    return true;
    }
  return result;
}

//----------------------------------------------------------------------------
bool vtkMathTextUtilities::RenderString(const char *str, vtkImageData *image,
                                       unsigned int dpi)
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

  PyObject *resultTuple = NULL;
  PyObject *numpyArray = NULL;
  PyObject *flatArray = NULL;
  PyObject *list = NULL;
  PyObject *dimTuple = NULL;
  PyObject *item = NULL;

  long int rows = 0;
  long int cols = 0;
  long int ind = 0;
  long int numPixels = 0;
  bool result = false;

  resultTuple = PyObject_CallMethod(this->MaskParser,
                                    const_cast<char*>("to_mask"),
                                    const_cast<char*>("si"),
                                    const_cast<char*>(str), dpi);

  if (this->CheckForError(resultTuple))
    {
    goto renderStringCleanup;
    }

  numpyArray = PyTuple_GetItem(resultTuple, 0);
  if (this->CheckForError(numpyArray))
    {
    goto renderStringCleanup;
    }

  flatArray = PyObject_CallMethod(numpyArray,
                                  const_cast<char*>("flatten"),
                                  const_cast<char*>(""));
  if (this->CheckForError(flatArray))
    {
    goto renderStringCleanup;
    }

  list = PyObject_CallMethod(flatArray, const_cast<char*>("tolist"),
                             const_cast<char*>(""));
  if (this->CheckForError(list))
    {
    goto renderStringCleanup;
    }
  dimTuple = PyObject_GetAttrString(numpyArray,
                                    const_cast<char*>("shape"));
  if (this->CheckForError(dimTuple))
    {
    goto renderStringCleanup;
    }

  PyArg_ParseTuple(dimTuple, "ii", &rows, &cols);
  if (this->CheckForError())
    {
    goto renderStringCleanup;
    }

  numPixels = PyObject_Length(list);
  if (this->CheckForError())
    {
    goto renderStringCleanup;
    }

  image->SetDimensions(cols, rows, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, rows*cols);
  for (long int row = rows-1; row >= 0; --row)
    {
    for (long int col = 0; col < cols; ++col)
      {
      item = PyList_GetItem(list, ind++);
      if (this->CheckForError(item))
        {
        goto renderStringCleanup;
        }
      unsigned char val = static_cast<unsigned char>(PyInt_AsLong(item));
      if (this->CheckForError())
        {
        goto renderStringCleanup;
        }
      *static_cast<unsigned char*>(image->GetScalarPointer(col, row, 0)) = val;
      }
    }

renderStringCleanup:

  Py_XDECREF(resultTuple);
  // Borrowed reference from resultTuple: Py_XDECREF(numpyArray);
  Py_XDECREF(flatArray);
  Py_XDECREF(list);
  Py_XDECREF(dimTuple);
  return result;
}

//----------------------------------------------------------------------------
void vtkMathTextUtilities::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PythonIsInitialized: " << this->PythonIsInitialized << endl;
  os << indent << "MaskParser: " << this->MaskParser << endl;
}
