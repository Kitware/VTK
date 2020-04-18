# node-numbering-change-for-VTK_LAGRANGE_HEXAHEDRON

The node numbering for VTK_LAGRANGE_HEXAHEDRON has been corrected,
to match the numbering of VTK_QUADRATIC_HEXAHEDRON when the Lagrange
cell is quadratic.
The change consists in inverting the node numbering on the edge (x=0, y=1)
with the edge (x=1, y=1), has shown below for a quadratic cell.


       quadratic                         VTK_QUADRATIC_HEXAHEDRON
VTK_LAGRANGE_HEXAHEDRON                  VTK_LAGRANGE_HEXAHEDRON
    before VTK 9.0                           after VTK 9.0

       +_____+_____+                        +_____+_____+
       |\          :\                       |\          :\
       | +         : +                      | +         : +
       |  \     19 +  \                     |  \     18 +  \
    18 +   +-----+-----+                 19 +   +-----+-----+
       |   |       :   |                    |   |       :   |
       |__ | _+____:   |                    |__ | _+____:   |
       \   +        \  +                    \   +        \  +
        +  |         + |                     +  |         + |
         \ |          \|                      \ |          \|
           +_____+_____+                        +_____+_____+

In order to maintain the compatibility, the readers can convert internally
existing files to the new numbering. To this end, The XML file version has been
bumped from 2.1 to 2.2 and the legacy file version has been bumped from 5.0 to 5.1.
