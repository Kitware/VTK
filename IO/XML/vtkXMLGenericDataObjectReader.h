// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLGenericDataObjectReader
 * @brief   Read any type of vtk data object
 *
 * vtkXMLGenericDataObjectReader reads any type of vtk data object encoded
 * in XML format.
 *
 * @sa
 * vtkGenericDataObjectReader
 */

#ifndef vtkXMLGenericDataObjectReader_h
#define vtkXMLGenericDataObjectReader_h

#include "vtkDeprecation.h"  // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOXMLModule.h"  // For export macro
#include "vtkSmartPointer.h" // for API
#include "vtkXMLDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHierarchicalBoxDataSet;
class vtkImageData;
class vtkMultiBlockDataSet;
class vtkOverlappingAMR;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTKIOXML_EXPORT vtkXMLGenericDataObjectReader : public vtkXMLDataReader
{
public:
  vtkTypeMacro(vtkXMLGenericDataObjectReader, vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLGenericDataObjectReader* New();

  ///@{
  /**
   * Get the reader's output.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int idx);
  ///@}

  ///@{
  /**
   * Get the output as various concrete types. This method is typically used
   * when you know exactly what type of data is being read.  Otherwise, use
   * the general GetOutput() method. If the wrong type is used nullptr is
   * returned.  (You must also set the filename of the object prior to
   * getting the output.)
   */
  VTK_DEPRECATED_IN_9_5_0("This function is deprecated, use GetOverlappingAMROutput")
  vtkHierarchicalBoxDataSet* GetHierarchicalBoxDataSetOutput();
  vtkOverlappingAMR* GetOverlappingAMROutput();
  vtkImageData* GetImageDataOutput();
  vtkMultiBlockDataSet* GetMultiBlockDataSetOutput();
  vtkPolyData* GetPolyDataOutput();
  vtkRectilinearGrid* GetRectilinearGridOutput();
  vtkStructuredGrid* GetStructuredGridOutput();
  vtkUnstructuredGrid* GetUnstructuredGridOutput();
  ///@}

  /**
   * Overridden method.
   */
  vtkIdType GetNumberOfPoints() override;

  /**
   * Overridden method.
   */
  vtkIdType GetNumberOfCells() override;

  /**
   * Overridden method. Not Used. Delegated.
   */
  void SetupEmptyOutput() override;

  /**
   * This method can be used to find out the type of output expected without
   * needing to read the whole file.
   */
  virtual int ReadOutputType(const char* name, bool& parallel);

  /**
   * Helper to create a reader based on the data object type.
   * Returns null if the reader cannot be determined.
   */
  static vtkSmartPointer<vtkXMLReader> CreateReader(int data_object_type, bool parallel);

protected:
  vtkXMLGenericDataObjectReader();
  ~vtkXMLGenericDataObjectReader() override;

  /**
   * Overridden method. Not used. Return "vtkDataObject".
   */
  const char* GetDataSetName() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  vtkXMLReader* Reader; // actual reader

private:
  vtkXMLGenericDataObjectReader(const vtkXMLGenericDataObjectReader&) = delete;
  void operator=(const vtkXMLGenericDataObjectReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
