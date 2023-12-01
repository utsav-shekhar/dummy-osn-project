#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "helper.h"
int writers = 0;
typedef struct
{
    char ip[24];
    int nm_port;
} NamingServerInfo;

typedef struct
{
    char ip[24];
    int ss_port;
} StorageServerInfo;

typedef struct
{
    char operation[50];
    char file_path[1024];
} ClientRequest;

StorageServerInfo storage_server;
NamingServerInfo naming_server;

void Connect_to_SS(int retrieved_ss_port, char ip[], int choice)
{
    int ss_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_sock < 0)
    {
        perror("[-] Socket error");
        // Handle error if needed
    }

    struct sockaddr_in ss_addr;
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(retrieved_ss_port);
    ss_addr.sin_addr.s_addr = inet_addr(ip); // Change this if required

    if (connect(ss_sock, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) == -1)
    {
        perror("SS connection error");
        close(ss_sock);
        // Handle error if needed
    }

    printf("Connected to SS at port: %d\n", retrieved_ss_port);

    // Perform operations with the connected SS if needed

    send(ss_sock, &choice, sizeof(choice), 0);
    // return;
    if (choice == 1) // read a file.
    {
        printf("Enter file name: ");
        char file[1024];
        scanf("%s", file);
        send(ss_sock, file, strlen(file), 0);
        char buffer[1024];
        bzero(buffer, 0);
        printf("Received data: ");
        // int ct=10;
        while (1)
        {
            // ct--;
            bzero(buffer, 1024);
            recv(ss_sock, buffer, sizeof(buffer), 0);
            printf("%s ", buffer);
            if (!strcmp(buffer, ""))
            {
                break;
            }
        }
        // recv(ss_sock, buffer, sizeof(buffer), 0);
        // printf("%s", buffer);
        printf("\n");
    }
    else if (choice == 2) // append on a file.
    {
        printf("Enter file name: ");
        char file[1024];
        scanf("%s", file);
        send(ss_sock, file, strlen(file), 0);
        char buffer[1024];
        // if (!writers)
        // {
        printf("Enter the data: ");
        // writers++;
        scanf("%s", buffer);
        send(ss_sock, buffer, strlen(buffer), 0);
        char response[1024];
        recv(ss_sock, response, sizeof(response), 0);
        printf("%s\n", response);

        // }
        // else
        // {
        //     printf("please connect again later\n");
        // }
    }
    else // get info.
    {
        printf("Enter file name: ");
        char file[1024];
        scanf("%s", file);
        send(ss_sock, file, strlen(file), 0);
        char buffer[1024];
        bzero(buffer, 0);
        recv(ss_sock, buffer, sizeof(buffer), 0);
        printf("%s\n", buffer);
    }

    close(ss_sock); // Close the connection when done
}

void Connect_to_NM(int nm_port, char ip[])
{
    int nm_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    struct sockaddr_in nm_addr;
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(nm_port);
    nm_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(nm_sock, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) == -1)
    {
        perror("NM connection error");
        close(nm_sock);
        exit(1);
    }



    // char resp[1024];
    // int bytes_received = recv(nm_sock, resp, sizeof(resp) - 1, 0);
    // if (bytes_received > 0)
    // {
    //     resp[bytes_received] = '\0'; // Null-terminate the received data
    //     // printf("Received Source Filename: %s\n", srcfilename);
    // }
    printf("Connection Successful\n");
    // char buffer[1024];
    // bzero(buffer, sizeof(buffer));
    // strcpy(buffer, "Hello NM, this is Client");

    // send(nm_sock, buffer, strlen(buffer), 0);

    while (1)
    {
        printf("1. Read\n2. Write\n3. Get info\n4. Create File\n5. Delete File\n6. Copy File\nEnter your choice: ");
        int choice;
        scanf("%d", &choice);
        char choice_str[1024];
        sprintf(choice_str, "%d", choice);
        choice_str[strlen(choice_str)] = '\0';
        send(nm_sock, choice_str, strlen(choice_str), 0);
        if (choice == 1)
        {
            char buffer[1024];
            printf("Enter Filename: ");
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            int retrieved_ss_port;
            recv(nm_sock, &retrieved_ss_port, sizeof(retrieved_ss_port), 0);
            // code for connecting to SS at this retrieved port
            if (retrieved_ss_port == -1)
            {
                printf("106: File not found\n");
                continue;
            }
            Connect_to_SS(retrieved_ss_port, storage_server.ip, choice);
        }
        else if (choice == 2)
        {
            char buffer[1024];
            printf("Enter Filename: ");
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            int retrieved_ss_port;
            recv(nm_sock, &retrieved_ss_port, sizeof(retrieved_ss_port), 0);

            Connect_to_SS(retrieved_ss_port, storage_server.ip, choice);

            // printf("someone is already there connect again later:)\n");

            // code for connecting to SS at this retrieved port
        }
        else if (choice == 3)
        {
            char buffer[1024];
            printf("Enter Filename: ");
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            int retrieved_ss_port;
            recv(nm_sock, &retrieved_ss_port, sizeof(retrieved_ss_port), 0);
            if (retrieved_ss_port == -1)
            {
                printf("106: File not found\n");
                continue;
            }
            Connect_to_SS(retrieved_ss_port, storage_server.ip, choice);

            // code for connecting to SS at this retrieved port
        }
        else if (choice == 4)
        {
            char buffer[1024];
            printf("Enter Filename: ");
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            char response[1024];
            recv(nm_sock, response, sizeof(response), 0);
            response[strlen(response)] = '\0';
            // printf("length = %d\n",strlen(response));
            printf("%s\n", response);
        }
        else if (choice == 5)
        {
            char buffer[1024];
            printf("Enter Filename: ");
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            char response[1024];
            recv(nm_sock, response, sizeof(response), 0);
            // printf("length = %d\n",strlen(response));
            printf("%s\n", response);
        }
        else if (choice == 6)
        {
            char buffer_1[1024];
            printf("Enter Source Filename: ");
            scanf("%s", buffer_1);
            send(nm_sock, buffer_1, strlen(buffer_1), 0);
            // char response_1[1024];
            // recv(nm_sock, response_1, sizeof(response_1), 0);
            // printf("length = %d\n",strlen(response));
            // printf("%s\n", response_1);
            // printf("hi\n");
            char buffer_2[1024];
            printf("Enter Destination Filename: ");
            scanf("%s", buffer_2);
            send(nm_sock, buffer_2, strlen(buffer_2), 0);
            char response[1024];
            recv(nm_sock, response, sizeof(response), 0);
            // printf("length = %d\n",strlen(response));
            printf("%s\n", response);
        }
        else
        {
        }
    }

    close(nm_sock);
}

int main()
{
    strcpy(naming_server.ip, "127.0.0.1");
    naming_server.nm_port = 5568; // Assuming your naming server runs on port 5566

    strcpy(storage_server.ip, "127.0.0.1");
    storage_server.ss_port = 5567;
    // Assuming your naming server runs on port 5566
    // int server = 0;
    // // Modify the port number for the storage server
    // // naming_server.client_port = 5568; // Assuming your storage server runs on port 5567
    // if (server)
    // {
    //     Connect_to_SS(storage_server.ss_port, storage_server.ip);
    // }
    // else
    // {
    Connect_to_NM(naming_server.nm_port, naming_server.ip);
    // }

    // perform_operation(naming_server, request);

    return 0;
}