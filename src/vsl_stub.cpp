#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

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

class StubUX {
public:
    enum class Mode { ZENITY, KDIALOG, TERMINAL };
    
    StubUX(const std::string& app_name) : app_name(app_name) {
        if (getenv("DISPLAY") || getenv("WAYLAND_DISPLAY")) {
            if (system("which zenity > /dev/null 2>&1") == 0) {
                mode = Mode::ZENITY;
            } else if (system("which kdialog > /dev/null 2>&1") == 0) {
                mode = Mode::KDIALOG;
            } else {
                mode = Mode::TERMINAL;
            }
        } else {
            mode = Mode::TERMINAL;
        }
    }

    ~StubUX() {
        hide_status();
    }

    void show_status(const std::string& message) {
        std::cout << "Vessel: " << message << std::endl;
        
        if (mode == Mode::ZENITY) {
            if (zenity_pid == -1) {
                fifo_path = "/tmp/vsl_fifo_" + std::to_string(getpid());
                mkfifo(fifo_path.c_str(), 0600);

                // Start Zenity in the background reading from the FIFO
                std::string cmd = "zenity --progress --pulsate --title=\"Vessel: " + app_name + 
                                  "\" --text=\"Starting setup...\" --auto-close --no-cancel < " + fifo_path + " & echo $!";
                
                FILE* p = popen(cmd.c_str(), "r");
                if (p) {
                    char buf[32];
                    if (fgets(buf, sizeof(buf), p)) {
                        zenity_pid = std::stoi(buf);
                    }
                    pclose(p);
                }

                // Open the FIFO for writing
                fifo_fd = open(fifo_path.c_str(), O_WRONLY | O_NONBLOCK);
            }

            if (fifo_fd != -1) {
                std::string msg = "# " + message + "\n";
                write(fifo_fd, msg.c_str(), msg.length());
            }
        } else if (mode == Mode::KDIALOG) {
            std::string cmd = "kdialog --title \"Vessel: " + app_name + "\" --passivepopup \"" + message + "\" 5 &";
            system(cmd.c_str());
        }
    }

    void hide_status() {
        if (zenity_pid != -1) {
            std::cout << "Vessel Debug: Triggering hide_status for PID " << zenity_pid << std::endl;
            if (fifo_fd != -1) {
                write(fifo_fd, "100\n", 4);
                close(fifo_fd);
                fifo_fd = -1;
            }
            
            // Kill the shell wrapper that spawned it using native POSIX kill
            kill(zenity_pid, SIGKILL);
            
            // Explicitly search and terminate the Zenity process directly if pkill is available
            if (system("which pkill > /dev/null 2>&1") == 0) {
                std::string pkill_cmd = "pkill -f 'zenity.*Vessel: " + app_name + "' 2>/dev/null";
                system(pkill_cmd.c_str());
            }
            
            if (!fifo_path.empty()) {
                unlink(fifo_path.c_str());
                fifo_path.clear();
            }
            zenity_pid = -1;
        }
    }

    void show_error(const std::string& title, const std::string& message) {
        std::cerr << "Vessel Error: " << title << " - " << message << std::endl;
        hide_status();

        if (mode == Mode::ZENITY) {
            std::string cmd = "zenity --error --title=\"" + title + "\" --text=\"" + message + "\"";
            system(cmd.c_str());
        } else if (mode == Mode::KDIALOG) {
            std::string cmd = "kdialog --title \"" + title + "\" --error \"" + message + "\"";
            system(cmd.c_str());
        }
    }

    bool has_internet() {
        return system("timeout 2 bash -c '</dev/tcp/8.8.8.8/53' > /dev/null 2>&1") == 0;
    }

private:
    Mode mode;
    std::string app_name;
    int zenity_pid = -1;
    int fifo_fd = -1;
    std::string fifo_path;
};

bool ensure_runtime(const std::string& type, const std::string& version, StubUX& ux) {
  if (type != "java") return true;

  const char *home = getenv("HOME");
  if (!home) return false;

  fs::path runtime_root = fs::path(home) / ".vessel" / "runtimes" / "java" / version;
  if (fs::exists(runtime_root / "bin" / "java")) {
      setenv("JAVA_HOME", runtime_root.string().c_str(), 1);
      return true;
  }

  ux.show_status("Required " + type + " runtime (" + version + ") not found locally.");
  
  if (!ux.has_internet()) {
      ux.show_error("Offline", "Vessel needs an internet connection to download the required " + type + " runtime for the first time.");
      return false;
  }

  ux.show_status("Fetching shared " + type + " runtime...");

  fs::create_directories(runtime_root);
  std::string download_url = "https://api.adoptium.net/v3/binary/latest/" + version + "/ga/linux/x64/jre/hotspot/normal/eclipse?project=jdk";
  std::string tarball = "/tmp/vessel_" + type + "_" + version + ".tar.gz";

  std::string curl_cmd = "curl -L -o " + tarball + " \"" + download_url + "\"";
  if (system(curl_cmd.c_str()) != 0) {
      ux.show_error("Download Failed", "Failed to fetch the runtime. Please check your network.");
      return false;
  }

  ux.show_status("Unpacking runtime...");
  std::string extract_cmd = "tar -xzf " + tarball + " -C " + runtime_root.string() + " --strip-components=1";
  if (system(extract_cmd.c_str()) != 0) {
      ux.show_error("Extraction Failed", "Failed to unpack the downloaded bundle.");
      fs::remove(tarball);
      return false;
  }

  fs::remove(tarball);
  setenv("JAVA_HOME", runtime_root.string().c_str(), 1);
  ux.show_status("Runtime ready.");
  return true;
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

  StubUX ux(app_name);

  bool install_only = false;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--install-only") {
      install_only = true;
      break;
    }
  }

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

    // Runtime requirement check
    size_t runtime_pos = content.find("\"runtime\"");
    if (runtime_pos != std::string::npos) {
        auto get_nested = [&](const std::string& outer_key, const std::string& inner_key) {
            size_t outer = content.find("\"" + outer_key + "\"", runtime_pos);
            if (outer == std::string::npos) return std::string("");
            size_t inner = content.find("\"" + inner_key + "\"", outer);
            if (inner == std::string::npos) return std::string("");
            size_t s = content.find("\"", inner + inner_key.length() + 2);
            size_t e = content.find("\"", s + 1);
            if (s != std::string::npos && e != std::string::npos) {
                return content.substr(s + 1, e - s - 1);
            }
            return std::string("");
        };
        std::string rt_type = get_nested("runtime", "type");
        std::string rt_ver = get_nested("runtime", "version");
        if (!rt_type.empty() && !rt_ver.empty()) {
            if (!ensure_runtime(rt_type, rt_ver, ux)) {
                return 1;
            }
        }
    }
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
      
      if (!fs::exists(desktop_apps_dir)) fs::create_directories(desktop_apps_dir);
      if (!fs::exists(icons_dir)) fs::create_directories(icons_dir);
      if (!fs::exists(central_apps_dir)) fs::create_directories(central_apps_dir);

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

      if (install_only) {
        std::cout << "Vessel installation for '" << app_name
                  << "' complete. Finalizing registry state...\n";
      } else {
        std::cout << "Installation complete! Executing primary bundle...\n";
        ux.hide_status(); // Close progress dialog before blocking on inner bundle
        std::string launch_cmd = final_vsl_location.string();
        return system(launch_cmd.c_str());
      }
    } else {
      // If we are already in the final home but someone ran with --install-only
      if (install_only) {
        std::cout << "Vessel app '" << app_name
                  << "' is already natively installed. Verifying registry state...\n";
      }
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

  fs::path exec_path = cache_dir / "bin" / "VesselRun";
  if (!fs::exists(exec_path)) {
    exec_path = cache_dir / "bin" / bin_file_name;
  }
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

  if (install_only) {
    ux.hide_status();
    std::cout << "Vessel installation/cache ready for '" << app_name << "'. Exiting.\n";
    return 0;
  }

  ux.hide_status();
  int ret = system(exec_str.c_str());
  return ret;
}
