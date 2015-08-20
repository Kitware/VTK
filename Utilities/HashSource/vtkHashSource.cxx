/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHashSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <string>
#include <fstream>
#include <iostream>
#include <vtksys/MD5.h>
#include <vtksys/SystemTools.hxx>
#include <vtksys/RegularExpression.hxx>

static std::string HashMD5(std::istream& in)
{
  char hash[33];
  vtksysMD5* hasher = vtksysMD5_New();
  vtksysMD5_Initialize(hasher);

  vtksys::RegularExpression key("\\$(Revision|Date|RCSfile):[^$]*\\$");

  std::string line;
  while(vtksys::SystemTools::GetLineFromStream(in, line))
    {
    // Remove CVS key values from the line (simulate -kk).
    while(key.find(line))
      {
      std::string tmp = line.substr(0, key.start());
      tmp += "$";
      tmp += key.match(1);
      tmp += "$";
      tmp += line.substr(key.end());
      line = tmp;
      }

    // Append the line and a newline.
    vtksysMD5_Append(hasher, (unsigned char const*)line.c_str(), -1);
    vtksysMD5_Append(hasher, (unsigned char const*)"\n", 1);
    }

  vtksysMD5_FinalizeHex(hasher, hash);
  vtksysMD5_Delete(hasher);
  hash[32] = 0;
  return std::string(hash);
}

int main(int argc, char *argv[])
{
  if(argc < 3)
    {
    std::cerr << "Usage: vtkHashSource input.cxx name [output.h]\n";
    return 1;
    }
  const char* inFile = argv[1];
  const char* name = argv[2];
  const char* outFile = argc > 3? argv[3] : 0;

  std::ifstream fin(inFile);
  if(!fin)
    {
    std::cerr << "Unable to read \"" << inFile << "\"\n";
    return 1;
    }

  std::string md5 = HashMD5(fin);

  std::ostream* out = &std::cout;
  std::ofstream fout;
  if(outFile)
    {
    fout.open(outFile);
    if(!fout)
      {
      std::cerr << "Unable to write \"" << outFile << "\"\n";
      return 1;
      }
    out = &fout;
    }

  *out
    << "#ifndef " << name << "\n"
    << "# define " << name << " \"" << md5 << "\"\n"
    << "#endif\n";

  return 0;
}
