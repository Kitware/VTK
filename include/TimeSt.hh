//
// Classes that need to keep track of modification / execution time use
// this class.
//
#ifndef __vlTimeStamp_h
#define __vlTimeStamp_h

class vlTimeStamp 
{
public:
  vlTimeStamp() {this->ModifiedTime = ++vlTime;};
  void Modified() {this->ModifiedTime = ++vlTime;};
  unsigned long int GetMtime() {return ModifiedTime;};
  int operator>(vlTimeStamp& ts) 
    {return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vlTimeStamp& ts) 
    {return (this->ModifiedTime < ts.ModifiedTime);};
  operator unsigned long int() {return this->ModifiedTime;};
private:
  unsigned long ModifiedTime;
  static unsigned long vlTime;
};

#endif
