#include "models/Gorilla.h"
#include "doctest.h"
#include "managers/ConfigManager.h"
#include "managers/ReaderManager.h"

// This function is required by Doctest to make it possible to have asserts inside the code.
// The function is passed to the Doctest context in our main function.
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


int main(int argc, char *argv[]) {
    
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
    std::string path = "moby.cfg";
    ReaderManager readerManager(path);
    readerManager.runCompressor();
	return res + client_stuff_return_code;
    
}