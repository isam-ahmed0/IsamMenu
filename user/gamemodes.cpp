#include "pch-il2cpp.h"
#include "gamemodes.h"
#include "state.hpp"
#include "utility.h"
#include "game.h"
#include "logger.h"
#include <algorithm>
#include <utility>

namespace {

    class ShiftAndSneakMode : public GameMode {
    public:
        CustomGameMode Id() const override { return CustomGameMode::ShiftAndSneak; }
        const char* Name() const override { return "Shift and Sneak"; }
        bool OverridesRoleAssignment() const override { return true; }

        void OnAssignRoles() override {
            if (!IsHost()) return;
            auto allPlayers = GetAllPlayerControl();
            int n = (int)allPlayers.size();
            if (n <= 0) return;

            int impostorCount = GetMaxImpostorAmount(n);
            if (impostorCount < 1) impostorCount = 1;
            if (impostorCount > n) impostorCount = n;

            std::vector<PlayerControl*> players(allPlayers.begin(), allPlayers.end());
            std::vector<int> order(n);
            for (int i = 0; i < n; i++) order[i] = i;
            for (int i = 0; i < n - 1; i++) {
                int j = GenerateRandomNumber(i, n - 1);
                std::swap(order[i], order[j]);
            }

            for (int i = 0; i < n; i++) {
                auto p = players[order[i]];
                if (p == nullptr) continue;
                RoleTypes__Enum role = (i < impostorCount) ? RoleTypes__Enum::Shapeshifter : RoleTypes__Enum::Engineer;
                PlayerControl_RpcSetRole(p, role, false, NULL);
            }
        }

        bool OnCheckMurder(PlayerControl* killer, PlayerControl* target) override {
            if (killer == nullptr || target == nullptr) return false;
            return killer->fields.shapeshifting
                && killer->fields.shapeshiftTargetPlayerId == target->fields.PlayerId;
        }

        std::string Rules() const override {
            return "Shift and Sneak:\n\n"
                "The impostor is a Shapeshifter and may only kill a player while shifted into that exact player.\n"
                "All crewmates are Engineers and may use vents.\n"
                "Crewmates win by completing their tasks.";
        }
    };

    class InfectionMode : public GameMode {
    public:
        CustomGameMode Id() const override { return CustomGameMode::Infection; }
        const char* Name() const override { return "Infection"; }

        void OnMurder(PlayerControl* killer, PlayerControl* target) override {
            if (!IsHost() || killer == nullptr || target == nullptr) return;
            auto killerData = GetPlayerData(killer);
            auto targetData = GetPlayerData(target);
            if (killerData == nullptr || targetData == nullptr) return;
            if (!PlayerIsImpostor(killerData) || PlayerIsImpostor(targetData)) return;

            PlayerControl_Revive(target, NULL);
            PlayerControl_RpcSetRole(target, RoleTypes__Enum::Impostor, true, NULL);
            targetData->fields.IsDead = false;

            killer->fields.killTimer = (std::max)(GameOptions().GetKillCooldown(), 0.f);
        }

        std::string Rules() const override {
            return "Infection:\n\n"
                "Killed crewmates are revived as impostors instead of dying.\n"
                "Impostors win once everyone has been infected.\n"
                "Crewmates win by completing their tasks.";
        }
    };

    ShiftAndSneakMode g_shiftAndSneak;
    InfectionMode g_infection;

}

namespace Gamemodes {

    const std::vector<CustomGameMode>& Order() {
        static const std::vector<CustomGameMode> order = {
            CustomGameMode::None,
            CustomGameMode::TaskSpeedrun,
            CustomGameMode::BattleRoyale,
            CustomGameMode::Infection,
            CustomGameMode::ShiftAndSneak
        };
        return order;
    }

    std::vector<const char*> Names() {
        static const std::vector<const char*> names = {
            "Default",
            "Task Speedrun",
            "Battle Royale",
            "Infection",
            "Shift and Sneak"
        };
        return names;
    }

    GameMode* Get(CustomGameMode mode) {
        switch (mode) {
        case CustomGameMode::ShiftAndSneak: return &g_shiftAndSneak;
        case CustomGameMode::Infection: return &g_infection;
        default: return nullptr;
        }
    }

    GameMode* Active() {
        int index = std::clamp(State.GameMode, 0, (int)Order().size() - 1);
        CustomGameMode mode = FromIndex(index);
        if (mode == CustomGameMode::BattleRoyale && !State.DisableHostAnticheat) return nullptr;
        return Get(mode);
    }

    const char* Name(CustomGameMode mode) {
        int index = ToIndex(mode);
        auto names = Names();
        return names[std::clamp(index, 0, (int)names.size() - 1)];
    }

    bool RequiresHostAnticheat(CustomGameMode mode) {
        return mode == CustomGameMode::BattleRoyale;
    }

    int ToIndex(CustomGameMode mode) {
        auto& order = Order();
        for (size_t i = 0; i < order.size(); i++) {
            if (order[i] == mode) return (int)i;
        }
        return 0;
    }

    CustomGameMode FromIndex(int index) {
        auto& order = Order();
        return order[std::clamp(index, 0, (int)order.size() - 1)];
    }

    void SyncSelection() {
        CustomGameMode mode = FromIndex(State.GameMode);
        State.TaskSpeedrun = (mode == CustomGameMode::TaskSpeedrun);
        State.BattleRoyale = (mode == CustomGameMode::BattleRoyale) && State.DisableHostAnticheat;
        State.Infection = (mode == CustomGameMode::Infection);
        State.ShiftAndSneak = (mode == CustomGameMode::ShiftAndSneak);
    }

    std::string ActiveRules() {
        if (auto* mode = Active()) return mode->Rules();
        return std::string();
    }

}
