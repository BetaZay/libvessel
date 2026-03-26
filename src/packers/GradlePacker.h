#pragma once
#include "Packer.h"

namespace vessel {

class GradlePacker : public Packer {
public:
    std::string get_default_manifest(const std::string& app_name) override;
    bool execute_build(const ManifestData& manifest) override;
    bool assemble_payload(const ManifestData& manifest, const std::filesystem::path& struct_dir) override;

private:
    std::string exec(const char *cmd);
};

} // namespace vessel
