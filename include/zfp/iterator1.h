// random access iterator that visits 1D array block by block; this class is nested within zfp::array1
class iterator {
public:
  // typedefs for STL compatibility
  typedef Scalar value_type;
  typedef ptrdiff_t difference_type;
  typedef typename array1::reference reference;
  typedef typename array1::pointer pointer;
  typedef std::random_access_iterator_tag iterator_category;

  iterator() : ref(0, 0) {}
  iterator operator=(const iterator& it) { ref.array = it.ref.array; ref.i = it.ref.i; return *this; }
  reference operator*() const { return ref; }
  reference operator[](difference_type d) const { return *operator+(d); }
  iterator& operator++() { increment(); return *this; }
  iterator& operator--() { decrement(); return *this; }
  iterator operator++(int) { iterator it = *this; increment(); return it; }
  iterator operator--(int) { iterator it = *this; decrement(); return it; }
  iterator operator+=(difference_type d) { ref.i += d; return *this; }
  iterator operator-=(difference_type d) { ref.i -= d; return *this; }
  iterator operator+(difference_type d) const { return iterator(ref.array, ref.i + d); }
  iterator operator-(difference_type d) const { return iterator(ref.array, ref.i - d); }
  difference_type operator-(const iterator& it) const { return static_cast<difference_type>(ref.i) - static_cast<difference_type>(it.ref.i); }
  bool operator==(const iterator& it) const { return ref.array == it.ref.array && ref.i == it.ref.i; }
  bool operator!=(const iterator& it) const { return !operator==(it); }
  bool operator<=(const iterator& it) const { return ref.array == it.ref.array && ref.i <= it.ref.i; }
  bool operator>=(const iterator& it) const { return ref.array == it.ref.array && ref.i >= it.ref.i; }
  bool operator<(const iterator& it) const { return !operator>=(it); }
  bool operator>(const iterator& it) const { return !operator<=(it); }
  uint i() const { return ref.i; }

protected:
  friend class array1;
  explicit iterator(array1* array, uint i) : ref(array, i) {}
  void increment() { ref.i++; }
  void decrement() { ref.i--; }
  reference ref;
};
