#ifndef GUARD_QUEST_MENU_H
#define GUARD_QUEST_MENU_H

#include "global.h"
#include "constants/quests.h"

struct QuestStepInfo {
    const u8 *text;
};

struct QuestInfo {
    const u8 *title;
    u8 category;
    u8 stepCount;
    const struct QuestStepInfo *steps; // length == stepCount, index 0 = step 1
};

extern const struct QuestInfo gQuests[QUEST_COUNT];
extern const u8 *const gQuestCategoryNames[QUEST_CAT_COUNT];

void CB2_OpenQuestMenu(void);

u8 Quest_GetStep(u8 questId);
void Quest_SetStep(u8 questId, u8 step);
void Quest_Activate(u8 questId);      // step = 1
void Quest_Advance(u8 questId);       // step++, or DONE if past last
void Quest_Complete(u8 questId);      // step = DONE
bool8 Quest_IsActive(u8 questId);
bool8 Quest_IsDone(u8 questId);
bool8 Quest_IsVisible(u8 questId);    // active or done

// Script specials: quest id in VAR_0x8004
void Special_QuestActivate(void);
void Special_QuestAdvance(void);
void Special_QuestComplete(void);
void Special_QuestGetStep(void);      // writes VAR_RESULT

#endif // GUARD_QUEST_MENU_H
