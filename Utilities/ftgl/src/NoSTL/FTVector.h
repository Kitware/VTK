/*
  Provides a non-STL alternative to the STL vector<...>
  used inside FTvectorizer and FTGlyphContainer.
  
  Implementation:
    - Dynamically resizable container.
    - Try to mimic the calls made to the STL vector API.

  Caveats:
    - No templates, use poor macro substition where :
      FT_VECTOR_CLASS_NAME: is the name of the class
      FT_VECTOR_ITEM_TYPE: is the type of the object to store
*/

#define FT_VECTOR_CLASS_DEBUG 0

#include "FTGL.h"

class FTGL_EXPORT FT_VECTOR_CLASS_NAME
{
public:
  
  typedef FT_VECTOR_ITEM_TYPE value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef size_t size_type;

  FT_VECTOR_CLASS_NAME();

  virtual ~FT_VECTOR_CLASS_NAME();

  FT_VECTOR_CLASS_NAME& operator =(const FT_VECTOR_CLASS_NAME& v);
  
  size_type size() const;
  size_type capacity() const;
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  bool empty() const;

  reference operator [](size_type pos);
  const_reference operator [](size_type pos) const;

  void clear();
  void reserve(size_type);
  void push_back(const value_type&);
  void resize(size_type, value_type);

protected:
  
  void expand(size_type = 0);

private:

  size_type Capacity;
  size_type Size;
  value_type* Items;
};


inline 
FT_VECTOR_CLASS_NAME::FT_VECTOR_CLASS_NAME()
{
  this->Capacity = this->Size = 0;
  this->Items = 0;
}


inline 
FT_VECTOR_CLASS_NAME::size_type FT_VECTOR_CLASS_NAME::size() const 
{ 
  return this->Size; 
}


inline 
FT_VECTOR_CLASS_NAME::size_type FT_VECTOR_CLASS_NAME::capacity() const 
{ 
  return this->Capacity; 
}


inline 
FT_VECTOR_CLASS_NAME::iterator FT_VECTOR_CLASS_NAME::begin() 
{ 
  return this->Items; 
}


inline 
FT_VECTOR_CLASS_NAME::const_iterator FT_VECTOR_CLASS_NAME::begin() const 
{ 
  return this->Items; 
}


inline 
FT_VECTOR_CLASS_NAME::iterator FT_VECTOR_CLASS_NAME::end() 
{ 
  return this->begin() + this->size(); 
}


inline 
FT_VECTOR_CLASS_NAME::const_iterator FT_VECTOR_CLASS_NAME::end() const 
{ 
  return this->begin() + this->size(); 
}


inline 
void FT_VECTOR_CLASS_NAME::clear()
{
  if (this->Capacity)
    {
#if FT_VECTOR_CLASS_DEBUG
    printf("FT_VECTOR_CLASS_NAME:     clear() (%d / %d)\n", 
           this->size(), this->capacity());
#endif
    delete [] this->Items;
    this->Capacity = this->Size = 0;
    this->Items = 0;
    }
}


inline 
FT_VECTOR_CLASS_NAME::~FT_VECTOR_CLASS_NAME()
{
  this->clear();
}


inline 
void FT_VECTOR_CLASS_NAME::expand(size_type capacity_hint)
{
#if FT_VECTOR_CLASS_DEBUG    
  printf("FT_VECTOR_CLASS_NAME:    expand() (%d / %d) hint: %d\n", 
         this->size(), this->capacity(), capacity_hint);
#endif

  // Allocate new vector (capacity doubles)

  size_type new_capacity = (this->capacity() == 0) ? 256 : this->capacity()* 2;
  if (capacity_hint)
    {
    while (new_capacity < capacity_hint)
      {
      new_capacity *= 2;
      }
    }
  
  value_type *new_items = new value_type[new_capacity];

  // Copy values to new vector

  iterator ibegin = this->begin();
  iterator iend = this->end();
  value_type *ptr = new_items;
  while (ibegin != iend)
    {
    *ptr++ = *ibegin++;
    }

  // Deallocate old vector and use new vector

  if (this->Capacity)
    {
    delete [] this->Items;
    }
  this->Items = new_items;
  this->Capacity = new_capacity;
}


inline 
void FT_VECTOR_CLASS_NAME::reserve(size_type n)
{
#if FT_VECTOR_CLASS_DEBUG    
  printf("FT_VECTOR_CLASS_NAME:   reserve() (%d / %d) n: %d\n", 
         this->size(), this->capacity(), n);
#endif
  if (this->capacity() < n)
    {
    this->expand(n);
    }
}


inline 
FT_VECTOR_CLASS_NAME& FT_VECTOR_CLASS_NAME::operator =(const FT_VECTOR_CLASS_NAME& v)
{
  // Warning: the vector is not cleared and resized to v capacity for
  // efficiency reasons.
  // this->clear();
  this->reserve(v.capacity());
  
  iterator ptr = this->begin();
  const_iterator vbegin = v.begin();
  const_iterator vend = v.end();

  while (vbegin != vend)
    {
    *ptr++ = *vbegin++;
    }
  this->Size = v.size();
  return *this;
}


inline 
bool FT_VECTOR_CLASS_NAME::empty() const 
{ 
  return this->size() == 0; 
}


inline 
FT_VECTOR_CLASS_NAME::reference FT_VECTOR_CLASS_NAME::operator [](FT_VECTOR_CLASS_NAME::size_type pos) 
{ 
#if FT_VECTOR_CLASS_DEBUG    
  printf("FT_VECTOR_CLASS_NAME:        []() (%d / %d) pos: %d\n", 
         this->size(), this->capacity(), pos);
#endif
  return (*(begin() + pos)); 
}


inline 
FT_VECTOR_CLASS_NAME::const_reference FT_VECTOR_CLASS_NAME::operator [](FT_VECTOR_CLASS_NAME::size_type pos) const 
{ 
#if FT_VECTOR_CLASS_DEBUG    
  printf("FT_VECTOR_CLASS_NAME:        []() (%d / %d) pos: %d\n", 
         this->size(), this->capacity(), pos);
#endif
  return (*(begin() + pos)); 
}


inline 
void FT_VECTOR_CLASS_NAME::push_back(const value_type& x)
{
#if FT_VECTOR_CLASS_DEBUG    
  printf("FT_VECTOR_CLASS_NAME: push_back() (%d / %d)\n", 
         this->size(), this->capacity());
#endif
  if (this->size() == this->capacity())
    {
    this->expand();
    }
  (*this)[this->size()] = x;
  this->Size++;
}


inline 
void FT_VECTOR_CLASS_NAME::resize(size_type n, value_type x)
{
#if FT_VECTOR_CLASS_DEBUG    
  printf("FT_VECTOR_CLASS_NAME:    resize() (%d / %d) n: %d\n", 
         this->size(), this->capacity(), n);
#endif
  if (n == this->size())
    {
    return;
    }
  this->reserve(n);
  iterator ibegin, iend;
  if (n >= this->Size)
    {
    ibegin = this->end();
    iend = this->begin() + n;
    }
  else
    {
    ibegin = this->begin() + n;
    iend = this->end();
    }
  while (ibegin != iend)
    {
    *ibegin++ = x;
    }
  this->Size = n;
}
