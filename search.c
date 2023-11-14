/*
 * Created by roberto on 3/5/21.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include <assert.h>
#include "search.h"
#include "odbc.h"

void results_search(char *from, char *to, char *departure_date,
                    int *n_choices, char ***choices,
                    int max_length, int max_rows)
{
  SQLHENV env;
  SQLHDBC dbc;
  SQLRETURN ret;
  SQLHSTMT stmt;
  SQLINTEGER flight_id;
  FILE *f;
  SQLCHAR scheduled_dep[128];
  SQLCHAR sqlstate[6];
  SQLINTEGER native_error;
  SQLCHAR message_text[256];
  SQLSMALLINT text_length;
  int i, j;
  char query_result[1024][1024];
  char query[256];

  /* Connect to the database*/
  ret = odbc_connect(&env, &dbc);
  if (!SQL_SUCCEEDED(ret))
  {
    fprintf(stderr, "Database connection failed.\n");
    return;
  }

  /* Allocate a statement handle*/
  SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

  f = fopen("p.log", "w");

  /* Construct the SQL query*/
  sprintf(query,
          "GRANT SELECT ON TABLE flights TO IgnacioSerena; SELECT * FROM flights");

  /* Execute the SQL query*/
  ret = SQLExecDirect(stmt, (SQLCHAR *)query, SQL_NTS);
  i = 1;
  while (SQL_SUCCEEDED(ret = SQLGetDiagRec(SQL_HANDLE_STMT, stmt, i, sqlstate, &native_error, message_text, sizeof(message_text), &text_length)))
  {
    fprintf(stderr, "SQL Error: %s - %s\n", sqlstate, message_text);
    i++;
  }

  if (!SQL_SUCCEEDED(ret))
  {
    fprintf(stderr, "Error executing SQL query.\n");
    return;
  }

  SQLBindCol(stmt, 1, SQL_INTEGER, &flight_id, sizeof(SQLINTEGER), NULL);
  SQLBindCol(stmt, 2, SQL_C_CHAR, scheduled_dep, sizeof(scheduled_dep), NULL);

  j = 0;
  while (SQL_SUCCEEDED(ret = SQLFetch(stmt)))
  {
    fprintf(f, "flight with ip number %d has its scheduled departure at %s", flight_id, scheduled_dep);
    sprintf(query_result[j], "flight with ip number %d has its scheduled departure at %s", flight_id, scheduled_dep);
    j++;
  }

  if (ret != SQL_NO_DATA)
  {
    fprintf(stderr, "Error fetching data or no data found.\n");
  }

  *n_choices = j;
  max_rows = MIN(*n_choices, max_rows);

  for (i = 0, j = 0; i < max_rows; i++)
  {
    j = (int)strlen(query_result[i]) + 1;
    j = MIN(j, max_length);
    strncpy((*choices)[i], query_result[i], j);
  }

  fclose(f);

  /* Free up memory and disconnect from the database*/
  SQLFreeHandle(SQL_HANDLE_STMT, stmt);
  odbc_disconnect(env, dbc);
  if (!SQL_SUCCEEDED(ret))
  {
    return;
  }

  return;
}

/*SKX SVO*/