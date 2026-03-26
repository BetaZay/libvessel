#include "vessel.h"
#include "packers/CMakePacker.h"
#include "packers/GradlePacker.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#ifndef VESSEL_DATA_DIR
#define VESSEL_DATA_DIR "/usr/local/share/vessel"
#endif

namespace vessel {

namespace {

struct GradleInitHints {
  bool detected = false;
  std::string java_version = "17";
  std::string build_cmd = "./gradlew build";
  std::string bin_file = "build/libs/app.jar";
};

std::string read_text_file(const fs::path &path) {
  if (!fs::exists(path)) return "";
  std::ifstream in(path);
  if (!in.is_open()) return "";
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

std::string normalize_java_version(const std::string &raw) {
  if (raw.empty()) return "";

  if (raw.rfind("1.", 0) == 0) {
    size_t i = 2;
    std::string major;
    while (i < raw.size() && std::isdigit(static_cast<unsigned char>(raw[i]))) {
      major.push_back(raw[i]);
      ++i;
    }
    return major;
  }

  std::string major;
  for (char c : raw) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      major.push_back(c);
    } else {
      break;
    }
  }
  return major;
}

std::string detect_java_version_from_content(const std::string &content) {
  if (content.empty()) return "";

  const std::vector<std::regex> patterns = {
      std::regex(R"(JavaLanguageVersion\s*\.of\((\d+)\))"),
      std::regex(R"(JavaVersion\.VERSION_(\d+))"),
      std::regex(R"((?:source|target)Compatibility\s*=\s*["']([0-9][0-9.]*)["'])")};

  for (const auto &pattern : patterns) {
    std::smatch m;
    if (std::regex_search(content, m, pattern) && m.size() > 1) {
      std::string normalized = normalize_java_version(m[1].str());
      if (!normalized.empty()) return normalized;
    }
  }

  return "";
}

bool has_shadow_plugin(const std::string &content) {
  return content.find("com.github.johnrengelman.shadow") != std::string::npos ||
         content.find("com.gradleup.shadow") != std::string::npos ||
         content.find("shadowJar") != std::string::npos;
}

GradleInitHints detect_gradle_hints(const fs::path &root) {
  GradleInitHints hints;

  const fs::path root_build_gradle = root / "build.gradle";
  const fs::path root_build_gradle_kts = root / "build.gradle.kts";
  const fs::path app_build_gradle = root / "app" / "build.gradle";
  const fs::path app_build_gradle_kts = root / "app" / "build.gradle.kts";

  const bool has_gradle_markers =
      fs::exists(root / "gradlew") || fs::exists(root / "gradlew.bat") ||
      fs::exists(root / "settings.gradle") ||
      fs::exists(root / "settings.gradle.kts") || fs::exists(root_build_gradle) ||
      fs::exists(root_build_gradle_kts) || fs::exists(app_build_gradle) ||
      fs::exists(app_build_gradle_kts);

  if (!has_gradle_markers) return hints;

  hints.detected = true;

  const std::vector<fs::path> java_version_sources = {
      app_build_gradle, app_build_gradle_kts, root_build_gradle,
      root_build_gradle_kts};
  for (const auto &source : java_version_sources) {
    const std::string content = read_text_file(source);
    const std::string detected = detect_java_version_from_content(content);
    if (!detected.empty()) {
      hints.java_version = detected;
      break;
    }
  }

  const std::string root_build_content =
      read_text_file(root_build_gradle) + "\n" + read_text_file(root_build_gradle_kts);
  const std::string app_build_content =
      read_text_file(app_build_gradle) + "\n" + read_text_file(app_build_gradle_kts);

  const bool has_app_module =
      fs::exists(app_build_gradle) || fs::exists(app_build_gradle_kts);
  const bool app_uses_shadow = has_shadow_plugin(app_build_content);
  const bool root_uses_shadow = has_shadow_plugin(root_build_content);

  if (has_app_module && app_uses_shadow) {
    hints.build_cmd = "./gradlew :app:shadowJar";
    hints.bin_file = "app/build/libs/app-all.jar";
  } else if (root_uses_shadow) {
    hints.build_cmd = "./gradlew shadowJar";
    hints.bin_file = "build/libs/app-all.jar";
  } else if (has_app_module) {
    hints.build_cmd = "./gradlew :app:build";
    hints.bin_file = "app/build/libs/app.jar";
  }

  return hints;
}

std::string build_gradle_init_manifest(const std::string &app_name,
                                       const GradleInitHints &hints) {
  std::string content = "{\n  \"name\": \"" + app_name + "\",\n";
  content += "  \"mode\": \"gradle\",\n";
  content += "  \"version\": \"1.0.0\",\n";
  content += "  \"description\": \"A Java application bundled with Vessel\",\n";
  content += "  \"build_cmd\": \"" + hints.build_cmd + "\",\n";
  content += "  \"bin_file\": \"" + hints.bin_file + "\",\n";
  content += "  \"launch_args\": [],\n";
  content += "  \"runtime\": {\n";
  content += "    \"type\": \"java\",\n";
  content += "    \"version\": \"" + hints.java_version + "\"\n";
  content += "  }\n}\n";
  return content;
}

} // namespace

int help() {
  std::cout << "Vessel CLI - The Native Linux Application Bundler\n\n";
  std::cout << "Commands:\n";
  std::cout << "  init      Creates a vessel.json file at the current dir and prints a link to docs\n";
  std::cout << "  pack [--keep]  Builds and packages your project into a .vsl bundle.\n";
  std::cout << "  import <file.AppImage>  Converts a third-party AppImage into a managed Vessel bundle.\n";
  std::cout << "  list      Lists all installed applications with their status and descriptions.\n";
  std::cout << "  update    Updates and cleans up the global registry\n";
  std::cout << "  remove <name> [-y] Removes an application from the global registry\n";
  std::cout << "  help      Show help menu\n";
  return 0;
}

int init(int argc, char* argv[]) {
  if (fs::exists("vessel.json")) {
    std::cerr << "vessel.json already exists in the current directory.\n";
    return 1;
  }

  std::string mode = "cpp";
  bool mode_explicit = false;
  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.find("--mode=") == 0) {
      mode = arg.substr(7);
      mode_explicit = true;
    }
  }

  GradleInitHints gradle_hints = detect_gradle_hints(fs::current_path());
  if (!mode_explicit && gradle_hints.detected) {
    mode = "gradle";
    std::cout << "Vessel: Detected Gradle project. Auto-selecting mode 'gradle'.\n";
  }

  std::string content;
  if (mode == "cpp") {
    CMakePacker packer;
    content = packer.get_default_manifest("untitled_app");
  } else if (mode == "gradle") {
    content = build_gradle_init_manifest("untitled_app", gradle_hints);
    if (gradle_hints.detected) {
      std::cout << "Vessel: Detected Java runtime version "
                << gradle_hints.java_version << " from Gradle configuration.\n";
    }
  } else {
    std::cerr << "Unknown mode: " << mode << "\n";
    return 1;
  }

  std::ofstream manifest("vessel.json");
  manifest << content;
  manifest.close();

  std::cout << "Initialized vessel.json in the current directory for mode: " << mode << ".\n";
  std::cout << "For full documentation and advanced manifest configuration, visit:\n";
  std::cout << "--> https://github.com/vessel-pkg/docs\n";
  return 0;
}

int pack(int argc, char *argv[]) {
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
  ManifestData manifest;
  std::ifstream manifest_file("vessel.json");
  if (manifest_file.is_open()) {
    std::string content((std::istreambuf_iterator<char>(manifest_file)),
                        std::istreambuf_iterator<char>());

    auto get_val = [&](const std::string &key) {
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

    std::string v;
    if (!(v = get_val("name")).empty()) manifest.name = v;
    if (!(v = get_val("bin_file")).empty()) manifest.bin_file = v;
    if (!(v = get_val("icon")).empty()) manifest.icon = v;
    if (!(v = get_val("build_dir")).empty()) manifest.build_dir = v;
    if (!(v = get_val("build_cmd")).empty()) manifest.build_cmd = v;
    if (!(v = get_val("dist_dir")).empty()) manifest.dist_dir = v;
    if (!(v = get_val("mode")).empty()) manifest.mode = v;
    if (!(v = get_val("version")).empty()) manifest.version = v;

    // Runtime parsing
    size_t runtime_pos = content.find("\"runtime\"");
    if (runtime_pos != std::string::npos) {
        auto parse_nested = [&](const std::string& key) {
            size_t key_pos = content.find("\"" + key + "\"", runtime_pos);
            if (key_pos != std::string::npos) {
                size_t colon = content.find(":", key_pos);
                if (colon != std::string::npos) {
                    size_t s = content.find("\"", colon);
                    size_t e = content.find("\"", s + 1);
                    if (s != std::string::npos && e != std::string::npos) {
                        return content.substr(s + 1, e - s - 1);
                    }
                }
            }
            return std::string("");
        };
        manifest.runtime.type = parse_nested("type");
        manifest.runtime.version = parse_nested("version");
    }

    size_t inc_pos = content.find("\"includes\"");
    if (inc_pos != std::string::npos) {
      size_t start_bracket = content.find("[", inc_pos);
      size_t end_bracket = content.find("]", start_bracket);
      if (start_bracket != std::string::npos && end_bracket != std::string::npos) {
        std::string array_content = content.substr(start_bracket + 1, end_bracket - start_bracket - 1);
        size_t quote_start = array_content.find("\"");
        while (quote_start != std::string::npos) {
          size_t quote_end = array_content.find("\"", quote_start + 1);
          if (quote_end != std::string::npos) {
            manifest.includes.push_back(array_content.substr(quote_start + 1, quote_end - quote_start - 1));
            quote_start = array_content.find("\"", quote_end + 1);
          } else break;
        }
      }
    }

    size_t launch_args_pos = content.find("\"launch_args\"");
    if (launch_args_pos != std::string::npos) {
      size_t start_bracket = content.find("[", launch_args_pos);
      size_t end_bracket = content.find("]", start_bracket);
      if (start_bracket != std::string::npos && end_bracket != std::string::npos) {
        std::string array_content =
            content.substr(start_bracket + 1, end_bracket - start_bracket - 1);
        size_t quote_start = array_content.find("\"");
        while (quote_start != std::string::npos) {
          size_t quote_end = array_content.find("\"", quote_start + 1);
          if (quote_end != std::string::npos) {
            manifest.launch_args.push_back(
                array_content.substr(quote_start + 1, quote_end - quote_start - 1));
            quote_start = array_content.find("\"", quote_end + 1);
          } else {
            break;
          }
        }
      }
    }
    manifest_file.close();
  }

  std::unique_ptr<Packer> packer;
  if (manifest.mode == "cpp") {
      packer = std::make_unique<CMakePacker>();
  } else if (manifest.mode == "gradle") {
      packer = std::make_unique<GradlePacker>();
  } else {
      std::cerr << "Error: Unsupported packing mode '" << manifest.mode << "'.\n";
      return 1;
  }

  if (!packer->execute_build(manifest)) {
      return 1;
  }

  std::string bundle_name = manifest.name + ".vsl";
  fs::path vsl_root = bundle_name + "_struct";
  fs::remove_all(vsl_root);
  fs::create_directories(vsl_root);

  if (!packer->assemble_payload(manifest, vsl_root)) {
      return 1;
  }

  std::cout << "Compressing and generating final single-file executable...\n";
  std::string archive_path = bundle_name + "_payload.tar.gz";
  std::string tar_cmd =
      "tar -czf \"" + archive_path + "\" -C \"" + vsl_root.string() + "\" .";
  if (system(tar_cmd.c_str()) != 0) {
    std::cerr << "Failed to compress payload.\n";
    return 1;
  }

  std::string pack_exe_path = "/proc/self/exe";
  fs::path stub_parent = fs::read_symlink(pack_exe_path).parent_path();
  std::string stub_path = (stub_parent / "vsl-stub").string();

  if (!fs::exists(stub_path)) {
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

  if (manifest.dist_dir != "." && !manifest.dist_dir.empty()) {
    fs::create_directories(manifest.dist_dir);
    fs::rename(bundle_name, fs::path(manifest.dist_dir) / bundle_name);
    std::cout << "Vessel Single-File Bundle '" << bundle_name
              << "' is ready in " << manifest.dist_dir << "!\n";
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
