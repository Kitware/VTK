Objects can now be assigned an object name which appears in error and warning
messages and in Print output. Changing the object name does not change the
object's MTime and does not invoke a ModifiedEvent. The object name is not
copied by ShallowCopy and DeepCopy implementations.
