/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortFileNames.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSortFileNames.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtkstd/list>
#include <vtkstd/algorithm>
#include <vtksys/SystemTools.hxx>

#include <ctype.h>

vtkStandardNewMacro(vtkSortFileNames);

// a container for holding string arrays

class vtkStringArrayVector
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkStringArray> > VectorType;

  static vtkStringArrayVector *New() {
    return new vtkStringArrayVector; }; 

  void Delete() {
    delete this; };

  void Reset() {
    this->Container.clear(); };

  void InsertNextStringArray(vtkStringArray *stringArray) {
    this->Container.push_back(stringArray); };

  vtkStringArray *GetStringArray(int i) {
    return this->Container[static_cast<VectorType::size_type>(i)]; };

  int GetNumberOfStringArrays() {
    return static_cast<int>(this->Container.size()); };


private:
  vtkStringArrayVector() : Container() {};
  ~vtkStringArrayVector() { this->Container.clear(); };

  VectorType Container;
};


//----------------------------------------------------------------------------
vtkSortFileNames::vtkSortFileNames()
{
  this->InputFileNames = 0;
  this->NumericSort = 0;
  this->IgnoreCase = 0;
  this->Grouping = 0;
  this->SkipDirectories = 0;
  this->FileNames = vtkStringArray::New();
  this->Groups = vtkStringArrayVector::New();
}

vtkSortFileNames::~vtkSortFileNames() 
{
  if (this->InputFileNames)
    {
    this->InputFileNames->Delete();
    this->InputFileNames = 0;
    }
  if (this->FileNames)
    {
    this->FileNames->Delete();
    this->FileNames = 0;
    }
  if (this->Groups)
    {
    this->Groups->Delete();
    this->Groups = 0;
    }
}

void vtkSortFileNames::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputFileNames:  (" << this->GetInputFileNames() << ")\n";
  os << indent << "NumericSort:  " << 
    (this->GetNumericSort() ? "On\n" : "Off\n");
  os << indent << "IgnoreCase:  " << 
    (this->GetIgnoreCase() ? "On\n" : "Off\n");
  os << indent << "Grouping:  " << 
    (this->GetGrouping() ? "On\n" : "Off\n");
  os << indent << "SkipDirectories:  " << 
    (this->GetSkipDirectories() ? "On\n" : "Off\n");
  
  os << indent << "NumberOfGroups: " << this->GetNumberOfGroups() << "\n";
  
  if (this->GetGrouping())
    {
    for (int i = 0; i < this->GetNumberOfGroups(); i++)
      {
      os << indent.GetNextIndent() << "Group[" << i << "]:  (" << 
        this->GetNthGroup(i) << ")\n";
      }
    }
  else
    {
    os << indent.GetNextIndent() << "FileNames:  ("
       << this->GetFileNames() << ")\n";
    }
}

void vtkSortFileNames::SetInputFileNames(vtkStringArray *input)
{
  vtkSetObjectBodyMacro(InputFileNames, vtkStringArray, input);
}

int vtkSortFileNames::GetNumberOfGroups()
{
  this->Update();

  return this->Groups->GetNumberOfStringArrays();
}

vtkStringArray *vtkSortFileNames::GetNthGroup(int i)
{
  this->Update();

  if (!this->GetGrouping())
    {
    vtkErrorMacro(<< "GetNthGroup(): Grouping not on.");
    return 0;
    }

  int n = this->Groups->GetNumberOfStringArrays();

  if (i >= 0 && i < n)
    {
    return this->Groups->GetStringArray(i);
    }
  else
    {
    vtkErrorMacro(<< "GetNthGroup(i): index " << i << " is out of range");
    return 0;
    }
}

vtkStringArray *vtkSortFileNames::GetFileNames()
{
  this->Update();

  return this->FileNames;
}

void vtkSortFileNames::GroupFileNames(vtkStringArray *input,
                                      vtkStringArrayVector *output)
{
  vtkstd::string baseName;
  vtkstd::string extension;
  vtkstd::string fileNamePath;
  vtkstd::string reducedName;

  vtkstd::list<unsigned int> ungroupedFiles;
  vtkstd::vector<vtkstd::string> reducedFileNames;

  unsigned int numberOfStrings = input->GetNumberOfValues();
  for (unsigned int i = 0; i < numberOfStrings; i++)
    {
    vtkstd::string& fileName = input->GetValue(i);
    extension = vtksys::SystemTools::GetFilenameLastExtension(fileName);
    fileNamePath = vtksys::SystemTools::GetFilenamePath(fileName);
    baseName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);
    
    // If extension is all digits, it is not a true extension, so
    // add it back onto the filename.  Note that the extension
    // includes the leading dot.
    int numericExtension = 1;
    for (unsigned int j = 1; j < extension.length(); j++)
      {
      if (!(extension[j] >= '0' && extension[j] <= '9'))
        {
        numericExtension = 0;
        }
      }
    if (numericExtension && extension.length() != 0)
      {
      baseName.append(extension);
      extension = "";
      }

    // Create a reduced filename that replaces all digit sequences
    // in the filename with a single digit "0". We begin by setting
    // the reduced filename to the path.
    reducedName = fileNamePath + "/";
    int inDigitBlock = 0;
    unsigned int charBlockStart = 0;
    unsigned int stringLength = static_cast<unsigned int>(baseName.length());
    for (unsigned int k = 0; k < stringLength; k++)
      {
      if (baseName[k] >= '0' && baseName[k] <= '9')
        {
        if (!inDigitBlock && k != 0)
          {
          reducedName.append(baseName.substr(charBlockStart,
                                             k-charBlockStart));
          reducedName.append("0");
          }
        inDigitBlock = 1;
        }
      else
        {
        if (inDigitBlock)
          {
          charBlockStart = k;
          inDigitBlock = 0;
          }
        }
      }
    if (!inDigitBlock)
      {
      reducedName.append(baseName.substr(charBlockStart,
                                         stringLength-charBlockStart));
      }
      
    // Add extension back to the filename.
    reducedName.append(extension);

    // If IgnoreCase is set, change to uppercase.
    if (this->IgnoreCase)
      {
      unsigned int n = static_cast<unsigned int>(reducedName.length());
      for (unsigned int j = 0; j < n; j++)
        {
        reducedName[j] = toupper(reducedName[j]);
        }
      }

    // The reduced filename has each block of digits replaced with "0".
    reducedFileNames.push_back(reducedName);

    // push the index onto the "ungrouped" list
    ungroupedFiles.push_back(i);
    }

  // now loop through all files and find all matches
  while (!ungroupedFiles.empty())
    {
    // get the first element in the list
    unsigned int fileIndex = ungroupedFiles.front();
    vtkstd::string& reducedFileName = reducedFileNames[fileIndex];

    vtkStringArray *newGroup = vtkStringArray::New();

    // find all matches and move them into the group
    vtkstd::list<unsigned int>::iterator p = ungroupedFiles.begin();
    while (p != ungroupedFiles.end())
      {
      unsigned int tryIndex = *p;

      if (reducedFileName == reducedFileNames[tryIndex])
        {
        newGroup->InsertNextValue(input->GetValue(tryIndex));
        p = ungroupedFiles.erase(p);
        }
      else
        {
        p++;
        }
      }

    // add the group to the output
    output->InsertNextStringArray(newGroup);
    newGroup->Delete();
    }
}

// Sort filenames lexicographically, ignoring case.
bool vtkCompareFileNamesIgnoreCase(const vtkstd::string s1,
                                   const vtkstd::string s2)
{
  unsigned int n1 = static_cast<unsigned int>(s1.length());
  unsigned int n2 = static_cast<unsigned int>(s2.length());

  // find the minimum of the two lengths
  unsigned int n = n1;
  if (n > n2)
    {
    n = n2;
    }

  // compare the strings with no case
  for (unsigned int i = 0; i < n; i++)
    {
    char c1 = toupper(s1[i]);
    char c2 = toupper(s2[i]);

    if (c1 < c2)
      {
      return 1;
      }
    if (c1 > c2)
      {
      return 0;
      }
    }
  
  // if it is a tie, then the short string is "less"
  if (n1 < n2)
    {
    return 1;
    }

  // if strings are equal, use case-sensitive comparison to break tie 
  if (n1 == n2)
    {
    return (s1 < s2);
    }

  // otherwise, if n1 > n2, then n1 wins
  return 0;
}

// Sort filenames numerically
bool vtkCompareFileNamesNumeric(const vtkstd::string s1,
                                const vtkstd::string s2)
{
  unsigned int n1 = static_cast<unsigned int>(s1.length());
  unsigned int n2 = static_cast<unsigned int>(s2.length());

  // compare the strings numerically
  unsigned int i1 = 0;
  unsigned int i2 = 0;
  while (i1 < n1 && i2 < n2)
    {
    char c1 = s1[i1++];
    char c2 = s2[i2++];

    if (0 && (c1 >= '0' && c1 <= '9') && (c2 >= '0' && c2 <= '9'))
      {
      // convert decimal numeric sequence into an integer
      unsigned int j1 = 0;
      while (c1 >= '0' && c1 <= '9')
        {
        j1 = (j1 << 3) + (j1 << 1) + (c1 - '0');
        if (i1 == n1)
          {
          break;
          }
        c1 = s1[i1++];
        }
    
      // convert decimal numeric sequence into an integer
      unsigned int j2 = 0;
      while (c2 >= '0' && c2 <= '9')
        {
        j2 = (j2 << 3) + (j2 << 1) + (c2 - '0');
        if (i2 == n2)
          {
          break;
          }
        c2 = s2[i2++];
        }

      // perform the numeric comparison
      if (j1 < j2)
        {
        return 1;
        }
      if (j1 > j2)
        {
        return 0;
        }
      }

    // case-insensitive lexicographic comparison of non-digits
    if ((c1 < '0' || c1 > '9') || (c2 < '0' || c2 > '9'))
      {
      if (c1 < c2)
        {
        return 1;
        }
      if (c1 > c2)
        {
        return 0;
        }
      }
    }
  
  // if it is a tie, then the shorter string is "less"
  if ((n1 - i1) < (n2 - i2))
    {
    return 1;
    }

  // if strings are otherwise equal, fall back to default to break tie 
  if ((i1 == n1) && (i2 == n2))
    {
    return (s1 < s2);
    }

  // otherwise, return false
  return 0;
}

// Sort filenames numerically
bool vtkCompareFileNamesNumericIgnoreCase(const vtkstd::string s1,
                                          const vtkstd::string s2)
{
  unsigned int n1 = static_cast<unsigned int>(s1.length());
  unsigned int n2 = static_cast<unsigned int>(s2.length());

  // compare the strings numerically
  unsigned int i1 = 0;
  unsigned int i2 = 0;
  while (i1 < n1 && i2 < n2)
    {
    char c1 = s1[i1++];
    char c2 = s2[i2++];

    if ((c1 >= '0' && c1 <= '9') && (c2 >= '0' && c2 <= '9'))
      {
      // convert decimal numeric sequence into an integer
      unsigned int j1 = 0;
      while (c1 >= '0' && c1 <= '9')
        {
        j1 = (j1 << 3) + (j1 << 1) + (c1 - '0');
        if (i1 == n1)
          {
          break;
          }
        c1 = s1[i1++];
        }
    
      // convert decimal numeric sequence into an integer
      unsigned int j2 = 0;
      while (c2 >= '0' && c2 <= '9')
        {
        j2 = (j2 << 3) + (j2 << 1) + (c2 - '0');
        if (i2 == n2)
          {
          break;
          }
        c2 = s2[i2++];
        }

      // perform the numeric comparison
      if (j1 < j2)
        {
        return 1;
        }
      if (j1 > j2)
        {
        return 0;
        }
      }

    // case-insensitive lexicographic comparison of non-digits
    if ((c1 < '0' || c1 > '9') || (c2 < '0' || c2 > '9'))
      {
      c1 = toupper(c1);
      c2 = toupper(c2);

      if (c1 < c2)
        {
        return 1;
        }
      if (c1 > c2)
        {
        return 0;
        }
      }
    }
  
  // if it is a tie, then the shorter string is "less"
  if ((n1 - i1) < (n2 - i2))
    {
    return 1;
    }

  // if strings are otherwise equal, fall back to default to break tie 
  if ((i1 == n1) && (i2 == n2))
    {
    return vtkCompareFileNamesIgnoreCase(s1, s2);
    }

  // otherwise, return false
  return 0;
}


void vtkSortFileNames::SortFileNames(vtkStringArray *input,
                                     vtkStringArray *output)
{
  // convert vtkStringArray into an STL vector
  vtkstd::vector<vtkstd::string> fileNames;
  unsigned int numberOfStrings = input->GetNumberOfValues();
  for (unsigned int j = 0; j < numberOfStrings; j++)
    {
    vtkstd::string& fileName = input->GetValue(j);

    // skip anything that is a directory
    if (this->SkipDirectories &&
        vtksys::SystemTools::FileIsDirectory(fileName.c_str()))
      {
      continue;
      }

    // build a new list
    fileNames.push_back(fileName);
    }

  // perform the sort according to the options that are set
  if (this->IgnoreCase)
    {
    if (this->NumericSort)
      {
      // numeric sort without case sensitivity
      vtkstd::sort(fileNames.begin(), fileNames.end(),
                   vtkCompareFileNamesNumericIgnoreCase);
      }
    else
      {
      // lexicographic sort without case sensitivity
      vtkstd::sort(fileNames.begin(), fileNames.end(),
                   vtkCompareFileNamesIgnoreCase);
      }
    }
  else
    {
    if (this->NumericSort)
      {
      // numeric sort
      vtkstd::sort(fileNames.begin(), fileNames.end(),
                   vtkCompareFileNamesNumeric);
      }
    else
      {
      // lexicographic sort (the default)
      vtkstd::sort(fileNames.begin(), fileNames.end());
      }
    }

  // build the output
  vtkstd::vector<vtkstd::string>::iterator iter = fileNames.begin();
  while (iter < fileNames.end())
    {
    output->InsertNextValue(*iter++);
    }
}


void vtkSortFileNames::Execute()
{
  // sort the input file names
  this->FileNames->Reset();
  this->SortFileNames(this->InputFileNames, this->FileNames);

  // group the sorted files if grouping is on
  this->Groups->Reset();
  if (this->Grouping)
    {
    this->GroupFileNames(this->FileNames, this->Groups);
    }
}


void vtkSortFileNames::Update()
{
  if (this->InputFileNames != 0)
    {
    if (this->GetMTime() > this->UpdateTime.GetMTime() ||
        this->InputFileNames->GetMTime() > this->UpdateTime.GetMTime())
      {
      this->Execute();
      this->UpdateTime.Modified();
      }
    }
}
