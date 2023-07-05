// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProgrammableAttributeDataFilter
 * @brief   manipulate attribute (cell and point) data via a user-specified function
 *
 * vtkProgrammableAttributeDataFilter is a filter that allows you to write a
 * custom procedure to manipulate attribute data - either point or cell
 * data. For example, you could generate scalars based on a complex formula;
 * convert vectors to normals; compute scalar values as a function of
 * vectors, texture coords, and/or any other point data attribute; and so
 * on. The filter takes multiple inputs (input plus an auxiliary input list),
 * so you can write procedures that combine several dataset point
 * attributes. Note that the output of the filter is the same type
 * (topology/geometry) as the input.
 *
 * The filter works as follows. It operates like any other filter (i.e.,
 * checking and managing modified and execution times, processing Update()
 * and Execute() methods, managing release of data, etc.), but the difference
 * is that the Execute() method simply invokes a user-specified function with
 * an optional (void *) argument (typically the "this" pointer in C++). It is
 * also possible to specify a function to delete the argument via
 * ExecuteMethodArgDelete().
 *
 * To use the filter, you write a procedure to process the input datasets,
 * process the data, and generate output data. Typically, this means grabbing
 * the input point or cell data (using GetInput() and maybe GetInputList()),
 * operating on it (creating new point and cell attributes such as scalars,
 * vectors, etc.), and then setting the point and/or cell attributes in the
 * output dataset (you'll need to use GetOutput() to access the output).
 * (Note: besides C++, it is possible to do the same thing in Java or
 * other languages that wrap the C++ core.) Remember, proper filter protocol
 * requires that you don't modify the input data - you create new output data
 * from the input.
 *
 * @warning
 * This filter operates on any combination of the filter input plus a list of
 * additional inputs (at a minimum you must set the filter input via
 * SetInput()).  It is up to you check whether the input is valid, and to
 * ensure that the output is valid. Also, you have to write the control
 * structure for the traversal and operation on the point and cell attribute
 * data.
 *
 * @warning
 * By default the output point and cell data will be copied through from the
 * input point data (using reference counting).  You can control this using
 * the output's CopyAllOff() flag, or by using individual flags for each
 * point data field (i.e., scalars, vectors, etc.)
 *
 * @warning
 * The output of this filter is the abstract type vtkDataSet, even if your
 * input is a concrete type like vtkPolyData. Thus you may need to use
 * vtkCastToConcrete to obtain the output as a particular concrete type, or
 * one of the special methods of the superclass (e.g.,
 * vtkDataSetAlgorithm::GetPolyDataOutput) to retrieve output of the
 * correct type.
 *
 * @warning
 * The filter correctly manages modified time and network execution in most
 * cases. However, if you change the definition of the filter function,
 * you'll want to send a manual Modified() method to the filter to force it
 * to reexecute.
 */

#ifndef vtkProgrammableAttributeDataFilter_h
#define vtkProgrammableAttributeDataFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersProgrammableModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSetCollection;

class VTKFILTERSPROGRAMMABLE_EXPORT vtkProgrammableAttributeDataFilter : public vtkDataSetAlgorithm
{
public:
  static vtkProgrammableAttributeDataFilter* New();
  vtkTypeMacro(vtkProgrammableAttributeDataFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a dataset to the list of data to process.
   */
  void AddInput(vtkDataSet* in);

  /**
   * Remove a dataset from the list of data to process.
   */
  void RemoveInput(vtkDataSet* in);

  /**
   * Return the list of inputs.
   */
  vtkDataSetCollection* GetInputList() { return this->InputList; }

  /**
   * Signature definition for programmable method callbacks. Methods passed to
   * SetExecuteMethod or SetExecuteMethodArgDelete must conform to this
   * signature.
   * The presence of this typedef is useful for reference and for external
   * analysis tools, but it cannot be used in the method signatures in these
   * header files themselves because it prevents the internal VTK wrapper
   * generators from wrapping these methods.
   */
  typedef void (*ProgrammableMethodCallbackType)(void* arg);

  /**
   * Specify the function to use to operate on the point attribute data. Note
   * that the function takes a single (void *) argument.
   */
  void SetExecuteMethod(void (*f)(void*), void* arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetExecuteMethodArgDelete(void (*f)(void*));

protected:
  vtkProgrammableAttributeDataFilter();
  ~vtkProgrammableAttributeDataFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkDataSetCollection* InputList;              // list of datasets to process
  ProgrammableMethodCallbackType ExecuteMethod; // function to invoke
  ProgrammableMethodCallbackType ExecuteMethodArgDelete;
  void* ExecuteMethodArg;

  void ReportReferences(vtkGarbageCollector*) override;

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject*)
  {
    vtkErrorMacro(<< "AddInput() must be called with a vtkDataSet not a vtkDataObject.");
  }

  vtkProgrammableAttributeDataFilter(const vtkProgrammableAttributeDataFilter&) = delete;
  void operator=(const vtkProgrammableAttributeDataFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
