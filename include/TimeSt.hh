//
// Classes that need to keep track of their modified time use this
// class as a superclass.
//
#ifndef TimeStamp_h
#define TimeStamp_h

class TimeStamp {
public:
  void modified() {mtime= ++time;};
  virtual unsigned long getMtime() {return mtime;};
  TimeStamp() : mtime(0) {};
  int operator>(TimeStamp& ts) {return (mtime > ts.mtime);};
  int operator<(TimeStamp& ts) {return (mtime < ts.mtime);};
protected:
  unsigned long mtime;
private:
  static unsigned long time;
};

#endif
