#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// A simple string search in a binary file
size_t find_marker(std::ifstream &file, const std::string &marker) {
  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(file_size);
  if (file.read(buffer.data(), file_size)) {
    for (size_t i = 0; i < file_size - marker.size(); ++i) {
      bool found = true;
      for (size_t j = 0; j < marker.size(); ++j) {
        if (buffer[i + j] != marker[j]) {
          found = false;
          break;
        }
      }
      if (found) {
        return i + marker.size(); // The offset right after the marker
      }
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  std::string self_path = "/proc/self/exe";
  std::ifstream self_file(self_path, std::ios::binary);

  if (!self_file.is_open()) {
    std::cerr << "Vessel Stub Error: Cannot open self.\n";
    return 1;
  }

  // Split strings prevent it from assembling to the exact marker in .rodata
  std::string marker = "[VSL_";
  marker += "ARCHIVE]";
  size_t archive_offset = find_marker(self_file, marker);

  if (archive_offset == 0) {
    std::cerr << "Vessel Stub Error: Archive marker not found. Is this file "
                 "corrupted?\n";
    return 1;
  }

  std::string actual_vsl_path = fs::read_symlink("/proc/self/exe").string();
  std::string app_name = fs::path(actual_vsl_path).stem().string();

  // 0. The Inspection Flag: Dump the internal filesystem for the user
  // dynamically
  if (argc >= 2 && std::string(argv[1]) == "--vsl-extract") {
    std::cout
        << "Vessel Debug: Extracting internal file system for inspection...\n";
    fs::path dump_dir = fs::path("./" + app_name + "_vsl_unpacked");
    fs::create_directories(dump_dir);

    fs::path temp_tar = dump_dir / "payload.tar.gz";
    std::ofstream tar_out(temp_tar, std::ios::binary);
    self_file.seekg(archive_offset, std::ios::beg);
    tar_out << self_file.rdbuf();
    tar_out.close();

    std::string extract_cmd =
        "tar -xzf " + temp_tar.string() + " -C " + dump_dir.string();
    system(extract_cmd.c_str());
    fs::remove(temp_tar);

    std::cout << "Successfully unpacked bundle structural contents into: "
              << dump_dir.string() << "\n";
    return 0;
  }

  // 1. Determine cache extraction directory
  fs::path cache_dir = fs::path("/tmp/vessel_cache_" + app_name);
  fs::path extract_marker = cache_dir / ".vsl_extracted";

  bool needs_extraction = true;
  if (fs::exists(cache_dir) && fs::exists(extract_marker)) {
    // Compare modification times
    auto bundle_time = fs::last_write_time(actual_vsl_path);
    auto marker_time = fs::last_write_time(extract_marker);

    if (marker_time >= bundle_time) {
      needs_extraction = false;
    }
  }

  if (needs_extraction) {
    if (fs::exists(cache_dir)) {
      fs::remove_all(cache_dir);
    }
    fs::create_directories(cache_dir);

    // 2. Write the archive to a temp file
    fs::path temp_tar = cache_dir / "payload.tar.gz";
    std::ofstream tar_out(temp_tar, std::ios::binary);

    self_file.seekg(archive_offset, std::ios::beg);
    tar_out << self_file.rdbuf();
    tar_out.close();
    self_file.close();

    // 3. Extract tar.gz into the cache directory
    std::string extract_cmd =
        "tar -xzf " + temp_tar.string() + " -C " + cache_dir.string();
    if (system(extract_cmd.c_str()) != 0) {
      std::cerr << "Vessel Stub Error: Failed to extract payload.\n";
      return 1;
    }

    // Create an extraction marker with the same timestamp as the bundle
    std::ofstream marker(extract_marker);
    marker.close();
    fs::last_write_time(extract_marker, fs::last_write_time(actual_vsl_path));

    fs::remove(temp_tar);
  } else {
    self_file.close();
  }

  // 4. Parse Manifest for Metadata
  std::string app_fancy_name = app_name;
  std::string bin_file_name = app_name;
  std::string icon_filename = "";

  fs::path manifest_path = cache_dir / "vessel.json";
  if (fs::exists(manifest_path)) {
    std::ifstream manifest_file(manifest_path);
    std::string content((std::istreambuf_iterator<char>(manifest_file)),
                        std::istreambuf_iterator<char>());

    auto get_val = [&](const std::string &key) {
      size_t pos = content.find("\"" + key + "\":");
      if (pos != std::string::npos) {
        size_t start = content.find("\"", pos + key.length() + 3);
        size_t end = content.find("\"", start + 1);
        if (start != std::string::npos && end != std::string::npos) {
          return content.substr(start + 1, end - start - 1);
        }
      }
      return std::string("");
    };

    std::string name_val = get_val("name");
    if (!name_val.empty())
      app_fancy_name = name_val;

    std::string bin_val = get_val("bin_file");
    if (!bin_val.empty())
      bin_file_name = bin_val;

    std::string icon_val = get_val("icon");
    if (!icon_val.empty())
      icon_filename = fs::path(icon_val).filename().string();
  }

  // 5. Autonomous Self-Installation on First Boot
  const char *home_dir = getenv("HOME");
  if (home_dir) {
    fs::path local_share = fs::path(home_dir) / ".local" / "share";
    fs::path vessel_bin_dir = local_share / "vessel" / "bin";
    fs::path final_vsl_location = vessel_bin_dir / (app_name + ".vsl");

    if (actual_vsl_path != final_vsl_location.string()) {
      std::cout
          << "First boot detected. Installing native OS integrations...\n";
      fs::create_directories(vessel_bin_dir);
      fs::copy_file(actual_vsl_path, final_vsl_location,
                    fs::copy_options::overwrite_existing);

      fs::path desktop_apps_dir = local_share / "applications";
      fs::path icons_dir = local_share / "icons";
      fs::path central_apps_dir = fs::path(home_dir) / "Vessel Apps";
      if (!fs::exists(central_apps_dir))
        fs::create_directories(central_apps_dir);

      // Harvest the Icon
      bool has_icon = false;
      fs::path installed_icon_path = "";
      fs::path res_dir = cache_dir / "res";

      if (!icon_filename.empty()) {
        fs::path icon_source = res_dir / icon_filename;
        if (fs::exists(icon_source)) {
          installed_icon_path =
              icons_dir /
              (app_name + fs::path(icon_filename).extension().string());
          fs::copy_file(icon_source, installed_icon_path,
                        fs::copy_options::overwrite_existing);
          has_icon = true;
        }
      }

      if (!has_icon && fs::exists(res_dir) && fs::is_directory(res_dir)) {
        for (const auto &res_file : fs::directory_iterator(res_dir)) {
          if (res_file.path().extension() == ".png" ||
              res_file.path().extension() == ".svg") {
            installed_icon_path =
                icons_dir / (app_name + res_file.path().extension().string());
            fs::copy_file(res_file.path(), installed_icon_path,
                          fs::copy_options::overwrite_existing);
            has_icon = true;
            break;
          }
        }
      }

      // Generate string configuration for the desktop shortcut
      std::string desktop_content =
          "[Desktop Entry]\nType=Application\nName=" + app_fancy_name + "\n";
      desktop_content += "Exec=" + final_vsl_location.string() + "\n";
      if (has_icon)
        desktop_content += "Icon=" + installed_icon_path.string() + "\n";
      desktop_content += "Terminal=false\nCategories=Utility;\n";

      // Write to global Start Menu
      fs::path start_menu_path = desktop_apps_dir / (app_name + ".desktop");
      std::ofstream sm_desktop(start_menu_path);
      sm_desktop << desktop_content;
      sm_desktop.close();

      // Write physical visual shortcut natively into ~/Applications
      fs::path physical_app_shortcut =
          central_apps_dir / (app_name + ".desktop");
      std::ofstream app_desktop(physical_app_shortcut);
      app_desktop << desktop_content;
      app_desktop.close();

      // Force it to be executable
      fs::permissions(physical_app_shortcut,
                      fs::perms::owner_exec | fs::perms::group_exec |
                          fs::perms::others_exec,
                      fs::perm_options::add);

      std::cout << "Installation complete! Executing primary bundle...\n";
      std::string launch_cmd = final_vsl_location.string();
      return system(launch_cmd.c_str());
    }

    // Self-Registration (Unique)
    fs::path registry_path = fs::path(home_dir) / ".vessel_registry";
    bool already_registered = false;
    if (fs::exists(registry_path)) {
      std::ifstream reg_in(registry_path);
      std::string line;
      while (std::getline(reg_in, line)) {
        if (line.find(app_fancy_name + "|") == 0) {
          already_registered = true;
          break;
        }
      }
    }

    if (!already_registered) {
      std::ofstream registry(registry_path, std::ios::app);
      if (registry.is_open()) {
        std::string app_desc = "No description provided.";
        if (fs::exists(manifest_path)) {
          std::ifstream manifest_file(manifest_path);
          std::string content((std::istreambuf_iterator<char>(manifest_file)),
                              std::istreambuf_iterator<char>());
          size_t pos = content.find("\"description\":");
          if (pos != std::string::npos) {
            size_t start = content.find("\"", pos + 14 + 1);
            size_t end = content.find("\"", start + 1);
            if (start != std::string::npos && end != std::string::npos) {
              app_desc = content.substr(start + 1, end - start - 1);
            }
          }
        }
        registry << app_fancy_name << "|" << app_desc << "|"
                 << final_vsl_location.string() << "\n";
        registry.close();
      }
    }

    // Clean up the original installer bundle if it's not already in the final
    // home
    if (fs::exists(actual_vsl_path) && actual_vsl_path != final_vsl_location) {
      try {
        fs::remove(actual_vsl_path);
      } catch (...) {
      }
    }
  }

  fs::path exec_path = cache_dir / "bin" / bin_file_name;
  if (!fs::exists(exec_path)) {
    exec_path = cache_dir / bin_file_name;
  }
  std::string exec_str = exec_path.string();
  std::string lib_path = (cache_dir / "lib").string();

  // 6. Launch
  std::string current_ld =
      getenv("LD_LIBRARY_PATH") ? getenv("LD_LIBRARY_PATH") : "";
  std::string new_ld = lib_path + ":" + current_ld;
  setenv("LD_LIBRARY_PATH", new_ld.c_str(), 1);

  int ret = system(exec_str.c_str());
  return ret;
}
