#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>
#include "bitreader.h"
struct Node {
    int data;
    int index;
    struct Node* next = NULL;
    struct Node* prev = NULL;
};

class Utils {
public:
    // Source: https://stackoverflow.com/questions/27431029/binary-search-with-returned-index-in-stl
    // (Response by Jonathan L.)
    static int BinarySearch(std::vector<int> v, int data);

    static Node * insert_end(Node **head, int new_data);

    static Node *forwardNode(Node *node, int skip);
};



