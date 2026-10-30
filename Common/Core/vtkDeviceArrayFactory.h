// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDeviceArrayFactory
 * @brief   makes a vtkDataArray over GPU memory a vtkMemoryDescriptor describes
 *
 * This is how wrapped code -- `vtkmodules.util.dlpack_support` above all --
 * puts GPU memory into VTK without copying it. Give it the shape, the VTK data
 * type and one descriptor with the role "data" (interleaved values), and
 * CreateArray() returns a vtkDeviceDataArray holding the memory. The array
 * keeps the descriptor, so the descriptor's owner or release callback decides
 * when the memory is freed.
 *
 * VTK needs no GPU toolkit for this: the vendor's driver is loaded the first
 * time it is needed (vtkDeviceMemory). IsMemorySpaceAvailable() says whether
 * it can be: "cuda", "hip" and "level_zero".
 *
 * Level Zero memory must be in VTK's Level Zero context, which the
 * GetLevelZero*Handle() functions report: a producer allocates there, or gives
 * VTK its own context first (SetLevelZeroContext).
 *
 * @sa vtkDeviceDataArray vtkMemoryDescriptor
 */

#ifndef vtkDeviceArrayFactory_h
#define vtkDeviceArrayFactory_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For the descriptor

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkMemoryDescriptor;

class VTKCOMMONCORE_EXPORT vtkDeviceArrayFactory : public vtkObject
{
public:
  static vtkDeviceArrayFactory* New();
  vtkTypeMacro(vtkDeviceArrayFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The array's shape and VTK data type (VTK_FLOAT, VTK_INT, ...).
   */
  vtkSetMacro(NumberOfTuples, vtkIdType);
  vtkGetMacro(NumberOfTuples, vtkIdType);
  vtkSetMacro(NumberOfComponents, int);
  vtkGetMacro(NumberOfComponents, int);
  vtkSetMacro(DataType, int);
  vtkGetMacro(DataType, int);
  ///@}

  ///@{
  /**
   * The memory: one descriptor, of role "data". Adding a second replaces the
   * first. The same names as vtkmDataArrayFactory's, so callers can use either.
   */
  void AddBuffer(vtkMemoryDescriptor* descriptor);
  void ClearBuffers();
  int GetNumberOfBuffers() const;
  ///@}

  /**
   * The array, or nullptr (with an error) if the descriptor, shape or type do
   * not fit, or the memory space cannot be read here.
   */
  VTK_NEWINSTANCE
  vtkDataArray* CreateArray();

  /**
   * Whether memory of the space named ("cuda", "hip", "level_zero") can be
   * held: whether the vendor's driver or runtime loads and has a device.
   */
  static bool IsMemorySpaceAvailable(const char* space);

  ///@{
  /**
   * VTK's Level Zero context, as integers: the `ze_driver_handle_t`,
   * `ze_device_handle_t` and `ze_context_handle_t`. VTK has one per process: one
   * given to SetLevelZeroContext (another library's, such as a SYCL runtime's
   * default context, which VTK cannot make), else one VTK makes on the first
   * Level Zero GPU the first time it is asked. The getters return 0 if there is
   * no Level Zero GPU. Set the context before making any Level Zero array;
   * setting a zero context forgets it. HasLevelZeroContext() says whether there
   * is one already, without making one. The handles belong to whoever made
   * them: use them, do not destroy them.
   */
  static void SetLevelZeroContext(
    vtkTypeUInt64 driver, vtkTypeUInt64 device, vtkTypeUInt64 context);
  static bool HasLevelZeroContext();
  static vtkTypeUInt64 GetLevelZeroDriverHandle();
  static vtkTypeUInt64 GetLevelZeroDeviceHandle();
  static vtkTypeUInt64 GetLevelZeroContextHandle();
  ///@}

protected:
  vtkDeviceArrayFactory();
  ~vtkDeviceArrayFactory() override;

private:
  vtkIdType NumberOfTuples = 0;
  int NumberOfComponents = 1;
  int DataType = 0;
  vtkSmartPointer<vtkMemoryDescriptor> Buffer;

  vtkDeviceArrayFactory(const vtkDeviceArrayFactory&) = delete;
  void operator=(const vtkDeviceArrayFactory&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
