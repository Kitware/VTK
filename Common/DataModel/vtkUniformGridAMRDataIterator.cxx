/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMRDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkAMRInformation.h"
#include "vtkAMRDataInternals.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkUniformGrid.h"
#include "vtkInformation.h"
#include <cassert>

//----------------------------------------------------------------
class AMRIndexIterator: public vtkObject
{
public:
  static AMRIndexIterator* New();
  vtkTypeMacro(AMRIndexIterator,vtkObject);

  void Initialize(const std::vector<int>* numBlocks)
  {
    assert(numBlocks && numBlocks->size()>=1);
    this->Level = 0;
    this->Index = -1;
    this->NumBlocks =  numBlocks;
    this->NumLevels = this->GetNumberOfLevels();
    this->Next();
  }
  void Next()
  {
    this->AdvanceIndex();
    //advanc the level either when we are at the right level of out of levels
    while(this->Level < this->NumLevels && static_cast<unsigned int>(this->Index)>= this->GetNumberOfBlocks(this->Level+1))
      {
      this->Level++;
      }
  }
  virtual bool IsDone() {   return this->Level>=this->NumLevels;}
  unsigned int GetLevel() { return this->Level;  }
  unsigned int GetId() { return this->Index - this->GetNumberOfBlocks(this->Level);}
  virtual unsigned int GetFlatIndex() { return this->Index;}
protected:
  AMRIndexIterator(): Level(0), Index(0) {}
  ~AMRIndexIterator(){};
  unsigned int Level;
  int Index;
  unsigned int NumLevels;
  const std::vector<int>* NumBlocks;
  virtual void AdvanceIndex() { this->Index++;}
  virtual unsigned int GetNumberOfLevels() { return static_cast<unsigned int>(this->NumBlocks->size()-1);};
  virtual unsigned int GetNumberOfBlocks(int i)
  {
    assert(i< static_cast<int>(this->NumBlocks->size()));
    return (*this->NumBlocks)[i];
  }


};
vtkStandardNewMacro(AMRIndexIterator);


//----------------------------------------------------------------

class AMRLoadedDataIndexIterator: public AMRIndexIterator
{
public:
  static AMRLoadedDataIndexIterator* New();
  vtkTypeMacro(AMRLoadedDataIndexIterator,AMRIndexIterator);
  AMRLoadedDataIndexIterator(){}
  void Initialize(const std::vector<int>* numBlocks, const vtkAMRDataInternals::BlockList* dataBlocks)
  {
    assert(numBlocks && numBlocks->size()>=1);
    this->Level = 0;
    this->InternalIdx = -1;
    this->NumBlocks =  numBlocks;
    this->DataBlocks = dataBlocks;
    this->NumLevels = this->GetNumberOfLevels();
    this->Next();
  }
protected:
  virtual void AdvanceIndex()
  {
    this->InternalIdx++;
    Superclass::Index = static_cast<size_t>(this->InternalIdx) < this->DataBlocks->size()? (*this->DataBlocks)[this->InternalIdx].Index : 0;
  }
  virtual bool IsDone() { return static_cast<size_t>(this->InternalIdx) >=  this->DataBlocks->size();}
  const vtkAMRDataInternals::BlockList* DataBlocks;
  int InternalIdx;
private:
  AMRLoadedDataIndexIterator(const AMRLoadedDataIndexIterator&);  //Not implemented
  void operator=(const AMRLoadedDataIndexIterator&);  //Not implemented
};
vtkStandardNewMacro(AMRLoadedDataIndexIterator);

//----------------------------------------------------------------

vtkStandardNewMacro(vtkUniformGridAMRDataIterator);

vtkUniformGridAMRDataIterator::vtkUniformGridAMRDataIterator()
{
  this->Information = vtkSmartPointer<vtkInformation>::New();
  this->AMR = NULL;
  this->AMRData = NULL;
  this->AMRInfo = NULL;
}

vtkUniformGridAMRDataIterator::~vtkUniformGridAMRDataIterator()
{
}


vtkDataObject* vtkUniformGridAMRDataIterator::GetCurrentDataObject()
{
  unsigned int level, id;
  this->GetCurrentIndexPair(level,id);
  vtkDataObject* obj = this->AMR->GetDataSet(level,id);
  return obj;
}


vtkInformation* vtkUniformGridAMRDataIterator::GetCurrentMetaData()
{
  double bounds[6];
  this->AMRInfo->GetBounds(this->GetCurrentLevel(), this->GetCurrentIndex(), bounds);
  this->Information->Set(vtkDataObject::BOUNDING_BOX(),bounds,6);
  return this->Information;
}


unsigned int vtkUniformGridAMRDataIterator::GetCurrentFlatIndex()
{
  return this->Iter->GetFlatIndex();
}

void vtkUniformGridAMRDataIterator::GetCurrentIndexPair(unsigned int& level, unsigned int& id)
{
  level = this->Iter->GetLevel();
  id = this->Iter->GetId();
}


unsigned int vtkUniformGridAMRDataIterator::GetCurrentLevel()
{
  unsigned int level, id;
  this->GetCurrentIndexPair(level,id);
  return level;
}


unsigned int vtkUniformGridAMRDataIterator::GetCurrentIndex()
{
  unsigned int level, id;
  this->GetCurrentIndexPair(level,id);
  return id;
}

void vtkUniformGridAMRDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkUniformGridAMRDataIterator::GoToFirstItem()
{
  if(!this->DataSet)
    {
    return;
    }
  this->AMR = vtkUniformGridAMR::SafeDownCast(this->DataSet);
  this->AMRInfo = this->AMR->GetAMRInfo();
  this->AMRData = this->AMR->GetAMRData();

  if(this->AMRInfo)
    {
    if(this->GetSkipEmptyNodes())
      {
      vtkSmartPointer<AMRLoadedDataIndexIterator> itr = vtkSmartPointer<AMRLoadedDataIndexIterator>::New();
      itr->Initialize(&this->AMRInfo->GetNumBlocks(), &this->AMR->GetAMRData()->GetAllBlocks());
      this->Iter = itr;
      }
    else
      {
      this->Iter = vtkSmartPointer<AMRIndexIterator>::New();
      this->Iter->Initialize(&this->AMRInfo->GetNumBlocks());
      }
    }
}


void vtkUniformGridAMRDataIterator::GoToNextItem()
{
  this->Iter->Next();
}

//----------------------------------------------------------------------------
int vtkUniformGridAMRDataIterator::IsDoneWithTraversal()
{
  return (!this->Iter) || this->Iter->IsDone();
}
