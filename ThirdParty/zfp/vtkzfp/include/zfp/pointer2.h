// pointer to a 2D array element; this class is nested within zfp::array2
class pointer {
public:
  pointer() : ref(0, 0, 0) {}
  pointer operator=(const pointer& p) { ref.array = p.ref.array; ref.i = p.ref.i; ref.j = p.ref.j; return *this; }
  reference operator*() const { return ref; }
  reference operator[](ptrdiff_t d) const { return *operator+(d); }
  pointer& operator++() { increment(); return *this; }
  pointer& operator--() { decrement(); return *this; }
  pointer operator++(int) { pointer p = *this; increment(); return p; }
  pointer operator--(int) { pointer p = *this; decrement(); return p; }
  pointer operator+=(ptrdiff_t d) { set(index() + d); return *this; }
  pointer operator-=(ptrdiff_t d) { set(index() - d); return *this; }
  pointer operator+(ptrdiff_t d) const { pointer p = *this; p += d; return p; }
  pointer operator-(ptrdiff_t d) const { pointer p = *this; p -= d; return p; }
  ptrdiff_t operator-(const pointer& p) const { return index() - p.index(); }
  bool operator==(const pointer& p) const { return ref.array == p.ref.array && ref.i == p.ref.i && ref.j == p.ref.j; }
  bool operator!=(const pointer& p) const { return !operator==(p); }

protected:
  friend class array2;
  friend class reference;
  explicit pointer(reference r) : ref(r) {}
  explicit pointer(array2* array, uint i, uint j) : ref(array, i, j) {}
  ptrdiff_t index() const { return ref.i + ref.array->nx * ref.j; }
  void set(ptrdiff_t index) { ref.array->ij(ref.i, ref.j, index); }
  void increment()
  {
    if (++ref.i == ref.array->nx) {
      ref.i = 0;
      ref.j++;
    }
  }
  void decrement()
  {
    if (!ref.i--) {
      ref.i = ref.array->nx - 1;
      ref.j--;
    }
  }
  reference ref;
};
