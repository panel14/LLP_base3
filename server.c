#include <pb_encode.h>
#include <pb_decode.h>
#include "pblib/message.pb.h"
#include "include/common.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "storage/UserAPI/InteractiveMode/interactiveMode.h"
#include "storage/handler.h"

#define PORT 3939

int main(int c, char** v) {
    setbuf(stdout, 0);

    FILE* fp = initialize(c, v);

    int listenFd, connectionFd;
    struct sockaddr_in serverAddress;
    int reuse = 1;

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddress.sin_port = htons(PORT);

    if (bind(listenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror("binding error");
        return 1;
    }

    if (listen(listenFd, 5) != 0) {
        perror("listening error");
        return 1;
    }

    printf("Server: waiting for connection. Port: %d\n", PORT);

    connectionFd = accept(listenFd, NULL, NULL);

    if (connectionFd < 0) {
        perror("accepting error");
        return 1;
    }

    printf("Get connection\n");

    pb_istream_t in = pbIstreamFromSocket(connectionFd);
    pb_ostream_t out = pbOstreamFromSocket(connectionFd);

    char* res = "";

    while(1) {
        Query q = {};

        if (!pb_decode_delimited(&in, Query_fields, &q)) {
            printf("Decoding failed: %s\n", PB_GET_ERROR(&in));
            return 2;
        }

        handleQuery(fp, q, &res);

        Response response = {};

        while(strlen(res) > 1023) {
            strncpy(response.rString, res, 1023);
            response.rString[1023] = 0;
            response.last = 0;

            if (!pb_decode_delimited(&out, Response_fields, &response)) {
                fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&out));
            }
            res += 1023;
        }
        strcpy(response.rString, res);
        response.last = 1;

        if (!pb_decode_delimited(&out, Response_fields, &response)) {
            fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&out));
        }
    }
}