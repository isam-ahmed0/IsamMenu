#pragma once
#include "state.hpp"
#include "utility.h"
#include <string>
#include <vector>

enum class CustomGameMode : int {
    None = 0,
    TaskSpeedrun = 1,
    BattleRoyale = 2,
    Infection = 3,
    ShiftAndSneak = 4
};

class GameMode {
public:
    virtual ~GameMode() = default;
    virtual CustomGameMode Id() const = 0;
    virtual const char* Name() const = 0;
    virtual bool RequiresHostAnticheat() const { return false; }
    virtual bool OverridesRoleAssignment() const { return false; }
    virtual void OnAssignRoles() {}
    virtual void OnGameStart() {}
    virtual void OnUpdate() {}
    virtual bool OnCheckMurder(PlayerControl* killer, PlayerControl* target) { return true; }
    virtual void OnMurder(PlayerControl* killer, PlayerControl* target) {}
    virtual std::string Rules() const { return std::string(); }
    virtual void Reset() {}
};

namespace Gamemodes {
    const std::vector<CustomGameMode>& Order();
    std::vector<const char*> Names();
    GameMode* Get(CustomGameMode mode);
    GameMode* Active();
    const char* Name(CustomGameMode mode);
    bool RequiresHostAnticheat(CustomGameMode mode);
    int ToIndex(CustomGameMode mode);
    CustomGameMode FromIndex(int index);
    void SyncSelection();
    std::string ActiveRules();
}
