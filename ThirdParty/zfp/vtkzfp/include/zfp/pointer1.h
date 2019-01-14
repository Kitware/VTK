// pointer to a 1D array element; this class is nested within zfp::array1
class pointer {
public:
  pointer() : ref(0, 0) {}
  pointer operator=(const pointer& p) { ref.array = p.ref.array; ref.i = p.ref.i; return *this; }
  reference operator*() const { return ref; }
  reference operator[](ptrdiff_t d) const { return *operator+(d); }
  pointer& operator++() { increment(); return *this; }
  pointer& operator--() { decrement(); return *this; }
  pointer operator++(int) { pointer p = *this; increment(); return p; }
  pointer operator--(int) { pointer p = *this; decrement(); return p; }
  pointer operator+=(ptrdiff_t d) { ref.i += d; return *this; }
  pointer operator-=(ptrdiff_t d) { ref.i -= d; return *this; }
  pointer operator+(ptrdiff_t d) const { pointer p = *this; p += d; return p; }
  pointer operator-(ptrdiff_t d) const { pointer p = *this; p -= d; return p; }
  ptrdiff_t operator-(const pointer& p) const { return index() - p.index(); }
  bool operator==(const pointer& p) const { return ref.array == p.ref.array && ref.i == p.ref.i; }
  bool operator!=(const pointer& p) const { return !operator==(p); }

protected:
  friend class array1;
  friend class reference;
  explicit pointer(reference r) : ref(r) {}
  explicit pointer(array1* array, uint i) : ref(array, i) {}
  ptrdiff_t index() const { return ref.i; }
  void set(ptrdiff_t index) { ref.i = index; }
  void increment() { ref.i++; }
  void decrement() { ref.i--; }
  reference ref;
};
