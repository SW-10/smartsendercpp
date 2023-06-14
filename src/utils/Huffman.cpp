#include "Huffman.h"
#include <queue>

void Huffman::runHuffmanEncoding(const std::vector<int> &uncompressed, bool print) {

    // Count frequencies of the elements in the uncompressed list
    std::map<int, int> frequencies = countFrequencies(uncompressed);

    // Run Huffman until there is only one node in the tree that contains all other nodes
    constructTree(frequencies);

    // Store the found codes in a map
    storeCodes(minHeap.top(), "", print);

    // Compress the data using the found codes
    compress(huffmanBuilder, uncompressed);
}

std::map<int, int> Huffman::countFrequencies(const std::vector<int> &uncompressed){
    std::map<int, int> frequencies;
    for(const auto elem : uncompressed){
        auto it = frequencies.find(elem);
        if ( it == frequencies.end() ) {
            // not found
            frequencies[elem] = 1;
        } else {
            frequencies[elem]++;
        }
    }
    return frequencies;
}

void Huffman::constructTree(const std::map<int, int> &frequencies){
    // Push all elements into the priority queue
    // The most frequent element is received in constant time
    for (std::map<int,int>::const_iterator iter = frequencies.begin(); iter != frequencies.end(); ++iter){
        minHeap.push(new MinHeapNode(iter->first, iter->second));
    }

    struct MinHeapNode *lChild, *rChild, *top;
    while (minHeap.size() != 1)
    {
        // Extract the two nodes with the highest frequency
        lChild = minHeap.top();
        minHeap.pop();
        rChild = minHeap.top();
        minHeap.pop();

        top = new MinHeapNode(-1, lChild->frequency + rChild->frequency);

        top->lChild = lChild;
        top->rChild = rChild;

        minHeap.push(top);
    }
}

void Huffman::storeCodes(struct MinHeapNode *root, std::string str, bool print)
{
    if (!root)
        return;

    if (root->value != -1){
        if (print) std::cout << root->value << ": " << str << std::endl;
        codes[root->value] = str;
    }

    storeCodes(root->lChild, str + "0", print);
    storeCodes(root->rChild, str + "1", print);
}

void Huffman::compress(BitVecBuilder &builder, const std::vector<int> &uncompressed){
    std::string compressed = "";
//    for(int i = 0; i < uncompressed.size(); i++){
    for(auto val = uncompressed.begin(); val != uncompressed.end(); ++val ) {
        auto it = codes.find(*val);
        if ( it == codes.end() ) {
            // not found
            std::cout<<"Element " << *val << " not found" << std::endl;
        } else {
//            std::cout << "Code for " << *i << " is " << it->second << std::endl;
            compressed += it->second;

            // Loop through the chars of the code and add each to the bitvecbuilder
            for(const auto c : it->second){
                switch (c) {
                    case '0':
                        appendAZeroBit(&builder);
                        break;
                    case '1':
                        appendAOneBit(&builder);
                        break;
                }
            }
        }
    }

    builder.bytes.emplace_back(builder.currentByte);
//    std::cout << "Compressed string: " << compressed << std::endl;
}

void Huffman::encodeTree() {
//    std::cout << "Tree: ";
    encodeTreeRec(getRoot());
//    std::cout << std::endl;
    treeBuilder.bytes.emplace_back(treeBuilder.currentByte);
}

void Huffman::encodeTreeRec(MinHeapNode* node){
    if (node->lChild==NULL && node->rChild==NULL){
        appendAOneBit(&treeBuilder);
//        std::cout << "1 ";
        appendBits(&treeBuilder, node->value, 32);
//        std::cout << "[" << node->value << "] ";
    } else {
        appendAZeroBit(&treeBuilder);
//        std::cout << "0 ";
        encodeTreeRec(node->lChild);
        encodeTreeRec(node->rChild);
    }
}

MinHeapNode* Huffman::decodeTree() {
    BitReader q(treeBuilder.bytes, treeBuilder.bytes.size());
    return decodeTreeRec(q);
}

MinHeapNode* Huffman::decodeTreeRec(BitReader &reader){
    if (readBit(&reader) == 1)
    {
        int32_t val = readBits(&reader, 32);
        return new MinHeapNode(val, NULL, NULL);
    }
    else
    {
        MinHeapNode* leftChild = decodeTreeRec(reader);
        MinHeapNode* rightChild = decodeTreeRec(reader);
        return new MinHeapNode(0, leftChild, rightChild);
    }
}


std::vector<std::pair<int, int>> Huffman::decodeGOL(struct MinHeapNode* root, std::vector<uint8_t> s){
    std::vector<std::pair<int, int>> GOLreconstructed;
    BitReader bitReader(s, s.size());
    struct MinHeapNode* curr = root;

    bool pairCompleted = true;
    std::pair<int,int> pair;
    while(true)
    {
        bool bit = readBit(&bitReader);
        if (bit == 0)
            curr = curr->lChild;
        else
            curr = curr->rChild;

        // reached leaf node
        if (curr->lChild==NULL && curr->rChild==NULL)
        {

            if(pairCompleted){
                pair.first = curr->value;
                pairCompleted = false;
            } else {
                pair.second = curr->value;
                pairCompleted = true;
                GOLreconstructed.emplace_back(pair);
            }

            if(curr->value == -3){
                break;
            }
            curr = root;
        }
    }
    return GOLreconstructed;
}


std::map<int, std::vector<std::pair<int, int>>> Huffman::decodeLOL(struct MinHeapNode* root, std::vector<uint8_t> s)
{
    std::map<int, std::vector<std::pair<int, int>>> LOLreconstructed;
    std::vector<int> firstTimestamps;
    int currentID = 1;
    BitReader bitReader(s, s.size());
    struct MinHeapNode* curr = root;

    bool pairCompleted = true;
    std::pair<int,int> pair;
    bool readyForFirstTS = true;
    while(true)
    {
        bool bit = readBit(&bitReader);
        if (bit == 0)
            curr = curr->lChild;
        else
            curr = curr->rChild;

        // reached leaf node
        if (curr->lChild==NULL && curr->rChild==NULL)
        {
            if(curr->value == -3){
                break;
            }
            if(curr->value == -2){
                currentID++;
                readyForFirstTS = true;
            } else if (readyForFirstTS){
//                std::cout << "First timestamp: " << curr->value << std::endl;
                firstTimestamps.emplace_back(curr->value);
                readyForFirstTS = false;
            } else {
                if(pairCompleted){
                    pair.first = curr->value;
                    pairCompleted = false;
                } else {
                    pair.second = curr->value;
                    pairCompleted = true;
                    LOLreconstructed[currentID].emplace_back(pair);
                }
            }

            curr = root;
        }
    }
    return LOLreconstructed;
}