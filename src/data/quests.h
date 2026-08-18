#ifndef GUARD_DATA_QUESTS_H
#define GUARD_DATA_QUESTS_H

#include "constants/quests.h"

// Placeholder English copy — replace when map/scripts are ready.

static const u8 sCatMain[]   = _("MAIN STORY");
static const u8 sCatGym[]    = _("GYM BADGES");
static const u8 sCatCup[]    = _("BATTLE CUPS");
static const u8 sCatCanopy[] = _("CANOPY CORPS");
static const u8 sCatSide[]   = _("SIDE QUESTS");

const u8 *const gQuestCategoryNames[QUEST_CAT_COUNT] = {
    [QUEST_CAT_MAIN]   = sCatMain,
    [QUEST_CAT_GYM]    = sCatGym,
    [QUEST_CAT_CUP]    = sCatCup,
    [QUEST_CAT_CANOPY] = sCatCanopy,
    [QUEST_CAT_SIDE]   = sCatSide,
};

// QUEST_MAIN_AWAKENING
static const u8 sMainTitle[] = _("Awakening");
static const u8 sMainS1[] = _("You woke up in the East with no memories.");
static const u8 sMainS2[] = _("Obtain your first POKéMON.");
static const u8 sMainS3[] = _("Travel to HEARTCOURT.");
static const struct QuestStepInfo sMainSteps[] = {
    {sMainS1}, {sMainS2}, {sMainS3},
};

// QUEST_GYM_FIRST_BADGE
static const u8 sGymTitle[] = _("The First Badge");
static const u8 sGymS1[] = _("Challenge the BUG-type GYM in MISTBROOK.");
static const u8 sGymS2[] = _("Defeat the MISTBROOK GYM LEADER.");
static const struct QuestStepInfo sGymSteps[] = {
    {sGymS1}, {sGymS2},
};

// QUEST_CUP_PIKA
static const u8 sCupTitle[] = _("Pika Cup");
static const u8 sCupS1[] = _("Learn about the PIKA CUP in the East.");
static const u8 sCupS2[] = _("Enter the PIKA CUP with a valid team.");
static const u8 sCupS3[] = _("Become the PIKA CUP champion.");
static const struct QuestStepInfo sCupSteps[] = {
    {sCupS1}, {sCupS2}, {sCupS3},
};

// QUEST_CANOPY_CONTACT
static const u8 sCanTitle[] = _("Contact Canopy");
static const u8 sCanS1[] = _("Find the path from the East up to CANOPY.");
static const u8 sCanS2[] = _("Speak with the CANOPY CORPS.");
static const struct QuestStepInfo sCanSteps[] = {
    {sCanS1}, {sCanS2},
};

// QUEST_SIDE_DAYCARE
static const u8 sDayTitle[] = _("Day Care Opening");
static const u8 sDayS1[] = _("Visit the DAY CARE in HEARTCOURT.");
static const u8 sDayS2[] = _("Help the DAY CARE after the TEAM ROCKET attack.");
static const u8 sDayS3[] = _("The DAY CARE is open for business.");
static const struct QuestStepInfo sDaySteps[] = {
    {sDayS1}, {sDayS2}, {sDayS3},
};

const struct QuestInfo gQuests[QUEST_COUNT] = {
    [QUEST_MAIN_AWAKENING] = {
        .title = sMainTitle,
        .category = QUEST_CAT_MAIN,
        .stepCount = 3,
        .steps = sMainSteps,
    },
    [QUEST_GYM_FIRST_BADGE] = {
        .title = sGymTitle,
        .category = QUEST_CAT_GYM,
        .stepCount = 2,
        .steps = sGymSteps,
    },
    [QUEST_CUP_PIKA] = {
        .title = sCupTitle,
        .category = QUEST_CAT_CUP,
        .stepCount = 3,
        .steps = sCupSteps,
    },
    [QUEST_CANOPY_CONTACT] = {
        .title = sCanTitle,
        .category = QUEST_CAT_CANOPY,
        .stepCount = 2,
        .steps = sCanSteps,
    },
    [QUEST_SIDE_DAYCARE] = {
        .title = sDayTitle,
        .category = QUEST_CAT_SIDE,
        .stepCount = 3,
        .steps = sDaySteps,
    },
};

#endif // GUARD_DATA_QUESTS_H
