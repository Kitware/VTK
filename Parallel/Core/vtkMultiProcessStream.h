// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiProcessStream
 * @brief   stream used to pass data across processes
 * using vtkMultiProcessController.
 *
 * vtkMultiProcessStream is used to pass data across processes. Using
 * vtkMultiProcessStream it is possible to send data whose length is not known
 * at the receiving end.
 *
 * @warning
 * Note, stream operators cannot be combined with the Push/Pop array operators.
 * For example, if you push an array to the stream,
 */

#ifndef vtkMultiProcessStream_h
#define vtkMultiProcessStream_h

#include "vtkObject.h"
#include "vtkParallelCoreModule.h" // For export macro
#include <string>                  // needed for string.
#include <vector>                  // needed for vector.

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkStringArray;
struct vtkMultiProcessStreamPushArray;
struct vtkMultiProcessStreamPopArray;
class VTKPARALLELCORE_EXPORT vtkMultiProcessStream
{
public:
  vtkMultiProcessStream();
  vtkMultiProcessStream(const vtkMultiProcessStream&);
  ~vtkMultiProcessStream();
  vtkMultiProcessStream& operator=(const vtkMultiProcessStream&);

  ///@{
  /**
   * Add-to-stream operators. Adds to the end of the stream.
   */
  vtkMultiProcessStream& operator<<(bool value);
  vtkMultiProcessStream& operator<<(char value);
  vtkMultiProcessStream& operator<<(signed char value);
  vtkMultiProcessStream& operator<<(unsigned char value);
  vtkMultiProcessStream& operator<<(short value);
  vtkMultiProcessStream& operator<<(unsigned short value);
  vtkMultiProcessStream& operator<<(int value);
  vtkMultiProcessStream& operator<<(unsigned int value);
  vtkMultiProcessStream& operator<<(long value);
  vtkMultiProcessStream& operator<<(unsigned long value);
  vtkMultiProcessStream& operator<<(long long value);
  vtkMultiProcessStream& operator<<(unsigned long long value);
  vtkMultiProcessStream& operator<<(float value);
  vtkMultiProcessStream& operator<<(double value);
  vtkMultiProcessStream& operator<<(const std::string& value);
  // Without this operator, the compiler would convert
  // a char* to a bool instead of a std::string.
  vtkMultiProcessStream& operator<<(const char* value);
  vtkMultiProcessStream& operator<<(const vtkMultiProcessStream&);
  ///@}

  ///@{
  /**
   * Remove-from-stream operators. Removes from the head of the stream.
   */
  vtkMultiProcessStream& operator>>(bool& value);
  vtkMultiProcessStream& operator>>(char& value);
  vtkMultiProcessStream& operator>>(signed char& value);
  vtkMultiProcessStream& operator>>(unsigned char& value);
  vtkMultiProcessStream& operator>>(short& value);
  vtkMultiProcessStream& operator>>(unsigned short& value);
  vtkMultiProcessStream& operator>>(int& value);
  vtkMultiProcessStream& operator>>(unsigned int& value);
  vtkMultiProcessStream& operator>>(long& value);
  vtkMultiProcessStream& operator>>(unsigned long& value);
  vtkMultiProcessStream& operator>>(long long& value);
  vtkMultiProcessStream& operator>>(unsigned long long& value);
  vtkMultiProcessStream& operator>>(float& value);
  vtkMultiProcessStream& operator>>(double& value);
  vtkMultiProcessStream& operator>>(std::string& value);
  vtkMultiProcessStream& operator>>(vtkMultiProcessStream&);
  ///@}

  ///@{
  /**
   * Add-array-to-stream methods. Adds to the end of the stream
   */
  void Push(char array[], unsigned int size);
  void Push(signed char array[], unsigned int size);
  void Push(unsigned char array[], unsigned int size);
  void Push(short array[], unsigned int size);
  void Push(unsigned short array[], unsigned int size);
  void Push(int array[], unsigned int size);
  void Push(unsigned int array[], unsigned int size);
  void Push(long array[], unsigned int size);
  void Push(unsigned long array[], unsigned int size);
  void Push(long long array[], unsigned int size);
  void Push(unsigned long long array[], unsigned int size);
  void Push(float array[], unsigned int size);
  void Push(double array[], unsigned int size);
  void Push(vtkDataArray* array);
  void Push(vtkStringArray* array);
  ///@}

  ///@{
  /**
   * Remove-array-to-stream methods. Removes from the head of the stream.
   * Note: If the input array is nullptr, the array will be allocated internally
   * and the calling application is responsible for properly de-allocating it.
   * If the input array is not nullptr, it is expected to match the size of the
   * data internally, and this method would just fill in the data.
   */
  void Pop(char*& array, unsigned int& size);
  void Pop(signed char*& array, unsigned int& size);
  void Pop(unsigned char*& array, unsigned int& size);
  void Pop(short*& array, unsigned int& size);
  void Pop(unsigned short*& array, unsigned int& size);
  void Pop(int*& array, unsigned int& size);
  void Pop(unsigned int*& array, unsigned int& size);
  void Pop(long*& array, unsigned int& size);
  void Pop(unsigned long*& array, unsigned int& size);
  void Pop(long long*& array, unsigned int& size);
  void Pop(unsigned long long*& array, unsigned int& size);
  void Pop(float*& array, unsigned int& size);
  void Pop(double*& array, unsigned int& size);
  void Pop(vtkDataArray*& array);
  void Pop(vtkStringArray*& array);
  ///@}

  /**
   * Clears everything in the stream.
   */
  void Reset();

  /**
   * Returns the size of the stream.
   */
  int Size();

  /**
   * Returns the size of the raw data returned by GetRawData. This
   * includes 1 byte to store the endian type.
   */
  int RawSize() { return (this->Size() + 1); }

  /**
   * Returns true iff the stream is empty.
   */
  bool Empty();

  ///@{
  /**
   * Serialization methods used to save/restore the stream to/from raw data.
   * Note: The 1st byte of the raw data buffer consists of the endian type.
   */
  void GetRawData(std::vector<unsigned char>& data) const;
  void GetRawData(unsigned char*& data, unsigned int& size);
  void SetRawData(const std::vector<unsigned char>& data);
  void SetRawData(const unsigned char*, unsigned int size);
  std::vector<unsigned char> GetRawData() const;
  ///@}

private:
  class vtkInternals;
  vtkInternals* Internals;
  unsigned char Endianness;
  enum
  {
    BigEndian,
    LittleEndian
  };
  friend struct vtkMultiProcessStreamPushArray;
  friend struct vtkMultiProcessStreamPopArray;

  template <typename T>
  inline void PushArray(T array[], unsigned int size);
  template <typename T>
  inline void PopArray(T*& array, unsigned int& size);
  template <typename T>
  inline vtkMultiProcessStream& OperatorPush(T value);
  template <typename T>
  inline vtkMultiProcessStream& OperatorPop(T& value);
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkMultiProcessStream.h
