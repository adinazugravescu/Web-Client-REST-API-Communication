#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

// Functie care compune o cerere de tip GET catre un anumit host,
// in functie de ruta, parametrii de query, cookie si tokenul de acces
char *compute_get_request(char *host, char *url, char *query_params,
                            char *cookie,char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (cookie != NULL) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }
    
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    compute_message(message, "\r\n");

    free(line);
    return message;
}

// Functie care compune o cerere de tip POST catre un anumit host,
// in functie de ruta, tipul de continut, datele de body, numarul de campuri si tokenul de acces
char *compute_post_request(char *host, char *url, char* content_type, char *body_data,
                            int body_data_fields_count, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %d", body_data_fields_count);
    compute_message(message, line);

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }


    compute_message(message, "");
    strcat(message, body_data);

    free(line);
    return message;
}

// Functie care compune o cerere de tip DELETE catre un anumit host,
// in functie de ruta, cookie si tokenul de acces
char *compute_delete_request(char *host, char *url, char* cookie, char* token) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (cookie != NULL) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    compute_message(message, "\r\n");
    free(line);
    return message;
}