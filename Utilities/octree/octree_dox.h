/*!
\mainpage N-Dimensional Binary Trees

This documentation describes how to use the n-dimensional binary tree C++ classes.
The classes are all templated and behave similarly to the STL.
Even though the class names all contain octree, they provide a generalization to any integer dimension.

The octree class stores the root node of the tree plus information about the geometric embedding of the tree (if any).
The octree_node class represents nodes in the tree and stores a pointer to the parent and child nodes plus a templated
class instance of your choosing.
Finally, octree_cursor and octree_iterator are subclasses of octree_path and provide a way to traverse
the tree in either free-form or a depth-first fashion, respectively.

*/
