#include "global.h"
#include "quest_menu.h"
#include "constants/quests.h"
#include "constants/vars.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "event_data.h"
#include "data/quests.h"

// Progress packed in VAR_QUEST_PACK0..2 (two quests per var: lo/hi byte)

static u16 *GetQuestPackVarPtr(u8 questId)
{
    static u16 dummy;
    u16 varId;
    switch (questId / 2)
    {
    case 0: varId = VAR_QUEST_PACK0; break;
    case 1: varId = VAR_QUEST_PACK1; break;
    default: varId = VAR_QUEST_PACK2; break;
    }
    // Vars are accessed via GetVarPointer in pokeemerald
    return GetVarPointer(varId);
}

u8 Quest_GetStep(u8 questId)
{
    u16 *pack;
    if (questId >= QUEST_COUNT)
        return QUEST_STEP_HIDDEN;
    pack = GetQuestPackVarPtr(questId);
    if (questId & 1)
        return (*pack >> 8) & 0xFF;
    return (*pack) & 0xFF;
}

void Quest_SetStep(u8 questId, u8 step)
{
    u16 *pack;
    if (questId >= QUEST_COUNT)
        return;
    pack = GetQuestPackVarPtr(questId);
    if (questId & 1)
        *pack = (*pack & 0x00FF) | (step << 8);
    else
        *pack = (*pack & 0xFF00) | step;
}

void Quest_Activate(u8 questId)
{
    if (questId >= QUEST_COUNT)
        return;
    if (Quest_GetStep(questId) == QUEST_STEP_HIDDEN)
        Quest_SetStep(questId, 1);
}

void Quest_Advance(u8 questId)
{
    u8 step;
    if (questId >= QUEST_COUNT)
        return;
    step = Quest_GetStep(questId);
    if (step == QUEST_STEP_HIDDEN || step == QUEST_STEP_DONE)
        return;
    if (step >= gQuests[questId].stepCount)
        Quest_SetStep(questId, QUEST_STEP_DONE);
    else
        Quest_SetStep(questId, step + 1);
}

void Quest_Complete(u8 questId)
{
    if (questId >= QUEST_COUNT)
        return;
    Quest_SetStep(questId, QUEST_STEP_DONE);
}

bool8 Quest_IsDone(u8 questId)
{
    return Quest_GetStep(questId) == QUEST_STEP_DONE;
}

bool8 Quest_IsActive(u8 questId)
{
    u8 step = Quest_GetStep(questId);
    return step != QUEST_STEP_HIDDEN && step != QUEST_STEP_DONE;
}

bool8 Quest_IsVisible(u8 questId)
{
    return Quest_GetStep(questId) != QUEST_STEP_HIDDEN;
}

void Special_QuestActivate(void)
{
    Quest_Activate(gSpecialVar_0x8004);
}

void Special_QuestAdvance(void)
{
    Quest_Advance(gSpecialVar_0x8004);
}

void Special_QuestComplete(void)
{
    Quest_Complete(gSpecialVar_0x8004);
}

void Special_QuestGetStep(void)
{
    gSpecialVar_Result = Quest_GetStep(gSpecialVar_0x8004);
}

// ---------------------------------------------------------------------------
// Menu UI
// ---------------------------------------------------------------------------

enum {
    WIN_CATEGORIES,
    WIN_QUESTS,
    WIN_DETAIL,
    WIN_COUNT
};

enum {
    STATE_CATEGORIES,
    STATE_QUEST_LIST,
    STATE_DETAIL,
};

static const struct WindowTemplate sWinTemplates[WIN_COUNT] = {
    [WIN_CATEGORIES] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 1,
        .width = 12, .height = 10, .paletteNum = 15, .baseBlock = 1
    },
    [WIN_QUESTS] = {
        .bg = 0, .tilemapLeft = 14, .tilemapTop = 1,
        .width = 15, .height = 10, .paletteNum = 15, .baseBlock = 121
    },
    [WIN_DETAIL] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 12,
        .width = 28, .height = 6, .paletteNum = 15, .baseBlock = 271
    },
};

static const struct BgTemplate sBgTemplates[] = {
    { .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0,
      .paletteMode = 0, .priority = 0, .baseTile = 0 },
};

struct QuestMenu
{
    u8 state;
    u8 categoryCursor;
    u8 questCursor;
    u8 numVisibleQuests;
    u8 visibleQuestIds[QUEST_COUNT];
    u8 windowIds[WIN_COUNT];
};

static EWRAM_DATA struct QuestMenu *sQuestMenu = NULL;

static void QuestMenu_InitWindows(void);
static void QuestMenu_Free(void);
static void Task_QuestMenuMain(u8 taskId);
static void QuestMenu_DrawCategoriesFixed(void);
static void QuestMenu_BuildVisibleList(void);
static void QuestMenu_DrawQuestList(void);
static void QuestMenu_DrawDetail(void);
static void QuestMenu_ClearDetail(void);
static void Task_QuestMenuExit(u8 taskId);

static void CB2_QuestMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_QuestMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_OpenQuestMenu(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetTasks();
        ResetPaletteFade();
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 1:
        sQuestMenu = AllocZeroed(sizeof(*sQuestMenu));
        if (sQuestMenu == NULL)
        {
            SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
            return;
        }
        sQuestMenu->state = STATE_CATEGORIES;
        sQuestMenu->categoryCursor = 0;
        sQuestMenu->questCursor = 0;
        gMain.state++;
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        gMain.state++;
        break;
    case 3:
        QuestMenu_InitWindows();
        QuestMenu_DrawCategoriesFixed();
        QuestMenu_ClearDetail();
        CreateTask(Task_QuestMenuMain, 0);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_QuestMenu);
        SetMainCallback2(CB2_QuestMenu);
        break;
    }
}

static void QuestMenu_InitWindows(void)
{
    u8 i;
    InitWindows(sWinTemplates);
    DeactivateAllTextPrinters();
    LoadMessageBoxAndBorderGfx();
    for (i = 0; i < WIN_COUNT; i++)
    {
        sQuestMenu->windowIds[i] = i;
        PutWindowTilemap(i);
        DrawStdWindowFrame(i, FALSE);
        FillWindowPixelBuffer(i, PIXEL_FILL(1));
        CopyWindowToVram(i, COPYWIN_FULL);
    }
}

static void QuestMenu_Free(void)
{
    u8 i;
    if (sQuestMenu == NULL)
        return;
    for (i = 0; i < WIN_COUNT; i++)
        RemoveWindow(i);
    FreeAllWindowBuffers();
    FREE_AND_SET_NULL(sQuestMenu);
}

static void QuestMenu_DrawCategoriesFixed(void)
{
    u8 i;
    FillWindowPixelBuffer(WIN_CATEGORIES, PIXEL_FILL(1));
    for (i = 0; i < QUEST_CAT_COUNT; i++)
    {
        if (i == sQuestMenu->categoryCursor)
            AddTextPrinterParameterized(WIN_CATEGORIES, FONT_NORMAL, gText_SelectorArrow, 0, 2 + i * 14, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(WIN_CATEGORIES, FONT_NORMAL,
            gQuestCategoryNames[i], 12, 2 + i * 14, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(WIN_CATEGORIES, COPYWIN_FULL);
}

static void QuestMenu_BuildVisibleList(void)
{
    u8 i, n = 0;
    u8 cat = sQuestMenu->categoryCursor;
    for (i = 0; i < QUEST_COUNT; i++)
    {
        if (gQuests[i].category == cat && Quest_IsVisible(i))
            sQuestMenu->visibleQuestIds[n++] = i;
    }
    sQuestMenu->numVisibleQuests = n;
    if (sQuestMenu->questCursor >= n && n > 0)
        sQuestMenu->questCursor = n - 1;
}

static const u8 sTextDone[] = _(" (DONE)");
static const u8 sTextEmpty[] = _("No quests.");
static const u8 sTextStepFmt[] = _("Step {STR_VAR_1}/{STR_VAR_2}");

static void QuestMenu_DrawQuestList(void)
{
    u8 i;
    FillWindowPixelBuffer(WIN_QUESTS, PIXEL_FILL(1));
    QuestMenu_BuildVisibleList();
    if (sQuestMenu->numVisibleQuests == 0)
    {
        AddTextPrinterParameterized(WIN_QUESTS, FONT_NORMAL, sTextEmpty, 4, 2, TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        for (i = 0; i < sQuestMenu->numVisibleQuests; i++)
        {
            u8 qid = sQuestMenu->visibleQuestIds[i];
            if (i == sQuestMenu->questCursor)
                AddTextPrinterParameterized(WIN_QUESTS, FONT_NORMAL, gText_SelectorArrow, 0, 2 + i * 14, TEXT_SKIP_DRAW, NULL);
            AddTextPrinterParameterized(WIN_QUESTS, FONT_NORMAL,
                gQuests[qid].title, 12, 2 + i * 14, TEXT_SKIP_DRAW, NULL);
        }
    }
    CopyWindowToVram(WIN_QUESTS, COPYWIN_FULL);
}

static void QuestMenu_ClearDetail(void)
{
    FillWindowPixelBuffer(WIN_DETAIL, PIXEL_FILL(1));
    CopyWindowToVram(WIN_DETAIL, COPYWIN_FULL);
}

static void QuestMenu_DrawDetail(void)
{
    u8 qid, step;
    FillWindowPixelBuffer(WIN_DETAIL, PIXEL_FILL(1));
    if (sQuestMenu->numVisibleQuests == 0)
    {
        CopyWindowToVram(WIN_DETAIL, COPYWIN_FULL);
        return;
    }
    qid = sQuestMenu->visibleQuestIds[sQuestMenu->questCursor];
    step = Quest_GetStep(qid);

    AddTextPrinterParameterized(WIN_DETAIL, FONT_NORMAL, gQuests[qid].title, 4, 2, TEXT_SKIP_DRAW, NULL);

    if (step == QUEST_STEP_DONE)
    {
        AddTextPrinterParameterized(WIN_DETAIL, FONT_SMALL, sTextDone, 4, 18, TEXT_SKIP_DRAW, NULL);
    }
    else if (step >= 1 && step <= gQuests[qid].stepCount)
    {
        ConvertIntToDecimalStringN(gStringVar1, step, STR_CONV_MODE_LEFT_ALIGN, 2);
        ConvertIntToDecimalStringN(gStringVar2, gQuests[qid].stepCount, STR_CONV_MODE_LEFT_ALIGN, 2);
        // Show step counter and step text
        AddTextPrinterParameterized(WIN_DETAIL, FONT_SMALL, gQuests[qid].steps[step - 1].text, 4, 18, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(WIN_DETAIL, COPYWIN_FULL);
}

static void Task_QuestMenuExit(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        QuestMenu_Free();
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

static void Task_QuestMenuMain(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    switch (sQuestMenu->state)
    {
    case STATE_CATEGORIES:
        QuestMenu_DrawCategoriesFixed();
        if (JOY_NEW(DPAD_UP))
        {
            PlaySE(SE_SELECT);
            if (sQuestMenu->categoryCursor == 0)
                sQuestMenu->categoryCursor = QUEST_CAT_COUNT - 1;
            else
                sQuestMenu->categoryCursor--;
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            sQuestMenu->categoryCursor++;
            if (sQuestMenu->categoryCursor >= QUEST_CAT_COUNT)
                sQuestMenu->categoryCursor = 0;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            sQuestMenu->questCursor = 0;
            sQuestMenu->state = STATE_QUEST_LIST;
            QuestMenu_DrawQuestList();
            QuestMenu_DrawDetail();
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_QuestMenuExit;
        }
        break;

    case STATE_QUEST_LIST:
        if (JOY_NEW(DPAD_UP) && sQuestMenu->numVisibleQuests > 0)
        {
            PlaySE(SE_SELECT);
            if (sQuestMenu->questCursor == 0)
                sQuestMenu->questCursor = sQuestMenu->numVisibleQuests - 1;
            else
                sQuestMenu->questCursor--;
            QuestMenu_DrawQuestList();
            QuestMenu_DrawDetail();
        }
        else if (JOY_NEW(DPAD_DOWN) && sQuestMenu->numVisibleQuests > 0)
        {
            PlaySE(SE_SELECT);
            sQuestMenu->questCursor++;
            if (sQuestMenu->questCursor >= sQuestMenu->numVisibleQuests)
                sQuestMenu->questCursor = 0;
            QuestMenu_DrawQuestList();
            QuestMenu_DrawDetail();
        }
        else if (JOY_NEW(A_BUTTON) && sQuestMenu->numVisibleQuests > 0)
        {
            PlaySE(SE_SELECT);
            sQuestMenu->state = STATE_DETAIL;
            QuestMenu_DrawDetail();
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sQuestMenu->state = STATE_CATEGORIES;
            FillWindowPixelBuffer(WIN_QUESTS, PIXEL_FILL(1));
            CopyWindowToVram(WIN_QUESTS, COPYWIN_FULL);
            QuestMenu_ClearDetail();
            QuestMenu_DrawCategoriesFixed();
        }
        break;

    case STATE_DETAIL:
        if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sQuestMenu->state = STATE_QUEST_LIST;
        }
        break;
    }
}
