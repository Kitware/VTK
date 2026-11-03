## Intersection Polydata edge case

When two points of one triangle lies on the plane of the other triangle, an intersection is created if those two points belong to triangle 0. If they belong to triangle 1, the intersection is created only if the third point is on the other triangle normal side.

Please note this means the filter is **not asymetric anymore**, meaning that switching input 0 and 1 changes the output if the edge case describe above is in the dataset.

## Example

C is the triangle third point. AB is the other triangle plane.

```ascii
Intersection Created       Intersection Created
     Triangle 0                 Triangle 1
         C                          C
        / \                        / \
       /   \    n                 /   \    n
      /     \   Λ                /     \   Λ
     /       \  |               /       \  |
A---X=========X---B        A---X=========X---B


Intersection Created     Intersection not Created
                n                          n
                Λ                          Λ
                |                          |
A---X=========X---B        A---X---------X---B
     \       /                  \       /
      \     /                    \     /
       \   /                      \   /
        \ /                        \ /
         C                          C
     Triangle 0                 Triangle 1
```
