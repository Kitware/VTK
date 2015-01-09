#ifndef PyMPI_MPE_LOG_H
#define PyMPI_MPE_LOG_H

typedef struct PyMPELogAPI {
  int (*Init)(void);
  int (*Finish)(void);
  int (*Initialized)(void);
  int (*SetFileName)(const char[]);
  int (*SyncClocks)(void);
  int (*Start)(void);
  int (*Stop)(void);
  int (*NewState)(int,
                  const char[],
                  const char[],
                  const char[],
                  int[2]);
  int (*NewEvent)(int,
                  const char[],
                  const char[],
                  const char[],
                  int[1]);
  int (*LogEvent)(int,
                  int,
                  const char[]);
  int (*PackBytes)(char[], int *,
                   char, int,
                   const void *);
} PyMPELogAPI;

#endif /*! PyMPI_MPE_LOG_H */
