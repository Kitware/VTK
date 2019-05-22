/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2Schema.h
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2SCHEMA_H_
#define VTK_IO_ADIOS2_ADIOS2SCHEMA_H_

#include <map>
#include <string>
#include <vector>

namespace adios2
{
class IO;
class Engine;
}

class vtkMultiBlockDataSet;
template<class T>
class vtkSmartPointer;
class vtkDataArray;

namespace adios2vtk
{

class ADIOS2Schema
{
public:
  const std::string m_Type;
  std::string m_Schema;

  std::map<double, size_t> m_Times;

  ADIOS2Schema(
    const std::string type, const std::string& schema, adios2::IO* io, adios2::Engine* engine);

  // can't use = default, due to forward class not defined
  virtual ~ADIOS2Schema();

  void Fill(vtkMultiBlockDataSet* multiBlock, const size_t step = 0);

protected:
  adios2::IO* m_IO = nullptr;
  adios2::Engine* m_Engine = nullptr;

  virtual void InitTimes() = 0;
  virtual void DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step) = 0;
  virtual void ReadPiece(const size_t step, const size_t pieceID) = 0;

  void GetTimes(const std::string& variableName = "");
  void GetDataArray(const std::string& variableName, vtkSmartPointer<vtkDataArray>& dataArray,
    const size_t step = 0, const std::string mode = "deferred");

private:
  template<class T>
  void GetTimesCommon(const std::string& variableName);

  template<class T>
  void GetDataArrayCommon(const std::string& variableName, vtkSmartPointer<vtkDataArray>& dataArray,
    const size_t step, const std::string mode);
};

} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_ADIOS2SCHEMA_H_ */
