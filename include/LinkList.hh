//
// Define link array
//
#ifndef __vlLinkList_h
#define __vlLinkList_h

struct vlLink {
    unsigned short ncells;
    int *cells;
};

class vlLinkList {
public:
  vlLinkList():Array(0),Size(0),MaxId(-1),Extend(1000) {};
  vlLinkList(const int sz, const int ext);
  ~vlLinkList();
  char *GetClassName() {return "vlLinkList";};
  vlLink &GetLink(const int id) {return this->Array[id];};
  unsigned short GetNcells(const int id) {return this->Array[id].ncells;};
  int *GetCells(const int id) {return this->Array[id].cells;};
  void InsertLink(const int id, const unsigned short ncells, int *cells);
  int InsertNextLink(const unsigned short ncells, int *cells);
  void Squeeze();
  void Reset();

private:
  vlLink *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  vlLink *Resize(const int sz);  // function to resize data
};

#endif
