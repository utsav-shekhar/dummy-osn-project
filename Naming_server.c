#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include "trie.h"
#include "convenience.h"

sem_t x, y;
pthread_t tid;
pthread_t writerthreads[100];
pthread_t readerthreads[100];
int readercount = 0;
int numFiles;
char *file_to_be_searched;
int portnum = -1;
int backupport = -1;
char *directory = NULL;
char *backup = NULL;

#define ERROR_OPENING_FILE 1
#define ERROR_CREATING_FILE 2
#define ERROR_DELETING_FILE 3
#define FILE_ALREADY_EXISTS 4
#define ERROR_GETTING_FILE_INFO 5
#define FILE_NOT_FOUND 6

typedef struct
{
  char ip[24];
  int NM_port;
  int client_port;
  char paths[5][1024];
  int num_paths;
} SS;

// typedef struct TrieNode
// {
//   struct TrieNode *children[26]; // Assuming English alphabet
//   int isEndOfWord;
//   int port;
// } TrieNode;
char **fileList;
#define MAX_FILES 100

int current_LRU_index = 0;
LRU circularArray[10];
LRU buffer_array[10];

int insertLRU(char *client_command)
{
  int port = 0;
  for (int i = 0; i < 10; i++)
  {
    if (!strcmp(circularArray[i].client_command, client_command))
    {
      printf("Cache Hit!\n");
      port = circularArray[i].port;
      // change the priority of commands.
      int copy = current_LRU_index;
      for (int j = 0; j < 10; j++)
      {
        strcpy(buffer_array[j].client_command, circularArray[copy].client_command);
        buffer_array[j].port = circularArray[copy].port;
        copy = (copy + 1) % 10;
      }
      for (int j = 0; j < 10; j++)
      {
        strcpy(circularArray[j].client_command, buffer_array[j].client_command);
        circularArray[j].port = buffer_array[j].port;
      }
      // return the file name.
      // return true;
    }
  }
  // strcpy( circularArray[ current_LRU_index ].client_command , client_command );
  // current_LRU_index = (current_LRU_index + 1) % 10; // Move to the next index in a circular manner
  // FILE *file = fopen("history.txt", "w");
  // if (file != NULL)
  // {
  //   for (int i = 0; i < 10; i++)
  //   {
  //     fprintf(file, "%s\n", circularArray[i].client_command);
  //     // printf("%s\n", circularArray[i].client_command);
  //   }
  //   fclose(file);
  // }
  // else
  // {
  //   printf("Error opening file history.txt for writing.\n");
  // }
  return port;
}

void insertIntoCircularArray(char client_command[1024], int port)
{
  for (int i = 0; i < 10; i++)
  {
    if (strcmp(circularArray[i].client_command, client_command) == 0 && circularArray[i].port == port)
    {
      printf("String '%s' with port %d is already in the array. Not inserting again.\n", client_command, port);
      return;
    }
  }

  // Insert the string into the circular array
  strcpy(circularArray[current_LRU_index].client_command, client_command);
  circularArray[current_LRU_index].port = port;
  current_LRU_index = (current_LRU_index + 1) % 10; // Move to the next index in a circular manner

  printf("String '%s' with port %d inserted into the circular array.\n", client_command, port);
  FILE *file = fopen("history.txt", "w");
  if (file != NULL)
  {
    for (int i = 0; i < 10; i++)
    {
      fprintf(file, "%s\n", circularArray[i].client_command);
      fflush(file);
    }
    fclose(file);
  }
  else
  {
    printf("Error opening file history.txt for writing.\n");
  }
}

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

TrieNode *root;
void executeCommand(int commandID, char *filePath, char *destinationPath, int sock)
{
  FILE *book = fopen("log.txt", "a+");
  // root = createNode();
  // create_backup_directory();
  if (portnum == -1)
  {
    printf("Storage server not connected\n");
    return;
  }
  struct stat path_stat;
  char new_filepath[1024];
  snprintf(new_filepath, sizeof(new_filepath), "%s/%s", directory, filePath);
  // printf("%s\n",new_filepath);
  char new_destpath[1024];
  snprintf(new_destpath, sizeof(new_destpath), "%s/%s", directory, destinationPath);
  // printf("%s\n",new_destpath);
  // printf("--->%s\n",directory);
  switch (commandID)
  {
  case 1: // Create file command
    if (stat(new_filepath, &path_stat) != 0 && searchTrie(root, filePath) != 0)
    {
      // File or directory doesn't exist, proceed to create
      if (new_filepath[strlen(new_filepath) - 1] == '/')
      {
        // Path indicates a directory
        if (mkdir(new_filepath, 0777) == -1)
        {
          printf("Error creating directory!\n");
          char *resp_msg = "102: Error creating directory!";
          send(sock, resp_msg, strlen(resp_msg), 0);
          // Handle error if needed
        }
        else
        {
          // insertTrie(root, filePath);
          // printf("Inserted to trie\n");
          printf("Directory created successfully!\n");
          char *resp_msg = "[ACK] Directory created successfully!";
          send(sock, resp_msg, strlen(resp_msg), 0);
          fprintf(book, "%s created successfully at %d port\n", filePath, portnum);
          fflush(book);
        }
        if (backupport == -1)
        {
          break;
        }
        char backupPath[1024];
        snprintf(backupPath, sizeof(backupPath), "%s/%s", backup, filePath);

        if (mkdir(backupPath, 0777) == -1)
        {
          printf("Error creating directory in backup directory!\n");
          // Handle error if needed
        }
        else
        {
          printf("Directory created successfully in backup directory!\n");
          // Log the creation inside the global file
          fprintf(book, "%s created successfully in backup directory at %d port\n", filePath, backupport);
          fflush(book);
        }
      }
      else
      {
        // Path indicates a file
        FILE *file = fopen(new_filepath, "w");
        if (file == NULL)
        {
          printf("Error creating file!\n");
          char *resp_msg = "102: Error creating file!";
          send(sock, resp_msg, strlen(resp_msg), 0);
          // Handle error if needed
        }
        else
        {
          // insertTrie(root, filePath);
          // printf("Inserted to trie\n");
          printf("File created successfully!\n");
          char *resp_msg = "[ACK] File creation successful!";
          send(sock, resp_msg, strlen(resp_msg), 0);
          fprintf(book, "%s created successfully at %d port\n", filePath, portnum);
          fflush(book);
          fclose(file);
        }
        if (backupport == -1)
        {
          break;
        }
        char backupPath[1024];
        snprintf(backupPath, sizeof(backupPath), "%s/%s", backup, filePath);

        FILE *backupFile = fopen(backupPath, "w");
        if (backupFile == NULL)
        {
          printf("Error creating file in backup directory!\n");
          // Handle error if needed
        }
        else
        {
          printf("File created successfully in backup directory!\n");
          // Log the creation inside the global file
          fprintf(book, "%s created successfully in backup directory at %d port\n", filePath, backupport);
          fflush(book);
          fclose(backupFile);
        }
      }
    }
    else
    {
      char *resp_msg = "104: File/Directory already exists!";
      send(sock, resp_msg, strlen(resp_msg), 0);
      printf("File or directory already exists!\n");
    }
    break;
  case 2: // Delete file command
    printf("Deleting file: %s\n", new_filepath);
    if (remove(new_filepath) == 0)
    {
      // removeTrie(root, filePath, 0);
      printf("File deleted successfully!\n");
      char *resp_msg = "[ACK] File deletion successful!";
      send(sock, resp_msg, strlen(resp_msg), 0);
      fprintf(book, "%s deleted successfully at %d port\n", filePath, portnum);
      fflush(book);
    }
    else
    {
      printf("Error deleting file!\n");
      char *resp_msg = "103: Error deleting file!";
      send(sock, resp_msg, strlen(resp_msg), 0);
    }
    break;
  case 3: // Copy file command
    printf("Copying file from %s to %s\n", filePath, destinationPath);
    FILE *source = fopen(new_filepath, "r");
    if (source == NULL)
    {
      printf("Error opening source file!\n");
      char *resp_msg = "101: Error opening source file!";
      send(sock, resp_msg, strlen(resp_msg), 0);
      break;
    }
    FILE *destination = fopen(new_destpath, "w");
    if (destination == NULL)
    {
      printf("Error creating destination file!\n");
      char *resp_msg = "102: Error creating destination file!";
      send(sock, resp_msg, strlen(resp_msg), 0);
      fclose(source);
      break;
    }
    char ch;
    while ((ch = fgetc(source)) != EOF)
    {
      fputc(ch, destination);
    }
    printf("File copied successfully!\n");
    char *resp_msg = "[ACK] File copy successful!";
    send(sock, resp_msg, strlen(resp_msg), 0);
    fprintf(book, "Copied %s to %s successfully at %d port\n", filePath, destinationPath, portnum);
    fflush(book);
    fclose(source);
    fclose(destination);
    break;
  default:
    printf("Unknown command received.\n");
    break;
  }
}
void *server_command_execute(void *new_socket)
{
  int client_sock = *(int *)new_socket;
  // while (1)
  // {
  // Add your logic for handling the client connection here
  char buffer[1030]; // Allowing space for both directory and port with a delimiter
  bzero(buffer, 1030);
  recv(client_sock, buffer, sizeof(buffer), 0);

  buffer[strlen(buffer)] = '\0';

  // print buffer
  // printf("%s\n", buffer);

  // Splitting the received data using the delimiter (':' in this case)
  char *token = strtok(buffer, ":");
  // directory = token;
  if (backup == NULL)
  {
    backup = malloc(strlen(token) + 1);
    strcpy(backup, token);
  }
  directory = malloc(strlen(token) + 1); // Allocate memory for directory
  if (directory != NULL)
  {
    strcpy(directory, token);
  }
  token = strtok(NULL, ":");
  char *portstr = token;

  // printf("Directory: %s\n", directory);
  // printf("Port String: %s\n", portstr);
  if (portnum == -1)
  {
    backupport = portnum;
  }
  portnum = atoi(portstr);

  // printf("Port Number: %d\n", portnum);
  // printf("%d\n",portnum);
  // fileList = getFilesInDirectory(buffer, &numFiles);
  // printf("%d\n", numFiles);

  // update_tries(buffer);

  fileList = getFilesInDirectory(buffer, &numFiles);
  // printf("%d\n", numFiles);
  for (int i = 0; i < numFiles; ++i)
  {
    // printf("%s\n", fileList[i]);
    insertTrie(root, fileList[i], portnum);
    // printf("Inserted %d\n", i + 1);
  }

  printf("SS Connected\n");

  // Creating a thread that will execute the command so that the nm is always listening
  // }
  close(client_sock); // Close the connection for now, you need to add your logic here
}

void listen_for_ss(int port, char *ip)
{
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  int server_sock, client_sock;

  int Client_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (Client_sock < 0)
  {
    perror(" [-] Socket error");
    exit(1);
  }

  printf("[+] Naming server socket created for SS.\n");
  // fprintf(globalfile, "%s deleted successfully\n", filePath);

  struct sockaddr_in Client_addr;
  memset(&Client_addr, '\0', sizeof(Client_addr));
  Client_addr.sin_family = AF_INET;
  Client_addr.sin_port = htons(port);
  Client_addr.sin_addr.s_addr = inet_addr(ip);

  if (bind(Client_sock, (struct sockaddr *)&Client_addr, sizeof(Client_addr)) < 0)
  {
    perror(" [-] Bind error for SS");
    exit(1);
  }

  printf("[+] Bind to the port number: %d\n", port);
  listen(Client_sock, 1); // Listen for 1 connection
  addr_size = sizeof(client_addr);

  // Acceping new connections in while loop

  while (1)
  {
    client_sock = accept(Client_sock, (struct sockaddr *)&client_addr, &addr_size);
    if (client_sock >= 0)
    {
      printf("[+] SS connected.\n");
      // char **fileList;
      // int numFiles;
      // while(numFiles==0){}
      // printf("%d\n", numFiles);

      // for (int i = 0; i < numFiles; ++i)
      // {
      //   insertTrie(root, fileList[i]);
      //   printf("Inserted %d\n", i + 1);
      // }
      int *new_socket = malloc(sizeof(int));
      *new_socket = client_sock;
      // printf("%d\n", numFiles);
      // if (fileList != NULL)
      // {
      //   printf("Files in the directory:\n");
      //   for (int i = 0; i < numFiles; i++)
      //   {
      //     printf("%s\n", fileList[i]);
      //     free(fileList[i]); // Free the memory allocated for each file name
      //   }
      //   // free(fileList); // Free the memory allocated for the array of strings
      // }
      pthread_t client_command_execute_thread;
      if (pthread_create(&client_command_execute_thread, NULL, server_command_execute, (void *)new_socket) < 0)
      {
        perror("could not create thread");
        free(new_socket);
      }

      pthread_detach(client_command_execute_thread);
    }
  }
}

void update_tries(char *path)
{
  TrieNode *root = createNode();
}

void *client_command_execute(void *new_socket)
{
  // root = createNode();
  // int listSize = getListSize(fileList);

  int client_sock = *(int *)new_socket;
  while (1)
  {
    // Add your logic for handling the client connection here
    char buffer[1024];
    bzero(buffer, 1024);
    recv(client_sock, buffer, sizeof(buffer), 0);

    // Creating a thread that will execute the command so that the nm is always listening

    buffer[strlen(buffer)] = '\0';
    // printf("%s-\n", buffer);
    if (!strcmp(buffer, "1"))
    {
      // recv(client_sock, filename, sizeof(filename), 0);
      // if(searchTrie(root,filename))
      char filename[1024];
      int bytes_received = recv(client_sock, filename, sizeof(filename) - 1, 0);
      if (bytes_received > 0)
      {
        filename[bytes_received] = '\0'; // Null-terminate the received data
        // printf("Received Source Filename: %s\n", srcfilename);
      }
      file_to_be_searched = filename;
      int k = insertLRU(file_to_be_searched);
      if (k != 0)
      {
        printf("File Found using Caching\n");
        int port = k;
        send(client_sock, &port, sizeof(port), 0);
      }
      else if (searchTrie(root, file_to_be_searched) != 0)
      {
        // for (int i = 0; i < 10; i++)
        // {
        //   printf("%s\n", circularArray[i].client_command);
        // }
        // printf("File found\n");
        int send_port = searchTrie(root, file_to_be_searched);
        insertIntoCircularArray(file_to_be_searched, send_port);
        send(client_sock, &send_port, sizeof(send_port), 0);
      }
      else
      {
        // printf("search = %d\n", searchTrie(root, file_to_be_searched));
        // printf("File not found ok\n");
        int resp = -1;
        send(client_sock, &resp, sizeof(resp), 0);
      }
    }
    if (!strcmp(buffer, "2"))
    {
      char filename[1024];
      int bytes_received = recv(client_sock, filename, sizeof(filename) - 1, 0);
      if (bytes_received > 0)
      {
        filename[bytes_received] = '\0'; // Null-terminate the received data
        // printf("Received Source Filename: %s\n", srcfilename);
      }
      // int send_port = 5567;
      file_to_be_searched = filename;
      int send_port;
      int k = insertLRU(filename);
      if (k != 0)
      {
        printf("File Found using Caching\n");
        send_port = k;
        send(client_sock, &send_port, sizeof(send_port), 0);
      }
      else if (searchTrie(root, file_to_be_searched) != 0)
      {
        // printf("File found\n");
        int send_port = searchTrie(root, file_to_be_searched);
        insertIntoCircularArray(filename, send_port);
        send(client_sock, &send_port, sizeof(send_port), 0);
      }
      else
      {
        // printf("search = %d\n", searchTrie(root, file_to_be_searched));
        int resp = portnum;
        send(client_sock, &resp, sizeof(resp), 0);
      }
    }
    if (!strcmp(buffer, "3"))
    {
      // recv(client_sock, filename, sizeof(filename), 0);
      // if(searchTrie(root,filename))
      char filename[1024];
      int bytes_received = recv(client_sock, filename, sizeof(filename) - 1, 0);
      if (bytes_received > 0)
      {
        filename[bytes_received] = '\0'; // Null-terminate the received data
        // printf("Received Source Filename: %s\n", srcfilename);
      }
      file_to_be_searched = filename;
      int send_port;
      int k = insertLRU(filename);
      if (k != 0)
      {
        printf("File Found using Caching\n");
        send_port = k;
        send(client_sock, &send_port, sizeof(send_port), 0);
      }
      else if (searchTrie(root, file_to_be_searched) != 0)
      {
        // for (int i = 0; i < 10; i++)
        // {
        //   printf("%s\n", circularArray[i].client_command);
        // }
        // printf("File found\n");
        int send_port = searchTrie(root, file_to_be_searched);
        insertIntoCircularArray(file_to_be_searched, send_port);
        send(client_sock, &send_port, sizeof(send_port), 0);
      }
      else
      {
        // printf("search = %d\n", searchTrie(root, file_to_be_searched));
        // printf("File not found ok\n");
        int resp = -1;
        send(client_sock, &resp, sizeof(resp), 0);
      }
    }
    if (!strcmp(buffer, "4"))
    {
      char filename[1024];
      int bytes_received = recv(client_sock, filename, sizeof(filename) - 1, 0);
      if (bytes_received > 0)
      {
        filename[bytes_received] = '\0'; // Null-terminate the received data
        // printf("Received Source Filename: %s\n", srcfilename);
      }

      // filename[strlen(filename)]='\0';
      insertTrie(root, filename, portnum);
      // printf("inserted to trie\n");
      // printf("%s", filename);
      executeCommand(1, filename, NULL, client_sock);
      // printf("1");
    }
    if (!strcmp(buffer, "5"))
    {
      char filename[1024];
      int bytes_received = recv(client_sock, filename, sizeof(filename) - 1, 0);
      if (bytes_received > 0)
      {
        filename[bytes_received] = '\0'; // Null-terminate the received data
        // printf("Received Source Filename: %s\n", srcfilename);
      }
      // printf("%s", filename);
      executeCommand(2, filename, NULL, client_sock);
      removeTrie(root, filename, 0);
      // printf("removed from trie\n");
      // printf("2");
    }
    if (!strcmp(buffer, "6"))
    {
      char srcfilename[1024];
      int bytes_received_src = recv(client_sock, srcfilename, sizeof(srcfilename) - 1, 0);
      if (bytes_received_src > 0)
      {
        srcfilename[bytes_received_src] = '\0'; // Null-terminate the received data
        // printf("Received Source Filename: %s\n", srcfilename);
      }

      char destfilename[1024];
      int bytes_received_dest = recv(client_sock, destfilename, sizeof(destfilename) - 1, 0);
      if (bytes_received_dest > 0)
      {
        destfilename[bytes_received_dest] = '\0'; // Null-terminate the received data
        // printf("Received Destination Filename: %s\n", destfilename);
      }

      executeCommand(3, srcfilename, destfilename, client_sock);
    }
    // break;
  }
  close(client_sock); // Close the connection for now, you need to add your logic here
}

void listen_for_clients(int client_port, char *ip)
{
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  int server_sock, client_sock;

  int Client_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (Client_sock < 0)
  {
    perror("[-] Socket error");
    exit(1);
  }

  printf("[+] Naming server socket created for Clients.\n");

  struct sockaddr_in Client_addr;
  memset(&Client_addr, '\0', sizeof(Client_addr));
  Client_addr.sin_family = AF_INET;
  Client_addr.sin_port = htons(client_port);
  Client_addr.sin_addr.s_addr = inet_addr(ip);

  if (bind(Client_sock, (struct sockaddr *)&Client_addr, sizeof(Client_addr)) < 0)
  {
    perror("[-] Bind error for Client");
    exit(1);
  }

  printf("[+] Bind to the port number: %d\n", client_port);
  listen(Client_sock, 1); // Listen for 1 connection
  addr_size = sizeof(client_addr);

  // Acceping new connections in while loop

  while (1)
  {
    client_sock = accept(Client_sock, (struct sockaddr *)&client_addr, &addr_size);
    if (client_sock >= 0)
    {
      printf("[+] Client connected.\n");
      // char*ack="Connection Successful";
      // send(Client_sock, ack, strlen(ack), 0);

      int *new_socket = malloc(sizeof(int));
      *new_socket = client_sock;

      pthread_t client_command_execute_thread;
      if (pthread_create(&client_command_execute_thread, NULL, client_command_execute, (void *)new_socket) < 0)
      {
        perror("could not create thread");
        free(new_socket);
      }

      pthread_detach(client_command_execute_thread);
    }
  }
}

void *ss_wrapper(void *arg)
{
  // Adjust this part to receive necessary arguments if needed
  listen_for_ss(5566, "127.0.0.1");
  return NULL;
}

void *clients_wrapper(void *arg)
{
  // Adjust this part to receive necessary arguments if needed
  listen_for_clients(5568, "127.0.0.1");
  return NULL;
}

int main()
{
  root = createNode();
  char *ip = "127.0.0.1";
  int port = 5566;
  int client_port = 5568;
  int storage_server_port = 5566;
  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  // char buffer[1024];
  int n;

  pthread_t storage_server_thread, client_thread;

  // Create thread for storage server
  if (pthread_create(&storage_server_thread, NULL, ss_wrapper, NULL) != 0)
  {
    perror("Error creating storage server thread");
    exit(1);
  }

  // Create thread for client connections
  if (pthread_create(&client_thread, NULL, clients_wrapper, NULL) != 0)
  {
    perror("Error creating client thread");
    exit(1);
  }

  // Join threads (if needed)
  pthread_join(storage_server_thread, NULL);
  pthread_join(client_thread, NULL);

  return 0;
}