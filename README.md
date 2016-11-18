# SimpleColumnStoredDBv2
A simple column-store data processing: MVCC implementation
--------------------
How to build and run
--------------------
- Prepare to build:
  + gcc 4.8+
  + Install boost library:
    + Download boost_1_61_0.tar.bz2: http://www.boost.org/users/history/version_1_61_0.html
    + Extract (or copy) boost folder to /usr/local/: tar --bzip2 -xf /path/to/boost_1_61_0.tar.bz2
  + Install SQl parser library:
    + Download the latest release: https://github.com/hyrise/sql-parser/releases
    + Compile the library make to create libsqlparser.so
    + Run '$ make install' to copy the library to /usr/local/lib/
    + Add /usr/local/lib to $LD_LIBRARY_PATH
    + Makefile already has link to sql-parser source
- Build server: 
  + $ make clean
  + $ make
- Build client:
  + $ cd client
  + $ make
- Run (maybe use the execution file included):
  + Remember to install libsqlparser.so and added /usr/local/lib to $LD_LIBRARY_PATH first
  + Run the server:
    + $ ./serverApp
  + Run the client:
    + $ ./client

------------
Architecture
------------
- ColumnBase: base class for all columns.
  + Enums:
    + COLUMN_TYPE: column types: intType, charType, varcharType.
    + OP_TYPE: operator types: equalOp (=), neOp (<>), gtOp (>), geOp (>=), ltOp (<), leOp (<=), likeOp (like).
  + Fields:
    + string name: column name.
    + COLUMN_TYPE type: column type.
    + int size: column type size.
- Dictionary: class to store and process dictionary encoding data
  + typename<T>: can keep any type (currently: int and string) which can support comparative operator.
  + Functions:
    + lookup(index): return an value from dictionary at position index.
    + search(value, opType, result): search value from dictionary based on opType (<, >, =,...), return result into a reference.
    + addNewElement(value, vecValue): add value into dictionary. This value can be existed on dictionary. After adding, update vecValue
                                      which is the vector value of column.
    + size(): get number of elements on dictionary.
- Column: class representing for a Column of table
  + Members:
    + vecValue: a vector keeps values encoded by the dictionary of column.
    + encodedVecValue: encode values of vecValue to bit set to compress memory usage.
    Note: currently haven't applied bitpacking because of some errors when using an open-source names PackedArray 
  (https://github.com/gpakosz/PackedArray) to pack and unpack vecValue.
    + dictionay: the dictionary of column.
  + Functions:
    + getVecValue(): return the value vector of this column.
    + getDictionary(): return the dictionary vector of this column.
    + updateEncodedVecValue(): encode vecValue to bit set.
- Table: class representing for a Table.
  + Members:
    + m_columns: a vector keeps all columns of this table.
    + name: table name
  + Functions:
    + getColumnByName(tableName): return the pointer to the column which has name equals to tableName.

-----------
Processing
-----------
- Load data into memory:
  + Read data from local file.
  + With each line, split by delimeter, get tokens.
  + Each token is a column value -> call addNewElement of dictionary object to add value to column object.
- SQL parser:
  + Use an open-source SQL parser at: https://github.com/hyrise/sql-parser
  + Use this to extract: table name, select fields, where fields, where operations, where computation values,...
- SQL processing:
  + With each where fields, call function "getColumnByName" of table object to get corresponding column pointer.
  + To process where conditions, use function "search" of dictionary on column pointer, result is all satisfied encode
    dictionary value. After that, search on vecValue to find corresponding row id which has value equals to encode value
    on dictionary. The result is a vector of row id.
  + Currently, only support AND on where conditions, so we will need to join all vector of row id with each where condition
    to get common row id => final vector of row id.
  + With vector of row id, we will find all values for query result based on vecValue and dictionary of selected columns.

  
