// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkThreadedImageAlgorithm
 * @brief   Generic filter that has one input.
 *
 * vtkThreadedImageAlgorithm is a filter superclass that hides much of the
 * pipeline complexity. It handles breaking the pipeline execution
 * into smaller extents so that the vtkImageData limits are observed. It
 * also provides support for multithreading. If you don't need any of this
 * functionality, consider using vtkSimpleImageToImageAlgorithm instead.
 * @sa
 * vtkSimpleImageToImageAlgorithm
 */

#ifndef vtkThreadedImageAlgorithm_h
#define vtkThreadedImageAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkImageAlgorithm.h"
#include "vtkThreads.h"       // for VTK_MAX_THREADS
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkMultiThreader;

class VTKCOMMONEXECUTIONMODEL_EXPORT VTK_MARSHALAUTO vtkThreadedImageAlgorithm
  : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkThreadedImageAlgorithm, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If the subclass does not define an Execute method, then the task
   * will be broken up, multiple threads will be spawned, and each thread
   * will call this method. It is public so that the thread functions
   * can call this method.
   */
  virtual void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int extent[6], int threadId);

  // also support the old signature
  virtual void ThreadedExecute(
    vtkImageData* inData, vtkImageData* outData, int extent[6], int threadId);

  ///@{
  /**
   * Enable/Disable SMP for threading.
   */
  vtkGetMacro(EnableSMP, bool);
  vtkSetMacro(EnableSMP, bool);
  ///@}

  ///@{
  /**
   * Global Disable SMP for all derived Imaging filters.
   */
  static void SetGlobalDefaultEnableSMP(bool enable);
  static bool GetGlobalDefaultEnableSMP();
  ///@}

  ///@{
  /**
   * The minimum piece size when volume is split for execution.
   * By default, the minimum size is (16,1,1).
   */
  vtkSetVector3Macro(MinimumPieceSize, int);
  vtkGetVector3Macro(MinimumPieceSize, int);
  ///@}

  ///@{
  /**
   * The desired bytes per piece when volume is split for execution.
   * When SMP is enabled, this is used to subdivide the volume into pieces.
   * Smaller pieces allow for better dynamic load balancing, but increase
   * the total overhead. The default is 65536 bytes.
   */
  vtkSetMacro(DesiredBytesPerPiece, vtkIdType);
  vtkGetMacro(DesiredBytesPerPiece, vtkIdType);
  ///@}

  ///@{
  /**
   * Set the method used to divide the volume into pieces.
   * Slab mode splits the volume along the Z direction first,
   * Beam mode splits evenly along the Z and Y directions, and
   * Block mode splits evenly along all three directions.
   * Most filters use Slab mode as the default.
   */
  vtkSetClampMacro(SplitMode, int, 0, 2);
  void SetSplitModeToSlab() { this->SetSplitMode(SLAB); }
  void SetSplitModeToBeam() { this->SetSplitMode(BEAM); }
  void SetSplitModeToBlock() { this->SetSplitMode(BLOCK); }
  vtkGetMacro(SplitMode, int);
  ///@}

  ///@{
  /**
   * Get/Set the number of threads to create when rendering.
   * This is ignored if EnableSMP is On.
   */
  vtkSetClampMacro(NumberOfThreads, int, 1, VTK_MAX_THREADS);
  vtkGetMacro(NumberOfThreads, int);
  ///@}

  /**
   * Putting this here until I merge graphics and imaging streaming.
   */
  virtual int SplitExtent(int splitExt[6], int startExt[6], int num, int total);

protected:
  vtkThreadedImageAlgorithm();
  ~vtkThreadedImageAlgorithm() override;

  vtkMultiThreader* Threader;
  int NumberOfThreads;

  bool EnableSMP;
  static bool GlobalDefaultEnableSMP;

  enum SplitModeEnum
  {
    SLAB = 0,
    BEAM = 1,
    BLOCK = 2
  };

  int SplitMode;
  int SplitPath[3];
  int SplitPathLength;
  int MinimumPieceSize[3];
  vtkIdType DesiredBytesPerPiece;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Execute ThreadedRequestData for the given set of pieces.
   * The extent will be broken into the number of pieces specified,
   * and ThreadedRequestData will be called for all pieces starting
   * at "begin" and up to but not including "end".
   */
  virtual void SMPRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    vtkIdType begin, vtkIdType end, vtkIdType pieces, int extent[6]);

  /**
   * Allocate space for output data and copy attributes from first input.
   * If the inDataObjects and outDataObjects are not passed as zero, then
   * they must be large enough to store the data objects for all inputs and
   * outputs.
   */
  virtual void PrepareImageData(vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inDataObjects = nullptr,
    vtkImageData** outDataObjects = nullptr);

private:
  vtkThreadedImageAlgorithm(const vtkThreadedImageAlgorithm&) = delete;
  void operator=(const vtkThreadedImageAlgorithm&) = delete;

  friend class vtkThreadedImageAlgorithmFunctor;
};

VTK_ABI_NAMESPACE_END
#endif
