#include "vtkDirectory.h"

vtkDirectory::vtkDirectory() 
  : Path(0), Files(0), NumberOfFiles(0)
{
}



vtkDirectory::~vtkDirectory() 
{
  for(int i =0; i < this->NumberOfFiles; i++)
    {
    delete [] this->Files[i];
    }
  delete [] this->Files;
  delete [] this->Path;
}



void vtkDirectory::PrintSelf(ostream& os, vtkIndent indent)
{ 
  vtkObject::PrintSelf(os, indent);
  if(!this->Path)
    {
    os << indent << "Directory not open\n";
    return;
    }
  
  os << indent << "Directory for: " <<  this->Path << "\n";
  os << indent << "Contains the following files:\n";
  indent = indent.GetNextIndent();
  for(int i =0; i < this->NumberOfFiles; i++)
    {
    os << indent << this->Files[i] << "\n";
    }
}


// First microsoft compilers

#ifdef _MSC_VER
#include <windows.h>
#include <io.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int vtkDirectory::Open(const char* name)
{
  char* buf;
  int n = strlen(name);
  if (name[n - 1] == '/') 
    {
    buf = new char[n + 1 + 1];
    sprintf(buf, "%s*", name);
    } 
  else
    {
    buf = new char[n + 2 + 1];
    sprintf(buf, "%s/*", name);
    }
  struct _finddata_t data;	// data of current file
  
  // First count the number of files in the directory
  long srchHandle = _findfirst(buf, &data);
  if (srchHandle == -1)
    {
    this->NumberOfFiles = 0;
    _findclose(srchHandle);
    return 0;
    }
  
  this->NumberOfFiles = 1;
  while(_findnext(srchHandle, &data) != -1)
    {
    this->NumberOfFiles++;
    }
  this->Files = new char*[this->NumberOfFiles];
  // close the handle 
  _findclose(srchHandle);
  // Now put them into the file array
  srchHandle = _findfirst(buf, &data);
  delete [] buf;
  
  if (srchHandle == -1)
    {
    this->NumberOfFiles = 0;
    _findclose(srchHandle);
    return 0;
    }
  
  // Loop through names
  int i = 0;
  do 
    {
    this->Files[i] = strcpy(new char[strlen(data.name)+1], data.name);
    i++;
    } 
  while (_findnext(srchHandle, &data) != -1);
  this->Path = strcpy(new char[strlen(name)+1], name);
  return _findclose(srchHandle) != -1;
}

#else

// Now the POSIX style directory access

#include <sys/types.h>
#include <dirent.h>

int vtkDirectory::Open(const char* name)
{
  DIR* dir = opendir(name);
  if (!dir) 
    {
    return 0;
    }
  this->NumberOfFiles = 0;
  dirent* d =0;
  
  for (d = readdir(dir); d; d = readdir(dir))
    {
    this->NumberOfFiles++;
    }
  this->Files = new char*[this->NumberOfFiles];
  closedir(dir);
  
  dir = opendir(name);
  if (!dir) 
    {
    return 0;
    }
  int i = 0;
  for (d = readdir(dir); d; d = readdir(dir))
    {
    this->Files[i] = strcpy(new char[strlen(d->d_name)+1], d->d_name);
    i++;
    }
  this->Path = strcpy(new char[strlen(name)+1], name);
  closedir(dir);
  return 1;
}

#endif


const char* vtkDirectory::GetFile(int index)
{
  if(index >= this->NumberOfFiles || index < 0)
    {
    vtkErrorMacro( << "Bad index for GetFile on vtkDirectory\n");
    return 0;
    }
  
  return this->Files[index];
}

