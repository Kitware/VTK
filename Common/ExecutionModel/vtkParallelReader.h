/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParallelReader
 * @brief   Superclass for algorithms that are parallel aware
 *
 * vtkParallelReader is a vtkReaderAlgorithm subclass that provides
 * a specialized API to develop readers that are parallel aware (i.e.
 * can handle piece requests) but do not natively support time series.
 * This reader adds support for file series in order to support time
 * series.
*/

#ifndef vtkParallelReader_h
#define vtkParallelReader_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkReaderAlgorithm.h"

#include <string> // needed for std::string in the interface

struct vtkParallelReaderInternal;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkParallelReader : public vtkReaderAlgorithm
{
public:
  vtkTypeMacro(vtkParallelReader,vtkReaderAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
  * Add a filename to be read. Since this superclass handles
  * file series to support time, multiple filenames can be added.
  * Note that the time values are either integers growing sequentially,
  * or are obtained from individual files as supported by the subclass.
  */
  void AddFileName(const char* fname);

  /**
  * Removes all filenames stored by the reader.
  */
  void ClearFileNames();

  /**
  * Returns the number of filenames stored by the reader.
  */
  int GetNumberOfFileNames() const;

  /**
  * Returns a particular filename stored by the reader.
  */
  const char* GetFileName(int i) const;

  /**
  * Returns the filename that was last loaded by the reader.
  * This is set internally in ReadMesh()
  */
  const char* GetCurrentFileName() const;

  //@{
  /**
  * This is the superclass API overridden by this class
  * to provide time support internally. Subclasses should
  * not normally have to override these methods.
  */
  int ReadMetaData(vtkInformation* metadata) override;
  int ReadMesh(
    int piece, int npieces, int nghosts, int timestep,
    vtkDataObject* output) override;
  int ReadPoints(
    int piece, int npieces, int nghosts, int timestep,
    vtkDataObject* output) override;
  int ReadArrays(
    int piece, int npieces, int nghosts, int timestep,
    vtkDataObject* output) override;
  //@}

protected:
  vtkParallelReader();
  ~vtkParallelReader() override;

  vtkExecutive* CreateDefaultExecutive() override;

  /**
  * A subclass can override this method to provide an actual
  * time value for a given file (this method is called for
  * each filename stored by the reader). If time values is not
  * available, the subclass does not have to override.
  */
  virtual double GetTimeValue(const std::string& fname);

  /**
  * A method that needs to be override by the subclass to provide
  * the mesh (topology). Note that the filename is passed to this
  * method and should be used by the subclass. The subclass directly
  * adds the structure/topology to the provided data object.
  */
  virtual int ReadMesh(const std::string& fname,
                        int piece,
                        int npieces,
                        int nghosts,
                        vtkDataObject* output) = 0;

  /**
  * A method that needs to be override by the subclass to provide
  * the point coordinates. Note that the filename is passed to this
  * method and should be used by the subclass. The subclass directly
  * adds the coordinates to the provided data object.
  */
  virtual int ReadPoints(const std::string& fname,
                         int piece,
                         int npieces,
                         int nghosts,
                         vtkDataObject* output) = 0;

  /**
  * A method that needs to be override by the subclass to provide
  * data arrays. Note that the filename is passed to this
  * method and should be used by the subclass. The subclass directly
  * adds data arrays to the provided data object.
  */
  virtual int ReadArrays(const std::string& fname,
                         int piece,
                         int npieces,
                         int nghosts,
                         vtkDataObject* output) = 0;

  int CurrentFileIndex;

private:
  vtkParallelReader(const vtkParallelReader&) = delete;
  void operator=(const vtkParallelReader&) = delete;

  vtkParallelReaderInternal* Internal;
};

#endif
