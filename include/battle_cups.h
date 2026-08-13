#ifndef GUARD_BATTLE_CUPS_H
#define GUARD_BATTLE_CUPS_H

#include "constants/battle_cups.h"

// Active challenge state (EWRAM; cleared when run ends)
struct BattleCupState
{
    u8 cupId;
    u8 rank;
    u8 battleNum;       // 0-7
    u8 rematchesLeft;
    u8 isRound2;
    u8 status;
    u8 selectedSlots[BATTLE_CUP_PARTY_SIZE]; // party indices of chosen 3
    u16 defeatedTrainerIds[BATTLE_CUP_BATTLES_PER_RUN]; // no repeat
    u8 defeatedCount;
    u8 perfectBattles;  // for rematch credit this run
};

extern struct BattleCupState gBattleCupState;

void InitBattleCupState(void);
u8 BattleCup_InitChallenge(void);          // uses VAR_BATTLE_CUP_ID/RANK/IS_R2; returns INIT_*
u8 BattleCup_ValidatePlayerParty(void);    // full party vs cup rules
u8 BattleCup_ValidateSelectedThree(void); // selectedSlots
bool8 BattleCup_IsFinalBattle(void);
void BattleCup_OnBattleWon(bool8 wasPerfect);
void BattleCup_OnBattleLost(void);
bool8 BattleCup_TryUseRematch(void);
void BattleCup_SetChampionFlags(void);
bool8 BattleCup_IsChampion(u8 cupId);
bool8 BattleCup_IsRound2Cleared(u8 cupId);
u8 BattleCup_GetUnlockedRank(u8 cupId);    // highest playable rank

// Script API (specials)
void Special_BattleCupInit(void);
void Special_BattleCupValidateParty(void);
void Special_BattleCupValidateSelected(void);
void Special_BattleCupIsFinal(void);
void Special_BattleCupGetBattleNum(void);
void Special_BattleCupGetRematches(void);
void Special_BattleCupSetSelectedFromVars(void);
void Special_BattleCupApplySelectedParty(void);
void Special_BattleCupRestoreParty(void);
void Special_BattleCupOnBattleWon(void);
void Special_BattleCupOnBattleLost(void);
void Special_BattleCupTryRematch(void);
void Special_BattleCupIsChampion(void);
void Special_BattleCupIsRound2Cleared(void);
u16 BattleCup_GenerateRandomOpponent(void);
u16 BattleCup_GenerateFinalOpponent(void);
void BattleCup_RegisterDefeatedTrainer(u16 trainerId);
void Special_BattleCupSetupOpponent(void);
void Special_BattleCupRegisterDefeated(void);

#endif // GUARD_BATTLE_CUPS_H
