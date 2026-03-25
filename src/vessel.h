#pragma once
#include <string>
#include <vector>

namespace vessel {
    int pack(int argc, char* argv[]);
    int update(int argc, char* argv[]);
    int remove(int argc, char* argv[]);
    int help();
    int init();
}
