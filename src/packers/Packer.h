#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace vessel {

struct RuntimeData {
    std::string type = "";
    std::string version = "";
};

struct ManifestData {
    std::string name = "untitled_app";
    std::string bin_file = "";
    std::string icon = "";
    std::string build_dir = "build";
    std::string build_cmd = "";
    std::string dist_dir = ".";
    std::string mode = "cpp";
    std::string version = "1.0.0";
    std::string description = "A native application bundled with Vessel";
    std::vector<std::string> includes;
    RuntimeData runtime;
};

class Packer {
public:
    virtual ~Packer() = default;

    // init(): Provides the default vessel.json template for this specific mode.
    virtual std::string get_default_manifest(const std::string& app_name) = 0;

    // build(): Executes the build system
    virtual bool execute_build(const ManifestData& manifest) = 0;

    // assemble(): Assembles the payload and generates VesselRun
    virtual bool assemble_payload(const ManifestData& manifest, const std::filesystem::path& struct_dir) = 0;
};

} // namespace vessel
