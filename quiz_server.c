
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



int main() {
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

    //   printf("File content contains \n %s \n", json_text); uncomment to see full json in questions.json
        free(json_text);
    return 0;
}