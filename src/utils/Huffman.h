#ifdef HUFFMAN_ENABLED

#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <forward_list>
#include "../utils/Utils.h"

// Huffman code is adapted from
// https://cppsecrets.com/users/14739971109711010010097115971101054864103109971051084699111109/C00-Program-for-Huffman-Code.php

// The decoding part is adapted from
// https://iamshnoo.github.io/huffman/index.html?amp=1

// Encoding and decoding of Huffman trees are adapted from Lasse V. Karlsens reply on
// https://stackoverflow.com/questions/759707/efficient-way-of-storing-huffman-tree

struct MinHeapNode
{
    int value;
    unsigned frequency;
    MinHeapNode *lChild, *rChild;

    MinHeapNode(int value, unsigned frequency)
    {
        lChild = rChild = NULL;
        this->value = value;
        this->frequency = frequency;
    }
    MinHeapNode(int value, MinHeapNode* lChild, MinHeapNode* rChild)
    {
        this->lChild = lChild;
        this->rChild = rChild;
        this->value = value;
    }
};


//function to compare
struct compare
{
    bool operator()(MinHeapNode *l, MinHeapNode *r)
    {
        return (l->frequency > r->frequency);
    }
};

class Huffman {
public:
    void runHuffmanEncoding(const std::vector<int> &uncompressed, bool print);
    std::map<int, std::vector<std::pair<int, int>>> decodeLOL(struct MinHeapNode* root, std::vector<uint8_t> s);
    std::vector<std::pair<int, int>> decodeGOL(struct MinHeapNode* root, std::vector<uint8_t> s);
    void encodeTree();
    MinHeapNode* decodeTree();
    BitVecBuilder huffmanBuilder;
    BitVecBuilder treeBuilder;

    MinHeapNode* getRoot(){
        return minHeap.top();
    }

private:
    std::map<int, int> countFrequencies(const std::vector<int> &uncompressed);
    void constructTree(const std::map<int, int> &frequencies);
    void storeCodes(struct MinHeapNode *root, std::string str, bool print);
    void compress(BitVecBuilder &builder, const std::vector<int> &uncompressed);
    MinHeapNode* decodeTreeRec(BitReader &reader);
    void encodeTreeRec(MinHeapNode* node);

    std::map<int, std::string> codes;
    std::priority_queue<MinHeapNode *, std::vector<MinHeapNode *>, compare> minHeap;
};

#endif
