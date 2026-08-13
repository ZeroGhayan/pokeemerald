#include "global.h"
#include "battle_cups.h"
#include "event_data.h"
#include "party_menu.h"
#include "pokemon.h"
#include "constants/species.h"
#include "constants/moves.h"
#include "constants/items.h"
#include "data.h"
#include "pokedex.h"

EWRAM_DATA struct BattleCupState gBattleCupState = {0};

// Progress bitfield in VAR_UNUSED_0x40FD:
// bits 0-3: R1 champ poke/pika/petit/prime
// bits 4-7: R2 cleared
// bits 8-10: poke rank unlocked (0=poke only, 1=great, 2=ultra, 3=master)
// bits 11-13: prime rank unlocked
#define VAR_BATTLE_CUP_PROGRESS 0x40FD

static u16 GetProgress(void)
{
    return VarGet(VAR_BATTLE_CUP_PROGRESS);
}

static void SetProgress(u16 value)
{
    VarSet(VAR_BATTLE_CUP_PROGRESS, value);
}

void InitBattleCupState(void)
{
    u8 i;
    gBattleCupState.cupId = 0;
    gBattleCupState.rank = 0;
    gBattleCupState.battleNum = 0;
    gBattleCupState.rematchesLeft = 0;
    gBattleCupState.isRound2 = 0;
    gBattleCupState.status = BATTLE_CUP_STATUS_NONE;
    gBattleCupState.defeatedCount = 0;
    gBattleCupState.perfectBattles = 0;
    for (i = 0; i < BATTLE_CUP_PARTY_SIZE; i++)
        gBattleCupState.selectedSlots[i] = PARTY_SIZE;
    for (i = 0; i < BATTLE_CUP_BATTLES_PER_RUN; i++)
        gBattleCupState.defeatedTrainerIds[i] = 0xFFFF;
}

bool8 BattleCup_IsChampion(u8 cupId)
{
    if (cupId >= BATTLE_CUP_COUNT)
        return FALSE;
    return (GetProgress() & (1 << cupId)) != 0;
}

bool8 BattleCup_IsRound2Cleared(u8 cupId)
{
    if (cupId >= BATTLE_CUP_COUNT)
        return FALSE;
    return (GetProgress() & (1 << (4 + cupId))) != 0;
}

u8 BattleCup_GetUnlockedRank(u8 cupId)
{
    u16 prog = GetProgress();
    u8 rankBits;

    if (cupId == BATTLE_CUP_POKE)
        rankBits = (prog >> 8) & 7;
    else if (cupId == BATTLE_CUP_PRIME)
        rankBits = (prog >> 11) & 7;
    else
        return 0; // Pika/Petit have no ranks

    if (rankBits > BATTLE_CUP_RANK_MASTER_BALL)
        rankBits = BATTLE_CUP_RANK_MASTER_BALL;
    return rankBits;
}

void BattleCup_SetChampionFlags(void)
{
    u16 prog = GetProgress();
    u8 cup = gBattleCupState.cupId;
    u8 rank = gBattleCupState.rank;

    if (gBattleCupState.isRound2)
    {
        prog |= (1 << (4 + cup));
    }
    else
    {
        prog |= (1 << cup);
        // Unlock next rank for Poké/Prime when winning current rank
        if (cup == BATTLE_CUP_POKE || cup == BATTLE_CUP_PRIME)
        {
            u8 unlocked = BattleCup_GetUnlockedRank(cup);
            if (rank >= unlocked && rank < BATTLE_CUP_RANK_MASTER_BALL)
            {
                u8 shift = (cup == BATTLE_CUP_POKE) ? 8 : 11;
                prog &= ~(7 << shift);
                prog |= ((rank + 1) & 7) << shift;
            }
            else if (rank == BATTLE_CUP_RANK_MASTER_BALL)
            {
                u8 shift = (cup == BATTLE_CUP_POKE) ? 8 : 11;
                prog &= ~(7 << shift);
                prog |= (BATTLE_CUP_RANK_MASTER_BALL & 7) << shift;
            }
        }
    }
    SetProgress(prog);
}

// ---- Validation helpers ----
static bool8 SpeciesHasPreEvolution(u16 species)
{
    u16 i, j;
    for (i = 1; i < NUM_SPECIES; i++)
    {
        for (j = 0; j < EVOS_PER_MON; j++)
        {
            if (gEvolutionTable[i][j].method != 0
             && gEvolutionTable[i][j].targetSpecies == species)
                return TRUE;
        }
    }
    return FALSE;
}

static bool8 BattleCup_IsSpeciesBanned(u8 cupId, u16 species)
{
    // Event clause: Lugia / Ho-Oh always banned
    if (species == SPECIES_LUGIA || species == SPECIES_HO_OH)
        return TRUE;

    switch (cupId)
    {
    case BATTLE_CUP_POKE:
    case BATTLE_CUP_PIKA:
    case BATTLE_CUP_PRIME:
        if (species == SPECIES_MEWTWO || species == SPECIES_MEW)
            return TRUE;
        break;
    case BATTLE_CUP_PETIT:
        if (species == SPECIES_MEW)
            return TRUE;
        break;
    }
    return FALSE;
}

static bool8 BattleCup_SpeciesPassesPetitPhysique(u16 species)
{
    u16 dex = SpeciesToNationalPokedexNum(species);
    u16 height = gPokedexEntries[dex].height; // dm
    u16 weight = gPokedexEntries[dex].weight; // hg
    if (height > PETIT_CUP_MAX_HEIGHT_DM)
        return FALSE;
    if (weight > PETIT_CUP_MAX_WEIGHT_HG)
        return FALSE;
    if (SpeciesHasPreEvolution(species))
        return FALSE; // lowest evolution only
    return TRUE;
}

static bool8 BattleCup_LevelOk(u8 cupId, u8 level)
{
    switch (cupId)
    {
    case BATTLE_CUP_POKE:
        return level >= POKE_CUP_MIN_LEVEL && level <= POKE_CUP_MAX_LEVEL;
    case BATTLE_CUP_PIKA:
        return level >= PIKA_CUP_MIN_LEVEL && level <= PIKA_CUP_MAX_LEVEL;
    case BATTLE_CUP_PETIT:
        return level >= PETIT_CUP_MIN_LEVEL && level <= PETIT_CUP_MAX_LEVEL;
    case BATTLE_CUP_PRIME:
        return TRUE;
    }
    return FALSE;
}

static u16 BattleCup_MaxTotalLevel(u8 cupId)
{
    switch (cupId)
    {
    case BATTLE_CUP_POKE:  return POKE_CUP_TOTAL_LEVEL;
    case BATTLE_CUP_PIKA:  return PIKA_CUP_TOTAL_LEVEL;
    case BATTLE_CUP_PETIT: return PETIT_CUP_TOTAL_LEVEL;
    default:               return 0xFFFF; // unlimited
    }
}

static bool8 BattleCup_CupUsesItemClause(u8 cupId)
{
    return cupId == BATTLE_CUP_POKE || cupId == BATTLE_CUP_PRIME;
}

// Validate a list of party slot indices (count mons). Eggs invalid.
static u8 BattleCup_ValidateSlotList(u8 cupId, const u8 *slots, u8 count)
{
    u8 i, j;
    u16 totalLevel = 0;
    u16 speciesSeen[PARTY_SIZE];
    u16 itemsSeen[PARTY_SIZE];
    u8 speciesCount = 0;
    u8 itemCount = 0;

    if (count == 0)
        return BATTLE_CUP_VALID_FAIL_GENERIC;

    for (i = 0; i < count; i++)
    {
        struct Pokemon *mon;
        u16 species;
        u8 level;
        u16 item;

        if (slots[i] >= PARTY_SIZE)
            return BATTLE_CUP_VALID_FAIL_GENERIC;

        mon = &gPlayerParty[slots[i]];
        species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL);
        if (species == SPECIES_NONE || species == SPECIES_EGG)
            return BATTLE_CUP_VALID_FAIL_GENERIC;

        if (BattleCup_IsSpeciesBanned(cupId, species))
            return BATTLE_CUP_VALID_FAIL_GENERIC;

        level = GetMonData(mon, MON_DATA_LEVEL, NULL);
        if (!BattleCup_LevelOk(cupId, level))
            return BATTLE_CUP_VALID_FAIL_GENERIC;

        totalLevel += level;

        if (cupId == BATTLE_CUP_PETIT && !BattleCup_SpeciesPassesPetitPhysique(species))
            return BATTLE_CUP_VALID_FAIL_GENERIC;

        // Species clause
        for (j = 0; j < speciesCount; j++)
        {
            if (speciesSeen[j] == species)
                return BATTLE_CUP_VALID_FAIL_GENERIC;
        }
        speciesSeen[speciesCount++] = species;

        // Item clause (Poké / Prime)
        item = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
        if (BattleCup_CupUsesItemClause(cupId) && item != ITEM_NONE)
        {
            for (j = 0; j < itemCount; j++)
            {
                if (itemsSeen[j] == item)
                    return BATTLE_CUP_VALID_FAIL_GENERIC;
            }
            itemsSeen[itemCount++] = item;
        }
    }

    if (totalLevel > BattleCup_MaxTotalLevel(cupId))
        return BATTLE_CUP_VALID_FAIL_GENERIC;

    return BATTLE_CUP_VALID_OK;
}

u8 BattleCup_ValidatePlayerParty(void)
{
    u8 slots[PARTY_SIZE];
    u8 count = 0;
    u8 i;
    u8 cupId = VarGet(VAR_BATTLE_CUP_ID);

    // All non-egg party mons must be legal for the cup (enter check)
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG, NULL);
        if (species != SPECIES_NONE && species != SPECIES_EGG)
            slots[count++] = i;
    }

    if (count < BATTLE_CUP_PARTY_SIZE)
        return BATTLE_CUP_VALID_FAIL_GENERIC;

    return BattleCup_ValidateSlotList(cupId, slots, count);
}

u8 BattleCup_ValidateSelectedThree(void)
{
    u8 cupId = VarGet(VAR_BATTLE_CUP_ID);
    return BattleCup_ValidateSlotList(cupId, gBattleCupState.selectedSlots, BATTLE_CUP_PARTY_SIZE);
}


u8 BattleCup_InitChallenge(void)
{
    u8 cup = VarGet(VAR_BATTLE_CUP_ID);
    u8 rank = VarGet(VAR_BATTLE_CUP_RANK);
    u8 isR2 = VarGet(VAR_BATTLE_CUP_IS_R2);

    InitBattleCupState();

    if (cup >= BATTLE_CUP_COUNT)
        return BATTLE_CUP_INIT_NOT_UNLOCKED;

    if (isR2 && !BattleCup_IsChampion(cup))
        return BATTLE_CUP_INIT_NOT_UNLOCKED;

    if ((cup == BATTLE_CUP_POKE || cup == BATTLE_CUP_PRIME) && !isR2)
    {
        if (rank > BattleCup_GetUnlockedRank(cup))
            return BATTLE_CUP_INIT_NOT_UNLOCKED;
    }

    if (BattleCup_ValidatePlayerParty() != BATTLE_CUP_VALID_OK)
        return BATTLE_CUP_INIT_INVALID_PARTY;

    gBattleCupState.cupId = cup;
    gBattleCupState.rank = rank;
    gBattleCupState.isRound2 = isR2;
    gBattleCupState.battleNum = 0;
    gBattleCupState.rematchesLeft = 0;
    gBattleCupState.status = BATTLE_CUP_STATUS_ACTIVE;
    gBattleCupState.defeatedCount = 0;
    gBattleCupState.perfectBattles = 0;

    VarSet(VAR_BATTLE_CUP_BATTLE_NUM, 0);
    VarSet(VAR_BATTLE_CUP_REMATCHES, 0);
    VarSet(VAR_BATTLE_CUP_STATUS, BATTLE_CUP_STATUS_ACTIVE);
    return BATTLE_CUP_INIT_OK;
}

bool8 BattleCup_IsFinalBattle(void)
{
    if (gBattleCupState.status != BATTLE_CUP_STATUS_ACTIVE)
        return FALSE;
    // Final fixed trainer only on Master rank (Poké/Prime) or always for Pika/Petit
    if (gBattleCupState.battleNum != BATTLE_CUP_BATTLES_PER_RUN - 1)
        return FALSE;
    if (gBattleCupState.cupId == BATTLE_CUP_PIKA || gBattleCupState.cupId == BATTLE_CUP_PETIT)
        return TRUE;
    return (gBattleCupState.rank == BATTLE_CUP_RANK_MASTER_BALL);
}

void BattleCup_OnBattleWon(bool8 wasPerfect)
{
    if (wasPerfect)
    {
        gBattleCupState.perfectBattles++;
        gBattleCupState.rematchesLeft++;
        VarSet(VAR_BATTLE_CUP_REMATCHES, gBattleCupState.rematchesLeft);
    }

    gBattleCupState.battleNum++;
    VarSet(VAR_BATTLE_CUP_BATTLE_NUM, gBattleCupState.battleNum);

    if (gBattleCupState.battleNum >= BATTLE_CUP_BATTLES_PER_RUN)
    {
        gBattleCupState.status = BATTLE_CUP_STATUS_WON;
        VarSet(VAR_BATTLE_CUP_STATUS, BATTLE_CUP_STATUS_WON);
        BattleCup_SetChampionFlags();
    }
}

void BattleCup_OnBattleLost(void)
{
    if (gBattleCupState.rematchesLeft > 0 && !gBattleCupState.isRound2)
    {
        // Script should ask; TryUseRematch consumes
        return;
    }
    gBattleCupState.status = BATTLE_CUP_STATUS_LOST;
    VarSet(VAR_BATTLE_CUP_STATUS, BATTLE_CUP_STATUS_LOST);
}

bool8 BattleCup_TryUseRematch(void)
{
    if (gBattleCupState.isRound2)
        return FALSE;
    if (gBattleCupState.rematchesLeft == 0)
        return FALSE;
    gBattleCupState.rematchesLeft--;
    VarSet(VAR_BATTLE_CUP_REMATCHES, gBattleCupState.rematchesLeft);
    // stay on same battleNum for retry
    gBattleCupState.status = BATTLE_CUP_STATUS_ACTIVE;
    VarSet(VAR_BATTLE_CUP_STATUS, BATTLE_CUP_STATUS_ACTIVE);
    return TRUE;
}

// ---- Specials ----
void Special_BattleCupInit(void)
{
    gSpecialVar_Result = BattleCup_InitChallenge();
}

void Special_BattleCupValidateParty(void)
{
    gSpecialVar_Result = BattleCup_ValidatePlayerParty();
}

void Special_BattleCupValidateSelected(void)
{
    gSpecialVar_Result = BattleCup_ValidateSelectedThree();
}

void Special_BattleCupIsFinal(void)
{
    gSpecialVar_Result = BattleCup_IsFinalBattle();
}

void Special_BattleCupGetBattleNum(void)
{
    gSpecialVar_Result = gBattleCupState.battleNum;
}

void Special_BattleCupGetRematches(void)
{
    gSpecialVar_Result = gBattleCupState.rematchesLeft;
}
