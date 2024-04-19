// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetReader
 * @brief   class to read any type of vtk dataset
 *
 * vtkDataSetReader is a class that provides instance variables and methods
 * to read any type of dataset in Visualization Toolkit (vtk) format.  The
 * output type of this class will vary depending upon the type of data
 * file. Convenience methods are provided to keep the data as a particular
 * type. (See text for format description details).
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkDataReader vtkPolyDataReader vtkRectilinearGridReader
 * vtkStructuredPointsReader vtkStructuredGridReader vtkUnstructuredGridReader
 */

#ifndef vtkDataSetReader_h
#define vtkDataSetReader_h

#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkStructuredPoints;
class vtkUnstructuredGrid;

class VTKIOLEGACY_EXPORT vtkDataSetReader : public vtkDataReader
{
public:
  static vtkDataSetReader* New();
  vtkTypeMacro(vtkDataSetReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this filter
   */
  vtkDataSet* GetOutput();
  vtkDataSet* GetOutput(int idx);
  ///@}

  ///@{
  /**
   * Get the output as various concrete types. This method is typically used
   * when you know exactly what type of data is being read.  Otherwise, use
   * the general GetOutput() method. If the wrong type is used nullptr is
   * returned.  (You must also set the filename of the object prior to
   * getting the output.)
   */
  vtkPolyData* GetPolyDataOutput();
  vtkStructuredPoints* GetStructuredPointsOutput();
  vtkStructuredGrid* GetStructuredGridOutput();
  vtkUnstructuredGrid* GetUnstructuredGridOutput();
  vtkRectilinearGrid* GetRectilinearGridOutput();
  ///@}

  /**
   * This method can be used to find out the type of output expected without
   * needing to read the whole file.
   */
  virtual int ReadOutputType();

  /**
   * Read metadata from file.
   */
  int ReadMetaDataSimple(VTK_FILEPATH const std::string& fname, vtkInformation* metadata) override;

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkDataSetReader();
  ~vtkDataSetReader() override;

  vtkDataObject* CreateOutput(vtkDataObject* currentOutput) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkDataSetReader(const vtkDataSetReader&) = delete;
  void operator=(const vtkDataSetReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
