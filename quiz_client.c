
#include "quiz_header.h"
#include "cJSON.h"

int main() {
    #if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2,2), &d)) { // winsock initializatin
        fprintf(stderr, "WSAStartup failed. \n");
        return 1;
    }
    #endif

    const char *server_host = "127.0.0.1";
    const char *server_port = "8080";

    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // ipv4 for now
    hints.ai_socktype = SOCK_STREAM;

    int result = getaddrinfo(server_host, server_port, &hints, &server_info);

    if(result != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(result));
        return 1;
    }

    SOCKET client_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if(!ISVALIDSOCKET(client_socket)) { // is valid socket should work same as in classwork for this, checking returns for different machine types
        fprintf(stderr, "socket() failed. (%d). \n", GETSOCKETERRNO());
        freeaddrinfo(server_info);
        return 1;
    }

    printf("Connecting to server\n");

    if (connect(client_socket, server_info->ai_addr, server_info->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        CLOSESOCKET(client_socket);
        freeaddrinfo(server_info);
        return 1;
    }

    printf("Connected to server!\n");
    freeaddrinfo(server_info);  


        // Buffer to store data from server
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received < 0) {
        fprintf(stderr, "recv() failed. (%d)\n", GETSOCKETERRNO());
        CLOSESOCKET(client_socket);
        return 1;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the data
    printf("Received JSON: %s\n", buffer);

    // Parse the received JSON
    cJSON *received_question = cJSON_Parse(buffer);
    if (!received_question) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        CLOSESOCKET(client_socket);
        return 1;
    }

    // Extract and display question text
    cJSON *question_text = cJSON_GetObjectItem(received_question, "text");
    printf("\nQuestion: %s\n", question_text->valuestring);

    // Extract options
    cJSON *options = cJSON_GetObjectItem(received_question, "options");
    if (options && cJSON_IsArray(options)) {
        int num_options = cJSON_GetArraySize(options);
        for (int i = 0; i < num_options; i++) {
            cJSON *opt = cJSON_GetArrayItem(options, i);
            printf("%d. %s\n", i + 1, opt->valuestring);
        }
    }

    cJSON_Delete(received_question);

        CLOSESOCKET(client_socket);
    #if defined(_WIN32)
    WSACleanup();
    #endif
    return 0;


}