#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"

#define HOST "34.246.184.49"
#define PORT 8080

void register_user();
void login_user(char *session_cookie);
void enter_library(char *session_cookie);
void get_books();
void get_book();
void add_book();
void delete_book();
void logout_user(char *session_cookie);
char *extract_cookie(char *response);
char *extract_jwt_token(char *response);

char *jwt_token = NULL;

void print_response(char *response) {
    if (response) {
        printf("%s\n", response);  
    }
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

char *extract_cookie(char *response) {
    const char *cookie_header = "Set-Cookie: ";
    char *start = strstr(response, cookie_header);
    if (start) {
        start += strlen(cookie_header);
        char *end = strstr(start, ";");
        if (end) {
            ptrdiff_t length = end - start;
            char *cookie = (char *)malloc(length + 1);
            strncpy(cookie, start, length);
            cookie[length] = '\0';
            return cookie;
        }
    }
    return NULL;
}

char* extract_jwt_token(char *response) {
    const char *token_key = "\"token\":\"";
    char *start = strstr(response, token_key);
    if (start) {
        start += strlen(token_key);
        char *end = strchr(start, '"');
        if (end) {
            ptrdiff_t length = end - start;
            char *token = malloc(length + 1);
            if (token) {
                strncpy(token, start, length);
                token[length] = '\0';
                return token;
            }
        }
    }
    return NULL;
}


void register_user() {
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char username[50], password[50];
    printf("username=");
    scanf("%s", username);
    printf("password=");
    scanf("%s", password);
    getchar(); 

    char request_body[200];
    sprintf(request_body, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    char message[1024];
    sprintf(message, "POST /api/v1/tema/auth/register HTTP/1.1\r\nHost: %s:%d\r\nContent-Type: application/json\r\nContent-Length: %ld\r\n\r\n%s", HOST, PORT, strlen(request_body), request_body);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    if (strstr(response, "201 Created")) {
        printf("Utilizator înregistrat cu succes! SUCCESS!\n");
    } else {
        printf("ERROR! Username deja folosit.\n");
    }
    free(response);
}

void login_user(char *session_cookie) {
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char username[50], password[50];
    printf("username=");
    scanf("%s", username);
    printf("password=");
    scanf("%s", password);
    getchar();
    char request_body[200];
    sprintf(request_body, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    char message[1024];
    sprintf(message, "POST /api/v1/tema/auth/login HTTP/1.1\r\nHost: %s:%d\r\nContent-Type: application/json\r\nContent-Length: %ld\r\n\r\n%s", HOST, PORT, strlen(request_body), request_body);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    char *cookie = extract_cookie(response);
    if (cookie) {
        strcpy(session_cookie, cookie);
        printf("Utilizatorul a fost logat cu succes. SUCCESS!\n");
        free(cookie);
    } else {
        printf("Login failed. ERROR!\n");
    }
    free(response);
}

void enter_library(char *session_cookie) {
    if (session_cookie == NULL || strlen(session_cookie) == 0) {
        printf("ERROR: No session cookie available. Please log in first.\n");
        return;
    }

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char headers[256];
    sprintf(headers, "Cookie: %s\r\n", session_cookie);
    char message[1024];
    snprintf(message, sizeof(message), 
             "GET /api/v1/tema/library/access HTTP/1.1\r\n"
             "Host: %s\r\n"
             "%s\r\n", 
             HOST, headers);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("Utilizatorul are acces la biblioteca. SUCCESS!\n");
        jwt_token = extract_jwt_token(response);
    } else {
        printf("ERROR: Unable to access the library. Please check your session.\n");
    }

    close_connection(sockfd);
    free(response);
}


void get_books(char *session_cookie) {
    if (jwt_token == NULL || session_cookie == NULL) {
        printf("No JWT token available. ERROR!\n");
        return;
    }

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char message[1024];
    snprintf(message, sizeof(message),
             "GET /api/v1/tema/library/books HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Authorization: Bearer %s\r\n"
             "\r\n",
             HOST, jwt_token);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    if (strstr(response, "200 OK")) {
        char *json_start = strstr(response, "\r\n\r\n");
        if (json_start) {
            printf("SUCCESS: Retrieved books:\n%s\n", json_start + 4);
        } else {
            printf("Error: Failed to parse book data.\n");
        }
    } else {
        printf("Error: Failed to retrieve books due to an unknown error.\n");
    }

    free(response);
}

void get_book(char *session_cookie) {
    if (jwt_token == NULL || session_cookie == NULL) {
        printf("No JWT token available. ERROR!\n");
        return;
    }

    int book_id;
    printf("id=");
    scanf("%d", &book_id);
    getchar();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char path[100];
    snprintf(path, sizeof(path), "/api/v1/tema/library/books/%d", book_id);

    char message[1024];
    snprintf(message, sizeof(message),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Authorization: Bearer %s\r\n"
             "\r\n",
             path, HOST, jwt_token);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    if (strstr(response, "200 OK")) {
        char *json_start = strstr(response, "\r\n\r\n");
        if (json_start) {
            printf("SUCCESS: Retrieved book:\n%s\n", json_start + 4);
        } else {
            printf("ERROR: Failed to parse book data.\n");
        }
    } else if (strstr(response, "404 Not Found")) {
        printf("ERROR: Book ID not found. Please check the book ID.\n");
    } else {
        printf("ERROR: Failed to retrieve book details due to an unknown error.\n");
    }

    free(response);
}

int is_digit_str(const char *str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}


void add_book(char *session_cookie) {
    if (jwt_token == NULL || session_cookie == NULL) {
        printf("No JWT token available. ERROR!\n");
        return;
    }

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char title[256], author[256], genre[256], publisher[256], page_count_str[256];
    int page_count;

    printf("\ntitle=");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;
    printf("author=");
    fgets(author, sizeof(author), stdin);
    author[strcspn(author, "\n")] = 0;

    printf("genre=");
    fgets(genre, sizeof(genre), stdin);
    genre[strcspn(genre, "\n")] = 0;

    printf("publisher=");
    fgets(publisher, sizeof(publisher), stdin);
    publisher[strcspn(publisher, "\n")] = 0;

    printf("page_count=");
    fgets(page_count_str, sizeof(page_count_str), stdin);
    page_count_str[strcspn(page_count_str, "\n")] = 0;
    
    if (!is_digit_str(page_count_str)) {
        printf("ERROR: Invalid page count.\n");
        close_connection(sockfd);
        return;
    }

    page_count = atoi(page_count_str);
    if (page_count <= 0) { 
        printf("ERROR: Invalid page count.\n");
        close_connection(sockfd);
        return;
    }
    char encoded_body[512];
    snprintf(encoded_body, sizeof(encoded_body),
             "title=%s&author=%s&genre=%s&publisher=%s&page_count=%d",
             title, author, genre, publisher, page_count);

    char message[1024];
    snprintf(message, sizeof(message),
             "POST /api/v1/tema/library/books HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Authorization: Bearer %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s",
             HOST, PORT, jwt_token, strlen(encoded_body), encoded_body);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    if (strstr(response, "200 OK") || strstr(response, "201 Created")) {
        printf("Book added successfully! SUCCESS!\n");
    } else if (strstr(response, "401 Unauthorized")) {
        printf("ERROR: Unauthorized access. Please check your JWT token.\n");
    } else if (strstr(response, "403 Forbidden")) {
        printf("ERROR: Access to the library is forbidden with the provided JWT token.\n");
    } else {
        printf("ERROR: Failed to add book due to an unknown error.\n");
    }

    free(response);
}

void delete_book(char *session_cookie) {
    if (jwt_token == NULL) {
        printf("No JWT token available. ERROR!\n");
        return;
    }

    int book_id;
    printf("id=");
    scanf("%d", &book_id);
    getchar(); 

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char path[100];
    snprintf(path, sizeof(path), "/api/v1/tema/library/books/%d", book_id);
    char message[1024];
    snprintf(message, sizeof(message),
             "DELETE %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Authorization: Bearer %s\r\n"
             "\r\n",
             path, HOST, PORT, jwt_token);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    
    if (strstr(response, "200 OK") || strstr(response, "204 No Content")) {
        printf("Book deleted successfully! SUCCESS!\n");
    } else if (strstr(response, "404 Not Found")) {
        printf("ERROR: Book ID not found. Please check the book ID.\n");
    } else {
        printf("ERROR: Failed to delete book due to an unknown error.\n");
    }
    free(response);
}



void logout_user(char *session_cookie) {
    if (session_cookie == NULL) {
        printf("You are not logged in: ERROR!\n");
        return;
    }

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char message[1024];
    snprintf(message, sizeof(message),
             "GET /api/v1/tema/auth/logout HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Cookie: %s\r\n"
             "\r\n",
             HOST, PORT, session_cookie);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    if (strstr(response, "200 OK")) {
        printf("User logged out: SUCCESS!\n");
        if (jwt_token != NULL) {
            free(jwt_token);
            jwt_token = NULL;
        }
    } else {
        printf("User could not be logged out: ERROR!\n");
    }
    free(response);
}

int main() {
    char command[50];
    char *session_cookie = malloc(300); 

    while (1) {
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "register") == 0) {
            register_user();
        } else if (strcmp(command, "login") == 0) {
            login_user(session_cookie);
        } else if (strcmp(command, "enter_library") == 0) {
            enter_library(session_cookie);
        } else if (strcmp(command, "get_books") == 0) {
            get_books(session_cookie);
        } else if (strcmp(command, "get_book") == 0) {
            get_book(session_cookie);
        } else if (strcmp(command, "add_book") == 0) {
            add_book(session_cookie);
        } else if (strcmp(command, "delete_book") == 0) {
            delete_book(session_cookie);
        } else if (strcmp(command, "logout") == 0) {
            logout_user(session_cookie);
        } else {
            printf("Unknown command\n");
        }
    }

    if (session_cookie != NULL) {
        free(session_cookie);
        session_cookie = NULL;
    }
    if (jwt_token != NULL) {
        free(jwt_token);
        jwt_token = NULL;
    }

    return 0;
}
