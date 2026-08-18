#ifndef GUARD_CONSTANTS_QUESTS_H
#define GUARD_CONSTANTS_QUESTS_H

// Categories shown in the Quests menu (grouped)
enum {
    QUEST_CAT_MAIN = 0,
    QUEST_CAT_GYM,
    QUEST_CAT_CUP,
    QUEST_CAT_CANOPY,
    QUEST_CAT_SIDE,
    QUEST_CAT_COUNT
};

// Quest IDs (first 5 for map progression scaffolding)
enum {
    QUEST_MAIN_AWAKENING = 0,   // Main story intro
    QUEST_GYM_FIRST_BADGE,      // Mistbrook Bug Gym
    QUEST_CUP_PIKA,             // Pika Cup
    QUEST_CANOPY_CONTACT,       // Reach Canopy / Corps
    QUEST_SIDE_DAYCARE,         // Day-Care opening
    QUEST_COUNT
};

// Per-quest progress byte stored in packed vars:
//   0     = hidden (not in log)
//   1..N  = active, current step (1-based index into step text)
//   0xFF  = completed
#define QUEST_STEP_HIDDEN  0
#define QUEST_STEP_DONE    0xFF
#define QUEST_MAX_STEPS    8

#endif // GUARD_CONSTANTS_QUESTS_H
