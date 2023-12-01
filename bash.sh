#!/bin/bash

# Compile StorageServerModule
gcc SS1.c -pthread -o s

# Compile NamingServerModule
gcc Naming_server.c -pthread -o n

# Compile clientfunctions
gcc client.c -o client

# Provide execution permissions to the compiled binaries
chmod +x s n client
