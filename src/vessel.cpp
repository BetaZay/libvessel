#include "vessel.h"
#include <string>
#include <unordered_map>

enum class Command { Pack, Update, Remove, Init, Help, Unknown };

Command getCommand(const std::string &cmd) {
  static const std::unordered_map<std::string, Command> map = {
      {"pack", Command::Pack},
      {"update", Command::Update},
      {"remove", Command::Remove},
      {"init", Command::Init},
      {"help", Command::Help}};
  auto it = map.find(cmd);
  return (it != map.end()) ? it->second : Command::Unknown;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return vessel::help();
  }

  std::string commandStr = argv[1];

  switch (getCommand(commandStr)) {
  case Command::Pack:
    return vessel::pack(argc - 2, argv + 2);
  case Command::Update:
    return vessel::update(argc - 2, argv + 2);
  case Command::Remove:
    return vessel::remove(argc - 2, argv + 2);
  case Command::Init:
    return vessel::init();
  case Command::Help:
  case Command::Unknown:
    return vessel::help();
  }
}