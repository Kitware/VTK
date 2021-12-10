## Create Join Tables Filter

vtkJoinTables is a filter that operates on two vtkTable objects to perform an SQL-style Join operation. It outputs one vtkTable object. The goal is to combine the rows of both tables into one bigger table based on a related "key column" between them. The two input tables are referred to as `left` and `right`. In each input table, the values in the key column act like unique IDs for their respective  rows. During the merge, the attributes of each item will be given with respect to its ID. Below is an example of a basic join operation :

First input Table (`left`)
| Name | Age |Sex |
|------|-----|----|
| Alex | 25  | M  |
| Beth | 41  | F  |
| Cory | 33  | M  |

Second input Table (`right`)
| Name | Height(cm) |Weight (kg) |
|------|-----|----|
| Cory | 184  | 88  |
| Alex | 175  | 74  |
| Beth | 168  | 70  |

Result of a Join operation performed with `LeftKey = "Name"`and `RightKey = "Name"` :
| Name | Age |Sex | Height(cm) |Weight (kg) |
|------|-----|----|-----|----|
| Alex | 25  | M  | 175  | 74  |
| Beth | 41  | F  |  168  | 70  |
| Cory | 33  | M  |184  | 88  |


This Filter exposes the following parameters :
-  Join Mode {`intersection`, `union`, `left`, `right`} : specifies which Join method to perform. This is useful when the key columns do not share exactly the same values. `left` keeps the keys of the left input table, `right` keeps the keys of the right input table, `intersection` keeps only the keys that are present in both tables, `union` keeps the keys from both tables.
-  Left Key (string) : Column name of the left table to treat as key
-  Right Key (string) : Column name of the right table to treat as key
-  Replacement Value (double) : Arbitrary value to be imputed in the output for unknown numerical data
