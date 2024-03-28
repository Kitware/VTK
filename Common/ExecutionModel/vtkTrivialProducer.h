// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrivialProducer
 * @brief   Producer for stand-alone data objects.
 *
 * vtkTrivialProducer allows stand-alone data objects to be connected
 * as inputs in a pipeline.  All data objects that are connected to a
 * pipeline involving vtkAlgorithm must have a producer.  This trivial
 * producer allows data objects that are hand-constructed in a program
 * without another vtk producer to be connected.
 */

#ifndef vtkTrivialProducer_h
#define vtkTrivialProducer_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkWrappingHints.h"              // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;

class VTKCOMMONEXECUTIONMODEL_EXPORT VTK_MARSHALAUTO vtkTrivialProducer : public vtkAlgorithm
{
public:
  static vtkTrivialProducer* New();
  vtkTypeMacro(vtkTrivialProducer, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Process upstream/downstream requests trivially.  The associated
   * output data object is never modified, but it is queried to
   * fulfill requests.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set the data object that is "produced" by this producer.  It is
   * never really modified.
   */
  VTK_MARSHALSETTER(OutputDataObject)
  virtual void SetOutput(vtkDataObject* output);

  /**
   * The modified time of this producer is the newer of this object or
   * the assigned output.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set the whole extent to use for the data this producer is producing.
   * This may be different than the extent of the output data when
   * the trivial producer is used in parallel.
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  ///@}

  /**
   * This method can be used to copy meta-data from an existing data
   * object to an information object. For example, whole extent,
   * image data spacing, origin etc.
   */
  static void FillOutputDataInformation(vtkDataObject* output, vtkInformation* outInfo);

protected:
  vtkTrivialProducer();
  ~vtkTrivialProducer() override;

  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;
  vtkExecutive* CreateDefaultExecutive() override;

  // The real data object.
  vtkDataObject* Output;

  int WholeExtent[6];

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkTrivialProducer(const vtkTrivialProducer&) = delete;
  void operator=(const vtkTrivialProducer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
