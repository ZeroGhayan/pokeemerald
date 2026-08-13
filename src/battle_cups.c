#include "global.h"
#include "battle_cups.h"
#include "event_data.h"
#include "constants/trainers.h"
#include "battle_setup.h"
#include "battle.h"
#include "random.h"
#include "party_menu.h"
#include "string_util.h"
#include "script_pokemon_util.h"
#include "load_save.h"
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


bool8 BattleCup_IsActive(void)
{
    return gBattleCupState.status == BATTLE_CUP_STATUS_ACTIVE;
}

bool8 BattleCup_SideAlreadyHasStatus(u8 side, u32 statusMask)
{
    u32 i;
    struct Pokemon *party = (side == B_SIDE_PLAYER) ? gPlayerParty : gEnemyParty;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
        u32 status;
        if (species == SPECIES_NONE || species == SPECIES_EGG)
            continue;
        status = GetMonData(&party[i], MON_DATA_STATUS, NULL);
        if (status & statusMask)
            return TRUE;
    }
    for (i = 0; i < gBattlersCount; i++)
    {
        if (GetBattlerSide(i) == side
         && gBattleMons[i].hp > 0
         && (gBattleMons[i].status1 & statusMask))
            return TRUE;
    }
    return FALSE;
}

bool8 BattleCup_IsLastMonOnSide(u8 battler)
{
    u8 side = GetBattlerSide(battler);
    struct Pokemon *party = (side == B_SIDE_PLAYER) ? gPlayerParty : gEnemyParty;
    u8 partyIndex = gBattlerPartyIndexes[battler];
    u32 i;
    u8 othersAlive = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
        if (species == SPECIES_NONE || species == SPECIES_EGG)
            continue;
        if (i == partyIndex)
            continue;
        if (GetMonData(&party[i], MON_DATA_HP, NULL) > 0)
            othersAlive++;
    }
    for (i = 0; i < gBattlersCount; i++)
    {
        if (i != battler && GetBattlerSide(i) == side && gBattleMons[i].hp > 0)
            othersAlive++;
    }
    return othersAlive == 0;
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

// ---- Step 4: random opponents (battles 1-7) ----

static bool8 BattleCup_SpeciesLegalForOpponent(u8 cupId, u16 species)
{
    if (species < SPECIES_BULBASAUR || species > SPECIES_MEW)
        return FALSE;
    if (BattleCup_IsSpeciesBanned(cupId, species))
        return FALSE;
    if (cupId == BATTLE_CUP_PETIT && !BattleCup_SpeciesPassesPetitPhysique(species))
        return FALSE;
    return TRUE;
}

static u8 BattleCup_PickLevelForSlot(u8 cupId, u8 slotIndex, u16 levelsLeft, u8 monsLeft)
{
    u8 minL, maxL, avg, level;

    switch (cupId)
    {
    case BATTLE_CUP_POKE:
        minL = POKE_CUP_MIN_LEVEL;
        maxL = POKE_CUP_MAX_LEVEL;
        break;
    case BATTLE_CUP_PIKA:
        minL = PIKA_CUP_MIN_LEVEL;
        maxL = PIKA_CUP_MAX_LEVEL;
        break;
    case BATTLE_CUP_PETIT:
        minL = PETIT_CUP_MIN_LEVEL;
        maxL = PETIT_CUP_MAX_LEVEL;
        break;
    default: // PRIME
        // Scale a bit with battle number
        minL = 50 + gBattleCupState.battleNum * 5;
        maxL = 80 + gBattleCupState.battleNum * 2;
        if (maxL > MAX_LEVEL)
            maxL = MAX_LEVEL;
        if (minL > maxL)
            minL = maxL;
        return minL + (Random() % (maxL - minL + 1));
    }

    // Leave room for remaining mons at min level
    {
        u16 maxAllowed = levelsLeft - (monsLeft - 1) * minL;
        if (maxAllowed > maxL)
            maxAllowed = maxL;
        if (maxAllowed < minL)
            maxAllowed = minL;
        if (slotIndex == monsLeft - 1)
        {
            // last mon: use remaining but clamp
            level = levelsLeft;
            if (level < minL)
                level = minL;
            if (level > maxL)
                level = maxL;
            return level;
        }
        avg = (minL + maxAllowed) / 2;
        level = minL + (Random() % (maxAllowed - minL + 1));
        (void)avg;
        return level;
    }
}

static u16 BattleCup_PickSpecies(u8 cupId, const u16 *already, u8 alreadyCount)
{
    u16 species;
    u16 tries = 0;
    u8 i;
    bool8 dup;

    do
    {
        species = SPECIES_BULBASAUR + (Random() % 151); // 1..151
        dup = FALSE;
        for (i = 0; i < alreadyCount; i++)
        {
            if (already[i] == species)
            {
                dup = TRUE;
                break;
            }
        }
        if (!dup && BattleCup_SpeciesLegalForOpponent(cupId, species))
            return species;
    } while (++tries < 500);

    // Fallback: first legal species
    for (species = SPECIES_BULBASAUR; species <= SPECIES_MEW; species++)
    {
        dup = FALSE;
        for (i = 0; i < alreadyCount; i++)
        {
            if (already[i] == species)
            {
                dup = TRUE;
                break;
            }
        }
        if (!dup && BattleCup_SpeciesLegalForOpponent(cupId, species))
            return species;
    }
    return SPECIES_RATTATA;
}

// Fills gEnemyParty with 3 legal mons. Returns synthetic trainer id for no-repeat tracking.

static void BattleCup_CreateFinalMon(struct Pokemon *mon, const struct BattleCupFinalMon *def)
{
    u8 i;
    CreateMon(mon, def->species, def->level, 31, FALSE, 0, OT_ID_RANDOM_NO_SHINY, 0);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, def->moves[i], i);
}

// Builds final team: pool[0] always + 2 random unique; order shuffled.
u16 BattleCup_GenerateFinalOpponent(void)
{
    const struct BattleCupFinalist *fin;
    u8 pick[BATTLE_CUP_PARTY_SIZE];
    u8 order[BATTLE_CUP_PARTY_SIZE];
    u8 used[16] = {0};
    u8 i, j, tmp, slot;
    u16 trainerId;

    fin = &sBattleCupFinalists[gBattleCupState.cupId];
    ZeroEnemyPartyMons();

    // Always take index 0
    pick[0] = 0;
    used[0] = TRUE;

    // Two more from 1..poolSize-1
    for (i = 1; i < BATTLE_CUP_PARTY_SIZE; i++)
    {
        u16 tries = 0;
        do {
            slot = 1 + (Random() % (fin->poolSize - 1));
            tries++;
        } while (used[slot] && tries < 64);

        if (used[slot])
        {
            for (slot = 1; slot < fin->poolSize; slot++)
            {
                if (!used[slot])
                    break;
            }
        }
        used[slot] = TRUE;
        pick[i] = slot;
    }

    // Shuffle order so fixed mon can be 1st/2nd/3rd
    order[0] = 0;
    order[1] = 1;
    order[2] = 2;
    for (i = 0; i < BATTLE_CUP_PARTY_SIZE; i++)
    {
        j = Random() % BATTLE_CUP_PARTY_SIZE;
        tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    for (i = 0; i < BATTLE_CUP_PARTY_SIZE; i++)
        BattleCup_CreateFinalMon(&gEnemyParty[i], &fin->pool[pick[order[i]]]);

    // Buffer name for scripts (StringVar1)
    StringCopy(gStringVar1, fin->name);

    trainerId = 9000 + gBattleCupState.cupId; // stable per-cup final id
    return trainerId;
}

u16 BattleCup_GenerateRandomOpponent(void)
{
    u8 cupId = gBattleCupState.cupId;
    u16 species[BATTLE_CUP_PARTY_SIZE];
    u8 levels[BATTLE_CUP_PARTY_SIZE];
    u16 totalBudget;
    u16 levelsLeft;
    u8 i;
    u16 trainerId;

    ZeroEnemyPartyMons();

    switch (cupId)
    {
    case BATTLE_CUP_POKE:  totalBudget = POKE_CUP_TOTAL_LEVEL; break;
    case BATTLE_CUP_PIKA:  totalBudget = PIKA_CUP_TOTAL_LEVEL; break;
    case BATTLE_CUP_PETIT: totalBudget = PETIT_CUP_TOTAL_LEVEL; break;
    default:               totalBudget = 0xFFFF; break;
    }

    levelsLeft = totalBudget;
    for (i = 0; i < BATTLE_CUP_PARTY_SIZE; i++)
    {
        species[i] = BattleCup_PickSpecies(cupId, species, i);
        levels[i] = BattleCup_PickLevelForSlot(cupId, i, levelsLeft, BATTLE_CUP_PARTY_SIZE - i);
        if (levelsLeft != 0xFFFF)
            levelsLeft -= levels[i];
    }

    for (i = 0; i < BATTLE_CUP_PARTY_SIZE; i++)
    {
        u8 iv = 16 + (Random() % 15); // mid-high IVs
        CreateMon(&gEnemyParty[i], species[i], levels[i], iv, FALSE, 0, OT_ID_RANDOM_NO_SHINY, 0);
        GiveMonInitialMoveset(&gEnemyParty[i]);
    }

    // Synthetic unique id for eliminatory tracking
    trainerId = 1000 + (Random() % 8000);
    for (i = 0; i < gBattleCupState.defeatedCount; i++)
    {
        if (gBattleCupState.defeatedTrainerIds[i] == trainerId)
            trainerId++;
    }
    return trainerId;
}

void BattleCup_RegisterDefeatedTrainer(u16 trainerId)
{
    if (gBattleCupState.defeatedCount < BATTLE_CUP_BATTLES_PER_RUN)
    {
        gBattleCupState.defeatedTrainerIds[gBattleCupState.defeatedCount++] = trainerId;
    }
}

void Special_BattleCupSetupOpponent(void)
{
    u16 trainerId;

    if (BattleCup_IsFinalBattle())
    {
        trainerId = BattleCup_GenerateFinalOpponent();
        gSpecialVar_Result = 1; // final battle
    }
    else
    {
        trainerId = BattleCup_GenerateRandomOpponent();
        gSpecialVar_Result = 0;
    }

    VarSet(VAR_TEMP_0, trainerId);
    gTrainerBattleOpponent_A = trainerId;
}

void Special_BattleCupRegisterDefeated(void)
{
    BattleCup_RegisterDefeatedTrainer(VarGet(VAR_TEMP_0));
    gSpecialVar_Result = 0;
}

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

// ---- Step 3: select 3 / apply party / win-lose / rematch ----

// Script sets VAR_0x8000/01/02 to party indices (0-based).
// Copies into gBattleCupState.selectedSlots and validates.
void Special_BattleCupSetSelectedFromVars(void)
{
    u8 i;
    gBattleCupState.selectedSlots[0] = VarGet(VAR_0x8000);
    gBattleCupState.selectedSlots[1] = VarGet(VAR_0x8001);
    gBattleCupState.selectedSlots[2] = VarGet(VAR_0x8002);

    // Reject duplicate slots
    if (gBattleCupState.selectedSlots[0] == gBattleCupState.selectedSlots[1]
     || gBattleCupState.selectedSlots[0] == gBattleCupState.selectedSlots[2]
     || gBattleCupState.selectedSlots[1] == gBattleCupState.selectedSlots[2])
    {
        gSpecialVar_Result = BATTLE_CUP_VALID_FAIL_GENERIC;
        return;
    }

    gSpecialVar_Result = BattleCup_ValidateSelectedThree();
}

// Backup full party, shrink to the 3 selected (Frontier-style, 1-based order array).
void Special_BattleCupApplySelectedParty(void)
{
    u8 i;
    SavePlayerParty();
    for (i = 0; i < BATTLE_CUP_PARTY_SIZE; i++)
        gSelectedOrderFromParty[i] = gBattleCupState.selectedSlots[i] + 1;
    for (; i < MAX_FRONTIER_PARTY_SIZE; i++)
        gSelectedOrderFromParty[i] = 0;
    ReducePlayerPartyToSelectedMons();
    gSpecialVar_Result = 0;
}

void Special_BattleCupRestoreParty(void)
{
    LoadPlayerParty();
    CalculatePlayerPartyCount();
    gSpecialVar_Result = 0;
}

// VAR_0x8000 = 1 if perfect (no player mon fainted this battle)
void Special_BattleCupOnBattleWon(void)
{
    bool8 perfect = (VarGet(VAR_0x8000) != 0);
    BattleCup_OnBattleWon(perfect);
    gSpecialVar_Result = gBattleCupState.status; // ACTIVE or WON
}

void Special_BattleCupOnBattleLost(void)
{
    BattleCup_OnBattleLost();
    // Result: TRUE if rematch available
    gSpecialVar_Result = (!gBattleCupState.isRound2 && gBattleCupState.rematchesLeft > 0);
}

void Special_BattleCupTryRematch(void)
{
    gSpecialVar_Result = BattleCup_TryUseRematch();
}

void Special_BattleCupIsChampion(void)
{
    gSpecialVar_Result = BattleCup_IsChampion(VarGet(VAR_BATTLE_CUP_ID));
}

void Special_BattleCupIsRound2Cleared(void)
{
    gSpecialVar_Result = BattleCup_IsRound2Cleared(VarGet(VAR_BATTLE_CUP_ID));
}
