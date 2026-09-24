// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkmDataArrayFactory
 * @brief   Build a vtkmDataArray over memory VTK did not allocate.
 *
 * Wraps externally owned host or device memory as a vtkmDataArray, without
 * copying, so data produced elsewhere -- a simulation, CuPy, PyTorch, Tack --
 * can be fed straight into Viskores-accelerated filters and stay where it is.
 *
 * @code
 * // AoS: one buffer
 * factory = vtkmDataArrayFactory()
 * factory.SetNumberOfTuples(1000)
 * factory.SetNumberOfComponents(3)
 * factory.SetDataType(VTK_FLOAT)
 * factory.AddBuffer(descriptor)
 * arr = factory.CreateArray()
 *
 * // SoA: one buffer per component
 * factory.AddBuffer(desc_x)
 * factory.AddBuffer(desc_y)
 * factory.AddBuffer(desc_z)
 * @endcode
 *
 * One buffer gives AoS, NumberOfComponents buffers give SoA (up to 4).
 *
 * ## Ownership
 *
 * Wrapping memory does not copy it, so something has to keep it alive for as
 * long as Viskores might read it -- which is longer than the caller can
 * predict, because a filter may hold an array handle well past the call that
 * created it.
 *
 * `CreateArray()` therefore *takes* each descriptor's release
 * (vtkMemoryDescriptor::TakeRelease) and installs it as the deleter on the
 * Viskores buffer. The producer's hold is then dropped exactly when Viskores
 * is finished with the memory, which is the only moment that is correct.
 *
 * A descriptor holding a VTK owner rather than a release callback is handled
 * the same way, by holding a reference for the buffer's lifetime.
 *
 * A descriptor with no ownership at all is accepted -- some callers really do
 * manage lifetime themselves -- but it is a deliberate choice, and a warning
 * says so, because the failure it invites is a use-after-free on device
 * memory, which reports itself as wrong numbers rather than a crash.
 *
 * ## Resizing
 *
 * External memory cannot be reallocated, so the buffers are installed with
 * Viskores' `InvalidRealloc`, which throws `viskores::cont::ErrorBadAllocation`
 * if a filter tries to grow them. That is deliberate: the alternative -- a
 * reallocation function that silently does nothing -- leaves Viskores
 * believing it has more memory than it does, and writing past the end of the
 * caller's buffer.
 *
 * @sa vtkMemoryDescriptor, vtkmDataArray
 */

#ifndef vtkmDataArrayFactory_h
#define vtkmDataArrayFactory_h

#include "vtkAcceleratorsVTKmCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkMemoryDescriptor;

class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArrayFactory : public vtkObject
{
public:
  static vtkmDataArrayFactory* New();
  vtkTypeMacro(vtkmDataArrayFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Number of tuples in the array to create.
   */
  vtkSetMacro(NumberOfTuples, vtkIdType);
  vtkGetMacro(NumberOfTuples, vtkIdType);
  ///@}

  ///@{
  /**
   * Number of components per tuple.
   */
  vtkSetMacro(NumberOfComponents, int);
  vtkGetMacro(NumberOfComponents, int);
  ///@}

  ///@{
  /**
   * The VTK data type (VTK_FLOAT, VTK_DOUBLE, VTK_INT, VTK_LONG_LONG, ...).
   */
  vtkSetMacro(DataType, int);
  vtkGetMacro(DataType, int);
  ///@}

  /**
   * Add a buffer descriptor.
   * - One buffer: AoS layout (ArrayHandleBasic), any number of components.
   * - NumberOfComponents buffers: SoA layout (ArrayHandleSOA), which Viskores
   *   limits to 4 components.
   */
  void AddBuffer(vtkMemoryDescriptor* descriptor);

  /**
   * Drop all added descriptors without creating an array. Any ownership they
   * hold is released, as it would be if they were simply destroyed.
   */
  void ClearBuffers();

  /**
   * Number of buffers added so far.
   */
  int GetNumberOfBuffers() const;

  /**
   * Create the vtkmDataArray, or nullptr on error.
   *
   * On success the caller owns the returned array, and each descriptor's
   * release has been transferred into it -- the descriptors no longer hold
   * the memory open. On failure nothing is transferred.
   *
   * VTK_NEWINSTANCE is what tells the wrappers the reference is owned
   * rather than borrowed. Without it Python adds one of its own, the array
   * is never destroyed, and the release the descriptors handed over is
   * never called -- so the producer is never told its memory is free.
   */
  VTK_NEWINSTANCE
  vtkDataArray* CreateArray();

protected:
  vtkmDataArrayFactory();
  ~vtkmDataArrayFactory() override;

  vtkIdType NumberOfTuples = 0;
  int NumberOfComponents = 1;
  int DataType = 10; // VTK_FLOAT

  std::vector<vtkMemoryDescriptor*> Buffers;

private:
  vtkmDataArrayFactory(const vtkmDataArrayFactory&) = delete;
  void operator=(const vtkmDataArrayFactory&) = delete;

  // Viskores has to be initialized before any of its devices is touched, and
  // this class is the one place in Core that hands it device memory. Every
  // vtkm *filter* carries this member; nothing in Core did, so a program that
  // only wrapped external memory -- which is exactly what the DLPack import
  // path does -- never initialized it. On a Kokkos-backed build that is not a
  // warning: the first host read of a device array calls Kokkos::HIP::HIP(),
  // which calls host_abort(). The process dies with no exception to catch.
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmDataArrayFactory_h
