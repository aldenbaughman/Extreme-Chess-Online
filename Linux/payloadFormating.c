#include "ECO_common.h"

void responseToPayload(char* payload, struct response clientRequest){
    snprintf(payload, BUFFER_SIZE, "%ld %d %ld %d %d %d %d", clientRequest.client_id, 
                                                                 clientRequest.sc_comm,
                                                                 clientRequest.board_id,
                                                                 clientRequest.move.startRow,
                                                                 clientRequest.move.startCol,
                                                                 clientRequest.move.endRow,
                                                                 clientRequest.move.endCol);
    printf("[responseToPayload]: %s\n", payload);
}

int splitPayloadBySpaces(char* payload, struct response responseInfo){
    char* temp = payload;
    int count = 0;
    char payloadCopy[sizeof payload];

    while (*temp)
    {
        if (" " == &temp[0])
        {
            count++;
        }
        temp++;
    }

    //Return error if incorrect payload is recieved
    if (count != 7){
        return -1;
    }

    responseInfo.client_id = (u_int64_t)strtol(strtok(payloadCopy, " "), NULL, 10);
    responseInfo.sc_comm = atoi(strtok(payloadCopy, " "));
    responseInfo.board_id = (u_int64_t)strtol(strtok(payloadCopy, " "), NULL, 10);
    responseInfo.move.startRow = atoi(strtok(payloadCopy, " "));
    responseInfo.move.startCol = atoi(strtok(payloadCopy, " "));
    responseInfo.move.endRow = atoi(strtok(payloadCopy, " "));
    responseInfo.move.endCol = atoi(strtok(payloadCopy, " "));

    printf("[payloadToResponse]: Checking Integrity of response: ");
    printf(" %ld", responseInfo.client_id);
    printf(" %d", responseInfo.sc_comm);
    printf(" %ld", responseInfo.board_id);
    printf(" %d", responseInfo.move.startRow);
    printf(" %d", responseInfo.move.startCol);
    printf(" %d", responseInfo.move.endRow);
    printf(" %d\n", responseInfo.move.endCol);


    return 1;
}


