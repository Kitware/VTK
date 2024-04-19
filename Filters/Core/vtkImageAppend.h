// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageAppend
 * @brief   Collects data from multiple inputs into one image.
 *
 * vtkImageAppend takes the components from multiple inputs and merges
 * them into one output. The output images are append along the "AppendAxis".
 * Except for the append axis, all inputs must have the same extent.
 * All inputs must have the same number of scalar components.
 * A future extension might be to pad or clip inputs to have the same extent.
 * The output has the same origin and spacing as the first input.
 * The origin and spacing of all other inputs are ignored.  All inputs
 * must have the same scalar type.
 */

#ifndef vtkImageAppend_h
#define vtkImageAppend_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkImageAppend : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageAppend* New();
  vtkTypeMacro(vtkImageAppend, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Replace one of the input connections with a new input.  You can
   * only replace input connections that you previously created with
   * AddInputConnection() or, in the case of the first input,
   * with SetInputConnection().
   */
  virtual void ReplaceNthInputConnection(int idx, vtkAlgorithmOutput* input);

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(int idx, vtkDataObject* input);
  void SetInputData(vtkDataObject* input) { this->SetInputData(0, input); }
  ///@}

  ///@{
  /**
   * Get one input to this filter. This method is only for support of
   * old-style pipeline connections.  When writing new code you should
   * use vtkAlgorithm::GetInputConnection(0, num).
   */
  vtkDataObject* GetInput(int idx);
  vtkDataObject* GetInput() { return this->GetInput(0); }
  ///@}

  /**
   * Get the number of inputs to this filter. This method is only for
   * support of old-style pipeline connections.  When writing new code
   * you should use vtkAlgorithm::GetNumberOfInputConnections(0).
   */
  int GetNumberOfInputs() { return this->GetNumberOfInputConnections(0); }

  ///@{
  /**
   * This axis is expanded to hold the multiple images.
   * The default AppendAxis is the X axis.
   * If you want to create a volue from a series of XY images, then you should
   * set the AppendAxis to 2 (Z axis).
   */
  vtkSetMacro(AppendAxis, int);
  vtkGetMacro(AppendAxis, int);
  ///@}

  ///@{
  /**
   * By default "PreserveExtents" is off and the append axis is used.
   * When "PreseveExtents" is on, the extent of the inputs is used to
   * place the image in the output.  The whole extent of the output is
   * the union of the input whole extents.  Any portion of the
   * output not covered by the inputs is set to zero.  The origin and
   * spacing is taken from the first input.
   */
  vtkSetMacro(PreserveExtents, vtkTypeBool);
  vtkGetMacro(PreserveExtents, vtkTypeBool);
  vtkBooleanMacro(PreserveExtents, vtkTypeBool);
  ///@}

protected:
  vtkImageAppend();
  ~vtkImageAppend() override;

  vtkTypeBool PreserveExtents;
  int AppendAxis;
  // Array holds the AppendAxisExtent shift for each input.
  int* Shifts;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData, int ext[6],
    int id) override;

  // see vtkAlgorithm for docs.
  int FillInputPortInformation(int, vtkInformation*) override;

  void InitOutput(int outExt[6], vtkImageData* outData);

  void InternalComputeInputUpdateExtent(int* inExt, int* outExt, int* inWextent, int whichInput);

  // overridden to allocate all of the output arrays, not just active scalars
  void AllocateOutputData(vtkImageData* out, vtkInformation* outInfo, int* uExtent) override;
  vtkImageData* AllocateOutputData(vtkDataObject* out, vtkInformation* outInfo) override;

  // overridden to prevent shallow copies across, since we have to do it elementwise
  void CopyAttributeData(
    vtkImageData* in, vtkImageData* out, vtkInformationVector** inputVector) override;

private:
  vtkImageAppend(const vtkImageAppend&) = delete;
  void operator=(const vtkImageAppend&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
