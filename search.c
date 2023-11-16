/*
 * Created by roberto on 3/5/21.
 */
#include "search.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"

void results_search(char *from, char *to, char *departure_date,
                    int *n_choices, char ***choices,
                    int max_length,
                    int max_rows)
/**here you need to do your query and fill the choices array of strings
 *
 * @param from form field from
 * @param to form field to
 * @param n_choices fill this with the number of results
 * @param choices fill this with the actual results
 * @param max_length output win maximum width
 * @param max_rows output win maximum number of rows
 */
{
  SQLHENV env;
  SQLHDBC dbc;
  SQLHSTMT stmt;
  SQLRETURN ret; /* ODBC API return status */
  SQLCHAR sqlstate[6];
  SQLINTEGER native_error;
  SQLCHAR message_text[256];
  SQLSMALLINT text_length;
  SQLINTEGER flight_id;
  SQLCHAR flight_type[64];
  SQLCHAR scheduled_departure[64];
  SQLCHAR scheduled_arrival[64];
  SQLCHAR aircraft_code[64];
  SQLINTEGER num_empty_seats;
  FILE *f;

  char query[1024];
  char **query_result_set;
  int i = 0;
  int t = 0;

  ret = odbc_connect(&env, &dbc);
  if (!SQL_SUCCEEDED(ret))
  {
    fprintf(stderr, "Error en la conexión mediante odbc");
    return;
  }

  /* Allocate a statement handle */
  SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

  query_result_set = (char **)malloc(max_rows * sizeof(char *));
  for (i = 0; i < max_rows; ++i)
    query_result_set[i] = (char *)malloc(max_length * sizeof(char));

  sprintf(query, "WITH empty_seats AS ( \
  SELECT \
    f.flight_id, \
    f.aircraft_code, \
    COUNT(s.seat_no) AS num_empty_seats \
  FROM \
    flights AS f \
    LEFT JOIN seats AS s ON f.aircraft_code = s.aircraft_code \
    LEFT JOIN boarding_passes AS b ON s.seat_no = b.seat_no AND b.flight_id = f.flight_id \
  WHERE \
    b.seat_no IS NULL \
  GROUP BY \
    f.flight_id, \
    f.aircraft_code \
), \
connecting_flights AS ( \
  SELECT \
    f1.flight_id AS first_flight_id, \
    f1.scheduled_departure AS first_scheduled_departure, \
    f1.scheduled_arrival AS first_scheduled_arrival, \
    f1.aircraft_code AS first_aircraft_code, \
    f2.flight_id AS second_flight_id, \
    f2.scheduled_departure AS second_scheduled_departure, \
    f2.scheduled_arrival AS second_scheduled_arrival, \
    f2.aircraft_code AS second_aircraft_code \
  FROM \
    flights f1 \
    JOIN flights f2 ON f1.arrival_airport = f2.departure_airport \
      AND f1.scheduled_arrival <= f2.scheduled_departure \
  WHERE \
    f1.departure_airport = '%s' \
    AND f2.arrival_airport = '%s' \
    AND DATE(f1.scheduled_departure) = '%s' \
    AND (f2.scheduled_arrival - f1.scheduled_departure) < interval '24 hours' \
    AND NOT f1.status = 'Cancelled' AND NOT f1.status = 'Arrived' \
    AND NOT f2.status = 'Cancelled' AND NOT f2.status = 'Arrived' \
), \
direct_flights AS ( \
  SELECT \
    f.flight_id, \
    f.scheduled_departure, \
    f.scheduled_arrival, \
    f.aircraft_code \
  FROM \
    flights f \
  WHERE \
    f.departure_airport = '%s' \
    AND f.arrival_airport = '%s' \
    AND DATE(f.scheduled_departure) = '%s' \
    AND NOT f.status = 'Cancelled' AND NOT f.status = 'Arrived' \
) \
SELECT \
  'Direct Flight' AS flight_type, \
  df.flight_id, \
  df.scheduled_departure, \
  df.scheduled_arrival, \
  df.aircraft_code, \
  es.num_empty_seats \
FROM \
  direct_flights df \
JOIN \
  empty_seats es ON df.flight_id = es.flight_id \
UNION ALL \
SELECT \
  'Connecting Flight' AS flight_type, \
  cf.first_flight_id AS flight_id, \
  cf.first_scheduled_departure AS scheduled_departure, \
  cf.second_scheduled_arrival AS scheduled_arrival, \
  cf.first_aircraft_code AS aircraft_code, \
  es.num_empty_seats \
FROM \
  connecting_flights cf \
JOIN \
  empty_seats es ON cf.first_flight_id = es.flight_id \
ORDER BY \
  scheduled_departure",
          from, to, departure_date, from, to, departure_date);

  SQLPrepare(stmt, (SQLCHAR *)query, SQL_NTS);

  SQLExecute(stmt);

  f = fopen("p.log", "w");

  i = 1;
  while (SQL_SUCCEEDED(ret = SQLGetDiagRec(SQL_HANDLE_STMT, stmt, i, sqlstate, &native_error, message_text, sizeof(message_text), &text_length)))
  {
    fprintf(f, "SQL Error: %s - %s\n", sqlstate, message_text);
    i++;
  }
  /* Loop through the rows in the result-set */
  i = 0;
  while (SQL_SUCCEEDED(ret = SQLFetch(stmt)))
  {
    ret = SQLGetData(stmt, 1, SQL_C_CHAR, flight_type, sizeof(flight_type), NULL);
    ret = SQLGetData(stmt, 2, SQL_INTEGER, &flight_id, sizeof(SQLINTEGER), NULL);
    ret = SQLGetData(stmt, 3, SQL_C_CHAR, scheduled_departure, sizeof(scheduled_departure), NULL);
    ret = SQLGetData(stmt, 4, SQL_C_CHAR, scheduled_arrival, sizeof(scheduled_arrival), NULL);
    ret = SQLGetData(stmt, 5, SQL_C_CHAR, aircraft_code, sizeof(aircraft_code), NULL);
    ret = SQLGetData(stmt, 6, SQL_INTEGER, &num_empty_seats, sizeof(SQLINTEGER), NULL);

    if (SQL_SUCCEEDED(ret))
    {
      sprintf(query_result_set[i], "%s %d %s %s %s %d", flight_type, flight_id, scheduled_departure,
              scheduled_arrival, aircraft_code, num_empty_seats);
      i++;
    }
  }

  *n_choices = i;
  max_rows = MIN(*n_choices, max_rows);
  
  for (i = 0; i < max_rows; i++)
  {
    fprintf(f, "%s\n", query_result_set[i]);
  }
  fclose(f);

  for (i = 0; i < max_rows; i++)
  {
    t = strlen(query_result_set[i]) + 1;
    t = MIN(t, max_length);
    strncpy((*choices)[i], query_result_set[i], t);
  }

  for (i = 0; i < max_rows; i++)
    free(query_result_set[i]);

  free(query_result_set);

  /* free up statement handle */
  SQLFreeHandle(SQL_HANDLE_STMT, stmt);

  /* DISCONNECT */
  ret = odbc_disconnect(env, dbc);
  if (!SQL_SUCCEEDED(ret))
  {
    fprintf(stderr, "Error en la desconexión de odbc");
    return;
  }
}

/*SKX SVO 2017-09-11*/