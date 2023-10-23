// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef ADIOSTestUtilities_h
#define ADIOSTestUtilities_h

// adios2::fstream has private copy c'tor and no move operations defined,
// so we can't create a wrapper to create and return an fstream
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#define ADIOS_OPEN(fw, fileName) adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm())
#else
#define ADIOS_OPEN(fw, fileName) adios2::fstream fw(fileName, adios2::fstream::out)
#endif

#endif // ADIOSTestUtilities_h
