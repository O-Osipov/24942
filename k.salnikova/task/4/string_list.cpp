#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace std;

struct Node {
    char* data;
    Node* next;
};

void appendNode(Node** head, Node** tail, const char* str) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == nullptr) {
        cerr << "Ошибка выделения памяти для узла" << endl;
        return;
    }
    
    size_t len = strlen(str);
    newNode->data = (char*)malloc(len + 1);
    if (newNode->data == nullptr) {
        cerr << "Ошибка выделения памяти для строки" << endl;
        free(newNode);
        return;
    }
    
    strcpy(newNode->data, str);
    newNode->next = nullptr;

    if (*head == nullptr) {
        *head = *tail = newNode;
    } else {
        (*tail)->next = newNode;
        *tail = newNode;
    }
}

void printList(Node* head) {
    Node* current = head;
    while (current != nullptr) {
        cout << current->data << endl;
        current = current->next;
    }
}

void freeList(Node* head) {
    Node* current = head;
    while (current != nullptr) {
        Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}

int main() {
    Node* head = nullptr;
    Node* tail = nullptr;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    while (true) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == nullptr) {
            break;
        }
        
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        
        if (len > 0 && buffer[0] == '.') {
            break;
        }
        
        appendNode(&head, &tail, buffer);
    }

    printList(head);
    
    freeList(head);
    
    return 0;
}

// g++ -o string_list string_list.cpp
// ./string_list
