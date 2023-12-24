#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

typedef struct TrieNode
{
    struct TrieNode *children[26]; // Assuming English alphabet
    int isEndOfWord;
    int port;
} TrieNode;

TrieNode *createNode()
{
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node)
    {
        for (int i = 0; i < 26; i++)
        {
            node->children[i] = NULL;
        }
        node->isEndOfWord = 0;
    }
    return node;
}

// Function to insert a string into the trie
void insertTrie(TrieNode *root, char *word, int ports)
{
    TrieNode *current = root;

    // printf("the assigned port : %d\n", ports);
    for (int i = 0; word[i] != '\0'; i++)
    {
        int index = tolower((unsigned char)word[i]) - 'a';

        if (index < 0 || index >= 26)
        {
            // fprintf(stderr, "Invalid character in the word: %c\n", word[i]);
            return;
        }

        if (!current->children[index])
        {
            current->children[index] = createNode();
        }

        current = current->children[index];
    }

    current->isEndOfWord = 1;
    current->port = ports;
    // printf("Inserted word: %s\n", word);
}

int removeTrie(TrieNode *root, char *word, int depth)
{
    if (!root)
    {
        return 0;
    }

    if (word[depth] == '\0')
    {
        if (root->isEndOfWord)
        {
            root->isEndOfWord = 0;

            // Check if the current node has no children
            int isEmpty = 1;
            for (int i = 0; i < 260; i++)
            {
                if (root->children[i])
                {
                    isEmpty = 0;
                    break;
                }
            }

            // If it has no children, it can be deleted
            if (isEmpty)
            {
                free(root);
                root = NULL;
            }

            return 1; // Word removed successfully
        }
        else
        {
            return 0; // Word not found
        }
    }

    int index = word[depth] - 'a';
    if (removeTrie(root->children[index], word, depth + 1))
    {
        // If the child node is deleted, remove the link from the parent
        root->children[index] = NULL;

        // Check if the current node has no children
        int isEmpty = 1;
        for (int i = 0; i < 260; i++)
        {
            if (root->children[i])
            {
                isEmpty = 0;
                break;
            }
        }

        // If it has no children and is not the end of a word, it can be deleted
        if (isEmpty && !root->isEndOfWord)
        {
            free(root);
            root = NULL;
            return 1; // Node removed successfully
        }

        return 0; // Node not removed (still has children or is the end of a word)
    }

    return 0; // Word not found
}
// Function to search for a string in the trie
int searchTrie(TrieNode *root, char *word)
{
    TrieNode *current = root;
    for (int i = 0; word[i] != '\0'; i++)
    {
        int index = word[i] - 'a';
        if (!current->children[index])
        {
            return 0; // Word not found
        }
        current = current->children[index];
    }
    // if()
    printf("current ka port: %d\n", current->port);
    if ((current != NULL && current->isEndOfWord))
    {

        return current->port;
    }
    else
    {
        return 0;
    }
}

// Function to free the memory allocated for the trie
void freeTrie(TrieNode *root)
{
    if (root)
    {
        for (int i = 0; i < 260; i++)
        {
            freeTrie(root->children[i]);
        }
        free(root);
    }
}
