#ifndef GUARD_DAYCARE_H
#define GUARD_DAYCARE_H

#include "constants/daycare.h"

struct RecordMixingDaycareMail
{
    struct DaycareMail mail[DAYCARE_MON_COUNT];
    u32 numDaycareMons;
    bool16 cantHoldItem[DAYCARE_MON_COUNT];
};

u8 *GetMonNickname2(struct Pokemon *mon, u8 *dest);
u8 *GetBoxMonNickname(struct BoxPokemon *mon, u8 *dest);
u8 CountPokemonInDaycare(struct DayCare *daycare);
void InitDaycareMailRecordMixing(struct DayCare *daycare, struct RecordMixingDaycareMail *mixMail);
void StoreSelectedPokemonInDaycare(void);
u16 TakePokemonFromDaycare(void);
void GetDaycareCostAndPrepareString(void);
u8 GetNumLevelsGainedFromDaycare(void);
void TriggerPendingDaycareEgg(void);
void RejectEggFromDayCare(void);
void CreateEgg(struct Pokemon *mon, u16 species, bool8 setHotSpringsLocation);
void GiveEggFromDaycare(void);
bool8 ShouldEggHatch(void);
u16 GetSelectedMonNicknameAndSpecies(void);
void GetDaycareMonNicknames(void);
u8 GetDaycareState(void);
u8 GetDaycareFriendship(void);
void SetDaycareFriendship(u8 value);
u8 GetDaycareBattlesToday(void);
bool8 Daycare_CanBattleToday(void);
void Daycare_IncrementBattlesToday(void);
bool8 Daycare_HasMon(void);
bool8 Daycare_HasActiveCondom(void);
bool8 Daycare_CondomBrokeThisDeposit(void);
void Daycare_BreakCondom(void);
u8 Daycare_BreederCheck(void);
void StartDaycareBattle(void);
void CB2_EndDaycareBattle(void);
extern bool8 gDaycarePlayBattle;
extern s16 gDaycareBattleSessionGain;
void SetDaycareCompatibilityString(void);
bool8 NameHasGenderSymbol(const u8 *name, u8 genderRatio);
void ShowDaycareLevelMenu(void);
void ChooseSendDaycareMon(void);

#endif // GUARD_DAYCARE_H
