/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtSQLDatabase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// Tests vtkQtSQLDatabase.

// Check for Qt SQL module before defining this test.
#include <qglobal.h>
#if (QT_EDITION & QT_MODULE_SQL)
#include "vtkQtSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <QApplication>
#include <QFile>
#include <QInputDialog>
#include <QStringList>
#include <QTableView>

int TestQtSQLDatabase(int argc, char* argv[])
{
  QApplication app(argc, argv);
  //QCoreApplication app(argc, argv);
  //for (int i = 0; i < QCoreApplication::libraryPaths().count(); i++)
  //  {
  //  cerr << QCoreApplication::libraryPaths().at(i).toLatin1().data() << endl;
  //  }

  bool interactive = false;

  // QMYSQL parameters:
  //QString dbtype = "QMYSQL";
  //QString database = "test";
  //QString user = "root";
  //bool askpass = true;
  //QString host = "localhost";
  //int port = 3306;

  // QSQLITE parameters:
  QString dbtype("QSQLITE");
  QString database(":memory:");
  QString user;
  bool askpass = false;
  QString host;
  int port = -1;
  QString queryText("SELECT name, age, weight FROM people WHERE age <= 20");

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
      continue;
    }
    if (!strcmp(argv[i], "-t"))
    {
      i++;
      dbtype = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-d"))
    {
      i++;
      database = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-u"))
    {
      i++;
      user = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-w"))
    {
      askpass = true;
      continue;
    }
    if (!strcmp(argv[i], "-h"))
    {
      i++;
      host = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-p"))
    {
      i++;
      port = atoi(argv[i]);
      continue;
    }
    if (!strcmp(argv[i], "-q"))
    {
      i++;
      queryText = argv[i];
      continue;
    }

    cerr << argv[0] << " Options:\n"
      << " -I (interactive, shows Qt table with query result)\n"
      << " -t database type (QSQLITE, QMYSQL, etc.; default: QSQLITE)\n"
      << " -h host (default: :memory:)\n"
      << " -p port (default: empty)\n"
      << " -d database (default: test)\n"
      << " -u username (default: empty)\n"
      << " -w (password required; default: no password required)\n"
      << " -q (query; default: select * from people ...)\n";
    return 0;
  }

  QString password;
  if (askpass)
  {
    password = QInputDialog::getText(NULL, "Enter password", "Password", QLineEdit::Password);
  }

  vtkQtSQLDatabase* db = vtkQtSQLDatabase::New();
  db->SetDatabaseType(dbtype.toLatin1().data());
  db->SetDatabaseName(database.toLatin1().data());
  db->SetUserName(user.toLatin1().data());
  db->SetPort(port);
  if (!db->Open(password.toLatin1().data()))
  {
    cerr << "Unable to open database" << endl;
    return 1;
  }
  vtkSQLQuery* query = db->GetQueryInstance();

  bool dataExists = false;
  query->SetQuery("SHOW TABLES");
  query->Execute();
  if (query->NextRow())
  {
    dataExists = true; // there is a table
  }

  if (!dataExists)
  {
    QString createQuery("CREATE TABLE IF NOT EXISTS people (name TEXT, age INTEGER, weight FLOAT)");
    cout << createQuery.toLatin1().data() << endl;
    query->SetQuery(createQuery.toLatin1().data());
    if (!query->Execute())
    {
      cerr << "Create query failed" << endl;
      return 1;
    }

    for (int i = 0; i < 40; i++)
    {
      QString insertQuery = QString("INSERT INTO people VALUES('John Doe %1', %1, %2)").arg(i).arg(10*i);
      cout << insertQuery.toLatin1().data() << endl;
      query->SetQuery(insertQuery.toLatin1().data());
      if (!query->Execute())
      {
        cerr << "Insert query failed" << endl;
        return 1;
      }
    }
  }

  query->SetQuery(queryText.toLatin1().data());
  cerr << endl << "Running query: " << query->GetQuery() << endl;

  cerr << endl << "Using vtkSQLQuery directly to execute query:" << endl;
  if (!query->Execute())
  {
    cerr << "Query failed" << endl;
    return 1;
  }
  for (int col = 0; col < query->GetNumberOfFields(); col++)
  {
    if (col > 0)
    {
      cerr << ", ";
    }
    cerr << query->GetFieldName(col);
  }
  cerr << endl;
  while (query->NextRow())
  {
    for (int field = 0; field < query->GetNumberOfFields(); field++)
    {
      if (field > 0)
      {
        cerr << ", ";
      }
      cerr << query->DataValue(field).ToString().c_str();
    }
    cerr << endl;
  }

  cerr << endl << "Using vtkSQLQuery to execute query and retrieve by row:" << endl;
  if (!query->Execute())
  {
    cerr << "Query failed" << endl;
    return 1;
  }
  for (int col = 0; col < query->GetNumberOfFields(); col++)
  {
    if (col > 0)
    {
      cerr << ", ";
    }
    cerr << query->GetFieldName(col);
  }
  cerr << endl;
  vtkVariantArray* va = vtkVariantArray::New();
  while (query->NextRow(va))
  {
    for (int field = 0; field < va->GetNumberOfValues(); field++)
    {
      if (field > 0)
      {
        cerr << ", ";
      }
      cerr << va->GetValue(field).ToString().c_str();
    }
    cerr << endl;
  }
  va->Delete();

  cerr << endl << "Using vtkRowQueryToTable to execute query:" << endl;
  vtkRowQueryToTable* reader = vtkRowQueryToTable::New();
  reader->SetQuery(query);
  reader->Update();
  vtkTable* table = reader->GetOutput();
  for (vtkIdType col = 0; col < table->GetNumberOfColumns(); col++)
  {
    table->GetColumn(col)->Print(cerr);
  }
  cerr << endl;
  for (vtkIdType row = 0; row < table->GetNumberOfRows(); row++)
  {
    for (vtkIdType col = 0; col < table->GetNumberOfColumns(); col++)
    {
      vtkVariant v = table->GetValue(row, col);
      cerr << "row " << row << ", col " << col << " - "
        << v.ToString() << " (" << vtkImageScalarTypeNameMacro(v.GetType()) << ")" << endl;
    }
  }

  // Put the table in a view ... just for fun
  if (interactive)
  {
    vtkQtTableModelAdapter* model = new vtkQtTableModelAdapter(table);
    QTableView* view = new QTableView();
    view->setModel(model);
    view->show();

    app.exec();

    delete view;
    delete model;
  }

  reader->Delete();
  query->Delete();
  db->Delete();
  return 0;
}
#else
#include "vtkObject.h" // for cerr.
int TestQtSQLDatabase(int, char*[])
{
  cerr << "QT_MODULE_SQL not enabled in this edition, so nothing to test." << endl;
  return 0;
}
#endif // (QT_EDITION & QT_MODULE_SQL)
