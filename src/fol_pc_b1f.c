// HACKROM (Forest of Love): subsolo (B1F) dos Centros Pokémon.
//
// Um único mapa MAP_POKEMON_CENTER_B1F é compartilhado por todos os Centros.
// A escada do 1F leva ao warp 0 do B1F, cujo destino é MAP_DYNAMIC: a pret
// (SetupWarp em field_control_avatar.c) grava sozinha em
// gSaveBlock1Ptr->dynamicWarp o mapa/warp de onde o jogador veio, e a escada do
// B1F devolve o jogador para esse 1F.
//
// O simulador de batalhas usa as rotinas da Battle Tower (tower_init etc.).
// InitTowerChallenge chama SetDynamicWarp(..., WARP_ID_NONE) para guardar
// "onde continuar depois da Tower"; aqui isso apagaria a volta do B1F, então o
// script salva e restaura o dynamicWarp em volta do tower_init.
#include "global.h"
#include "event_data.h"

static EWRAM_DATA struct WarpData sSavedDynamicWarp = {0};

void FolSimulator_SaveDynamicWarp(void)
{
    sSavedDynamicWarp = gSaveBlock1Ptr->dynamicWarp;
}

void FolSimulator_RestoreDynamicWarp(void)
{
    gSaveBlock1Ptr->dynamicWarp = sSavedDynamicWarp;
}
