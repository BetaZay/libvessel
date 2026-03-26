#include "CMakePacker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdlib>

namespace fs = std::filesystem;

namespace vessel {

std::string CMakePacker::exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::vector<std::string> CMakePacker::get_dependencies(const std::string &binary_path) {
  std::vector<std::string> deps;
  std::string command = "ldd " + binary_path;
  std::string output = exec(command.c_str());

  std::istringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    size_t arrow_pos = line.find("=> ");
    if (arrow_pos != std::string::npos) {
      size_t start = arrow_pos + 3;
      size_t end = line.find(" (", start);
      if (start != std::string::npos && end != std::string::npos) {
        std::string lib_path = line.substr(start, end - start);
        if (lib_path.find("/libc.so") == std::string::npos &&
            lib_path.find("/libdl.so") == std::string::npos &&
            lib_path.find("/libpthread.so") == std::string::npos &&
            lib_path.find("/libm.so") == std::string::npos) {
          deps.push_back(lib_path);
        }
      }
    }
  }
  return deps;
}

std::string CMakePacker::get_default_manifest(const std::string& app_name) {
    std::string content = "{\n  \"name\": \"" + app_name + "\",\n";
    content += "  \"mode\": \"cpp\",\n";
    content += "  \"bin_file\": \"" + app_name + "\",\n";
    content += "  \"version\": \"1.0.0\",\n";
    content += "  \"description\": \"A native C++ application bundled with Vessel\",\n";
    content += "  \"icon\": \"res/icon.png\",\n";
    content += "  \"build_dir\": \"build\",\n";
    content += "  \"build_cmd\": \"cmake .. && make\",\n";
    content += "  \"dist_dir\": \".\",\n";
    content += "  \"includes\": [\n    \"res\"\n  ]\n}\n";
    return content;
}

bool CMakePacker::execute_build(const ManifestData& manifest) {
    if (fs::exists("CMakeLists.txt") || !manifest.build_cmd.empty()) {
        std::cout << "Vessel: Orchestrating build in '" << manifest.build_dir << "'...\n";
        fs::create_directories(manifest.build_dir);
        std::string cmd = "cd " + manifest.build_dir + " && " + manifest.build_cmd;
        if (system(cmd.c_str()) != 0) {
            std::cerr << "Vessel Error: Build command failed.\n";
            return false;
        }
    }
    return true;
}

bool CMakePacker::assemble_payload(const ManifestData& manifest, const fs::path& struct_dir) {
    fs::path bin_dir = struct_dir / "bin";
    fs::path lib_dir = struct_dir / "lib";
    fs::path res_dir = struct_dir / "res";

    fs::create_directories(bin_dir);
    fs::create_directories(lib_dir);
    fs::create_directories(res_dir);

    // Copy Icon
    if (!manifest.icon.empty() && fs::exists(manifest.icon)) {
        fs::path target_icon = res_dir / fs::path(manifest.icon).filename();
        std::cout << "Packing icon: " << manifest.icon << "\n";
        fs::copy(manifest.icon, target_icon, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    }

    // Resolve Target Binary
    std::string bin_file_name = manifest.bin_file.empty() ? manifest.name : manifest.bin_file;
    fs::path binary_path = fs::path(manifest.build_dir) / bin_file_name;
    if (!fs::exists(binary_path)) binary_path = bin_file_name;

    if (!fs::exists(binary_path)) {
        std::cerr << "Error: Target binary '" << binary_path.string() << "' not found.\n";
        return false;
    }

    fs::path target_bin = bin_dir / bin_file_name;
    fs::copy_file(binary_path, target_bin, fs::copy_options::overwrite_existing);

    // Custom Includes
    for (const auto &custom_path : manifest.includes) {
        if (fs::exists(custom_path)) {
            if (custom_path.find(".so") != std::string::npos) {
                std::cout << "  Packing custom library: " << fs::path(custom_path).filename() << "\n";
                fs::copy(custom_path, lib_dir / fs::path(custom_path).filename(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            } else {
                std::cout << "  Packing custom resource: " << fs::path(custom_path).filename() << "\n";
                fs::copy(custom_path, res_dir / fs::path(custom_path).filename(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
        }
    }

    // Dependencies
    std::cout << "Crawling dependencies...\n";
    auto dependencies = get_dependencies(binary_path);
    for (const auto &lib : dependencies) {
        fs::path lib_path(lib);
        if (fs::exists(lib_path)) {
            std::cout << "  Packing auto-dependency: " << lib_path.filename() << "\n";
            fs::copy_file(lib_path, lib_dir / lib_path.filename(), fs::copy_options::overwrite_existing);
        }
    }

    // Patchelf logic 
    std::cout << "Relocating internal library paths natively...\n";
    std::string patchelf_base_path = fs::read_symlink("/proc/self/exe").string();
    std::string internal_patchelf = (fs::path(patchelf_base_path).parent_path() / "patchelf").string();

    std::string patch_cmd = "\"" + internal_patchelf + "\" --set-rpath '$ORIGIN/../lib' \"" + target_bin.string() + "\" 2>/dev/null";
    if (system(patch_cmd.c_str()) != 0) {
        std::cerr << "Warning: Vendor ELF execution failed. Binary may not find packaged libs.\n";
    }

    // Write VesselRun Wrapper
    std::cout << "Generating VesselRun execution wrapper...\n";
    fs::path vessel_run_path = bin_dir / "VesselRun";
    std::ofstream vessel_run(vessel_run_path);
    vessel_run << "#!/bin/bash\n";
    vessel_run << "DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n";
    vessel_run << "export LD_LIBRARY_PATH=\"$DIR/../lib:$LD_LIBRARY_PATH\"\n";
    vessel_run << "exec \"$DIR/" << bin_file_name << "\" \"$@\"\n";
    vessel_run.close();

    system(("chmod +x \"" + vessel_run_path.string() + "\"").c_str());

    // Generate manifest fallback
    fs::path logical_vessel_json = "vessel.json";
    if (!fs::exists(logical_vessel_json) && fs::exists("../vessel.json")) logical_vessel_json = "../vessel.json";
    fs::path target_manifest = struct_dir / "vessel.json";
    
    if (fs::exists(logical_vessel_json)) {
        fs::copy_file(logical_vessel_json, target_manifest, fs::copy_options::overwrite_existing);
    } else {
        std::ofstream fallback(target_manifest);
        fallback << "{\n  \"name\": \"" << manifest.name << "\",\n  \"bin_file\": \"" << bin_file_name << "\",\n  \"version\": \"" << manifest.version << "\"\n}\n";
        fallback.close();
    }

    // Standard res folder copy if exists
    fs::path logical_res = "res";
    if (!fs::exists(logical_res) && fs::exists("../res")) logical_res = "../res";

    if (fs::exists(logical_res) && fs::is_directory(logical_res)) {
        for (const auto &entry : fs::recursive_directory_iterator(logical_res)) {
            fs::path target_path = res_dir / entry.path().lexically_relative(logical_res);
            if (entry.is_directory()) {
                fs::create_directories(target_path);
            } else {
                fs::copy_file(entry.path(), target_path, fs::copy_options::overwrite_existing);
            }
        }
    }

    return true;
}

} // namespace vessel
