#include <iostream>
#include "models/Gorilla.h"
#include "doctest.h"
#include "managers/ReaderManager.h"
#include "arrow/api.h"

int main() {
    //Doctest things
    doctest::Context context;
    int res = context.run();
    int client_stuff_return_code = 0;
    std::string path = "moby.cfg";
    ReaderManager readerManager(path);
    arrow::Status st = readerManager.runCompressor();
    if (!st.ok()) {
        std::cerr << st << std::endl;
        return 1;
    }

    return res + client_stuff_return_code;
}


