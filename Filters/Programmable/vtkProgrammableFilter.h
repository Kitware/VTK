/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProgrammableFilter
 * @brief   a user-programmable filter
 *
 * vtkProgrammableFilter is a filter that can be programmed by the user.  To
 * use the filter you define a function that retrieves input of the correct
 * type, creates data, and then manipulates the output of the filter.  Using
 * this filter avoids the need for subclassing - and the function can be
 * defined in an interpreter wrapper language such as Java.
 *
 * The trickiest part of using this filter is that the input and output
 * methods are unusual and cannot be compile-time type checked. Instead, as a
 * user of this filter it is your responsibility to set and get the correct
 * input and output types.
 *
 * @warning
 * The filter correctly manages modified time and network execution in most
 * cases. However, if you change the definition of the filter function,
 * you'll want to send a manual Modified() method to the filter to force it
 * to reexecute.
 *
 * @sa
 * vtkProgrammablePointDataFilter vtkProgrammableSource
*/

#ifndef vtkProgrammableFilter_h
#define vtkProgrammableFilter_h

#include "vtkFiltersProgrammableModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"


class VTKFILTERSPROGRAMMABLE_EXPORT vtkProgrammableFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkProgrammableFilter *New();
  vtkTypeMacro(vtkProgrammableFilter,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Signature definition for programmable method callbacks. Methods passed to
   * SetExecuteMethod or SetExecuteMethodArgDelete must conform to this
   * signature.
   * The presence of this typedef is useful for reference and for external
   * analysis tools, but it cannot be used in the method signatures in these
   * header files themselves because it prevents the internal VTK wrapper
   * generators from wrapping these methods.
   */
  typedef void (*ProgrammableMethodCallbackType)(void *arg);

  /**
   * Specify the function to use to operate on the point attribute data. Note
   * that the function takes a single (void *) argument.
   */
  void SetExecuteMethod(void (*f)(void *), void *arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetExecuteMethodArgDelete(void (*f)(void *));

  //@{
  /**
   * Get the input as a concrete type. This method is typically used by the
   * writer of the filter function to get the input as a particular type (i.e.,
   * it essentially does type casting). It is the users responsibility to know
   * the correct type of the input data.
   */
  vtkPolyData *GetPolyDataInput();
  vtkStructuredPoints *GetStructuredPointsInput();
  vtkStructuredGrid *GetStructuredGridInput();
  vtkUnstructuredGrid *GetUnstructuredGridInput();
  vtkRectilinearGrid *GetRectilinearGridInput();
  vtkGraph *GetGraphInput();
  vtkMolecule *GetMoleculeInput();
  vtkTable *GetTableInput();
  //@}

  //@{
  /**
   * When CopyArrays is true, all arrays are copied to the output
   * iff input and output are of the same type. False by default.
   */
  vtkSetMacro(CopyArrays, bool);
  vtkGetMacro(CopyArrays, bool);
  vtkBooleanMacro(CopyArrays, bool);
  //@}

protected:
  vtkProgrammableFilter();
  ~vtkProgrammableFilter() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  ProgrammableMethodCallbackType ExecuteMethod; //function to invoke
  ProgrammableMethodCallbackType ExecuteMethodArgDelete;
  void *ExecuteMethodArg;

  bool CopyArrays;

private:
  vtkProgrammableFilter(const vtkProgrammableFilter&) = delete;
  void operator=(const vtkProgrammableFilter&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkProgrammableFilter.h
