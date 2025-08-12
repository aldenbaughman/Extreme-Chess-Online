#include "ECO_common.h"

void responseToPayload(char* payload, struct response clientRequest){
    /*
    if (clientRequest.client_id != NULL){
        snprintf(payload, BUFFER_SIZE, "%ld", clientRequest.client_id);
    }
    else{
        snprintf(payload, BUFFER_SIZE, "0");
    }

    if (clientRequest.sc_comm != NULL){
        snprintf(payload, BUFFER_SIZE, "%s %d", payload, clientRequest.sc_comm);
    }
    else{
        snprintf(payload, BUFFER_SIZE, "%s 0", payload);
    }

    if (clientRequest.board_id != NULL){
        snprintf(payload, BUFFER_SIZE, "%s %ld", payload, clientRequest.board_id);
    }
    else{
        snprintf(payload, BUFFER_SIZE, "%s 0", payload);
    }

    if (clientRequest.move.startRow != NULL){
        snprintf(payload, BUFFER_SIZE, "%s %d", payload, clientRequest.move.startRow);
    }
    else{
        snprintf(payload, BUFFER_SIZE, "%s 0", payload);
    }

    if (clientRequest.move.startCol != NULL){
        snprintf(payload, BUFFER_SIZE, "%s %d", payload, clientRequest.move.startCol);
    }
    else{
        snprintf(payload, BUFFER_SIZE, "%s 0", payload);
    }

    if (clientRequest.move.endCol != NULL){
        snprintf(payload, BUFFER_SIZE, "%s %d", payload, clientRequest.move.endCol);
    }
    else{
        snprintf(payload, BUFFER_SIZE, "%s 0", payload);
    }
    
    */

    
    snprintf(payload, BUFFER_SIZE, "%ld %d %ld %d %d %d %d", clientRequest.client_id, 
                                                                 clientRequest.sc_comm,
                                                                 clientRequest.board_id,
                                                                 clientRequest.move.startRow,
                                                                 clientRequest.move.startCol,
                                                                 clientRequest.move.endRow,
                                                                 clientRequest.move.endCol);
    

    //printf("[responseToPayload]: %s\n", payload);
}

int payloadToResponse(char* payload, struct response* responseInfo){
    //printf("[payloadToResponse] input payload: %s\n", payload);
    char* temp = payload;
    int count = 0;
    char payloadCopy[BUFFER_SIZE];
    memcpy(payloadCopy, payload, BUFFER_SIZE);

    //ADD PROPER ERR CHECKING FOR NUMBER OF INPUT SPACES
    /*
    while (*temp)
    {
        if (" " == &temp[0])
        {
            count++;
        }
        temp++;
    }
    */
    
    //Return error if incorrect payload is recieved

    /*
    char testString[] = "12 13";
    printf("[payloadToResponse]: Testing atoi %s\n", strtok(testString, " "));
    printf("[payloadToResponse]: Testing atoi %d\n", atoi(strtok(NULL, " ")));

    //printf("[payloadToResponse]: testString after strtok %s", testString);

    */

    //printf("[payloadToResponse]: Copied payload: %s\n", payloadCopy);

    //printf("[payloadToResponse]: Checking Integrity of response: ");

    

    

    responseInfo->client_id = atoi(strtok(payload, " "));
    //printf(" %ld", responseInfo->client_id);

    responseInfo->sc_comm = atoi(strtok(NULL, " "));
    //printf(" %d", responseInfo->sc_comm);
    
    responseInfo->board_id = atoi(strtok(NULL, " "));
    //printf(" %ld", responseInfo->board_id);

    responseInfo->move.startRow = atoi(strtok(NULL, " "));
    //printf(" %d", responseInfo->move.startRow);

    responseInfo->move.startCol = atoi(strtok(NULL, " "));
    //printf(" %d", responseInfo->move.startCol);

    responseInfo->move.endRow = atoi(strtok(NULL, " "));
    //printf(" %d", responseInfo->move.endRow);

    responseInfo->move.endCol = atoi(strtok(NULL, " "));
    //printf(" %d\n", responseInfo->move.endCol);

    return 1;
}

/*
int main(int argc, char* argv[]){
    struct response client_request;
    client_request.client_id = 10;
    client_request.sc_comm = 10;
    client_request.board_id = 10;
    client_request.move.startRow = 10;
    client_request.move.startCol = 10;
    client_request.move.endRow = 10;
    client_request.move.endCol = 10;

    printf("\n[payloadToResponse]: client request before payloadToPresponse: ");
    printf(" %ld", client_request.client_id);
    printf(" %d", client_request.sc_comm);
    printf(" %ld", client_request.board_id);
    printf(" %d", client_request.move.startRow);
    printf(" %d", client_request.move.startCol);
    printf(" %d", client_request.move.endRow);
    printf(" %d\n", client_request.move.endCol);

    char payload[14] = "5 5 5 5 5 5 5";

    payloadToResponse(payload, &client_request);

    printf("\n[payloadToResponse]: Checking Integrity of response: ");
    printf(" %ld", client_request.client_id);
    printf(" %d", client_request.sc_comm);
    printf(" %ld", client_request.board_id);
    printf(" %d", client_request.move.startRow);
    printf(" %d", client_request.move.startCol);
    printf(" %d", client_request.move.endRow);
    printf(" %d\n", client_request.move.endCol);
    


}

*/
