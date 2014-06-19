#if HAVE_MPE
  #include <string.h>
  #if defined(_WIN32) && defined(__GNUC__)
    #include <stdint.h>
    #include <inttypes.h>
  #endif
  #include "mpe.h"
  #if defined(MPE_LOG_OK)
    #define MPE_VERSION 2
  #elif defined(MPE_Log_OK)
    #define MPE_VERSION 1
  #else
    #undef  HAVE_MPE
    #define HAVE_MPE 0
  #endif
#endif

#if HAVE_MPE && MPE_VERSION==2
/* This is a hack for old MPE2 API's distributed with MPICH2 < 1.0.6 */
#if (defined(MPICH2) && !defined(MPICH2_NUMVERSION)) || defined(DEINO_MPI)
#define MPE_Describe_comm_state(comm,s0,s1,n,c,f) \
        MPE_Describe_comm_state(comm,0,s0,s1,n,c,f)
#define MPE_Describe_comm_event(comm,e,n,c,f) \
        MPE_Describe_comm_event(comm,0,e,n,c,f)
#define MPE_Log_comm_event(comm,e,b) \
        MPE_Log_comm_event(comm,0,e,b)
#endif
#endif

#include "mpe-log.h"

#if HAVE_MPE
static char logFileName[256] = { 0 };
#endif

static int PyMPELog_Init(void)
{
  int ierr = 0;
#if HAVE_MPE
  if (MPE_Initialized_logging() != 1)
    ierr = MPE_Init_log();
#endif
  return ierr;
}

static int PyMPELog_Finish(void)
{
  int ierr = 0;
#if HAVE_MPE
  const char *filename = logFileName;
  if (!filename[0]) filename = "Unknown";
  if (MPE_Initialized_logging() == 1)
    ierr = MPE_Finish_log((char *)filename);
#endif
  return ierr;
}

static int PyMPELog_Initialized(void)
{
  int status = 1;
#if HAVE_MPE
  status = MPE_Initialized_logging();
#else
  status = 1;
#endif /* HAVE_MPE */
  return status;
}

static int PyMPELog_SetFileName(const char filename[])
{
  int ierr = 0;
#if HAVE_MPE
  if (!filename) return ierr;
  strncpy(logFileName, filename, sizeof(logFileName));
  logFileName[sizeof(logFileName)-1] = 0;
#endif
  return ierr;
}

static int PyMPELog_SyncClocks(void)
{
  int ierr = 0;
#if HAVE_MPE
  #if MPE_VERSION==2
  ierr = MPE_Log_sync_clocks();
  #endif
#endif /* HAVE_MPE */
  return ierr;
}

static int PyMPELog_Start(void)
{
  int ierr = 0;
#if HAVE_MPE
  ierr = MPE_Start_log();
#endif /* HAVE_MPE */
  return ierr;
}

static int PyMPELog_Stop(void)
{
  int ierr = 0;
#if HAVE_MPE
  ierr = MPE_Stop_log();
#endif /* HAVE_MPE */
  return ierr;
}

#if HAVE_MPE
static MPI_Comm PyMPELog_GetComm(int commID)
{
  switch (commID) {
  case 0:  return MPI_COMM_NULL;
  case 1:  return MPI_COMM_SELF;
  case 2:  return MPI_COMM_WORLD;
  default: return MPI_COMM_WORLD;
  }
}
#endif

static int PyMPELog_NewState(int commID,
                             const char name[],
                             const char color[],
                             const char format[],
                             int stateID[2])
{
  int ierr = 0;
#if HAVE_MPE
  MPI_Comm comm = PyMPELog_GetComm(commID);
  if (comm == MPI_COMM_NULL) return 0;
  #if MPE_VERSION==2
  ierr = MPE_Log_get_state_eventIDs(&stateID[0], &stateID[1]);
  if (ierr == -99999) { ierr = 0; stateID[0] = stateID[1] = -99999; }
  if (ierr != 0) return ierr;
  ierr = MPE_Describe_comm_state(comm, stateID[0], stateID[1],
                                 name, color, format);
  #else
  stateID[0] = MPE_Log_get_event_number();
  stateID[1] = MPE_Log_get_event_number();
  ierr = MPE_Describe_state(stateID[0], stateID[1],
                            (char *)name, (char *)color);
  #endif
#endif /* HAVE_MPE */
  return ierr;
}

static int PyMPELog_NewEvent(int commID,
                             const char name[],
                             const char color[],
                             const char format[],
                             int eventID[1])
{
  int ierr = 0;
#if HAVE_MPE
  MPI_Comm comm = PyMPELog_GetComm(commID);
  if (comm == MPI_COMM_NULL) return 0;
  #if MPE_VERSION==2
  ierr = MPE_Log_get_solo_eventID(&eventID[0]);
  if (ierr == -99999) { ierr = 0; eventID[0] = -99999; }
  if (ierr != 0) return ierr;
  ierr = MPE_Describe_comm_event(comm, eventID[0],
                                 name, color, format);
  #else
  eventID[0] = MPE_Log_get_event_number();
  MPE_Describe_event (eventID[0], (char *)name);
  #endif
#endif /* HAVE_MPE */
  return ierr;
}

static int PyMPELog_LogEvent(int commID,
                             const int eventID,
                             const char bytebuf[])
{
  int ierr = 0;
#if HAVE_MPE
  MPI_Comm comm = PyMPELog_GetComm(commID);
  if (comm == MPI_COMM_NULL) return 0;
  #if MPE_VERSION==2
  ierr = MPE_Log_comm_event(comm, eventID, bytebuf);
  #else
  ierr = MPE_Log_event(eventID, 0, /*NULL*/0);
  #endif
#endif /* HAVE_MPE */
  return ierr;
}

static int PyMPELog_PackBytes(char bytebuf[], int *position,
                              char tokentype, int count,
                              const void *data)
{
  int ierr = 0;
#if HAVE_MPE
  #if MPE_VERSION==2
  if (((unsigned)*position) <= sizeof(MPE_LOG_BYTES))
    ierr = MPE_Log_pack(bytebuf, position,
                        tokentype, count, data);
  #endif
#endif /* HAVE_MPE */
  return ierr;
}

static PyMPELogAPI PyMPELog_ = {
  PyMPELog_Init,
  PyMPELog_Finish,
  PyMPELog_Initialized,
  PyMPELog_SetFileName,
  PyMPELog_SyncClocks,
  PyMPELog_Start,
  PyMPELog_Stop,
  PyMPELog_NewState,
  PyMPELog_NewEvent,
  PyMPELog_LogEvent,
  PyMPELog_PackBytes
};

static PyMPELogAPI *PyMPELog = &PyMPELog_;

/*
  Local Variables:
  c-basic-offset: 2
  indent-tabs-mode: nil
  End:
*/
