/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageBenchmarkDriver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This program is a driver for the benchmarking tests.
// It runs several benchmarks and writes them to the output directory.

#include "vtkMultiThreader.h"
#include "vtkThreadedImageAlgorithm.h"
#include "vtkVersion.h"

#include <vtksys/Process.h>

#include <iostream>
#include <string>
#include <vector>

const char *HelpText =
"Usage: ImageBenchmarkDriver --prefix <path/prefix> ...\n"
"\n"
"Options:\n"
"  --prefix <path/prefix>  Prefix for output filenames.\n"
"  Any options from ImageBenchmark can also be used.\n"
"\n"
"Details:\n"
"\n"
"This program runs a series of image processing benchmarks,\n"
"by running ImageBenchmark with various parameters:\n"
"\n";

// A struct to give a short name to each ImageBenchmark option
struct BenchOption
{
  const char *Name;
  const char *Option;
};

// A struct to link a range of options to a benchmark parameter
struct BenchParameter
{
  const char *Parameter;
  BenchOption *Options;
};

static BenchOption FilterList[] =
{
  { "Median3",        "median:kernelsize=3" },
  { "Reslice2D",      "reslice:kernel=linear:rotation=45/0/0/1" },
  { "Reslice3D",      "reslice:kernel=linear:rotation=60/0/1/1" },
  { "Colors4",        "colormap:components=4" },
  { NULL, NULL }
};

static BenchOption SplitModeList[] =
{
  { "Slab", "slab" },
  { "Beam", "beam" },
  { "Block", "block" },
  { NULL, NULL }
};

// These are only for --enable-smp on
static BenchOption BlockByteList[] =
{
  { "1KiB",   "1024" },
  { "4KiB",   "4096" },
  { "16KiB",  "16384" },
  { "64KiB",  "65536" },
  { "256KiB", "262144" },
  { "1MiB",   "1048576" },
  { "4MiB",   "4194304" },
  { "16MiB",  "16777216" },
  { NULL, NULL }
};

static BenchOption ImageSizeList[] =
{
  { "4096x4096",   "4096x4096x1" },
  { "256x256x256", "256x256x256" },
  { NULL, NULL }
};

static BenchParameter Parameters[] =
{
  { "--filter", FilterList },
  { "--split-mode", SplitModeList },
  { "--bytes-per-piece", BlockByteList },
  { "--size", ImageSizeList },
  { NULL, NULL }
};

int main(int argc, char *argv[])
{
  // Create the path to the ImageBenchmark executable (assume that
  // it is in same directory)
  std::string exename = argv[0];
  size_t l = 0;
  for (size_t j = 0; j != exename.length(); j++)
  {
    if (exename[j] == '/' || exename[j] == '\\')
    {
      l = j + 1;
    }
  }
  exename = exename.substr(0, l) + "ImageBenchmark";

  // Go through the arguments to create args for ImageBenchmark
  bool useSMP = vtkThreadedImageAlgorithm::GetGlobalDefaultEnableSMP();
  std::string prefix;
  std::vector<const char *> args;
  args.push_back(exename.c_str());
  int argi = 1;
  while (argi < argc)
  {
    std::string arg = argv[argi];
    if (arg == "-h" || arg == "-help" || arg == "--help")
    {
      std::cout << HelpText;
      for (BenchParameter *p = Parameters; p->Parameter; p++)
      {
        std::string opt = p->Parameter;
        for (BenchOption *o = p->Options; o->Option; o++)
        {
          std::cout << "  " << opt << " " << o->Option << "    ("
                    << o->Name << ")\n";
        }
        std::cout << std::endl;
      }
      return 0;
    }
    else if (arg == "--version")
    {
      std::cout << "ImageBenchmarkDriver "
                << vtkVersion::GetVTKVersion() << "\n";
      return 0;
    }
    else if (arg == "--prefix")
    {
      if (++argi < argc)
      {
        prefix = argv[argi++];
      }
    }
    else if (arg == "--enable-smp")
    {
      args.push_back(argv[argi++]);
      if (argi < argc)
      {
        std::string s = argv[argi];
        if (s == "on" || s == "yes" || s == "true")
        {
          useSMP = true;
        }
        else if (s == "off" || s == "no" || s == "false")
        {
          useSMP = false;
        }
        args.push_back(argv[argi++]);
      }
    }
    else
    {
      args.push_back(argv[argi++]);
    }
  }

  // Count the number of benchmarks to do
  int total = 1;
  std::vector<int> overrides;
  for (BenchParameter *p = Parameters; p->Parameter; p++)
  {
    // Check if this parameter was overridden on the command line
    std::string arg = p->Parameter;
    bool overRidden = false;
    for (size_t j = 1; j < args.size(); j++)
    {
      if (arg == args[j] ||
          (!useSMP && arg == "--bytes-per-piece"))
      {
        overRidden = true;
        break;
      }
    }
    overrides.push_back(overRidden);
    if (overRidden)
    {
      continue;
    }

    int count = 0;
    for (BenchOption *o = p->Options; o->Option; o++)
    {
      count++;
    }
    total *= count;
  }

  for (int i = 0; i < total; i++)
  {
    std::string filename = prefix;
    if (prefix.empty() ||
        prefix[prefix.length()-1] == '/' ||
        prefix[prefix.length()-1] == '\\')
    {
      filename += (useSMP ? "SMP" : "MT");
    }

    std::vector<const char *> commandLine;
    commandLine.push_back(args[0]);
    for (size_t j = 1; j < args.size(); j++)
    {
      commandLine.push_back(args[j]);
    }

    int part = 1;
    std::vector<int>::iterator skip = overrides.begin();
    for (BenchParameter *p = Parameters; p->Parameter; ++p, ++skip)
    {
      if (*skip)
      {
        continue;
      }

      int count = 0;
      for (BenchOption *o = p->Options; o->Option; o++)
      {
        count++;
      }
      int k = (i/part) % count;
      commandLine.push_back(p->Parameter);
      commandLine.push_back(p->Options[k].Option);
      filename.push_back('_');
      filename += p->Options[k].Name;
      part *= count;
    }

    commandLine.push_back("--slave");
    commandLine.push_back(NULL);

    filename += ".csv";
    std::ofstream outfile(filename.c_str());

    // create and run the subprocess
    vtksysProcess *process = vtksysProcess_New();
    vtksysProcess_SetCommand(process, &commandLine[0]);
    vtksysProcess_Execute(process);

    int pipe;
    do
    {
      char *cp;
      int length;
      pipe = vtksysProcess_WaitForData(process, &cp, &length, NULL);
      switch (pipe)
      {
        case vtksysProcess_Pipe_STDOUT:
          outfile.write(cp, length);
          break;

        case vtksysProcess_Pipe_STDERR:
          std::cerr.write(cp, length);
          break;
      }
    }
    while (pipe != vtksysProcess_Pipe_None);

    vtksysProcess_WaitForExit(process, NULL);
    int rval = vtksysProcess_GetExitValue(process);
    if (rval != 0)
    {
      return rval;
    }

    vtksysProcess_Delete(process);

    outfile.close();

    std::cout << (i + 1) << " of " << total << ": " << filename << std::endl;
  }

  return 0;
}
