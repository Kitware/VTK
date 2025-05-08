//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Token.h>

#include <list>

using LockType = std::unique_lock<std::mutex>;

class viskores::cont::Token::InternalStruct
{
  std::mutex Mutex;
  std::list<viskores::cont::Token::HeldReference> HeldReferences;

  VISKORES_CONT void CheckLock(const LockType& lock) const
  {
    VISKORES_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
  }

public:
  LockType GetLock() { return LockType(this->Mutex); }
  std::list<viskores::cont::Token::HeldReference>* GetHeldReferences(const LockType& lock)
  {
    this->CheckLock(lock);
    return &this->HeldReferences;
  }
};

struct viskores::cont::Token::HeldReference
{
  std::unique_ptr<viskores::cont::Token::ObjectReference> ObjectReference;
  viskores::cont::Token::ReferenceCount* ReferenceCountPointer;
  std::mutex* MutexPointer;
  std::condition_variable* ConditionVariablePointer;

  HeldReference(std::unique_ptr<viskores::cont::Token::ObjectReference>&& objRef,
                viskores::cont::Token::ReferenceCount* refCountP,
                std::mutex* mutexP,
                std::condition_variable* conditionVariableP)
    : ObjectReference(std::move(objRef))
    , ReferenceCountPointer(refCountP)
    , MutexPointer(mutexP)
    , ConditionVariablePointer(conditionVariableP)
  {
  }
};

viskores::cont::Token::Token() {}

viskores::cont::Token::Token(Token&& rhs)
  : Internals(std::move(rhs.Internals))
{
}

viskores::cont::Token::~Token()
{
  this->DetachFromAll();
}

void viskores::cont::Token::DetachFromAll()
{
  if (!this->Internals)
  {
    // If internals is NULL, then we are not attached to anything.
    return;
  }
  LockType localLock = this->Internals->GetLock();
  auto heldReferences = this->Internals->GetHeldReferences(localLock);
  for (auto&& held : *heldReferences)
  {
    LockType objectLock(*held.MutexPointer);
    *held.ReferenceCountPointer -= 1;
    objectLock.unlock();
    held.ConditionVariablePointer->notify_all();
  }
  heldReferences->clear();
}

viskores::cont::Token::Reference viskores::cont::Token::GetReference() const
{
  if (!this->Internals)
  {
    this->Internals.reset(new InternalStruct);
  }

  return this->Internals.get();
}

void viskores::cont::Token::Attach(
  std::unique_ptr<viskores::cont::Token::ObjectReference>&& objectRef,
  viskores::cont::Token::ReferenceCount* referenceCountPointer,
  std::unique_lock<std::mutex>& lock,
  std::condition_variable* conditionVariablePointer)
{
  if (!this->Internals)
  {
    this->Internals.reset(new InternalStruct);
  }
  LockType localLock = this->Internals->GetLock();
  if (this->IsAttached(localLock, referenceCountPointer))
  {
    // Already attached.
    return;
  }
  if (!lock.owns_lock())
  {
    lock.lock();
  }
  *referenceCountPointer += 1;
  this->Internals->GetHeldReferences(localLock)->emplace_back(
    std::move(objectRef), referenceCountPointer, lock.mutex(), conditionVariablePointer);
}

inline bool viskores::cont::Token::IsAttached(
  LockType& lock,
  viskores::cont::Token::ReferenceCount* referenceCountPointer) const
{
  if (!this->Internals)
  {
    return false;
  }
  for (auto&& heldReference : *this->Internals->GetHeldReferences(lock))
  {
    if (referenceCountPointer == heldReference.ReferenceCountPointer)
    {
      return true;
    }
  }
  return false;
}

bool viskores::cont::Token::IsAttached(
  viskores::cont::Token::ReferenceCount* referenceCountPointer) const
{
  if (!this->Internals)
  {
    return false;
  }
  LockType lock = this->Internals->GetLock();
  return this->IsAttached(lock, referenceCountPointer);
}
