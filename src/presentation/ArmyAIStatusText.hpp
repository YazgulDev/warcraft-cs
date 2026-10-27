#pragma once
#include "../squad/ArmyAIStatus.hpp"

// The HUD translates copied domain status; it never issues native orders.
inline const char* ArmyAIStatusText(ArmyAIStatus status) {
    switch(status) {
    case ArmyAIStatus::Off:return "ARMY AI OFF";
    case ArmyAIStatus::Scanning:return "ARMY AI ON: CHECKING BASE";
    case ArmyAIStatus::Working:return "ARMY AI: DEVELOPING BASE";
    case ArmyAIStatus::Waiting:return "ARMY AI: CONSTRUCTION / TRAINING IN PROGRESS";
    case ArmyAIStatus::NoEconomy:return "ARMY AI: NO SUPPORTED WORKERS OR BASE";
    case ArmyAIStatus::NoWorkers:return "ARMY AI: NO WORKERS / WORKER TRAINING BLOCKED";
    case ArmyAIStatus::WorkersBusy:return "ARMY AI: WORKERS BUSY OR ASSIGNED TO SQUAD";
    case ArmyAIStatus::NoResources:return "ARMY AI: NO NEARBY GOLD MINE OR TREES";
    case ArmyAIStatus::Blocked:return "ARMY AI: BUILD / TRAIN BLOCKED BY RESOURCES OR MAP";
    case ArmyAIStatus::Ready:return "ARMY AI: BASE TARGETS REACHED";
    default:return "ARMY AI: CONTROLLED UNIT UNAVAILABLE";
    }
}
