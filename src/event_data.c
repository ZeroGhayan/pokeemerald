#include "global.h"
#include "event_data.h"
#include "pokedex.h"

#define SPECIAL_FLAGS_SIZE  (NUM_SPECIAL_FLAGS / 8)

static void ClearTempFieldEventData(void);
static void ClearDailyFlags(void);
static void DisableNationalPokedex(void);

void ClearTempFieldEventData(void)
{
    memset(&gSaveBlock1Ptr->flags[TEMP_FLAGS_START / 8], 0, TEMP_FLAGS_SIZE);
    memset(&gSaveBlock1Ptr->vars[TEMP_VARS_START], 0, TEMP_VARS_SIZE * 2);
}

void ClearDailyFlags(void)
{
    memset(&gSaveBlock1Ptr->flags[DAILY_FLAGS_START / 8], 0, DAILY_FLAGS_SIZE);
}

void DisableNationalPokedex(void)
{
    u16 *nationalDexVar = GetVarPointer(VAR_NATIONAL_DEX);
    gSaveBlock2Ptr->pokedex.nationalMagic = 0;
    *nationalDexVar = 0;
    FlagClear(FLAG_SYS_NATIONAL_DEX);
}

void EnableNationalPokedex(void)
{
    // National Dex is permanently disabled in this HackROM (Kanto-only)
    // Do nothing on purpose.
}

bool32 IsNationalPokedexEnabled(void)
{
    // Always return FALSE - National Dex can never be enabled
    return FALSE;
}

void DisableMysteryEvent(void)
{
    FlagClear(FLAG_SYS_MYSTERY_EVENT_ENABLE);
}

void EnableMysteryEvent(void)
{
    FlagSet(FLAG_SYS_MYSTERY_EVENT_ENABLE);
}

bool32 IsMysteryEventEnabled(void)
{
    return FlagGet(FLAG_SYS_MYSTERY_EVENT_ENABLE);
}

void DisableMysteryGift(void)
{
    FlagClear(FLAG_SYS_MYSTERY_GIFT_ENABLE);
}

void EnableMysteryGift(void)
{
    FlagSet(FLAG_SYS_MYSTERY_GIFT_ENABLE);
}

bool32 IsMysteryGiftEnabled(void)
{
    return FlagGet(FLAG_SYS_MYSTERY_GIFT_ENABLE);
}

void ClearMysteryEventFlags(void)
{
    FlagClear(FLAG_MYSTERY_EVENT_DONE);
    FlagClear(FLAG_MYSTERY_EVENT_1);
    FlagClear(FLAG_MYSTERY_EVENT_2);
    FlagClear(FLAG_MYSTERY_EVENT_3);
    FlagClear(FLAG_MYSTERY_EVENT_4);
    FlagClear(FLAG_MYSTERY_EVENT_5);
    FlagClear(FLAG_MYSTERY_EVENT_6);
    FlagClear(FLAG_MYSTERY_EVENT_7);
    FlagClear(FLAG_MYSTERY_EVENT_8);
    FlagClear(FLAG_MYSTERY_EVENT_9);
    FlagClear(FLAG_MYSTERY_EVENT_10);
    FlagClear(FLAG_MYSTERY_EVENT_11);
    FlagClear(FLAG_MYSTERY_EVENT_12);
    FlagClear(FLAG_MYSTERY_EVENT_13);
    FlagClear(FLAG_MYSTERY_EVENT_14);
    FlagClear(FLAG_MYSTERY_EVENT_15);
}

void ClearMysteryEventVars(void)
{
    VarSet(VAR_EVENT_PICHU_SLOT, 0);
    VarSet(VAR_MYSTERY_EVENT_1, 0);
    VarSet(VAR_MYSTERY_EVENT_2, 0);
    VarSet(VAR_MYSTERY_EVENT_3, 0);
    VarSet(VAR_MYSTERY_EVENT_4, 0);
    VarSet(VAR_MYSTERY_EVENT_5, 0);
    VarSet(VAR_0x4030, 0);
    VarSet(VAR_0x4031, 0);
    VarSet(VAR_0x4032, 0);
    VarSet(VAR_0x4033, 0);
    VarSet(VAR_0x4034, 0);
}

void ClearSpecialFlags(void)
{
    memset(&gSpecialFlags[0], 0, SPECIAL_FLAGS_SIZE);
}
