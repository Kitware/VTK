/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelTimer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// .NAME vtkParallelTimer -- Distributed log for timing parallel algorithms
// .SECTION Description
//
//  Provides ditributed log functionality. When the file is
//  written each process data is collected by rank 0 who
//  writes the data to a single file in rank order.
//
//  The log works as an event stack. EventStart pushes the
//  event identifier and its start time onto the stack. EventEnd
//  pops the most recent event time and identifier computes the
//  ellapsed time and adds an entry to the log recording the
//  event, it's start and end times, and its ellapsed time.
//  EndEventSynch includes a barrier before the measurement.
//
//  The log class implements the singleton patern so that it
//  may be shared accross class boundaries. If the log instance
//  doesn't exist then one is created. It will be automatically
//  destroyed at exit by the signleton destructor. It can be
//  destroyed explicitly by calling DeleteGlobalInstance.

#ifndef vtkParallelTimer_h
#define vtkParallelTimer_h

#define vtkParallelTimerDEBUG -1

#include "vtkObject.h"
#include "vtkRenderingParallelLICModule.h" // for export

//BTX
#include <vector> // for vector
#include <string> // for string
#include <sstream> // for sstream
#if vtkParallelTimerDEBUG > 0
#include <iostream> // for cerr
#endif
//ETX

class vtkParallelTimerBuffer;

class VTKRENDERINGPARALLELLIC_EXPORT vtkParallelTimer : public vtkObject
{
public:
  static vtkParallelTimer *New();
  vtkTypeMacro(vtkParallelTimer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Type used to direct an output stream into the log's header. The header
  // is a buffer used only by the root rank.
  class LogHeaderType
    {
    public:
      template<typename T> LogHeaderType &operator<<(const T& s);
    };

  // Description:
  // Type used to direct an output stream into the log's body. The body is a
  // buffer that all ranks write to.
  class LogBodyType
    {
    public:
      template<typename T> LogBodyType &operator<<(const T& s);
    };

  // Description:
  // Set the rank who writes.
  vtkSetMacro(WriterRank,int);
  vtkGetMacro(WriterRank,int);

  // Description:
  // Set the filename that is used during write when the object
  // is used as a singleton. If nothing is set the default is
  // ROOT_RANKS_PID.log
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //BTX
  void SetFileName(const std::string &fileName)
    { this->SetFileName(fileName.c_str()); }
  //ETX

  // Description:
  // The log works as an event stack. EventStart pushes the
  // event identifier and its start time onto the stack. EventEnd
  // pops the most recent event time and identifier computes the
  // ellapsed time and adds an entry to the log recording the
  // event, it's start and end times, and its ellapsed time.
  // EndEventSynch includes a barrier before the measurement.
  void StartEvent(const char *event);
  void StartEvent(int rank, const char *event);
  void EndEvent(const char *event);
  void EndEvent(int rank, const char *event);
  void EndEventSynch(const char *event);
  void EndEventSynch(int rank, const char *event);

  //BTX
  // Description:
  // Insert text into the log header on the writer rank.
  template<typename T>
  vtkParallelTimer &operator<<(const T& s);

  // Description:
  // stream output to the log's header(root rank only).
  vtkParallelTimer::LogHeaderType GetHeader()
    { return vtkParallelTimer::LogHeaderType(); }

  // Description:
  // stream output to log body(all ranks).
  vtkParallelTimer::LogBodyType GetBody()
    { return vtkParallelTimer::LogBodyType(); }
  //ETX

  // Description:
  // Clear the log.
  void Clear();

  // Description:
  // When an object is finished writing data to the log
  // object it must call Update to send the data to the writer
  // rank.
  // This ensures that all data is transfered to the root before
  // MPI_Finalize is called while allowing the write to occur
  // after Mpi_finalize. Note: This is a collective call.
  void Update();

  // Description:
  // Write the log contents to a file.
  int Write();

  // Description:
  // The log class implements the singleton patern so that it
  // may be shared accross class boundaries. If the log instance
  // doesn't exist then one is created. It will be automatically
  // destroyed at exit by the signleton destructor. It can be
  // destroyed explicitly by calling DeleteGlobalInstance.
  static vtkParallelTimer *GetGlobalInstance();

  // Description:
  // Explicitly delete the singleton.
  static void DeleteGlobalInstance();

  // Description:
  // If enabled and used as a singleton the log will write
  // it's contents to disk during program termination.
  vtkSetMacro(WriteOnClose, int);
  vtkGetMacro(WriteOnClose, int);

  // Description:
  // Set/Get the global log level. Applications can set this to the
  // desired level so that all pipeline objects will log data.
  vtkSetMacro(GlobalLevel, int);
  vtkGetMacro(GlobalLevel, int);

protected:
  vtkParallelTimer();
  virtual ~vtkParallelTimer();

private:
  vtkParallelTimer(const vtkParallelTimer&); // Not implemented
  void operator=(const vtkParallelTimer&); // Not implemented

//BTX
  // Description:
  // A class responsible for delete'ing the global instance of the log.
  class VTKRENDERINGPARALLELLIC_EXPORT vtkParallelTimerDestructor
    {
    public:
      vtkParallelTimerDestructor() : Log(0) {}
      ~vtkParallelTimerDestructor();

      void SetLog(vtkParallelTimer *log){ this->Log = log; }

    private:
      vtkParallelTimer *Log;
    };
//ETX

private:
  int GlobalLevel;
  int Initialized;
  int WorldRank;
  int WriterRank;
  char *FileName;
  int WriteOnClose;
  std::vector<double> StartTime;
  #if vtkParallelTimerDEBUG < 0
  std::vector<std::string> EventId;
  #endif

  vtkParallelTimerBuffer *Log;

  static vtkParallelTimer *GlobalInstance;
  static vtkParallelTimerDestructor GlobalInstanceDestructor;

  std::ostringstream HeaderBuffer;

  friend class LogHeaderType;
  friend class LogBodyType;
};

//BTX
//-----------------------------------------------------------------------------
template<typename T>
vtkParallelTimer &vtkParallelTimer::operator<<(const T& s)
{
  if (this->WorldRank == this->WriterRank)
    {
    this->HeaderBuffer << s;
    #if vtkParallelTimerDEBUG > 0
    std::cerr << s;
    #endif
    }
  return *this;
}

//-----------------------------------------------------------------------------
template<typename T>
vtkParallelTimer::LogHeaderType &vtkParallelTimer::LogHeaderType::operator<<(const T& s)
{
  vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();

  if (log->WorldRank == log->WriterRank)
    {
    log->HeaderBuffer << s;
    #if vtkParallelTimerDEBUG > 0
    std::cerr << s;
    #endif
    }

  return *this;
}

//-----------------------------------------------------------------------------
template<typename T>
vtkParallelTimer::LogBodyType &vtkParallelTimer::LogBodyType::operator<<(const T& s)
{
  vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();

  *(log->Log) <<  s;
  #if vtkParallelTimerDEBUG > 0
  std::cerr << s;
  #endif

  return *this;
}
//ETX

#endif
