
#include "quiz_header.h"
#include "cJSON.h"

char* read_file(const char* filename) { // this func should take a filename i.e char *json_text = read_file("questions.json") Might be a better way to do this, just what I know for now
    FILE* file = fopen(filename, "rb"); // should open this in a byte format. 'raw bytes'
    if(!file) { // if failed to open print an error
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END); // should get the file length
    long length = ftell(file); // should then 'tell' how long the file is. 
    fseek(file, 0, SEEK_SET); // puts it 'back' to the beginning of the file
    printf("File length is %ld\n", length ); // TODO: take this out when done 

    char* buffer = malloc(length + 1); // allocating memory just beyond the length to prevent null termination
    if(!buffer) {
        perror("Malloc memory allocation failed");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file); // should put the file (questions.json) bytes into the buffer
    buffer[length] = '\0';
    fclose(file);

    return buffer;
}

int create_listen_socket() {
    struct addrinfo hints; // usual structs for the addrinfo we have been using in the course
    struct addrinfo *bind_address;
    memset(&hints, 0, sizeof(hints)); // clears garbage values from 'hints'
    hints.ai_family = AF_UNSPEC; // both IPv4 and Pv6, can change if it becomes too much work to do both later
    hints.ai_socktype = SOCK_STREAM; // will be a TCP connection. Need TCP as a message is being sent
    hints.ai_flags = AI_PASSIVE; // local address binding, server, not a client. Will this need changed? TODO: research then when 'sending' clients
    int result = getaddrinfo(NULL, "8080", &hints, &bind_address);
    if(result != 0) {
       fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(result)); // not sure if this error handling works here? 
        exit(1);
    }


    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

   if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) { // bind the socket and check for error
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    freeaddrinfo(bind_address);
    CLOSESOCKET(socket_listen);
    exit(1);
   }

   freeaddrinfo(bind_address);

   // listen
   if(listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    CLOSESOCKET(socket_listen);
    exit(1);
   }

   printf("Listening on port %s....\n ", "8080");

    return socket_listen;

}



int main() {
    #if defined(_WIN32)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
        }
    #endif


    char *json_text = read_file("questons_db/questions.json");
    if(!json_text) {
      return 1;
    }

    cJSON *root = cJSON_Parse(json_text); // should parse the json so we can put it in a cjson object and leverage cjson for our questions
    if(!root) {
        printf("Error: [%s]\n", cJSON_GetErrorPtr());
        free(json_text);
        return 1;
    }

    cJSON *questions = cJSON_GetObjectItem(root, "questions"); // should get the 'questions' array from our json
    if(!questions || !cJSON_IsArray(questions)) { // need to ensure it exist and has been pulled in correctly thus far
        printf("Error: the json has not been found\n");
        cJSON_Delete(root);
        return 1;
    }

    int num_of_questions = cJSON_GetArraySize(questions); // grab the number of entries in the array
    printf("Total questions in json: %d\n", num_of_questions);

    srand((unsigned int)time(NULL));
    int random_index_for_question = rand() % num_of_questions; // randomly grab index number
    cJSON *selected_question = cJSON_GetArrayItem(questions, random_index_for_question); // gets the question from 'questions' in json
    cJSON *question_text = cJSON_GetObjectItem(selected_question, "text"); // grabs the data in the 'text' field from the json
    printf("Question: %s\n", question_text->valuestring); // should present the message as a string in console

    cJSON *options = cJSON_GetObjectItem(selected_question, "options"); // grab the question options related to the selected question
    if(options && cJSON_IsArray(options)) { // some validation
        int num_options = cJSON_GetArraySize(options); // grab array size to use for looping
        for(int i = 0; i < num_options; i++) {
            cJSON *option = cJSON_GetArrayItem(options, i);
            if(option && cJSON_IsString(option)) {
                printf("%d. %s\n", i + 1, option->valuestring);
            }
        }
    } 

    // have the question and options, now need to rebuild what we will send through the sockets

    cJSON *question_to_send = cJSON_CreateObject(); // should just create an empty json object
    // add each feild, cJSON has functions to do this with as I build a json object as we go. 
    cJSON_AddNumberToObject(question_to_send, "id", cJSON_GetObjectItem(selected_question, "id")->valueint); // puting value from selected_question into question to send id
    cJSON_AddStringToObject(question_to_send, "text", cJSON_GetObjectItem(selected_question, "text")->valuestring);  // same with question 'text'
    cJSON *options_array = cJSON_CreateArray(); // not sure if this is right, think This array will need to be instantiated to hold the answer options inside question_to_send
    cJSON *options_src = cJSON_GetObjectItem(selected_question, "options");
    int num_options = cJSON_GetArraySize(options_src);
    for(int i = 0; i < num_options; i++) {
        cJSON *opt = cJSON_GetArrayItem(options_src, i); // adds each option to the array
        cJSON_AddItemToArray(options_array, cJSON_CreateString(opt->valuestring));
    }
    cJSON_AddItemToObject(question_to_send, "options", options_array); // should attach the new options array to the question to send we are sending cliesnts

    char *question_json = cJSON_PrintUnformatted(question_to_send); // just a test printout below.
    printf("Prepared JSON to send: %s\n", question_json);

    int listen_socket = create_listen_socket(); // listening socket on server side created

    // keep server open with loop

    while(1) {
        // find out how to accept client
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);

        SOCKET client_socket = accept(listen_socket, (struct sockaddr*) &client_address, &client_len); // passing in listening socket, returns a socket for client communicatin
        if(!ISVALIDSOCKET(client_socket)) {
            fprintf(stderr, "accept failed. (%d)\n", GETSOCKETERRNO());
            continue; // do we want to do this?, we would 'wait' for the next client instead of exiting. 
        }
        printf("Client side has connected!");
        // find out how to send json , question_to_send
          // Send the JSON string
        int sent = send(client_socket, question_json, strlen(question_json), 0); // should send the json, no good way to test this yet without client.
        if (sent < 0) {
            fprintf(stderr, "send() failed. (%d)\n", GETSOCKETERRNO());
        }

        CLOSESOCKET(client_socket);
        printf("Client disconnected.\n");
        // remember to close
    }

    cJSON_Delete(question_to_send);
    free(question_json);


    //   printf("File content contains \n %s \n", json_text); uncomment to see full json in questions.json
    free(json_text);
    #if defined(_WIN32)
         WSACleanup();
    #endif
    return 0;
}