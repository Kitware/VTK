This example demonstrates reading data from an SQL database
(as tables for each query response), computing statistics on
the tables (which in turn produce tables of statistical model
data), and displaying the resulting tables using Qt.

To generate some example data that the `StatsView` application
can read, run `sqlite3 example.db` and enter the following at
the prompt:

```sql
CREATE TABLE main_tbl (
  id INTEGER PRIMARY KEY NOT NULL,
  Temp1 REAL NOT NULL,
  Temp2 REAL NOT NULL
);

INSERT INTO main_tbl
    WITH RECURSIVE
      cnt( id, Temp1, Temp2 ) AS (
      VALUES(1 , random()/1e16,  random()/1e16) UNION ALL
      SELECT id+1,random()/1e16, random()/1e16 FROM cnt WHERE ID<1000)
    select * from cnt;

.exit
```

This will create a file named `example.db` that you can load
into `StatsView`.
