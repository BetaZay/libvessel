#pragma once
#include <string>
#include <vector>

namespace vessel {
    int pack(int argc, char* argv[]);
    int update(int argc, char* argv[]);
    int remove(int argc, char* argv[]);
    int import_appimage(int argc, char* argv[]);
    int list();
    int help();
    int init(int argc, char* argv[]);
}
