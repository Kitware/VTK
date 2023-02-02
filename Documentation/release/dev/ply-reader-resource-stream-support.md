## vtkPLYReader support for vtkResourceStream

vtkPLYReader now support reading from vtkResourceStream, old API (filename and string) is still supported for compatibility.

Notes:
 - `vtkPLY::get_ascii_item` signature changed from `void(const char*, int, int*, unsigned int*, double*)` to `void(vtkResourceParser*, int, int*, unsigned int*, double*)`
 - `vtkPLY::ply_read` signature changed from `PlyFile*(std::istream*, int*, char***)` to `PlyFile*(vtkResourceStream*, int*, char***)`
 - `vtkPLY::get_words` signature changed from `void(std::istream* is, std::vector<char*>* words, char line_words[], char orig_line[])` to `void(vtkResourceParser* is, std::vector<char*>* words, char line_words[], char orig_line[])`
