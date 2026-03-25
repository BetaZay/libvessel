#include "vessel.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#ifndef VESSEL_DATA_DIR
#define VESSEL_DATA_DIR "/usr/local/share/vessel"
#endif

namespace vessel {

// Execute a command and get output
std::string exec(const char *cmd) {
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

// Simple ldd parser to find dependencies
std::vector<std::string> get_dependencies(const std::string &binary_path) {
  std::vector<std::string> deps;
  std::string command = "ldd " + binary_path;
  std::string output = exec(command.c_str());

  // Parse ldd output (Format: libsomething.so => /path/to/libsomething.so
  // (0x...))
  std::istringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    size_t arrow_pos = line.find("=> ");
    if (arrow_pos != std::string::npos) {
      size_t start = arrow_pos + 3;
      size_t end = line.find(" (", start);
      if (start != std::string::npos && end != std::string::npos) {
        std::string lib_path = line.substr(start, end - start);

        // Exclude Base Layer plumbing (libc, libdl, libpthread, librt, libm)
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

int help() {
  std::cout << "Vessel CLI - The Native Linux Application Bundler\n\n";
  std::cout << "Commands:\n";
  std::cout << "  init      Creates a vessel.json file at the current dir and "
               "prints a link to docs\n";
  std::cout << "  pack [--keep]  Works with cmake to build and package your "
               "application automatically. Use --keep to preserve the "
               "intermediate structure.\n";
  std::cout << "  update    Updates and cleans up the global registry\n";
  std::cout << "  remove <name> [-y] Removes an application from the global "
               "registry\n";
  std::cout << "  list      List all installed applications\n";
  std::cout << "  import <file.AppImage>  Converts an AppImage into a Vessel "
               "bundle\n";
  std::cout << "  help      Show help menu\n";
  return 0;
}

int init() {
  if (fs::exists("vessel.json")) {
    std::cerr << "vessel.json already exists in the current directory.\n";
    return 1;
  }

  std::ofstream manifest("vessel.json");
  manifest << "{\n";
  manifest << "  \"name\": \"untitled_app\",\n";
  manifest << "  \"bin_file\": \"untitled_app\",\n";
  manifest << "  \"version\": \"1.0.0\",\n";
  manifest << "  \"description\": \"A native C++ application bundled with "
              "Vessel\",\n";
  manifest << "  \"icon\": \"res/icon.png\",\n";
  manifest << "  \"build_dir\": \"build\",\n";
  manifest << "  \"build_cmd\": \"cmake .. && make\",\n";
  manifest << "  \"dist_dir\": \".\",\n";
  manifest << "  \"includes\": [\n    \"res\"\n  ]\n";
  manifest << "}\n";
  manifest.close();

  std::cout << "Initialized vessel.json in the current directory.\n";
  std::cout
      << "For full documentation and advanced manifest configuration, visit:\n";
  std::cout << "--> https://github.com/vessel-pkg/docs\n";
  return 0;
}

int pack(int argc, char *argv[]) {
  // 1. Parse Manifest to determine Target Binary
  if (!fs::exists("vessel.json")) {
    std::cerr << "Error: vessel.json missing. Run 'vsl init' first.\n";
    return 1;
  }

  bool keep_struct = false;
  for (int i = 0; i < argc; ++i) {
    if (std::string(argv[i]) == "--keep") {
      keep_struct = true;
    }
  }

  std::cout << "Vessel: Parsing vessel.json...\n";
  std::string app_name = "untitled_app";
  std::string bin_file = "";
  std::string icon_path = "";
  std::string build_dir = "build";
  std::string build_cmd = ""; // Only build if CMakeLists.txt is found
  std::string dist_dir = ".";

  if (fs::exists("CMakeLists.txt")) {
    build_cmd = "cmake .. && make";
  }

  std::ifstream manifest_file("vessel.json");
  if (manifest_file.is_open()) {
    std::string content((std::istreambuf_iterator<char>(manifest_file)),
                        std::istreambuf_iterator<char>());

    auto get_manifest_value = [&](const std::string &key) {
      size_t key_pos = content.find("\"" + key + "\":");
      if (key_pos != std::string::npos) {
        size_t start_quote = content.find("\"", key_pos + key.length() + 3);
        size_t end_quote = content.find("\"", start_quote + 1);
        if (start_quote != std::string::npos &&
            end_quote != std::string::npos) {
          return content.substr(start_quote + 1, end_quote - start_quote - 1);
        }
      }
      return std::string("");
    };

    std::string v = get_manifest_value("name");
    if (!v.empty())
      app_name = v;
    v = get_manifest_value("bin_file");
    if (!v.empty())
      bin_file = v;
    v = get_manifest_value("icon");
    if (!v.empty())
      icon_path = v;
    v = get_manifest_value("build_dir");
    if (!v.empty())
      build_dir = v;
    v = get_manifest_value("build_cmd");
    if (!v.empty())
      build_cmd = v;
    v = get_manifest_value("dist_dir");
    if (!v.empty())
      dist_dir = v;

    manifest_file.close();
  }

  // 2. Automated Build Orchestration
  if (fs::exists("CMakeLists.txt") || !build_cmd.empty()) {
    std::cout << "Vessel: Orchestrating build in '" << build_dir << "'...\n";
    fs::create_directories(build_dir);
    std::string cmd = "cd " + build_dir + " && " + build_cmd;
    if (system(cmd.c_str()) != 0) {
      std::cerr << "Vessel Error: Build command failed.\n";
      return 1;
    }
  }

  // 3. Resolve Target Binary
  if (bin_file.empty())
    bin_file = app_name;

  fs::path binary_path = fs::path(build_dir) / bin_file;
  if (!fs::exists(binary_path))
    binary_path = bin_file; // Fallback

  if (!fs::exists(binary_path)) {
    std::cerr << "Error: Target binary '" << binary_path.string()
              << "' not found.\n";
    return 1;
  }

  std::string bundle_name = app_name + ".vsl";
  // Sanitize bundle name for filesystems if needed, but for now we'll keep it simple
  std::cout << "Vesselizing '" << bin_file << "' into '" << bundle_name
            << "'...\n";

  // 4. Create the Vessel Structure in a temporary folder
  fs::path vsl_root = bundle_name + "_struct";
  fs::path bin_dir = vsl_root / "bin";
  fs::path lib_dir = vsl_root / "lib";
  fs::path res_dir = vsl_root / "res";

  fs::create_directories(bin_dir);
  fs::create_directories(lib_dir);
  fs::create_directories(res_dir);

  // 5. Copy the Icon if specified
  if (!icon_path.empty() && fs::exists(icon_path)) {
    fs::path target_icon = res_dir / fs::path(icon_path).filename();
    std::cout << "Packing icon: " << icon_path << "\n";
    fs::copy(icon_path, target_icon,
             fs::copy_options::overwrite_existing |
                 fs::copy_options::recursive);
  }

  // 6. Copy the Brain
  fs::path target_bin = bin_dir / bin_file;
  fs::copy_file(binary_path, target_bin, fs::copy_options::overwrite_existing);

  // 6. Resolve logical paths for resources and manifest
  fs::path logical_res = "res";
  if (!fs::exists(logical_res) && fs::exists("../res"))
    logical_res = "../res";

  fs::path logical_vessel_json = "vessel.json";
  if (!fs::exists(logical_vessel_json) && fs::exists("../vessel.json"))
    logical_vessel_json = "../vessel.json";

  // 7. Custom Includes Tracking
  std::vector<std::string> custom_includes;
  std::ifstream manifest_reader(logical_vessel_json);
  if (manifest_reader.is_open()) {
    std::cout << "Checking vessel.json for custom includes...\n";
    std::string content((std::istreambuf_iterator<char>(manifest_reader)),
                        std::istreambuf_iterator<char>());
    size_t inc_pos = content.find("\"includes\"");
    if (inc_pos != std::string::npos) {
      size_t start_bracket = content.find("[", inc_pos);
      size_t end_bracket = content.find("]", start_bracket);
      if (start_bracket != std::string::npos &&
          end_bracket != std::string::npos) {
        std::string array_content =
            content.substr(start_bracket + 1, end_bracket - start_bracket - 1);
        size_t quote_start = array_content.find("\"");
        while (quote_start != std::string::npos) {
          size_t quote_end = array_content.find("\"", quote_start + 1);
          if (quote_end != std::string::npos) {
            custom_includes.push_back(array_content.substr(
                quote_start + 1, quote_end - quote_start - 1));
            quote_start = array_content.find("\"", quote_end + 1);
          } else
            break;
        }
      }
    }
    manifest_reader.close();
  }

  for (const auto &custom_path : custom_includes) {
    if (fs::exists(custom_path)) {
      if (custom_path.find(".so") != std::string::npos) {
        std::cout << "  Packing custom library: "
                  << fs::path(custom_path).filename() << "\n";
        fs::copy(custom_path, lib_dir / fs::path(custom_path).filename(),
                 fs::copy_options::recursive |
                     fs::copy_options::overwrite_existing);
      } else {
        std::cout << "  Packing custom resource: "
                  << fs::path(custom_path).filename() << "\n";
        fs::copy(custom_path, res_dir / fs::path(custom_path).filename(),
                 fs::copy_options::recursive |
                     fs::copy_options::overwrite_existing);
      }
    } else {
      std::cerr << "Warning: Custom include '" << custom_path
                << "' not found.\n";
    }
  }

  // 4. Crawl and Copy Foreign Furniture (Dependencies)
  std::cout << "Crawling dependencies...\n";
  auto dependencies = get_dependencies(binary_path);
  for (const auto &lib : dependencies) {
    fs::path lib_path(lib);
    if (fs::exists(lib_path)) {
      std::cout << "  Packing auto-dependency: " << lib_path.filename() << "\n";
      fs::copy_file(lib_path, lib_dir / lib_path.filename(),
                    fs::copy_options::overwrite_existing);
    }
  }

  // 5. Relocation
  std::cout << "Relocating internal library paths natively...\n";
  std::string patchelf_base_path = fs::read_symlink("/proc/self/exe").string();
  std::string internal_patchelf =
      (fs::path(patchelf_base_path).parent_path() / "patchelf").string();

  std::string patch_cmd = "\"" + internal_patchelf + "\" --set-rpath '$ORIGIN/../lib' \"" +
                          target_bin.string() + "\" 2>/dev/null";
  if (system(patch_cmd.c_str()) != 0) {
    std::cerr << "Warning: Vendor ELF execution failed. Binary may not find "
                 "packaged libs.\n";
  }

  // 6. Build or Copy vessel.json manifest
  fs::path target_manifest = vsl_root / "vessel.json";
  if (fs::exists(logical_vessel_json)) {
    std::cout << "Copying user-provided vessel.json manifest...\n";
    fs::copy_file(logical_vessel_json, target_manifest,
                  fs::copy_options::overwrite_existing);
  } else {
    std::cout << "Generating fallback vessel.json manifest...\n";
    std::ofstream manifest(target_manifest);
    manifest << "{\n  \"name\": \"" << app_name
             << "\",\n  \"bin_file\": \"" << bin_file
             << "\",\n  \"version\": \"1.0.0\"\n}\n";
    manifest.close();
  }

  // Copy standard resource folder if it exists
  if (fs::exists(logical_res) && fs::is_directory(logical_res)) {
    std::cout << "Packing structural resources...\n";
    for (const auto &entry : fs::recursive_directory_iterator(logical_res)) {
      fs::path target_path =
          res_dir / entry.path().lexically_relative(logical_res);
      if (entry.is_directory()) {
        fs::create_directories(target_path);
      } else {
        std::cout << "  Packing resource file: "
                  << entry.path().filename().string() << "\n";
        fs::copy_file(entry.path(), target_path,
                      fs::copy_options::overwrite_existing);
      }
    }
  }

  // 7. The Single-File Magic
  std::cout << "Compressing and generating final single-file executable...\n";
  std::string archive_path = bundle_name + "_payload.tar.gz";
  std::string tar_cmd =
      "tar -czf \"" + archive_path + "\" -C \"" + vsl_root.string() + "\" .";
  if (system(tar_cmd.c_str()) != 0) {
    std::cerr << "Failed to compress payload.\n";
    return 1;
  }

  // 8. Resolve Stub Path
  std::string pack_exe_path = "/proc/self/exe";
  fs::path stub_parent = fs::read_symlink(pack_exe_path).parent_path();
  std::string stub_path = (stub_parent / "vsl-stub").string();

  if (!fs::exists(stub_path)) {
    // Fallback to system install data dir
    stub_path = fs::path(VESSEL_DATA_DIR) / "vsl-stub";
  }

  if (!fs::exists(stub_path)) {
    std::cerr << "Error: vsl-stub not found in " << stub_parent
              << " or " << VESSEL_DATA_DIR << "\n";
    return 1;
  }
  std::string concat_cmd = "cat \"" + stub_path + "\" > \"" + bundle_name + "\"";
  system(concat_cmd.c_str());
  system(("echo -n '[VSL_ARCHIVE]' >> \"" + bundle_name + "\"").c_str());
  system(("cat \"" + archive_path + "\" >> \"" + bundle_name + "\"").c_str());
  system(("chmod +x \"" + bundle_name + "\"").c_str());

  if (!keep_struct) {
    fs::remove_all(vsl_root);
  } else {
    std::cout << "Vessel: Structure folder preserved at: " << vsl_root << "\n";
  }
  fs::remove(archive_path);

  if (dist_dir != "." && !dist_dir.empty()) {
    fs::create_directories(dist_dir);
    fs::rename(bundle_name, fs::path(dist_dir) / bundle_name);
    std::cout << "Vessel Single-File Bundle '" << bundle_name
              << "' is ready in " << dist_dir << "!\n";
  } else {
    std::cout << "Vessel Single-File Bundle '" << bundle_name
              << "' is ready to sail!\n";
  }
  return 0;
}

int update(int argc, char *argv[]) {
  std::cout << "Vessel Update Manager\n";
  std::cout << "==========================\n";

  const char *home_dir = getenv("HOME");
  if (!home_dir) {
    std::cerr << "Cannot locate home directory.\n";
    return 1;
  }

  fs::path registry_path = fs::path(home_dir) / ".vessel_registry";
  if (!fs::exists(registry_path)) {
    std::cout
        << "Registry empty. No registered vessels found on this system.\n";
    return 0;
  }

  std::ifstream registry_in(registry_path);
  std::string line;
  struct AppEntry {
    std::string name;
    std::string desc;
    std::string path;
  };
  std::vector<AppEntry> entries;
  std::vector<std::string> known_paths;

  while (std::getline(registry_in, line)) {
    size_t p1 = line.find("|");
    if (p1 != std::string::npos) {
      size_t p2 = line.find("|", p1 + 1);
      std::string app_name, app_desc, app_path;
      if (p2 != std::string::npos) {
        // New format: name|desc|path
        app_name = line.substr(0, p1);
        app_desc = line.substr(p1 + 1, p2 - p1 - 1);
        app_path = line.substr(p2 + 1);
      } else {
        // Old format: name|path
        app_name = line.substr(0, p1);
        app_desc = "No description provided.";
        app_path = line.substr(p1 + 1);
      }

      if (std::find(known_paths.begin(), known_paths.end(), app_path) ==
          known_paths.end()) {
        if (fs::exists(app_path)) {
          entries.push_back({app_name, app_desc, app_path});
          known_paths.push_back(app_path);
        }
      }
    }
  }
  registry_in.close();

  int updated_count = 0;
  std::cout << "Checking for updates across " << entries.size()
            << " registered applications:\n";
  for (const auto &entry : entries) {
    fs::path source_path(entry.path);
    fs::path local_share = fs::path(home_dir) / ".local" / "share";
    fs::path vessel_bin_dir = local_share / "vessel" / "bin";
    fs::path installed_path = vessel_bin_dir / source_path.filename();

    if (fs::exists(installed_path)) {
      // Use system-independent time comparison to see if we have a newer build
      auto source_time = fs::last_write_time(source_path);
      auto installed_time = fs::last_write_time(installed_path);

      if (source_time > installed_time) {
        std::cout << "  [UPDATING] " << entry.name << " ("
                  << source_path.filename().string() << ")...\n";
        fs::copy_file(source_path, installed_path,
                      fs::copy_options::overwrite_existing);
        updated_count++;
      } else {
        std::cout << "  [CURRENT]  " << entry.name << "\n";
      }
    } else {
      std::cout << "  [SKIPPED]  " << entry.name << " (Not installed natively)\n";
    }
  }

  // Rewrite registry with existing apps only (after potential updates)
  std::ofstream registry_out(registry_path, std::ios::trunc);
  for (const auto &entry : entries) {
    registry_out << entry.name << "|" << entry.desc << "|" << entry.path << "\n";
  }
  registry_out.close();

  std::cout << "--------------------------\n";
  std::cout << "Registry cleanup complete. " << updated_count
            << " applications updated.\n";
  return 0;
}

int remove(int argc, char *argv[]);
int list();
int import_appimage(int argc, char *argv[]);
int help(int argc, char *argv[]);
int remove(int argc, char *argv[]) {
  std::string target_name = "";
  bool force = false;

  for (int i = 0; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-y") {
      force = true;
    } else if (target_name.empty()) {
      target_name = arg;
    }
  }

  if (target_name.empty()) {
    std::cerr << "Error: 'remove' requires an application name.\n";
    std::cerr << "Usage: vsl remove <app_name> [-y]\n";
    return 1;
  }

  const char *home_dir = getenv("HOME");
  if (!home_dir) {
    std::cerr << "Cannot locate home directory.\n";
    return 1;
  }

  if (!force) {
    std::cout << "Are you sure you want to remove '" << target_name
              << "'? [y/N]: ";
    std::string response;
    std::getline(std::cin, response);
    if (response != "y" && response != "Y") {
      std::cout << "Operation canceled.\n";
      return 0;
    }
  }

  fs::path registry_path = fs::path(home_dir) / ".vessel_registry";
  if (!fs::exists(registry_path)) {
    std::cerr << "Error: Registry empty. No applications to remove.\n";
    return 1;
  }

  std::ifstream registry_in(registry_path);
  std::string line;
  std::vector<std::string> lines_to_keep;
  bool found = false;

  while (std::getline(registry_in, line)) {
    size_t pipe_pos = line.find("|");
    if (pipe_pos != std::string::npos) {
      std::string app_name = line.substr(0, pipe_pos);
      if (app_name == target_name) {
        found = true;
        continue; // Skip this one
      }
    }
    lines_to_keep.push_back(line);
  }
  registry_in.close();

  if (found) {
    std::ofstream registry_out(registry_path, std::ios::trunc);
    for (const auto &l : lines_to_keep) {
      registry_out << l << "\n";
    }
    registry_out.close();

    // Clean up OS integrations
    fs::path local_share = fs::path(home_dir) / ".local" / "share";
    fs::path vessel_bin_dir = local_share / "vessel" / "bin";
    fs::path desktop_apps_dir = local_share / "applications";
    fs::path icons_dir = local_share / "icons";
    fs::path central_apps_dir = fs::path(home_dir) / "Vessel Apps";
    fs::path final_vsl_location = vessel_bin_dir / (target_name + ".vsl");
    fs::path start_menu_path = desktop_apps_dir / (target_name + ".desktop");

    if (fs::exists(final_vsl_location)) fs::remove(final_vsl_location);
    if (fs::exists(start_menu_path)) fs::remove(start_menu_path);

    if (fs::exists(central_apps_dir)) {
      fs::path physical_app_shortcut =
          central_apps_dir / (target_name + ".desktop");
      if (fs::exists(physical_app_shortcut))
        fs::remove(physical_app_shortcut);
    }

    // Clean up icons (check for both .png and .svg)
    fs::path icon_png = icons_dir / (target_name + ".png");
    fs::path icon_svg = icons_dir / (target_name + ".svg");
    if (fs::exists(icon_png))
      fs::remove(icon_png);
    if (fs::exists(icon_svg))
      fs::remove(icon_svg);

    std::cout << "Successfully removed '" << target_name
              << "' and all OS integrations.\n";
  } else {
    std::cerr << "Error: Application '" << target_name
              << "' not found in registry.\n";
    return 1;
  }

  return 0;
}

int list() {
  const char *home_dir = getenv("HOME");
  if (!home_dir) return 1;

  fs::path registry_path = fs::path(home_dir) / ".vessel_registry";
  if (!fs::exists(registry_path)) {
    std::cout << "No registered vessels found.\n";
    return 0;
  }

  std::cout << "\nManaged Vessel Applications:\n";
  std::cout << "--------------------------------------------------\n";

  std::ifstream registry_in(registry_path);
  std::string line;
  int count = 0;
  while (std::getline(registry_in, line)) {
    size_t p1 = line.find("|");
    if (p1 != std::string::npos) {
      size_t p2 = line.find("|", p1 + 1);
      std::string name, desc, path;
      if (p2 != std::string::npos) {
        name = line.substr(0, p1);
        desc = line.substr(p1 + 1, p2 - p1 - 1);
        path = line.substr(p2 + 1);
      } else {
        name = line.substr(0, p1);
        desc = "No description provided.";
        path = line.substr(p1 + 1);
      }
      std::cout << "  " << name << "\n";
      std::cout << "  - Status: " << (fs::exists(path) ? "Ready" : "Missing") << "\n";
      std::cout << "  - Desc:   " << desc << "\n\n";
      count++;
    }
  }
  if (count == 0) {
    std::cout << "Registry empty.\n";
  }
  std::cout << "--------------------------------------------------\n";
  return 0;
}

int import_appimage(int argc, char *argv[]) {
  if (argc < 1) {
    std::cerr << "Usage: vsl import <path_to_appimage>\n";
    return 1;
  }

  fs::path appimage_path = argv[0];
  if (!fs::exists(appimage_path)) {
    std::cerr << "Error: File '" << appimage_path.string() << "' not found.\n";
    return 1;
  }

  std::cout << "Vessel Importer: Analyzing AppImage '" << appimage_path.filename().string() << "'...\n";

  // 1. Extract
  fs::path full_path = fs::absolute(appimage_path);
  std::string extract_cmd = "\"" + full_path.string() + "\" --appimage-extract > /dev/null 2>&1";
  if (system(extract_cmd.c_str()) != 0) {
    std::cerr << "Error: Failed to extract AppImage. Is it a valid AppImage?\n";
    return 1;
  }

  fs::path root = "squashfs-root";
  if (!fs::exists(root)) {
    std::cerr << "Error: squashfs-root not found after extraction.\n";
    return 1;
  }

  std::string app_name = appimage_path.stem().string();
  std::string app_desc = "Imported AppImage bundle.";
  std::string icon_filename = "";

  // Try to find a desktop file for name, description, and icon name
  for (const auto& entry : fs::directory_iterator(root)) {
    if (entry.path().extension() == ".desktop") {
        std::ifstream desktop(entry.path());
        std::string line;
        while (std::getline(desktop, line)) {
            if (line.find("Name=") == 0) {
                app_name = line.substr(5);
            }
            if (line.find("Comment=") == 0) {
                app_desc = line.substr(8);
            }
            if (line.find("Icon=") == 0) {
                icon_filename = line.substr(5);
            }
        }
        break;
    }
  }

  // Resolve Icon Path (Prioritize .DirIcon -> Name from Desktop -> Root match)
  fs::path final_icon_source = "";
  if (fs::exists(root / ".DirIcon")) {
      final_icon_source = root / ".DirIcon";
  } else if (!icon_filename.empty()) {
      // Check for common extensions if not provided
      if (fs::exists(root / (icon_filename + ".png"))) final_icon_source = root / (icon_filename + ".png");
      else if (fs::exists(root / (icon_filename + ".svg"))) final_icon_source = root / (icon_filename + ".svg");
  }

  std::cout << "Detected App Name: " << app_name << "\n";
  std::cout << "Detected Description: " << app_desc << "\n";
  if (!final_icon_source.empty()) {
    std::cout << "Detected Icon: " << final_icon_source.filename().string() << "\n";
  }

  // 3. Repackaging as Vessel (Manual construction to preserve AppImage layout)
  std::string bundle_name = app_name + ".vsl";
  fs::path vsl_root = bundle_name + "_struct";
  fs::create_directories(vsl_root);

  // Copy everything from squashfs-root to vsl_root
  std::cout << "Importing filesystem structure...\n";
  for (const auto& entry : fs::directory_iterator(root)) {
      std::string cmd = "cp -r \"";
      cmd += entry.path().string();
      cmd += "\" \"";
      cmd += vsl_root.string();
      cmd += "/\"";
      system(cmd.c_str());
  }

  // Create a minimal manifest inside
  std::ofstream manifest(vsl_root / "vessel.json");
  manifest << "{\n  \"name\": \"" << app_name << "\",\n";
  manifest << "  \"description\": \"" << app_desc << "\",\n";
  if (!final_icon_source.empty()) {
      // Copy icon specifically to res/ for consistency
      fs::create_directories(vsl_root / "res");
      std::string ext = final_icon_source.extension().string();
      if (ext.empty()) ext = ".png"; // .DirIcon often has no ext
      fs::copy_file(final_icon_source, vsl_root / ("res/icon" + ext), fs::copy_options::overwrite_existing);
      manifest << "  \"icon\": \"res/icon" << ext << "\",\n";
  }
  manifest << "  \"bin_file\": \"AppRun\",\n  \"version\": \"AppImage-Import\"\n}\n";
  manifest.close();

  // 4. Finalize Bundle
  std::cout << "Finalizing Vessel bundle...\n";
  
  // Compression logic (mirrored from pack)
  std::string archive_path = bundle_name + "_payload.tar.gz";
  std::string tar_cmd = "tar -czf \"" + archive_path + "\" -C \"" + vsl_root.string() + "\" .";
  system(tar_cmd.c_str());

  std::string pack_exe_path = "/proc/self/exe";
  fs::path stub_parent = fs::read_symlink(pack_exe_path).parent_path();
  std::string stub_path = (stub_parent / "vsl-stub").string();
  if (!fs::exists(stub_path)) {
    stub_path = fs::path(VESSEL_DATA_DIR) / "vsl-stub";
  }

  std::string concat_cmd = "cat \"" + stub_path + "\" > \"" + bundle_name + "\"";
  system(concat_cmd.c_str());
  system(("echo -n '[VSL_ARCHIVE]' >> \"" + bundle_name + "\"").c_str());
  system(("cat \"" + archive_path + "\" >> \"" + bundle_name + "\"").c_str());
  system(("chmod +x \"" + bundle_name + "\"").c_str());

  // Cleanup
  fs::remove_all(root);
  fs::remove_all(vsl_root);
  fs::remove(archive_path);

  std::cout << "\nImport Successful! '" << bundle_name << "' created.\n";

  return 0;
}

} // namespace vessel
