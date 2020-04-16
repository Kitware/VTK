COMP: Anonymous non-C-compatible types named

From https://reviews.llvm.org/D74103:

  Due to a recent (but retroactive) C++ rule change, only sufficiently
  C-compatible classes are permitted to be given a typedef name for
  linkage purposes. Add an enabled-by-default warning for these cases, and
  rephrase our existing error for the case where we encounter the typedef
  name for linkage after we've already computed and used a wrong linkage
  in terms of the new rule.

This commit fixes warnings like the following:
  warning: anonymous non-C-compatible type given name for linkage purposes by typedef declaration; add a tag name here [-Wnon-c-typedef-for-linkage]
    typedef struct
                  ^
                   ColumnInfo
  note: type is not C-compatible due to this default member initializer
      int ScalarType = VTK_STRING;
      ^~~~~~~~~~~~~~
  note: type is given name 'ColumnInfo' for linkage purposes by this typedef declaration
    } ColumnInfo;
      ^

For consistency, Use 'using' to a named structure definintion for all structures.
