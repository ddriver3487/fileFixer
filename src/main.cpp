#include <fmt/core.h>
#include "../include/worker.h"

void printTitle() {
    fmt::print("{:^{}}\n", "   ______ _  __       ______ _                  \n"
                           "   / ____/(_)/ /___   / ____/(_)_  __ ___   _____\n"
                           "  / /_   / // // _ \\ / /_   / /| |/_// _ \\ / ___/\n"
                           " / __/  / // //  __// __/  / /_>  < /  __// /    \n"
                           "/_/    /_//_/ \\___//_/    /_//_/|_| \\___//_/   \n\n\n",
               100);
}

int main() {

    printTitle();
    FileFixer::Ring ring(4096);

    FileFixer::Worker worker;
    auto path { std::filesystem::path("~/test") };
    worker.InputPathHandler(path);
    worker.ProcessFiles(&ring);
    worker.PrintContainerCount();



    return 0;
}
