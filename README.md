--> Assumptions we've made:
    - Files that are created should not have dots in the name of the file, and the name of the file should be in Lower case.

    - If the file is empty, the data read will be "STOP" ( for ACK ).

    - Once a file is created and any of the Read, Write, or Get info operations are performed on it, It'll get added to the LRU array.
      Even though the file gets deleted, it'll remain in the LRU array but the data read will be nothing.

    - While writing into a file, no spaces should be present.
    
    - Multiple writers can enter a file, but the data will only be written by the first one to write. If a tie occurs, the first one to 
      initialize data will be written. If multiple readers try to access a file, all of them can connect to the same port simultaneously. But a reader can't access it if another is reading.
    
    - If a server crashes in between it should be restarted at the same port otherwise, NM should be started again.

    - For the Copy operation, the two file names should be given in their absolute paths and the destination file name should have 
    the file name ( not the location only ).

    - All the changes are reflected on the current Storage server.

--> Error codes for the mentioned scenarios:
    
    - ERROR_OPENING_FILE: 101
    
    - ERROR_CREATING_FILE: 102
    
    - ERROR_DELETING_FILE: 103
    
    - FILE_ALREADY_EXISTS: 104
    
    - ERROR_GETTING_FILE_INFO: 105
    
    - FILE_NOT_FOUND: 106
