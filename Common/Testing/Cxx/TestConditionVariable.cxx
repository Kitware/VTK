#include "vtkConditionVariable.h"
#include "vtkMultiThreader.h"

typedef struct {
  vtkMutexLock* Lock;
  vtkConditionVariable* Condition;
  int Done;
  int NumberOfWorkers;
} vtkThreadUserData;

static void vtkDoNothingButEliminateCompilerWarnings()
{
}

VTK_THREAD_RETURN_TYPE vtkTestCondVarThread( void* arg )
{
  int threadId = static_cast<vtkMultiThreader::ThreadInfo*>(arg)->ThreadID;
  int threadCount = static_cast<vtkMultiThreader::ThreadInfo*>(arg)->NumberOfThreads;
  vtkThreadUserData* td = static_cast<vtkThreadUserData*>(
    static_cast<vtkMultiThreader::ThreadInfo*>(arg)->UserData );
  if ( td )
    {
    if ( threadId == 0 )
      {
      td->Done = 0;
      td->Lock->Lock();
      cout << "Thread " << ( threadId + 1 ) << " of " << threadCount << " initializing.\n";
      cout.flush();
      td->Lock->Unlock();

      for ( int i = 0; i < 2 * threadCount; ++ i )
        {
        td->Lock->Lock();
        cout << "Signaling (count " << i << ")...";
        cout.flush();
        td->Lock->Unlock();
        td->Condition->Signal();

        //sleep( 1 );
        }

      do
        {
        td->Lock->Lock();
        td->Done = 1;
        cout << "Broadcasting...\n";
        cout.flush();
        td->Lock->Unlock();
        td->Condition->Broadcast();
        //sleep( 1 );
        }
      while ( td->NumberOfWorkers > 0 );
      }
    else
      {
      // Wait for thread 0 to initialize... Ugly but effective
      while ( td->Done < 0 )
        vtkDoNothingButEliminateCompilerWarnings();

      // Wait for the condition and then note we were signaled.
      while ( td->Done <= 0 )
        {
        td->Lock->Lock();
        td->Condition->Wait( td->Lock );
        cout << " Thread " << ( threadId + 1 ) << " responded.\n";
        cout.flush();
        td->Lock->Unlock();
        }
      -- td->NumberOfWorkers;
      }

    td->Lock->Lock();
    cout << "  Thread " << ( threadId + 1 ) << " of " << threadCount << " exiting.\n";
    cout.flush();
    td->Lock->Unlock();
    }
  else
    {
    cout << "No thread data!\n";
    cout << "  Thread " << ( threadId + 1 ) << " of " << threadCount << " exiting.\n";
    -- td->NumberOfWorkers;
    cout.flush();
    }

  return VTK_THREAD_RETURN_VALUE;
}

int TestConditionVariable( int, char*[] )
{
  vtkMultiThreader* threader = vtkMultiThreader::New();
  int numThreads = threader->GetNumberOfThreads();

  vtkThreadUserData data;
  data.Lock = vtkMutexLock::New();
  data.Condition = vtkConditionVariable::New();
  data.Done = -1;
  data.NumberOfWorkers = numThreads - 1;

  threader->SetNumberOfThreads( numThreads );
  threader->SetSingleMethod( vtkTestCondVarThread, &data );
  threader->SingleMethodExecute();

  cout << "Done with threader.\n";
  cout.flush();

  vtkIndent indent;
  indent = indent.GetNextIndent();
  data.Condition->PrintSelf( cout, indent );

  data.Lock->Delete();
  data.Condition->Delete();
  threader->Delete();
  return 1; // Always fail so the messages we print will show up on the dashboard.
}
