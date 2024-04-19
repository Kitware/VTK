// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * allows users of interpreters (e.g., Java) the ability to write
 * source objects without having to recompile C++ code or generate new
 * libraries.
 * @sa
 * vtkProgrammableFilter vtkProgrammableAttributeDataFilter
 * vtkProgrammableDataObjectSource
 */

#ifndef vtkProgrammableSource_h
#define vtkProgrammableSource_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersSourcesModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;
class vtkMolecule;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkStructuredPoints;
class vtkTable;
class vtkUnstructuredGrid;

class VTKFILTERSSOURCES_EXPORT vtkProgrammableSource : public vtkDataObjectAlgorithm
{
public:
  static vtkProgrammableSource* New();
  vtkTypeMacro(vtkProgrammableSource, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Signature definition for programmable method callbacks. Methods passed
   * to SetExecuteMethod, SetExecuteMethodArgDelete or
   * SetRequestInformationMethod must conform to this signature.
   * The presence of this typedef is useful for reference and for external
   * analysis tools, but it cannot be used in the method signatures in these
   * header files themselves because it prevents the internal VTK wrapper
   * generators from wrapping these methods.
   */
  typedef void (*ProgrammableMethodCallbackType)(void* arg);

  /**
   * Specify the function to use to generate the source data. Note
   * that the function takes a single (void *) argument.
   */
  void SetExecuteMethod(void (*f)(void*), void* arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetExecuteMethodArgDelete(void (*f)(void*));

  /**
   * Specify the function to use to fill in information about the source data.
   */
  void SetRequestInformationMethod(void (*f)(void*));

  ///@{
  /**
   * Get the output as a concrete type. This method is typically used by the
   * writer of the source function to get the output as a particular type
   * (i.e., it essentially does type casting). It is the users responsibility
   * to know the correct type of the output data.
   */
  vtkPolyData* GetPolyDataOutput();
  vtkStructuredPoints* GetStructuredPointsOutput();
  vtkStructuredGrid* GetStructuredGridOutput();
  vtkUnstructuredGrid* GetUnstructuredGridOutput();
  vtkRectilinearGrid* GetRectilinearGridOutput();
  vtkGraph* GetGraphOutput();
  vtkMolecule* GetMoleculeOutput();
  vtkTable* GetTableOutput();
  ///@}

protected:
  vtkProgrammableSource();
  ~vtkProgrammableSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ProgrammableMethodCallbackType ExecuteMethod; // function to invoke
  ProgrammableMethodCallbackType ExecuteMethodArgDelete;
  void* ExecuteMethodArg;
  ProgrammableMethodCallbackType RequestInformationMethod; // function to invoke

  vtkTimeStamp ExecuteTime;
  int RequestedDataType;

private:
  vtkProgrammableSource(const vtkProgrammableSource&) = delete;
  void operator=(const vtkProgrammableSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
