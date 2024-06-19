#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

char webpage[] = "HTTP/1.1 200 OK\r\n"
    "Server: Linux Web Server\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html>\r\n"
    "<html><head><title> My Web Page </title>\r\n"
    "<style>body {background-color: #FFFF00}</style></head>\r\n"
    "<body><center><h1>Hello World</h1><br>\r\n"
    "<img src=\"game.jpg\"></center></body></html>\r\n";

char* readImageFile(const char* filename, size_t* file_size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    char* content = (char*)malloc(*file_size);
    if (!content) {
        fclose(file);
        fprintf(stderr, "Failed to allocate memory for file content.\n");
        return NULL;
    }

    if (fread(content, 1, *file_size, file) != *file_size) {
        fclose(file);
        free(content);
        fprintf(stderr, "Failed to read file: %s\n", filename);
        return NULL;
    }

    fclose(file);
    return content;
}

int main() {
    // 웹 서버 설정
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind");
        return 1;
    }

    if (listen(server_socket, 1) < 0) {
        perror("Failed to listen");
        return 1;
    }

    printf("Web server started. Listening on port 8080...\n");

    // 클라이언트 요청 처리
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Failed to accept client connection");
            continue;
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // 클라이언트로부터 HTTP 요청 메시지 수신
                ssize_t bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read < 0) {
            perror("Failed to receive client request");
            close(client_socket);
            continue;
        }

        // HTTP 요청 메시지에서 요청 경로 추출
        char* request_line = strtok(buffer, "\r\n");
        char* method = strtok(request_line, " ");
        char* path = strtok(NULL, " ");

        // "/" 경로에 접근한 경우
        if (strcmp(path, "/") == 0) {
            // HTTP 응답 메시지 전송
            send(client_socket, webpage, strlen(webpage), 0);
        } else if (strcmp(path, "/game.jpg") == 0) {
            // game.jpg 파일 읽기
            size_t image_size;
            char* image_content = readImageFile("game.jpg", &image_size);
            if (!image_content) {
                const char* response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
                send(client_socket, response, strlen(response), 0);
            } else {
                // HTTP 응답 메시지 생성
                const char* response_template =
                    "HTTP/1.1 200 OK\r\n"
                    "Server: Linux Web Server\r\n"
                    "Content-Type: image/jpeg\r\n"
                    "Content-Length: %zu\r\n\r\n";
                int response_size = snprintf(NULL, 0, response_template, image_size);
                char* http_response = (char*)malloc(response_size + image_size);
                if (!http_response) {
                    fprintf(stderr, "Failed to allocate memory for HTTP response.\n");
                    free(image_content);
                    close(client_socket);
                    continue;
                }

                sprintf(http_response, response_template, image_size);
                memcpy(http_response + response_size, image_content, image_size);

                // HTTP 응답 메시지 전송
                send(client_socket, http_response, response_size + image_size, 0);

                free(http_response);
                free(image_content);
            }
        } else {
            // 요청 경로가 유효하지 않은 경우
            const char* response = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(client_socket, response, strlen(response), 0);
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}
