#include <iostream>
#include <sstream>
#include "models/Gorilla.h"
#include "managers/ConfigManager.h"
#include "managers/ReaderManager.h"
#include "utils/Huffman.h"

#ifndef NDEBUG
#include "doctest.h"
#include "utils/Huffman.h"
// This function is required by Doctest to make it possible to have asserts inside the code.
// The function is passed to the Doctest context in our main function.
// Source:
// https://github.com/doctest/doctest/blob/master/doc/markdown/assertions.md - Example is found
// under "Using asserts out of a testing context":
// https://github.com/doctest/doctest/blob/master/examples/all_features/asserts_used_outside_of_tests.cpp
static void handler(const doctest::AssertData& ad) {
    using namespace doctest;

    // uncomment if asserts will be used in a multi-threaded context
    // std::lock_guard<std::mutex> lock(g_mut);

    // here we can choose what to do:
    // - log the failed assert
    // - throw an exception
    // - call std::abort() or std::terminate()

    std::cout << Color::LightGrey << skipPathFromFilename(ad.m_file) << "(" << ad.m_line << "): ";
    std::cout << Color::Red << failureString(ad.m_at) << ": ";

    // handling only normal (comparison and unary) asserts - exceptions-related asserts have been skipped
    if(ad.m_at & assertType::is_normal) {
        std::cout << Color::Cyan << assertString(ad.m_at) << "( " << ad.m_expr << " ) ";
        std::cout << Color::None << (ad.m_threw ? "THREW exception: " : "is NOT correct!\n");
        if(ad.m_threw)
            std::cout << ad.m_exception;
        else
            std::cout << "  values: " << assertString(ad.m_at) << "( " << ad.m_decomp << " )";
    } else {
        std::cout << Color::None << "an assert dealing with exceptions has failed!";
    }

    std::cout << std::endl;
}
#endif

std::vector<int> read_fields_as_integers_exclude_first_column(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << file_path << std::endl;
        return {};
    }

    std::string line;
    std::string field;
    std::vector<int> fields;
    bool first_column = true;
    bool first_row = true;
    int first_column_value;

    while (std::getline(file, line)) {
        if (first_row) {
            first_row = false;
            continue;
        }

        std::istringstream line_stream(line);
        first_column = true;
        while (std::getline(line_stream, field, ',')) {
            if (first_column) {
                try {
                    first_column_value = std::stoi(field);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Invalid integer value: " << field << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Error: Integer value out of range: " << field << std::endl;
                }
                first_column = false;
            } else if (!field.empty()) {
                fields.push_back(first_column_value);
            }
        }
    }

    file.close();
    return fields;
}

int main(int argc, char *argv[]) {

#ifndef NDEBUG
   	//Doctest things
	doctest::Context context;
    // sets the context as the default one - so asserts used outside of a testing context do not
    // crash
    context.setAsDefaultForAssertsOutOfTestCases();

    // set a handler with a signature: void(const doctest::AssertData&)
    // without setting a handler we would get std::abort() called when an assert fails
    context.setAssertHandler(handler);

    int res = context.run();
	int client_stuff_return_code = 0;
#endif
    Timekeeper *timekeeper = new Timekeeper;

    std::string path = "moby.cfg";
    ReaderManager readerManager(path, *timekeeper);
    readerManager.runCompressor();


    std::string file_path = "../cobham_hour_reloaded.csv";
    std::vector<int> allTimestamps = read_fields_as_integers_exclude_first_column(file_path);

    Huffman huffmanLOL;
    huffmanLOL.runHuffmanEncoding(allTimestamps, false);
    huffmanLOL.encodeTree();

    int temp = 0;

    temp += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();


    std::cout << "Size of timestamps without offset lists: " << allTimestamps.size() * sizeof(int) << std::endl;

    std::cout << "Size of timestamps without offset lists with huffman encoding: " << temp << std::endl;

    
#ifndef NDEBUG
    return res + client_stuff_return_code;
#else
    return 0;
#endif

}