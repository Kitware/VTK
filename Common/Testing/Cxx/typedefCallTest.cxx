#include "vtkObject.h"

class A
{
public:
  virtual int Height() { return 1; }
};

class B: public A
{
public:
  typedef A Superclass;
  
  virtual int Height() { return this->Superclass::Height()+1; }
};

class C: public B
{
public:
  typedef B Superclass;
  
  virtual int Height() { return this->Superclass::Height()+1; }
};

int main()
{
  A* a = new A;
  A* b = new B;
  A* c = new C;
  
  if(c->Height() != 3) { return 3; }
  if(b->Height() != 2) { return 2; }
  if(a->Height() != 1) { return 1; }
  
  delete a;
  delete b;
  delete c;
  
  return 0;
}
