/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVDBWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVDBWriter
 * @brief   OpenVDB writer for vtkImageData or vtkPointSet
 * Writes a vtkImageData or vtkPointSet as a VDB file.
 */

#ifndef vtkOpenVDBWriter_h
#define vtkOpenVDBWriter_h

#include "vtkIOOpenVDBModule.h" //needed for exports
#include "vtkSmartPointer.h"    // For protected ivars
#include "vtkWriter.h"

class vtkDataSetAttributes;
class vtkImageData;
class vtkMultiProcessController;
class vtkOpenVDBWriterInternals;
class vtkPointSet;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKIOOPENVDB_EXPORT vtkOpenVDBWriter : public vtkWriter
{
public:
  static vtkOpenVDBWriter* New();
  vtkTypeMacro(vtkOpenVDBWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the filename for the file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set whether or not to save all time steps or
   * just the current time step. Default is false
   * (save only the current time step).
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  //@}

  //@{
  /**
   * A lookup table can be specified in order to convert data arrays to
   * RGBA colors.
   */
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);
  //@}

  //@{
  /**
   * Enable coloring channel output based on LookupTable. The output
   * channel will be named 'color'.
   */
  vtkSetMacro(EnableColoring, bool);
  vtkGetMacro(EnableColoring, bool);
  //@}

  //@{
  /**
   * Enable alpha channel output based on LookupTable. The output
   * channel will be name 'alpha'.
   */
  vtkSetMacro(EnableAlpha, bool);
  vtkGetMacro(EnableAlpha, bool);
  //@}

  //@{
  /**
   * Get/Set the controller to use. By default,
   * `vtkMultiProcessController::GetGlobalController` will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkOpenVDBWriter();
  ~vtkOpenVDBWriter() override;

  void WriteData() override;

  void WriteImageData(vtkImageData* imageData);
  void WritePointSet(vtkPointSet* pointSet);

  // see algorithm for more info.
  // This writer takes in vtkTable, vtkDataSet or vtkCompositeDataSet.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // see algorithm for more info. needed here so we can request pieces.
  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void SetRGBA(vtkIdType num, vtkUnsignedCharArray* rgba, vtkDataSetAttributes* attributes);

  char* FileName;

  //@{
  /**
   * Whether or not to write out all time steps.
   * The default is to not write out all time steps.
   */
  bool WriteAllTimeSteps;
  //@}

  //@{
  /**
   * For outputting the Lookup Table in the VDB file.
   * Copying what's done in vtkPLYWriter.
   */
  vtkScalarsToColors* LookupTable;
  bool EnableColoring;
  bool EnableAlpha;
  //@}

private:
  vtkOpenVDBWriter(const vtkOpenVDBWriter&) = delete;
  void operator=(const vtkOpenVDBWriter&) = delete;

  //@{
  /**
   * The controller for the writer to work in parallel.
   */
  vtkMultiProcessController* Controller;
  //@}

  vtkOpenVDBWriterInternals* Internals;
  friend class vtkOpenVDBWriterInternals;
};

#endif
