#include "Utils.h"

// Source: https://stackoverflow.com/questions/27431029/binary-search-with-returned-index-in-stl
// (Response by Jonathan L.)
int Utils::BinarySearch(std::vector<int> v, int data) {
    auto it = std::lower_bound(v.begin(), v.end(), data);
    if (it == v.end() || *it != data) {
        return -1;
    } else {
        std::size_t index = std::distance(v.begin(), it);
        return index;
    }
}

// Source: https://www.softwaretestinghelp.com/doubly-linked-list-2/
// Modified

Node * Utils::insert_end(struct Node** head, int new_data)
{
    //allocate memory for node
    struct Node* newNode = new Node;

    struct Node* last = *head; //set last node value to head

    //set data for new node
    newNode->data = new_data;

    //new node is the last node , so set next of new node to null
    newNode->next = NULL;

    //check if list is empty, if yes make new node the head of list
    if (*head == NULL) {
        newNode->prev = NULL;
        *head = newNode;
        newNode->index=0;
        return newNode;
    }

//otherwise traverse the list to go to last node
    while (last->next != NULL)
        last = last->next;

//set next of last to new node
    last->next = newNode;
    newNode->index = last->index+1;

//set last to prev of new node
    newNode->prev = last;
    return newNode;
}

Node* Utils::forwardNode(struct Node* node, int skip){
    for(int i=0; i<skip; i++){
        node = node->next;
    }
    return node;
}


