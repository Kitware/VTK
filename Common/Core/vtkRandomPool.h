/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomPool.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRandomPool
 * @brief   convenience class to quickly generate an array of random numbers
 *
 * vtkRandomPool generates random numbers, and can do using multithreading.
 * It supports parallel applications where generating random numbers on the
 * fly is difficult. Also, it can be used to populate vtkDataArrays in an
 * efficient manner. By default it uses an instance of vtkMersenneTwister to
 * generate random sequences, but any subclass of vtkRandomSequence may be
 * used. It also supports simple methods to generate, access, and pass random
 * memory pools between objects.
 *
 * In threaded applications, these class may be conveniently used to
 * pre-generate a sequence of random numbers, followed by the use of
 * deterministic accessor methods to produce random sequences without
 * problems etc. due to unpredictable work load and order of thread
 * execution.
 *
 * @warning
 * The class uses vtkMultiThreader if the size of the pool is larger than
 * the specified chunk size. Also, vtkSMPTools may be used to scale the
 * components in the method PopulateDataArray().
 */

#ifndef vtkRandomPool_h
#define vtkRandomPool_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkRandomSequence;
class vtkDataArray;

class VTKCOMMONCORE_EXPORT vtkRandomPool : public vtkObject
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkRandomPool* New();
  vtkTypeMacro(vtkRandomPool,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the random sequence generator used to produce the random pool.
   * By default vtkMersenneTwister is used.
   */
  virtual void SetSequence(vtkRandomSequence *seq);
  vtkGetObjectMacro(Sequence,vtkRandomSequence);
  //@}

  //@{
  /**
   * Methods to set and get the size of the pool. The size must be specified
   * before invoking GeneratePool(). Note the number of components will
   * affect the total size (allocated memory is PoolSize*NumberOfComponents).
   */
  vtkSetClampMacro(PoolSize,vtkIdType,1000,VTK_ID_MAX);
  vtkGetMacro(PoolSize,vtkIdType);
  //@}

  //@{
  /**
   * Methods to set and get the number of components in the pool. This is a
   * convenience capability used to interface with vtkDataArrays. By default
   * the number of components is =1.
   */
  vtkSetClampMacro(NumberOfComponents,vtkIdType,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfComponents,vtkIdType);
  //@}

  /**
   * This convenience method returns the total size of the memory pool, i.e.,
   * Size*NumberOfComponents.
   */
  vtkIdType GetTotalSize() {return this->TotalSize;}

  //@{
  /**
   * These methods provide access to the raw random pool as a double
   * array. The size of the array is Size*NumberOfComponents. Each x value
   * ranges (0<=x<=1). The class will generate the pool if necessary. Also a
   * method is available for getting the value at the ith postion and compNum
   * component. Finally, note that the GetValue() method uses modulo
   * reduction to ensure that the request remains inside of the pool. Two
   * forms are provided, the first assumes NumberOfComponents=1; the second
   * allows access to a particular component. The GetPool() and GetValue()
   * methods should only be called after GeneratePool() has been invoked;
   */
  void GeneratePool();
  const double* GetPool() {return this->Pool;}
  const double GetValue(vtkIdType i) {return this->Pool[(i%this->TotalSize)];}
  const double GetValue(vtkIdType i, int compNum)
  {return this->Pool[(compNum + this->NumberOfComponents*i) % this->TotalSize];}
  //@}

  /**
   * A method to populate data arrays of various types within specified
   * range. Note the specified compNumber specifies the range for a particular
   * component. Thus it is possible to make multiple calls to generate random
   * numbers for each component with different ranges. Internally the type of
   * the data array passed in is used to cast to the appropriate type. Also
   * the size and number of components of the vtkRandomPool is used to
   * configure the data array.
   */
  void PopulateDataArray(vtkDataArray *da, int compNumber,
                         double minRange, double maxRange);

  //@{
  /**
   * Specify the work chunk size at which point multithreading kicks in. For small
   * memory pools < ChunkSize, no threading is used. Larger pools are computed using
   * vtkMultiThreader.
   */
  vtkSetClampMacro(ChunkSize, vtkIdType, 1000, VTK_INT_MAX);
  vtkGetMacro(ChunkSize, vtkIdType);
  //@}


protected:
  vtkRandomPool();
  ~vtkRandomPool() override;

  // Keep track of last generation time
  vtkTimeStamp GenerateTime;

  // Data members to support public API
  vtkRandomSequence *Sequence;
  vtkIdType PoolSize;
  int NumberOfComponents;
  vtkIdType ChunkSize;

  // Internal data members
  vtkIdType TotalSize;
  double *Pool;

private:
  vtkRandomPool(const vtkRandomPool&) = delete;
  void operator=(const vtkRandomPool&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkRandomPool.h
