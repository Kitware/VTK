//
// Classes that need to keep track of mdofication / execution time use
// this class.
//
#ifndef __vlTimeStamp_h
#define __vlTimeStamp_h

class vlTimeStamp {
public:
  vlTimeStamp() {this->ModifiedTime = ++vlTime;};
  void Modified() {this->ModifiedTime= ++vlTime;};
  int operator>(vlTimeStamp& ts) 
    {return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vlTimeStamp& ts) 
    {return (this->ModifiedTime < ts.ModifiedTime);};

private:
  unsigned long ModifiedTime;
  static unsigned long vlTime;
};

#endif
