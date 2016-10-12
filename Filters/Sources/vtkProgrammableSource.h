/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProgrammableSource
 * @brief   generate source dataset via a user-specified function
 *
 * vtkProgrammableSource is a source object that is programmable by the
 * user. To use this object, you must specify a function that creates the
 * output.  It is possible to generate an output dataset of any (concrete)
 * type; it is up to the function to properly initialize and define the
 * output. Typically, you use one of the methods to get a concrete output
 * type (e.g., GetPolyDataOutput() or GetStructuredPointsOutput()), and
 * then manipulate the output in the user-specified function.
 *
 * Example use of this include writing a function to read a data file or
 * interface to another system. (You might want to do this in favor of
 * deriving a new class.) Another important use of this class is that it
 * allows users of interpreters (e.g., Tcl or Java) the ability to write
 * source objects without having to recompile C++ code or generate new
 * libraries.
 * @sa
 * vtkProgrammableFilter vtkProgrammableAttributeDataFilter
 * vtkProgrammableDataObjectSource
*/

#ifndef vtkProgrammableSource_h
#define vtkProgrammableSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;

class VTKFILTERSSOURCES_EXPORT vtkProgrammableSource : public vtkDataSetAlgorithm
{
public:
  static vtkProgrammableSource *New();
  vtkTypeMacro(vtkProgrammableSource,vtkDataSetAlgorithm);

  /**
   * Signature definition for programmable method callbacks. Methods passed
   * to SetExecuteMethod, SetExecuteMethodArgDelete or
   * SetRequestInformationMethod must conform to this signature.
   * The presence of this typedef is useful for reference and for external
   * analysis tools, but it cannot be used in the method signatures in these
   * header files themselves because it prevents the internal VTK wrapper
   * generators from wrapping these methods.
   */
  typedef void (*ProgrammableMethodCallbackType)(void *arg);

  /**
   * Specify the function to use to generate the source data. Note
   * that the function takes a single (void *) argument.
   */
  void SetExecuteMethod(void (*f)(void *), void *arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetExecuteMethodArgDelete(void (*f)(void *));

  /**
   * Specify the function to use to fill in information about the source data.
   */
  void SetRequestInformationMethod(void (*f)(void *));

  /**
   * Get the output as a concrete type. This method is typically used by the
   * writer of the source function to get the output as a particular type
   * (i.e., it essentially does type casting). It is the users responsibility
   * to know the correct type of the output data.
   */
  vtkPolyData *GetPolyDataOutput();

  /**
   * Get the output as a concrete type.
   */
  vtkStructuredPoints *GetStructuredPointsOutput();

  /**
   * Get the output as a concrete type.
   */
  vtkStructuredGrid *GetStructuredGridOutput();

  /**
   * Get the output as a concrete type.
   */
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  /**
   * Get the output as a concrete type.
   */
  vtkRectilinearGrid *GetRectilinearGridOutput();

protected:
  vtkProgrammableSource();
  ~vtkProgrammableSource() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestDataObject(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  ProgrammableMethodCallbackType ExecuteMethod; //function to invoke
  ProgrammableMethodCallbackType ExecuteMethodArgDelete;
  void *ExecuteMethodArg;
  ProgrammableMethodCallbackType RequestInformationMethod; // function to invoke

  vtkTimeStamp ExecuteTime;
  int RequestedDataType;

private:
  vtkProgrammableSource(const vtkProgrammableSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProgrammableSource&) VTK_DELETE_FUNCTION;
};

#endif

// VTK-HeaderTest-Exclude: vtkProgrammableSource.h
