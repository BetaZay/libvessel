#include "GradlePacker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdlib>

namespace fs = std::filesystem;

namespace vessel {
 
 std::string GradlePacker::exec(const char *cmd) {
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

std::string GradlePacker::get_default_manifest(const std::string& app_name) {
    std::string content = "{\n  \"name\": \"" + app_name + "\",\n";
    content += "  \"mode\": \"gradle\",\n";
    content += "  \"version\": \"1.0.0\",\n";
    content += "  \"description\": \"A Java application bundled with Vessel\",\n";
    content += "  \"build_cmd\": \"./gradlew build\",\n";
    content += "  \"bin_file\": \"build/libs/app.jar\",\n";
    content += "  \"runtime\": {\n";
    content += "    \"type\": \"java\",\n";
    content += "    \"version\": \"17\"\n";
    content += "  }\n}\n";
    return content;
}

bool GradlePacker::execute_build(const ManifestData& manifest) {
    std::cout << "Vessel: Orchestrating Gradle build...\n";
    std::string build_dir = manifest.build_dir.empty() ? "." : manifest.build_dir;
    std::string cmd = manifest.build_cmd;
    if (cmd.empty()) {
        cmd = "./gradlew build";
    }

    // Keep behavior consistent with CMake mode: execute inside build_dir.
    // Also ensure wrapper is executable when the command uses ./gradlew.
    std::string full_cmd = "cd \"" + build_dir + "\" && ";
    if (cmd.find("./gradlew") != std::string::npos) {
        full_cmd += "chmod +x ./gradlew && ";
    }
    full_cmd += cmd;

    if (system(full_cmd.c_str()) != 0) {
        std::cerr << "Vessel Error: Gradle build failed.\n";
        return false;
    }
    return true;
}

bool GradlePacker::assemble_payload(const ManifestData& manifest, const fs::path& struct_dir) {
    fs::path bin_dir = struct_dir / "bin";
    fs::path res_dir = struct_dir / "res";

    fs::create_directories(bin_dir);
    fs::create_directories(res_dir);

    // Copy JAR file
    std::string jar_file_name = manifest.bin_file;
    if (jar_file_name.empty()) {
        std::cerr << "Error: 'bin_file' (JAR path) MUST be specified for Gradle mode.\n";
        return false;
    }

    fs::path jar_source = fs::absolute(jar_file_name);
    if (!fs::exists(jar_source)) {
        std::cerr << "Error: Target JAR '" << jar_source.string() << "' not found.\n";
        return false;
    }

    fs::path target_jar = bin_dir / fs::path(jar_file_name).filename();
    std::cout << "Packing JAR: " << target_jar.filename().string() << "\n";
    fs::copy_file(jar_source, target_jar, fs::copy_options::overwrite_existing);
    
    // Custom Includes
    for (const auto &custom_path : manifest.includes) {
        if (fs::exists(custom_path)) {
            std::cout << "  Packing custom resource: " << fs::path(custom_path).filename() << "\n";
            fs::copy(custom_path, res_dir / fs::path(custom_path).filename(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
    }

    // Write VesselRun Wrapper
    // For now, assume 'java' is in PATH or bundled later.
    // If we bundle a JRE, we'll point to it here.
    std::cout << "Generating VesselRun execution wrapper for Java...\n";
    fs::path vessel_run_path = bin_dir / "VesselRun";
    std::ofstream vessel_run(vessel_run_path);
    vessel_run << "#!/bin/bash\n";
    vessel_run << "DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n";
    vessel_run << "if [ -z \"$JAVA_HOME\" ]; then\n";
    vessel_run << "  if [ -d \"$DIR/../runtime\" ]; then\n";
    vessel_run << "    export JAVA_HOME=\"$DIR/../runtime\"\n";
    vessel_run << "  fi\n";
    vessel_run << "fi\n";
    vessel_run << "if [ -n \"$JAVA_HOME\" ]; then\n";
    vessel_run << "  EXEC_JAVA=\"$JAVA_HOME/bin/java\"\n";
    vessel_run << "else\n";
    vessel_run << "  EXEC_JAVA=\"java\"\n";
    vessel_run << "fi\n";
    vessel_run << "exec \"$EXEC_JAVA\" -jar \"$DIR/" << target_jar.filename().string() << "\" \"$@\"\n";
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
        fallback << "{\n  \"name\": \"" << manifest.name << "\",\n";
        fallback << "  \"bin_file\": \"" << target_jar.filename().string() << "\",\n";
        fallback << "  \"version\": \"" << manifest.version << "\",\n";
        fallback << "  \"mode\": \"gradle\"\n";
        if (!manifest.runtime.type.empty()) {
            fallback << ", \"runtime\": { \"type\": \"" << manifest.runtime.type << "\", \"version\": \"" << manifest.runtime.version << "\" }\n";
        }
        fallback << "}\n";
        fallback.close();
    }

    return true;
}

} // namespace vessel
