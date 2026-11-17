#ifndef GUARD_FOL_HOUSES_H
#define GUARD_FOL_HOUSES_H

#include "constants/fol_houses.h"

// HACKROM (Forest of Love): casas de viagem rápida (ver docs/casas_fast_travel.md).

// Índices de gSaveBlock1Ptr->folHouseMisc[]
#define FOL_HOUSE_MISC_RADIO_TRACK   0  // faixa do rádio (FOL_RADIO_*)
#define FOL_HOUSE_MISC_CURRENT_HOUSE 1  // casa onde o jogador está / entrou por último

// Dados das casas no espaço dos antigos secretBases[1..19].
#define gFolHouseSave ((struct FolHouseSave *)&gSaveBlock1Ptr->secretBases[1])

void FolHouses_NewGameInit(void);
bool8 FolMaps_GetRealMapSec(mapsec_u16_t *mapSec);
bool8 FolHouses_IsHouseMap(void);
bool8 FolHouses_IsOwned(u8 houseId);
void FolHouses_DrawFixedDecorations(void);
void FolHouses_TryRedirectBedroomWarp(struct WarpData *dest);

// Usados pelo código vanilla do quarto (que não existe mais depois da intro).
extern u8 gFolUnusedPlayerRoomDecor[DECOR_MAX_PLAYERS_HOUSE];
extern u8 gFolUnusedPlayerRoomDecorPos[DECOR_MAX_PLAYERS_HOUSE];

#endif // GUARD_FOL_HOUSES_H
