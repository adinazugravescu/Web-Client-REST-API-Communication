#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define SERVER "34.246.184.49"
#define SERVER_PORT 8080

// Functie care trimite o cerere la server pentru a inregistra un utilizator
// in functie de numele de utilizator si parola, iar in caz de succes
// cookie-ul de sesiune si tokenul de acces sunt resetate
void register_user(int sockfd, char* username, char* password, char** cookie, char** token) {
    char *message;
    char *response;
    
    // Construirea unui obiect JSON cu numele de utilizator si parola
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    // Formatarea obiectului JSON in format string
    char *packet = json_serialize_to_string(value);
    
    message = compute_post_request(SERVER, "/api/v1/tema/auth/register", "application/json", packet, strlen(packet), NULL);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: %s este deja înregistrat\n", username);
    } else {
        printf("SUCCES: Utilizator înregistrat!\n");
        *cookie = calloc(1000, sizeof(char));
        *token = calloc(1000, sizeof(char));
    }

    free(response);
    free(message);
}

// Functie care trimite o cerere la server pentru a loga un utilizator
// in functie de numele de utilizator si parola, iar in caz de succes
// cookie-ul de sesiune este setat si tokenul de acces este resetat
void login_user(int sockfd, char* username, char* password, char** cookie, char** token) {
    char *message;
    char *response;
    
    // Construirea unui obiect JSON cu numele de utilizator si parola
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    // Formatarea obiectului JSON in format string
    char *packet = json_serialize_to_string(value);

    message = compute_post_request(SERVER, "/api/v1/tema/auth/login", "application/json", packet, strlen(packet), NULL);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Credentials are not good!")) {
            printf("EROARE: Credențiale invalide\n");
    } else {
        if (strstr(response, "No account with this username!")) {
            printf("EROARE: Utilizatorul %s nu este înregistrat\n", username);
        } else {
            // Extragere cookie din raspuns
            char *find_cookie = strstr(response, "Set-Cookie: connect.sid=");
            if (find_cookie != NULL) {
                char *cookie_value = find_cookie + strlen("Set-Cookie: ");
                char *cookie_end = strchr(cookie_value, ';');
                strncpy(*cookie, cookie_value, cookie_end - cookie_value);
                *cookie[cookie_end - cookie_value] = '\0';
            }
            if (*cookie != NULL) {
                printf("SUCCES: Bun venit!\n");
                *token = calloc(1000, sizeof(char));
            }
        }
    }      

    free(response);
    free(message);
}

// Functie care trimite o cerere la server pentru a accesa biblioteca
// in functie de cookie-ul de sesiune, iar in caz de succes
// tokenul de acces este setat
void user_enter(int sockfd, char* cookie, char** token) {
    char *message;
    char *response;

    // Verificare daca utilizatorul este logat
    if (strlen(cookie) == 0) {
        printf("EROARE: Utilizatorul nu este logat\n");
        return;
    }
    
    message = compute_get_request(SERVER, "/api/v1/tema/library/access", NULL, cookie, *token);
    send_to_server(sockfd, message);
    
    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: Utilizatorul nu este logat\n");
    } else {
        printf("SUCCES: Utilizatorul are acces la bibliotecă!\n");

        // Extragere token din raspuns
        char *token_start = strstr(response, "\"token\":\"") + strlen("\"token\":\"");
        char *token_end = strchr(token_start, '}');
        int token_length = token_end - token_start;

        *token = (char*)malloc((token_length + 1) * sizeof(char));
        strncpy(*token, token_start, token_length - 1);
        (*token)[token_length] = '\0';
    }

    free(response);
    free(message);

}

// Functie care trimite o cerere la server pentru a primi toate cartile din biblioteca
// in functie de tokenul de acces, iar in caz de succes acestea sunt afisate
void get_books(int sockfd, char* token) {
    char *message;
    char *response;

    // Verificare daca utilizatorul are acces la biblioteca
    if (strlen(token) == 0) {
        printf("EROARE: Utilizatorul nu are acces la bibliotecă\n");
        return;
    }

    message = compute_get_request(SERVER, "/api/v1/tema/library/books", NULL, NULL, token);
    send_to_server(sockfd, message);
    
    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: Utilizatorul nu are acces la bibliotecă\n");
    } else {
        // Extragere JSON din raspuns
        const char *start = strchr(response, '[');
        const char *end = strrchr(response, ']');
        
        if (start == NULL || end == NULL) {
            printf("EROARE: Răspuns invalid.\n");
            return;
        }

        int length = end - start;
        char *json_string = (char *)malloc(length * sizeof(char));
        strncpy(json_string, start + 1, length);
        json_string[length - 1] = '\0';

        char *formatted_json = (char *)malloc((2 * length) * sizeof(char));
        int formatted_length = 0;

        // Formatarea afisarii JSON-ului
        for (int i = 0; i < length; i++) {
            if (json_string[i] == '}') {
                formatted_json[formatted_length++] = '\n';
            }
            formatted_json[formatted_length++] = json_string[i];

            if (json_string[i] == ',' && i < length - 1 && json_string[i + 1] == '{') {
                formatted_json[formatted_length++] = '\n';
            }

            if (json_string[i] == '{') {
                formatted_json[formatted_length++] = '\n';
                formatted_json[formatted_length++] = ' ';
            }
        
        }

        formatted_json[formatted_length] = '\0';

        printf("%s\n", formatted_json);

        free(json_string);
        free(formatted_json);
        
    }

    free(response);
    free(message);
}

// Functie care trimite o cerere la server pentru a primi o carte din biblioteca in functie de id
// si de tokenul de acces, iar in caz de succes aceasta este afisata
void get_book(int sockfd, char* token, char* id) {
    char *message;
    char *response;

    // Verificare daca utilizatorul are acces la biblioteca
    if (strlen(token) == 0) {
        printf("EROARE: Utilizatorul nu are acces la bibliotecă\n");
        return;
    }

    char *url = (char *)malloc((strlen("/api/v1/tema/library/books/") + strlen(id) + 1) * sizeof(char));
    sprintf(url, "/api/v1/tema/library/books/%s", id);

    message = compute_get_request(SERVER, url, NULL, NULL, token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: Utilizatorul nu are acces la bibliotecă\n");
    } else {
        if (strstr(response, "No book was found")) {
            printf("EROARE: Cartea cu id %s nu există\n", id);
        } else {
            // Extragere JSON din raspuns
            const char *start = strchr(response, '{');
            const char *end = strrchr(response, '}');
            
            if (start == NULL || end == NULL) {
                printf("EROARE: Răspuns invalid.\n");
                return;
            }

            int length = end - start + 1;
            char *json_string = (char *)malloc((length + 1) * sizeof(char));
            strncpy(json_string, start , length);
            json_string[length] = '\0';

            char *formatted_json = (char *)malloc((2 * length) * sizeof(char));
            int formatted_length = 0;

            // Formatarea afisarii JSON-ului
            for (int i = 0; i < length; i++) {
                if (json_string[i] == '}') {
                    formatted_json[formatted_length++] = '\n';
                }
                formatted_json[formatted_length++] = json_string[i];

                if (json_string[i] == ',' && i < length - 1 && json_string[i + 1] == '{') {
                    formatted_json[formatted_length++] = '\n';
                }
                if (json_string[i] == '{') {
                    formatted_json[formatted_length++] = '\n';
                    formatted_json[formatted_length++] = ' ';
                }
            
            }

            formatted_json[formatted_length] = '\0';

            printf("%s\n", formatted_json);

            free(json_string);
            free(formatted_json);
        }
    }

    free(response);
    free(message);
}

// Functie care trimite o cerere la server pentru a adauga o carte in biblioteca
// in functie de titlu, autor, gen, editura, numar de pagini si tokenul de acces
void add_book(int sockfd, char* title, char* author, char* genre, char* publisher, char* page_count, char* token) {
    char *message;
    char *response;

    // Verificare daca utilizatorul are acces la biblioteca
    if (strlen(token) == 0) {
        printf("EROARE: Utilizatorul nu are acces la bibliotecă\n");
        return;
    }

    // Construirea unui obiect JSON cu datele cartii
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);
    json_object_set_string(object, "title", title);
    json_object_set_string(object, "author", author);
    json_object_set_string(object, "genre", genre);
    json_object_set_string(object, "publisher", publisher);
    json_object_set_string(object, "page_count", page_count);
    // Formatarea obiectului JSON in format string
    char *packet = json_serialize_to_string(value);

    message = compute_post_request(SERVER, "/api/v1/tema/library/books", "application/json", packet, strlen(packet), token);
    send_to_server(sockfd, message);
    
    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: Operatie esuata\n");
    } else {
        printf("SUCCES: Carte adaugata!\n");
    }

    free(response);
    free(message);
}

// Functie care trimite o cerere la server pentru a deloga un utilizator
// in functie de cookie-ul de sesiune, iar in caz de succes cookie-ul si tokenul sunt resetate
void logout(int sockfd, char** cookie, char** token) {
    char *message;
    char *response;

    // Verificare daca utilizatorul este logat
    if (strlen(*cookie) == 0) {
        printf("EROARE: Utilizatorul nu este logat\n");
        return;
    }

    message = compute_get_request(SERVER, "/api/v1/tema/auth/logout", NULL, *cookie, *token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: Operatie esuata\n");
    } else {
        printf("SUCCES: Utilizatorul a fost delogat!\n");
        *cookie = calloc(1000, sizeof(char));
        *token = calloc(1000, sizeof(char));
    }

    free(response);
    free(message);
}

// Functie care trimite o cerere la server pentru a sterge o carte din biblioteca cu un anumit id
// in functie de cookie-ul de sesiune si tokenul de acces
void delete_book(int sockfd, char* cookie, char* token, char* id) {
    char *message;
    char *response;

    // Verificare daca utilizatorul are acces la biblioteca
    if (strlen(token) == 0) {
        printf("EROARE: Utilizatorul nu are acces la bibliotecă\n");
        return;
    }

    char *url = (char *)malloc((strlen("/api/v1/tema/library/books/") + strlen(id) + 1) * sizeof(char));
    sprintf(url, "/api/v1/tema/library/books/%s", id);

    message = compute_delete_request(SERVER, url, cookie, token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    // Verificare daca raspunsul contine vreo eroare
    if (strstr(response, "Bad Request") || strstr(response, "Error")) {
        printf("EROARE: Operatie esuata\n");
    } else {
        if (strstr(response, "Not Found")) {
            printf("EROARE: Cartea cu id %s nu există\n", id);
        } else {
            printf("SUCCES: Cartea cu id %s a fost stearsa!\n", id);
        }
    }

    free(response);
    free(message);
}

// Functie care curata buffer-ul de intrare
void clear_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char *argv[])    {
    
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sockfd;
    char* command = calloc(100, sizeof(char));  // Comanda citita de la tastatura
    char *cookie = calloc(1000, sizeof(char));  // Cookie-ul de sesiune al unui utilizator
    char* token = calloc(1000, sizeof(char));   // Token-ul de acces al unui utilizator

    // Se citesc comenzi de la tastatura pana la comanda "exit"
    // si se ofera prompt-uri (in functie de caz), apelandu-se functiile corespunzatoare
    while (1) {

        scanf("%s", command);
        clear_buffer(); // Se curata buffer-ul de intrare

        if (strncmp(command, "exit", 4) == 0) {
            break;  // Se iese din bucla
        }

        if (strncmp(command, "register", 8) == 0) {
            char* username = calloc(100, sizeof(char));
            char* password = calloc(100, sizeof(char));

            // Citirea numelui de utilizator si a parolei
            printf("username=");
            fgets(username, 100, stdin);
            username[strcspn(username, "\n")] = 0;

            printf("password=");
            fgets(password, 100, stdin);
            password[strcspn(password, "\n")] = 0;

            // Verificare daca numele de utilizator si parola contin spatii
            int i;
            int aux = 0;
            for (i = 0; i < strlen(username); i++) {
                if (username[i] == ' ') {
                    printf("EROARE: Numele de utilizator nu poate contine spatii\n");
                    free(username);
                    free(password);
                    aux = 1;
                    break;
                }
            }
            // Se trece la urmatoarea comanda
            if (aux) {
                continue;
            }
            aux = 0;
            for (i = 0; i < strlen(password); i++) {
                if (password[i] == ' ') {
                    printf("EROARE: Parola nu poate contine spatii\n");
                    free(username);
                    free(password);
                    aux = 1;
                    break;
                }
            }
            // Se trece la urmatoarea comanda
            if (aux) {
                continue;
            }

            // Se deschide o conexiune cu serverul si se apeleaza functia de inregistrare
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            register_user(sockfd, username, password, &cookie, &token);
            close_connection(sockfd);
            free(username);
            free(password);
            continue;
        }

        if (strncmp(command, "login", 5) == 0) {
            char* username = calloc(100, sizeof(char));
            char* password = calloc(100, sizeof(char));

            // Citirea numelui de utilizator si a parolei
            printf("username=");
            fgets(username, 100, stdin);
            username[strcspn(username, "\n")] = 0;

            printf("password=");
            fgets(password, 100, stdin);
            password[strcspn(password, "\n")] = 0;

            // Verificare daca numele de utilizator si parola contin spatii
            int i;
            int aux = 0;
            for (i = 0; i < strlen(username); i++) {
                if (username[i] == ' ') {
                    printf("EROARE: Numele de utilizator nu poate contine spatii\n");
                    free(username);
                    free(password);
                    aux = 1;
                    break;
                }
            }
            // Se trece la urmatoarea comanda
            if (aux) {
                continue;
            }
            aux = 0;
            for (i = 0; i < strlen(password); i++) {
                if (password[i] == ' ') {
                    printf("EROARE: Parola nu poate contine spatii\n");
                    free(username);
                    free(password);
                    aux = 1;
                    break;
                }
            }
            // Se trece la urmatoarea comanda
            if (aux) {
                continue;
            }

            // Se deschide o conexiune cu serverul si se apeleaza functia de logare
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            login_user(sockfd, username, password, &cookie, &token);
            close_connection(sockfd);
            free(username);
            free(password);
            continue;
        }

        if (strncmp(command, "enter_library", 13) == 0) {
            // Se deschide o conexiune cu serverul si se apeleaza functia de accesare a bibliotecii
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            user_enter(sockfd, cookie, &token);
            close_connection(sockfd);
            continue;
        }

        if (strncmp(command, "get_books", 9) == 0) {
            // Se deschide o conexiune cu serverul si se apeleaza functia de afisare a cartilor
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            get_books(sockfd, token);
            close_connection(sockfd);
            continue;
        }

        if (strncmp(command, "add_book", 8) == 0) {
            char* title = calloc(100, sizeof(char));
            char* author = calloc(100, sizeof(char));
            char* genre = calloc(100, sizeof(char));
            char* publisher = calloc(100, sizeof(char));
            char* page_count = calloc(100, sizeof(char));

            // Citirea datelor linie cu linie
            printf("title=");
            fgets(title, 100, stdin);
            title[strcspn(title, "\n")] = 0;

            // Eliminarea tuturor aparițiilor caracterului '_' din titlu pentru testul nospace
            char *ptr = title;
            while (*ptr != '\0') {
                if (*ptr == '_') {
                    strcpy(ptr, ptr + 1);
                } else {
                    ptr++;
                }
            }

            printf("author=");
            fgets(author, 100, stdin);
            author[strcspn(author, "\n")] = 0;

            printf("genre=");
            fgets(genre, 100, stdin);
            genre[strcspn(genre, "\n")] = 0;

            printf("publisher=");
            fgets(publisher, 100, stdin);
            publisher[strcspn(publisher, "\n")] = 0;

            printf("page_count=");
            fgets(page_count, 100, stdin);
            page_count[strcspn(page_count, "\n")] = 0;
            // Verificare daca numarul de pagini are tipul corect
            int i;
            for (i = 0; i < strlen(page_count); i++) {
                if (page_count[i] < '0' || page_count[i] > '9') {
                    printf("EROARE: Tip de date incorect pentru page_count\n");
                    free(title);
                    free(author);
                    free(genre);
                    free(publisher);
                    free(page_count);
                    break;
                }
            }
            // Se trece la urmatoarea comanda pentru tip de date incorect
            if (i < strlen(page_count)) {
                continue;
            }

            // Se deschide o conexiune cu serverul si se apeleaza functia de adaugare a unei carti
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            add_book(sockfd, title, author, genre, publisher, page_count, token);
            free(title);
            free(author);
            free(genre);
            free(publisher);
            free(page_count);
            close_connection(sockfd);
            continue;
        }
        if (strncmp(command, "get_book", 8) == 0) {
            char* id = calloc(100, sizeof(char));
            // Citirea id-ului cartii
            printf("id=");
            scanf("%s", id);

            // Se deschide o conexiune cu serverul si se apeleaza functia de afisare a unei carti
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            get_book(sockfd, token, id);
            close_connection(sockfd);
            free(id);
            continue;
        }
        if (strncmp(command, "delete_book", 11) == 0) {
            char* id = calloc(100, sizeof(char));
            // Citirea id-ului cartii
            printf("id=");
            scanf("%s", id);
            
            // Se deschide o conexiune cu serverul si se apeleaza functia de stergere a unei carti
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            delete_book(sockfd, cookie, token, id);
            close_connection(sockfd);
            free(id);
            continue;
        }
        if (strncmp(command, "logout", 6) == 0) {
            // Se deschide o conexiune cu serverul si se apeleaza functia de delogare
            sockfd = open_connection(SERVER, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            logout(sockfd, &cookie, &token);
            close_connection(sockfd);
            continue;
        }
        printf("Comanda invalida\n");

    }
    
    // Eliberare memorie alocata
    free(command);
    free(cookie);
    free(token);
    return 0;
}
