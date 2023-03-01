## Support for 12-node wedges in IOSS

The IOSS reader now supports mixed-order, 12-node wedge elements.
These elements have quadratic triangle faces but quadrilaterals
with 2 linear sides and are sometimes used to represent material
failure along a shared boundary between two tetrahedra (i.e.,
the wedge is assigned a zero or negligible stiffness, allowing
the neighboring tetrahedra to move relative to one another without
inducing strain.
