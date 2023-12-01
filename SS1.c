#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
// #include "helper.h"

int writers = 0;
// Server listens for Clients to connect. Inorder to connect, Client should have the ip address of Server and
// the corresponding port number, used in connect() function.

// pointers aren't used, as while sending the data, it'll be a SEG FAULT as it's a local address.
typedef struct SS_1
{
    char ip[24];
    int nm_port;
    int client_port;
    char accessible_paths[5][1024];
    int num_paths;
    char current_directory[1024];
} SS_1;

#define MAX_FILES 100

char **fileList;

char **getFilesInDirectory(const char *dirPath, int *numFiles)
{
    DIR *dir;
    struct dirent *ent;
    char **files = NULL;
    int count = 0;

    // Open the directory
    if ((dir = opendir(dirPath)) != NULL)
    {
        // Allocate memory for the array of strings
        files = (char **)malloc(MAX_FILES * sizeof(char *));

        // Read directory entries
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_REG)
            { // Check if it's a regular file
                // Allocate memory for the file name
                files[count] = (char *)malloc(strlen(ent->d_name) + 1);
                strcpy(files[count], ent->d_name);
                count++;

                // Check if we've reached the maximum number of files
                if (count >= MAX_FILES)
                {
                    break;
                }
            }
        }

        // Close the directory
        closedir(dir);
    }
    else
    {
        perror("Unable to open directory");
    }

    // Set the number of files
    *numFiles = count;

    return files;
}

int NM_port = 5566;
int Client_port;

void executeCommand(int commandID, char *filePath, char *destinationPath)
{
    switch (commandID)
    {
    case 1: // Create file command
        printf("Creating file: %s\n", filePath);
        // File creation logic
        FILE *file = fopen(filePath, "w");
        if (file == NULL)
        {
            printf("Error creating file!\n");
        }
        else
        {
            printf("File created successfully!\n");
            fclose(file);
        }
        break;
    case 2: // Delete file command
        printf("Deleting file: %s\n", filePath);
        if (remove(filePath) == 0)
        {
            printf("File deleted successfully!\n");
        }
        else
        {
            printf("Error deleting file!\n");
        }
        break;
    case 3: // Copy file command
        printf("Copying file from %s to %s\n", filePath, destinationPath);
        FILE *source = fopen(filePath, "r");
        if (source == NULL)
        {
            printf("Error opening source file!\n");
            break;
        }
        FILE *destination = fopen(destinationPath, "w");
        if (destination == NULL)
        {
            printf("Error creating destination file!\n");
            fclose(source);
            break;
        }
        char ch;
        while ((ch = fgetc(source)) != EOF)
        {
            fputc(ch, destination);
        }
        printf("File copied successfully!\n");
        fclose(source);
        fclose(destination);
        break;
    default:
        printf("Unknown command received.\n");
        break;
    }
}

void initialize_SS_1(SS_1 *ss_1, int nm_port, int client_port, int numpaths)
{
    bzero(ss_1->ip, 24);
    strcpy(ss_1->ip, "127.0.0.1");

    ss_1->nm_port = nm_port;
    ss_1->client_port = client_port;
    ss_1->num_paths = numpaths;

    // Get the current working directory
    if (getcwd(ss_1->current_directory, sizeof(ss_1->current_directory)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numpaths; i++)
    {
        bzero(ss_1->accessible_paths[i], 1024);
    }
}

void listen_for_NamingServer(SS_1 ss_1)
{
    int naming_server_sock, naming_server_len;
    struct sockaddr_in naming_server_addr;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("Socket error");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ss_1.nm_port);
    inet_pton(AF_INET, ss_1.ip, &server_addr.sin_addr);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind error");
        exit(1);
    }

    listen(server_sock, 1); // Listen for 1 connection
    printf("Listening for Naming Server on port %d...\n", ss_1.nm_port);
    naming_server_len = sizeof(naming_server_addr);
    naming_server_sock = accept(server_sock, (struct sockaddr *)&naming_server_addr, &naming_server_len);
    if (naming_server_sock == -1)
    {
        perror("Accept error");
        exit(1);
    }
    printf("Connected to Naming Server.\n");
    // char *buff="from ss to nm";
    // send(naming_server_sock, buff, sizeof(buff), 0);
    while (1)
    {
        // Handle communication with the Naming Server here
        char buffer[1024];
        recv(naming_server_sock, buffer, sizeof(buffer), 0);
        buffer[strlen(buffer)] = '\0';

        if (!strcmp(buffer, "1"))
        {
            char buffer[1024];
            printf("Enter Filename: ");
            scanf("%s", buffer);
            recv(naming_server_sock, buffer, strlen(buffer), 0);
            // printf("1");
        }
        if (!strcmp(buffer, "2"))
        {
            char filename[1024];
            recv(naming_server_sock, filename, sizeof(filename), 0);
            executeCommand(2, filename, NULL);
            // printf("2");
        }
        if (!strcmp(buffer, "3"))
        {
            char srcfilename[1024];
            recv(naming_server_sock, srcfilename, sizeof(srcfilename), 0);
            char destfilename[1024];
            recv(naming_server_sock, destfilename, sizeof(destfilename), 0);
            executeCommand(3, srcfilename, destfilename);
        }
        if (!strcmp(buffer, "4"))
        {
            int port;
            // printf("reached here\n");
            recv(naming_server_sock, &port, sizeof(port), 0);
            printf("%d\n", port);
        }

        // Close the client socket after processing a command
    }
    close(naming_server_sock);

    // Close the server socket when done
    close(server_sock);
}

// connecting to Naming Server
void connect_to_NM(SS_1 ss_1)
{
    int nm_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_sock == -1)
    {
        perror("Socket error");
        exit(1);
    }

    struct sockaddr_in nm_addr;
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = ss_1.nm_port;
    nm_addr.sin_addr.s_addr = inet_addr(ss_1.ip);

    if (connect(nm_sock, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) == -1)
    {
        perror("NM connection error");
        close(nm_sock);
        exit(1);
    }
    // initialization done, hence sending ss_1 data to NM.
    // send(nm_sock, &ss_1, sizeof(SS_1), 0);
    printf("Data sent to NM server succefully\n");
    char buffer[1024];
    recv(nm_sock, buffer, sizeof(buffer), 0);
    buffer[strlen(buffer) - 1] = '\0';
    // printf("%s", buffer);
    close(nm_sock);
    return;
    while (1)
    {
        printf("1. Create\n2. Delete\n3. Copy\nEnter your choice:");
        int choice;
        scanf("%d", &choice);
        char choice_str[1024];
        sprintf(choice_str, "%d", choice);
        choice_str[strlen(choice_str) - 1] = '\0';
        send(nm_sock, choice_str, strlen(choice_str), 0);
        if (choice == 1)
        {
            char buffer[1024];
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            break;
        }
        else if (choice == 2)
        {
            char buffer[1024];
            scanf("%s", buffer);
            send(nm_sock, buffer, strlen(buffer), 0);
            break;
        }
    }

    close(nm_sock);
}

void listen_for_clients(SS_1 ss_1)
{
    int server_sock;
    int client_sock, client_len;
    struct sockaddr_in client_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("Socket error");
        close(server_sock);
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ss_1.client_port);
    inet_pton(AF_INET, ss_1.ip, &server_addr.sin_addr);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind error");
        close(server_sock);
        exit(1);
    }
    while (1)
    {
        FILE *book = fopen("log.txt", "a+");
        listen(server_sock, 4);
        printf("Listening for clients on port %d...\n", ss_1.client_port);

        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1)
        {
            perror("Accept error");
            // continue;
        }

        printf("Client Connected\n");
        int choice = -1;
        // printf("here\n");
        recv(client_sock, &choice, sizeof(choice), 0);
        // printf("%d\n", choice);
        // return;
        if (choice == 1) // read oprn
        {

            char buffer[1024];
            bzero(buffer, 1024);
            recv(client_sock, buffer, sizeof(buffer), 0);
            // printf("%s\n", buffer);

            // char str[1024] = "hi this is the server";
            // send(client_sock, str, strlen(str), 0);
            FILE *fileptr;
            fileptr = fopen(buffer, "r");
            if (fileptr == NULL)
            {
                printf("Error opening source file!\n");
                char *resp = "101: Error opening source file";
                send(client_sock, resp, strlen(resp), 0);
                return;
            }

            char newbuffer[1024];
            size_t bytesRead;

            char line[1024];
            while (fgets(line, sizeof(line), fileptr) != NULL)
            {
                printf("%s\n", line);
                send(client_sock, line, strlen(line), 0);
            }

            // Once the file is fully sent, send a termination message
            char stopMessage[] = "STOP\nFile read successfully";
            send(client_sock, stopMessage, strlen(stopMessage), 0);
            fprintf(book, "%s read successfully at %d port\n", buffer, Client_port );
          fflush(book);

            fclose(fileptr);
        }
        else if (choice == 2) // append oprn
        {
            char filepath[1024];
            bzero(filepath, 1024);
            recv(client_sock, filepath, sizeof(filepath), 0);
            writers = 1;
            FILE *fileptr = fopen(filepath, "a+");
            if (fileptr == NULL)
            {
                printf("Error opening source file!\n");
                char *resp = "101: Error opening source file";
                send(client_sock, resp, strlen(resp), 0);
                return;
            }
            char buffer[1024];
            bzero(buffer, 1024);
            recv(client_sock, buffer, sizeof(buffer), 0);
            fwrite(buffer, 1, sizeof(buffer), fileptr);
            char *resp = "File Write Successful";
            send(client_sock, resp, strlen(resp), 0);
            fprintf(book, "%s written successfully at %d port\n", filepath , Client_port );
            fflush(book);
            fclose(fileptr);
        }
        else // Get size and permissions.
        {
            char filepath[1024];
            bzero(filepath, 1024);
            recv(client_sock, filepath, sizeof(filepath), 0);
            FILE *fileptr = fopen(filepath, "a+");
            if (fileptr == NULL)
            {
                printf("Error opening source file!\n");
                char *resp = "101: Error opening source file!";
                send(client_sock, resp, strlen(resp), 0);
                return;
            }
            struct stat fileStat;
            if (stat(filepath, &fileStat) == 0)
            {
                // printf("File size: %lld bytes\n", (long long)fileStat.st_size);
                // printf("File permissions (in octal): %o\n", fileStat.st_mode & 0777);
                char combinedInfo[1000]; // Adjust the size according to your needs
                snprintf(combinedInfo, sizeof(combinedInfo), "Size: %lld bytes | Permissions: %o",
                         (long long)fileStat.st_size, fileStat.st_mode & 0777);
                send(client_sock, combinedInfo, strlen(combinedInfo), 0);
            }
            else
            {
                printf("Error getting file information!\n");
                char *resp = " 105: Error getting file information!";
                send(client_sock, resp, strlen(resp), 0);
            }
        }
        close(client_sock);
    }
    close(server_sock);
}

void send_hearbeat()
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    int port = 5566;
    char *ip = "127.0.0.1";
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to Naming Server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    // char* msg="Connected";
    // send(sock, msg, strlen(msg), 0);
}

void add_new_server(int nm_port, int client_port, int numpaths)
{
    // printf("reached function\n");
    SS_1 ss_1;

    initialize_SS_1(&ss_1, nm_port, client_port, numpaths);
    listen_for_NamingServer(ss_1);
}

void ConnectingWithNM(SS_1 SS1, int new_port)
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    int port = 5566;
    char *ip = "127.0.0.1";
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to Naming Server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // send the port number of this SS to Naming Server
    // send(sock, SS1.current_directory, strlen(SS1.current_directory), 0);

    // char portStr[6]; // Assuming the port won't exceed 5 characters (+1 for null terminator)
    // snprintf(portStr, sizeof(portStr), "%d", new_port);
    // send(sock, portStr, strlen(portStr), 0);
    char combinedInfo[1030]; // To accommodate both directory and port with a delimiter
    snprintf(combinedInfo, sizeof(combinedInfo), "%s:%d", SS1.current_directory, new_port);
    send(sock, combinedInfo, strlen(combinedInfo), 0);
    printf("Connected to Naming Server.\n");

    close(sock);
}
int numFiles = 0;

int main(int argc, char *argv[])
{
    SS_1 SS1;
    Client_port = atoi(argv[1]);
    initialize_SS_1(&SS1, NM_port, Client_port, 5);
    // printf("Current working directory: %s\n", SS1.current_directory);
    fileList = (char **)malloc((MAX_FILES + 1) * sizeof(char *));
    if (fileList == NULL)
    {
        printf("Memory allocation failed for fileList\n");
        return 1;
    }

    fileList = getFilesInDirectory(SS1.current_directory, &numFiles);
    // printf("%d", numFiles);
    // return;
    // fileList[MAX_FILES] = NULL;
    // if (numFiles)
    // {
    ConnectingWithNM(SS1, Client_port);
    // }

    // if (fileList != NULL)
    // {
    //     printf("Files in the directory:\n");
    //     for (int i = 0; i < numFiles; i++)
    //     {
    //         printf("%s\n", fileList[i]);
    //         // free(fileList[i]); // Free the memory allocated for each file name
    //     }
    //     // free(fileList); // Free the memory allocated for the array of strings
    // }

    listen_for_clients(SS1);
    // }
}

// Adding new SS -> Creating a new thread prolly.s