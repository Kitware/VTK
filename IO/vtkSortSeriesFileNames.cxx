/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortSeriesFileNames.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSortSeriesFileNames.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/list>
#include <vtkstd/algorithm>
#include <vtksys/SystemTools.hxx>

#include <ctype.h>

vtkCxxRevisionMacro(vtkSortSeriesFileNames, "1.5");
vtkStandardNewMacro(vtkSortSeriesFileNames);

// a container for holding string arrays

class vtkStringArrayVector
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkStringArray> > VectorType;

  static vtkStringArrayVector *New() {
    return new vtkStringArrayVector; }; 

  void Delete() {
    delete this; };

  void Clear() {
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
vtkSortSeriesFileNames::vtkSortSeriesFileNames()
{
  this->InputFileNames = 0;
  this->NumericSort = 0;
  this->IgnoreCase = 0;
  this->Grouping = 0;
  this->SkipDirectories = 0;
  this->FileNames = vtkStringArrayVector::New();
}

vtkSortSeriesFileNames::~vtkSortSeriesFileNames() 
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
}

void vtkSortSeriesFileNames::PrintSelf(ostream& os, vtkIndent indent)
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
      os << indent.GetNextIndent() << "FileNames[" << i << "]:  (" << 
        this->GetFileNames(i) << ")\n";
      }
    }
  else
    {
    os << indent.GetNextIndent() << "FileNames:  ("
       << this->GetFileNames() << ")\n";
    }
}

void vtkSortSeriesFileNames::SetInputFileNames(vtkStringArray *input)
{
  vtkSetObjectBodyMacro(InputFileNames, vtkStringArray, input);
}

int vtkSortSeriesFileNames::GetNumberOfGroups()
{
  this->Update();

  return this->FileNames->GetNumberOfStringArrays();
}

vtkStringArray *vtkSortSeriesFileNames::GetFileNames(int i)
{
  this->Update();

  int n = this->FileNames->GetNumberOfStringArrays();

  if (i >= 0 && i < n)
    {
    return this->FileNames->GetStringArray(i);
    }
  else
    {
    vtkErrorMacro(<< "GetFileNames(i): index " << i << " is out of range");
    return 0;
    }
}

vtkStringArray *vtkSortSeriesFileNames::GetFileNames()
{
  this->Update();

  if (this->GetGrouping())
    {
    vtkErrorMacro(<< "GetFileNames(): Grouping is on, please specify"
                  " an integer to identify the group");
    return 0;
    }

  if (this->FileNames->GetNumberOfStringArrays() > 0)
    {
    return this->FileNames->GetStringArray(0);
    }
  else
    {
    return 0;
    }
}

void vtkSortSeriesFileNames::GroupFileNames(vtkStringArray *input,
                                            vtkStringArrayVector *output)
{
  vtkstd::string baseName;
  vtkstd::string extension;
  vtkstd::string fileNamePath;
  vtkstd::string prefix;
  vtkstd::string postfix;

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

    // Find the last block of digits in the filename.
    // Set 'postfix' to the portion of the string after the digits,
    // and set 'prefix' to the portion of the string before the digits.
    int inDigitBlock = 0;
    for (unsigned int k = baseName.length(); k > 0; k--)
      {
      if (!inDigitBlock)
        {
        if (baseName[k-1] >= '0' && baseName[k-1] <= '9')
          {
          postfix = baseName.substr(k);
          inDigitBlock = 1;
          }
        }
      else
        {
        if (!(baseName[k-1] >= '0' && baseName[k-1] <='9'))
          {
          prefix = baseName.substr(0,k);
          break;
          }
        }
      }

    // If no digits were found
    if (!inDigitBlock)
      {
      prefix = "";
      postfix = baseName;
      }

    if (this->IgnoreCase)
      {
      unsigned int n = prefix.length();
      for (unsigned int j = 0; j < n; j++)
        {
        prefix[j] = toupper(prefix[j]);
        }
      unsigned int m = postfix.length();
      for (unsigned int k = 0; k < m; k++)
        {
        postfix[k] = toupper(postfix[k]);
        }
      }

    // the reduced filename has the block of digits replaced with "0"
    reducedFileNames.push_back(fileNamePath + "/" +
                               prefix + "0" + postfix + extension);

    // push the index onto the "ungrouped" list
    ungroupedFiles.push_back(i);
    }

  // prepare the output for new data
  output->Clear();

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


void vtkSortSeriesFileNames::SortFileNames(vtkStringArray *input,
                                           vtkStringArray *output)
{
  vtkstd::string baseName;
  vtkstd::string extension;
  vtkstd::string fileNamePath;

  // list of true files (i.e. reject paths that are directories)
  vtkStringArray *fileNames = vtkStringArray::New();

  // list of filenames decomposed into digit vs. non-digit segments
  vtkStringArrayVector *decompList = vtkStringArrayVector::New();

  // perform the decomposition of each of the filenames
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
    fileNames->InsertNextValue(fileName);

    vtkStringArray *decomp = vtkStringArray::New();
    vtkstd::string characters = "";
    vtkstd::string digits = "";
    
    fileNamePath = vtksys::SystemTools::GetFilenamePath(fileName); 
    baseName = vtksys::SystemTools::GetFilenameName(fileName);

    if (this->IgnoreCase)
      {
      unsigned int n = fileNamePath.length();
      for (unsigned int i = 0; i < n; i++)
        {
        fileNamePath[i] = toupper(fileNamePath[i]);
        }
      }

    // the directory is the first segment
    decomp->InsertNextValue(fileNamePath + "/");

    // the filename is broken into digit and character segments
    unsigned int m = baseName.length();
    for (unsigned int k = 0; k < m; k++)
      {
      char c = baseName[k];

      if (c >= '0' && c <= '9')
        {
        if (!characters.empty())
          {
          decomp->InsertNextValue(characters);
          characters = "";
          }
        digits.append(1, c);
        }
      else
        {
        if (!digits.empty())
          {
          decomp->InsertNextValue(digits);
          digits = "";
          }
        if (this->IgnoreCase)
          {
          c = toupper(c);
          }
        characters.append(1, c);
        }
      }
    if (!digits.empty())
      {
      decomp->InsertNextValue(digits);
      }
    else
      {
      decomp->InsertNextValue(characters);
      }
    decompList->InsertNextStringArray(decomp);
    decomp->Delete();
    }// end of internal loop j
    
  // find the maximum number of segments that any file was broken into
  unsigned int maxSegments = 0;
  unsigned int decompListLength =  decompList->GetNumberOfStringArrays();
  for(unsigned int j = 0; j < decompListLength; j++)
    {
    unsigned int maxId = decompList->GetStringArray(j)->GetMaxId();
    if(maxId + 1 > maxSegments)
      {
      maxSegments = decompList->GetStringArray(j)->GetMaxId() + 1;
      }
    }

  // if numeric sort, pad integer segments with zeros to make them
  // the same length before we sort
  if (this->NumericSort)
    {
    // loop over the segments, skipping the first because it is
    // the directory
    for(unsigned int p = 1; p < maxSegments; p++)
      {
      // find the maximum segment length over all filenames
      unsigned int l = 0;
      for (unsigned int q = 0; q < decompListLength; q++)
        {
        unsigned int maxId = decompList->GetStringArray(q)->GetMaxId();
        if (maxId >= p)
          {
          vtkstd::string& segment =
            decompList->GetStringArray(q)->GetValue(p);
          
          if (segment[0] >= '0' && segment[0] <= '9')
            {
            if (segment.length() > l)
              {
              l = segment.length();
              }
            } 
          }
        }// end of q

      // pad all numeric segments out to the same length with zeros,
      // that will make lexicographic sort equivalent to numeric sort
      for (unsigned int r = 0; r < decompListLength; r++)
        {
        unsigned int maxId = decompList->GetStringArray(r)->GetMaxId();
        if (maxId >= p)
          {
          vtkstd::string segment = 
            decompList->GetStringArray(r)->GetValue(p);

          if (segment[0] >= '0' && segment[0] <= '9')
            {
            unsigned int n = l - segment.length();
            // cast zero to size_type to avoid ambiguity
            segment.insert(static_cast<vtkstd::string::size_type>(0), n, '0');
            decompList->GetStringArray(r)->SetValue(p, segment);
            }  
          }
        }//end of r
      }// end of p
    }

  // create a dict to map filtered filenames to the original filenames
  vtkstd::map<vtkstd::string, vtkstd::string>  newFileNameDict;
  vtkstd::vector<vtkstd::string> newFileNameList;
  unsigned int numberOfStringArrays = decompList->GetNumberOfStringArrays();
  for (unsigned int t = 0; t < numberOfStringArrays; t++)
    {
    vtkstd::string newName = "";
    unsigned int numberOfSegments = 
      decompList->GetStringArray(t)->GetNumberOfValues();
    for (unsigned int q = 0; q < numberOfSegments; q++)
      {
      newName.append(decompList->GetStringArray(t)->GetValue(q));
      }
    newFileNameDict[newName] = fileNames->GetValue(t);
    newFileNameList.push_back(newName);
    }

  // sort the filtered filenames
  vtkstd::sort(newFileNameList.begin(), newFileNameList.end());
    
  // create a sorted list of original filenames
  output->Initialize();
  for(vtkstd::vector<vtkstd::string>::iterator lit = newFileNameList.begin();
      lit != newFileNameList.end();
      lit++)
    {
    output->InsertNextValue(newFileNameDict.find(*lit)->second);
    }
   
  decompList->Delete();
  fileNames->Delete();
}


void vtkSortSeriesFileNames::Execute()
{
  // create a new string array
  vtkStringArray *sortedInputStringArray = vtkStringArray::New();

  // sort the input file names
  this->SortFileNames(this->InputFileNames, sortedInputStringArray);

  // group the sorted files if grouping is on
  if (this->Grouping)
    {
    this->GroupFileNames(sortedInputStringArray, this->FileNames);
    }
  else
    {
    this->FileNames->Clear();
    this->FileNames->InsertNextStringArray(sortedInputStringArray);
    }
  
  sortedInputStringArray->Delete();
}


void vtkSortSeriesFileNames::Update()
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
