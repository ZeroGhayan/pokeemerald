// HACKROM (Forest of Love): casas de viagem rápida, montadas sobre o código de
// Secret Base da pret. Visão geral (detalhes em docs/casas_fast_travel.md):
//
// - Todas as casas usam UM mapa/layout compartilhado: MAP_FOL_HOUSE. Por isso
//   todas mostram as mesmas decorações. Para o jogo, ele é uma "Secret Base"
//   (MAP_TYPE_SECRET_BASE, VAR_CURRENT_SECRET_BASE = 0): o menu vanilla de
//   decoração funciona sem mudanças sobre gSaveBlock1Ptr->secretBases[0].
// - Decorações FIXAS (cama, TV, console, mesa, banheira, fogão, geladeira,
//   tapete e o item ao lado do PC) são escolhidas no PC, estilo GSC. Ficam em
//   gSaveBlock1Ptr->folFixedDecor[] (os antigos playerRoomDecorations) e são
//   desenhadas em slots fixos do mapa com ShowDecorationOnMap.
// - A casa "atual" fica em folHouseMisc[FOL_HOUSE_MISC_CURRENT_HOUSE]. A saída
//   do mapa é um warp MAP_DYNAMIC: o dynamicWarp aponta para fora da casa atual.
//   Viajar = trocar a casa atual + o dynamicWarp e re-entrar no mesmo mapa.
// - O quarto de Littleroot é a casa FOL_HOUSE_BEDROOM: depois que o relógio é
//   acertado na intro, todo warp para o quarto do jogador vira um warp para
//   MAP_FOL_HOUSE (FolHouses_TryRedirectBedroomWarp, chamado em overworld.c).
// - A posse das casas fica em gFolHouseSave->ownedHouses, no espaço dos antigos
//   secretBases[1..19] (bases recebidas por record mixing, agora desativadas).

#include "global.h"
#include "fol_houses.h"
#include "decoration.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "money.h"
#include "overworld.h"
#include "script.h"
#include "script_menu.h"
#include "secret_base.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "window.h"
#include "constants/decorations.h"
#include "constants/flags.h"
#include "constants/maps.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
#include "constants/vars.h"

void InitSecretBaseVars(void); // secret_base.c (sem protótipo em header)

// O espaço reaproveitado tem que caber nos 19 SecretBase restantes.
STATIC_ASSERT(sizeof(struct FolHouseSave) <= sizeof(struct SecretBase) * (SECRET_BASES_COUNT - 1), FolHouseSaveFitsInSecretBases);
// ClearSecretBase() enche trainerName (offsets 2..8) de EOS: ownedHouses fica depois.
STATIC_ASSERT(offsetof(struct FolHouseSave, ownedHouses) >= offsetof(struct SecretBase, trainerName) + PLAYER_NAME_LENGTH, FolHouseOwnedAfterVanillaName);
STATIC_ASSERT(FOL_HOUSE_COUNT <= FOL_HOUSE_MAX, FolHouseCountFitsInBits);
STATIC_ASSERT(FOL_FIXED_COUNT <= DECOR_MAX_PLAYERS_HOUSE, FolFixedDecorFitsInOldPlayerRoomArray);

// Buffers sem uso real: o código vanilla do quarto (que só é visitado durante a
// intro, antes do relógio) aponta para cá em vez de playerRoomDecorations.
EWRAM_DATA u8 gFolUnusedPlayerRoomDecor[DECOR_MAX_PLAYERS_HOUSE] = {0};
EWRAM_DATA u8 gFolUnusedPlayerRoomDecorPos[DECOR_MAX_PLAYERS_HOUSE] = {0};

// -----------------------------------------------------------------------------
// Tabela de casas
// -----------------------------------------------------------------------------
struct FolHouseInfo
{
    const u8 *name;
    u16 outsideMap;   // MAP_* do lado de fora (destino da porta / viagem)
    s8 outsideX;      // tile onde o jogador aparece ao sair
    s8 outsideY;
    u32 price;        // 0 = grátis
};

static const u8 sText_HouseBedroom[]   = _("LITTLEROOT");
static const u8 sText_HouseOldale[]    = _("OLDALE TOWN");
static const u8 sText_HousePetalburg[] = _("PETALBURG CITY");
static const u8 sText_HouseRustboro[]  = _("RUSTBORO CITY");

static const struct FolHouseInfo sFolHouses[FOL_HOUSE_COUNT] =
{
    // O quarto é tratado à parte (o 1F depende do gênero do jogador).
    [FOL_HOUSE_BEDROOM]   = { sText_HouseBedroom,   MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F, -1, -1, 0 },
    [FOL_HOUSE_OLDALE]    = { sText_HouseOldale,    MAP_OLDALE_TOWN,     3,  9, 10000 },
    [FOL_HOUSE_PETALBURG] = { sText_HousePetalburg, MAP_PETALBURG_CITY, 18, 26, 20000 },
    [FOL_HOUSE_RUSTBORO]  = { sText_HouseRustboro,  MAP_RUSTBORO_CITY,  31, 21, 30000 },
};

// Warp do 1F de casa que leva à escada do quarto (warp 2 nos dois 1F).
#define BEDROOM_1F_STAIRS_WARP 2

// -----------------------------------------------------------------------------
// Decorações fixas
// -----------------------------------------------------------------------------
struct FolFixedDecorOption
{
    const u8 *name;
    u8 decorId;   // gráfico: decoração vanilla usada como placeholder
};

struct FolFixedDecorSlot
{
    const u8 *name;
    u8 x, y;      // canto inferior esquerdo no mapa FolHouse
    const struct FolFixedDecorOption *options;
    u8 count;
};

static const u8 sText_CatBed[]     = _("BED");
static const u8 sText_CatTV[]      = _("TV");
static const u8 sText_CatConsole[] = _("GAME CONSOLE");
static const u8 sText_CatTable[]   = _("CENTER TABLE");
static const u8 sText_CatBathtub[] = _("BATHTUB");
static const u8 sText_CatStove[]   = _("STOVE");
static const u8 sText_CatFridge[]  = _("FRIDGE");
static const u8 sText_CatCarpet[]  = _("CARPET");
static const u8 sText_CatPcSide[]  = _("ITEM BY THE PC");

static const u8 sText_FolOpt1[] = _("WOODEN BED");
static const u8 sText_FolOpt2[] = _("RUSTIC BED");
static const u8 sText_FolOpt3[] = _("COMFY BED");
static const u8 sText_FolOpt4[] = _("TV");
static const u8 sText_FolOpt5[] = _("ROUND TV");
static const u8 sText_FolOpt6[] = _("CUTE TV");
static const u8 sText_FolOpt7[] = _("CLASSIC CONSOLE");
static const u8 sText_FolOpt8[] = _("POKé CONSOLE");
static const u8 sText_FolOpt9[] = _("PRETTY TABLE");
static const u8 sText_FolOpt10[] = _("BRICK TABLE");
static const u8 sText_FolOpt11[] = _("CAMP TABLE");
static const u8 sText_FolOpt12[] = _("HARD TABLE");
static const u8 sText_FolOpt13[] = _("ROUND TUB");
static const u8 sText_FolOpt14[] = _("GARDEN TUB");
static const u8 sText_FolOpt15[] = _("RED STOVE");
static const u8 sText_FolOpt16[] = _("YELLOW STOVE");
static const u8 sText_FolOpt17[] = _("BLUE FRIDGE");
static const u8 sText_FolOpt18[] = _("RED FRIDGE");
static const u8 sText_FolOpt19[] = _("SURF CARPET");
static const u8 sText_FolOpt20[] = _("THUNDER CARPET");
static const u8 sText_FolOpt21[] = _("FIRE CARPET");
static const u8 sText_FolOpt22[] = _("SNOW CARPET");
static const u8 sText_FolOpt23[] = _("HEART CARPET");
static const u8 sText_FolOpt24[] = _("RADIO");
static const u8 sText_FolOpt25[] = _("SYLVARIA TROPHY");
static const u8 sText_FolOpt26[] = _("PHONE");
static const u8 sText_FolOpt27[] = _("TURN OFF");
static const u8 sText_FolOpt28[] = _("LITTLEROOT");
static const u8 sText_FolOpt29[] = _("ROUTE 101");
static const u8 sText_FolOpt30[] = _("LILYCOVE");
static const u8 sText_FolOpt31[] = _("SLATEPORT");
static const u8 sText_FolOpt32[] = _("SURFING");
static const u8 sText_FolOpt33[] = _("CYCLING");
static const u8 sText_FolOpt34[] = _("PALLET TOWN");

// Placeholders: todos usam gráficos de decorações vanilla do tileset de Secret
// Base. Todas as opções de uma categoria têm o MESMO formato (shape), para que
// trocar a opção nunca invada outro slot ou decorações móveis.
static const struct FolFixedDecorOption sBedOptions[] = {
    { sText_FolOpt1, DECOR_HEAVY_DESK },   // 3x2
    { sText_FolOpt2, DECOR_RAGGED_DESK },
    { sText_FolOpt3,  DECOR_COMFORT_DESK },
};
static const struct FolFixedDecorOption sTVOptions[] = {
    { sText_FolOpt4,       DECOR_TV },             // 1x1
    { sText_FolOpt5, DECOR_ROUND_TV },
    { sText_FolOpt6,  DECOR_CUTE_TV },
};
static const struct FolFixedDecorOption sConsoleOptions[] = {
    { sText_FolOpt7, DECOR_SMALL_DESK },   // 1x1
    { sText_FolOpt8,    DECOR_POKEMON_DESK },
};
static const struct FolFixedDecorOption sTableOptions[] = {
    { sText_FolOpt9, DECOR_PRETTY_DESK },     // 3x3
    { sText_FolOpt10,  DECOR_BRICK_DESK },
    { sText_FolOpt11,   DECOR_CAMP_DESK },
    { sText_FolOpt12,   DECOR_HARD_DESK },
};
static const struct FolFixedDecorOption sBathtubOptions[] = {
    { sText_FolOpt13,  DECOR_TIRE },              // 2x2
    { sText_FolOpt14, DECOR_BIG_PLANT },
};
static const struct FolFixedDecorOption sStoveOptions[] = {
    { sText_FolOpt15,    DECOR_RED_BRICK },       // 1x2
    { sText_FolOpt16, DECOR_YELLOW_BRICK },
};
static const struct FolFixedDecorOption sFridgeOptions[] = {
    { sText_FolOpt17, DECOR_BLUE_BRICK },       // 1x2
    { sText_FolOpt18,  DECOR_RED_BRICK },
};
static const struct FolFixedDecorOption sCarpetOptions[] = {
    { sText_FolOpt19,    DECOR_SURF_MAT },      // 3x3
    { sText_FolOpt20, DECOR_THUNDER_MAT },
    { sText_FolOpt21,    DECOR_FIRE_BLAST_MAT },
    { sText_FolOpt22,    DECOR_POWDER_SNOW_MAT },
    { sText_FolOpt23,   DECOR_ATTRACT_MAT },
};
static const struct FolFixedDecorOption sPcSideOptions[] = {
    [FOL_PC_SIDE_RADIO]  = { sText_FolOpt24,           DECOR_SAND_ORNAMENT },  // 1x2
    [FOL_PC_SIDE_TROPHY] = { sText_FolOpt25, DECOR_GOLD_SHIELD },
    [FOL_PC_SIDE_PHONE]  = { sText_FolOpt26,           DECOR_GLASS_ORNAMENT },
};

static const struct FolFixedDecorSlot sFixedDecorSlots[FOL_FIXED_COUNT] =
{
    [FOL_FIXED_BED]     = { sText_CatBed,     11, 2, sBedOptions,     ARRAY_COUNT(sBedOptions) },
    [FOL_FIXED_TV]      = { sText_CatTV,       7, 1, sTVOptions,      ARRAY_COUNT(sTVOptions) },
    [FOL_FIXED_CONSOLE] = { sText_CatConsole,  8, 1, sConsoleOptions, ARRAY_COUNT(sConsoleOptions) },
    [FOL_FIXED_TABLE]   = { sText_CatTable,    6, 6, sTableOptions,   ARRAY_COUNT(sTableOptions) },
    [FOL_FIXED_BATHTUB] = { sText_CatBathtub, 12, 9, sBathtubOptions, ARRAY_COUNT(sBathtubOptions) },
    [FOL_FIXED_STOVE]   = { sText_CatStove,    1, 5, sStoveOptions,   ARRAY_COUNT(sStoveOptions) },
    [FOL_FIXED_FRIDGE]  = { sText_CatFridge,   1, 3, sFridgeOptions,  ARRAY_COUNT(sFridgeOptions) },
    [FOL_FIXED_CARPET]  = { sText_CatCarpet,   1, 9, sCarpetOptions,  ARRAY_COUNT(sCarpetOptions) },
    [FOL_FIXED_PC_SIDE] = { sText_CatPcSide,   4, 2, sPcSideOptions,  ARRAY_COUNT(sPcSideOptions) },
};

// -----------------------------------------------------------------------------
// Rádio
// -----------------------------------------------------------------------------
struct FolRadioTrack
{
    const u8 *name;
    u16 song;
};

static const struct FolRadioTrack sRadioTracks[] =
{
    [FOL_RADIO_OFF] = { sText_FolOpt27,       MUS_DUMMY },
    { sText_FolOpt28,     MUS_LITTLEROOT },
    { sText_FolOpt29,      MUS_ROUTE101 },
    { sText_FolOpt30,       MUS_LILYCOVE },
    { sText_FolOpt31,      MUS_SLATEPORT },
    { sText_FolOpt32,        MUS_SURF },
    { sText_FolOpt33,        MUS_CYCLING },
    { sText_FolOpt34,    MUS_RG_PALLET },
};

// -----------------------------------------------------------------------------
// Utilitários
// -----------------------------------------------------------------------------
// new_game.c, logo depois de ClearSecretBases: zera posse, decorações fixas
// (opção 0 de cada categoria) e rádio. O quarto não precisa de bit (sempre seu).
void FolHouses_NewGameInit(void)
{
    memset(gFolHouseSave, 0, sizeof(struct FolHouseSave));
    gFolHouseSave->version = 1;
    memset(gSaveBlock1Ptr->folFixedDecor, 0, sizeof(gSaveBlock1Ptr->folFixedDecor));
    memset(gSaveBlock1Ptr->folHouseMisc, 0, sizeof(gSaveBlock1Ptr->folHouseMisc));
}

// Mapas compartilhados (casa e B1F dos Centros) usam MAPSEC_DYNAMIC, que a pret
// chama de "FERRY". Para o menu de save etc., a cidade "real" é a do mapa para
// onde o warp dinâmico (a porta/escada de saída) leva.
bool8 FolMaps_GetRealMapSec(mapsec_u16_t *mapSec)
{
    const struct MapHeader *header;
    s8 group = gSaveBlock1Ptr->location.mapGroup;
    s8 num = gSaveBlock1Ptr->location.mapNum;

    if (!((group == MAP_GROUP(MAP_FOL_HOUSE) && num == MAP_NUM(MAP_FOL_HOUSE))
       || (group == MAP_GROUP(MAP_POKEMON_CENTER_B1F) && num == MAP_NUM(MAP_POKEMON_CENTER_B1F))))
        return FALSE;
    if (gSaveBlock1Ptr->dynamicWarp.mapGroup < 0 || gSaveBlock1Ptr->dynamicWarp.mapNum < 0)
        return FALSE;
    header = Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->dynamicWarp.mapGroup, gSaveBlock1Ptr->dynamicWarp.mapNum);
    if (header->regionMapSectionId >= MAPSEC_NONE || header->regionMapSectionId == MAPSEC_DYNAMIC)
        return FALSE;
    *mapSec = header->regionMapSectionId;
    return TRUE;
}

bool8 FolHouses_IsHouseMap(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_FOL_HOUSE)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_FOL_HOUSE);
}

bool8 FolHouses_IsOwned(u8 houseId)
{
    if (houseId == FOL_HOUSE_BEDROOM)
        return TRUE; // o quarto é sempre do jogador
    if (houseId >= FOL_HOUSE_COUNT)
        return FALSE;
    return (gFolHouseSave->ownedHouses >> houseId) & 1;
}

static u8 GetFixedOption(u8 category)
{
    u8 option = gSaveBlock1Ptr->folFixedDecor[category];
    if (option >= sFixedDecorSlots[category].count)
        option = 0;
    // O troféu só existe depois de zerar o jogo.
    if (category == FOL_FIXED_PC_SIDE && option == FOL_PC_SIDE_TROPHY && !FlagGet(FLAG_SYS_GAME_CLEAR))
        option = FOL_PC_SIDE_RADIO;
    return option;
}

static u8 GetCurrentHouse(void)
{
    u8 houseId = gSaveBlock1Ptr->folHouseMisc[FOL_HOUSE_MISC_CURRENT_HOUSE];
    if (houseId >= FOL_HOUSE_COUNT)
        houseId = FOL_HOUSE_BEDROOM;
    return houseId;
}

static u16 GetBedroomMap(void)
{
    return gSaveBlock2Ptr->playerGender == MALE ? MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F : MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F;
}

// Faz a porta (warp MAP_DYNAMIC) do mapa compartilhado levar para fora da casa houseId.
static void SetHouseExitWarp(u8 houseId)
{
    if (houseId == FOL_HOUSE_BEDROOM)
    {
        u16 map = gSaveBlock2Ptr->playerGender == MALE ? MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F : MAP_LITTLEROOT_TOWN_MAYS_HOUSE_1F;
        SetDynamicWarp(0, MAP_GROUP(map), MAP_NUM(map), BEDROOM_1F_STAIRS_WARP);
    }
    else
    {
        const struct FolHouseInfo *house = &sFolHouses[houseId];
        SetDynamicWarpWithCoords(0, MAP_GROUP(house->outsideMap), MAP_NUM(house->outsideMap), WARP_ID_NONE, house->outsideX, house->outsideY);
    }
    gSaveBlock1Ptr->folHouseMisc[FOL_HOUSE_MISC_CURRENT_HOUSE] = houseId;
}

// -----------------------------------------------------------------------------
// Chamado pelo código de mapa/warp
// -----------------------------------------------------------------------------
void FolHouses_DrawFixedDecorations(void)
{
    u8 i;
    for (i = 0; i < FOL_FIXED_COUNT; i++)
    {
        const struct FolFixedDecorSlot *slot = &sFixedDecorSlots[i];
        ShowDecorationOnMap(slot->x + MAP_OFFSET, slot->y + MAP_OFFSET, slot->options[GetFixedOption(i)].decorId);
    }
}

// overworld.c (ApplyCurrentWarp): depois da intro (relógio acertado), qualquer
// warp para o quarto do jogador vai para a casa compartilhada. Isso cobre a
// escada do 1F e o "continuar" depois do Hall of Fame (heal location do quarto).
void FolHouses_TryRedirectBedroomWarp(struct WarpData *dest)
{
    u16 bedroom = GetBedroomMap();

    if (dest->mapGroup != MAP_GROUP(bedroom) || dest->mapNum != MAP_NUM(bedroom))
        return;
    if (!FlagGet(FLAG_SET_WALL_CLOCK))
        return; // intro: o quarto vanilla ainda é usado (acertar o relógio etc.)

    SetHouseExitWarp(FOL_HOUSE_BEDROOM);
    VarSet(VAR_CURRENT_SECRET_BASE, 0);
    dest->mapGroup = MAP_GROUP(MAP_FOL_HOUSE);
    dest->mapNum = MAP_NUM(MAP_FOL_HOUSE);
    if (dest->warpId >= 0)
    {
        // Pela escada: chega no tapete da porta.
        dest->warpId = FOL_HOUSE_DOOR_WARP;
    }
    else
    {
        // Heal location / continuar após o Hall of Fame: acorda ao lado da cama.
        dest->warpId = WARP_ID_NONE;
        dest->x = FOL_HOUSE_BED_FRONT_X;
        dest->y = FOL_HOUSE_BED_FRONT_Y;
    }
}

// -----------------------------------------------------------------------------
// Specials usados pelos scripts (data/maps/FolHouse/scripts.inc)
// -----------------------------------------------------------------------------

// ON_TRANSITION do mapa: base do jogador + música do rádio.
void FolHouse_OnTransition(void)
{
    u8 track = gSaveBlock1Ptr->folHouseMisc[FOL_HOUSE_MISC_RADIO_TRACK];

    VarSet(VAR_CURRENT_SECRET_BASE, 0);
    InitSecretBaseVars();
    if (track != FOL_RADIO_OFF && track < ARRAY_COUNT(sRadioTracks) && GetFixedOption(FOL_FIXED_PC_SIDE) == FOL_PC_SIDE_RADIO)
        Overworld_SetSavedMusic(sRadioTracks[track].song);
}

// VAR_0x8004 = casa. STR_VAR_3 = nome (STR_VAR_1 é usado pela moneybox), STR_VAR_2 = preço.
void FolHouse_BufferHouseInfo(void)
{
    u8 houseId = gSpecialVar_0x8004;
    if (houseId >= FOL_HOUSE_COUNT)
        houseId = FOL_HOUSE_BEDROOM;
    StringCopy(gStringVar3, sFolHouses[houseId].name);
    ConvertIntToDecimalStringN(gStringVar2, sFolHouses[houseId].price, STR_CONV_MODE_LEFT_ALIGN, 6);
}

// VAR_0x8004 = casa. VAR_RESULT = TRUE se já é do jogador.
void FolHouse_CheckOwned(void)
{
    gSpecialVar_Result = FolHouses_IsOwned(gSpecialVar_0x8004);
}

// VAR_0x8004 = casa. Desconta o dinheiro e marca a posse. VAR_RESULT = TRUE se comprou.
void FolHouse_TryBuy(void)
{
    u8 houseId = gSpecialVar_0x8004;
    u32 price;

    gSpecialVar_Result = FALSE;
    if (houseId >= FOL_HOUSE_COUNT)
        return;
    price = sFolHouses[houseId].price;
    if (!IsEnoughMoney(&gSaveBlock1Ptr->money, price))
        return;
    RemoveMoney(&gSaveBlock1Ptr->money, price);
    gFolHouseSave->ownedHouses |= 1u << houseId;
    gSpecialVar_Result = TRUE;
}

// VAR_0x8004 = casa. Entrando pela porta/agente: a saída volta para onde o
// jogador está agora. O script faz o warp para MAP_FOL_HOUSE em seguida.
void FolHouse_PrepareEnter(void)
{
    s16 x, y;
    u8 houseId = gSpecialVar_0x8004;

    if (houseId >= FOL_HOUSE_COUNT)
        houseId = FOL_HOUSE_BEDROOM;
    PlayerGetDestCoords(&x, &y);
    SetDynamicWarpWithCoords(0, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE, x - MAP_OFFSET, y - MAP_OFFSET);
    gSaveBlock1Ptr->folHouseMisc[FOL_HOUSE_MISC_CURRENT_HOUSE] = houseId;
    VarSet(VAR_CURRENT_SECRET_BASE, 0);
}

// VAR_0x8004 = casa de destino (já escolhida na lista). O script re-entra no
// mapa em frente ao PC: como o interior é o mesmo, só muda para onde a porta leva.
void FolHouse_PrepareFastTravel(void)
{
    u8 houseId = gSpecialVar_0x8004;
    if (houseId >= FOL_HOUSE_COUNT || !FolHouses_IsOwned(houseId))
        houseId = GetCurrentHouse();
    SetHouseExitWarp(houseId);
    VarSet(VAR_CURRENT_SECRET_BASE, 0);
}

// VAR_0x8004 = categoria, VAR_0x8005 = opção.
void FolHouse_SetFixedDecor(void)
{
    u8 category = gSpecialVar_0x8004;
    if (category >= FOL_FIXED_COUNT || gSpecialVar_0x8005 >= sFixedDecorSlots[category].count)
        return;
    gSaveBlock1Ptr->folFixedDecor[category] = gSpecialVar_0x8005;
}

// VAR_RESULT = opção atual do item ao lado do PC (FOL_PC_SIDE_*).
void FolHouse_GetPcSideOption(void)
{
    gSpecialVar_Result = GetFixedOption(FOL_FIXED_PC_SIDE);
}

// VAR_0x8004 = faixa escolhida no rádio. Toca agora e grava para as próximas visitas.
void FolHouse_SetRadioTrack(void)
{
    u8 track = gSpecialVar_0x8004;
    if (track >= ARRAY_COUNT(sRadioTracks))
        return;
    gSaveBlock1Ptr->folHouseMisc[FOL_HOUSE_MISC_RADIO_TRACK] = track;
    if (track == FOL_RADIO_OFF)
    {
        Overworld_ClearSavedMusic();
        PlayNewMapMusic(gMapHeader.music);
    }
    else
    {
        Overworld_SetSavedMusic(sRadioTracks[track].song);
        PlayNewMapMusic(sRadioTracks[track].song);
    }
}

// Gancho do telefone: o PokéNav vai sair do jogo e as chamadas (rematches,
// Match Call) vão passar por aqui no futuro. Por enquanto não faz nada.
void FolHouse_PhoneHook(void)
{
    gSpecialVar_Result = FALSE; // FALSE = ninguém para ligar
}

// Cama: depois de salvar, volta para a tela de título.
void FolHouse_ReturnToTitle(void)
{
    DoSoftReset();
}

// -----------------------------------------------------------------------------
// Listas (viagem rápida, categorias, opções, rádio)
// -----------------------------------------------------------------------------
#define FOL_LIST_MAX 12


#define tListTaskId data[0]
#define tWindowId   data[1]

static EWRAM_DATA struct ListMenuItem *sFolListItems = NULL;

static void Task_FolListInput(u8 taskId);

static const struct ListMenuTemplate sFolListTemplate =
{
    .items = NULL,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NORMAL,
    .cursorKind = CURSOR_BLACK_ARROW,
};

static void ShowFolList(u8 count)
{
    struct WindowTemplate template;
    struct ListMenuTemplate list;
    u8 taskId, i, maxShowed;
    u32 width = 0;

    for (i = 0; i < count; i++)
        width = DisplayTextAndGetWidth(sFolListItems[i].name, width);
    width = ConvertPixelWidthToTileWidth(width);
    maxShowed = count < 6 ? count : 6;

    taskId = CreateTask(Task_FolListInput, 8);
    template = CreateWindowTemplate(0, 1, 1, width, maxShowed * 2, 0xF, 0x64);
    gTasks[taskId].tWindowId = AddWindow(&template);
    SetStandardWindowBorderStyle(gTasks[taskId].tWindowId, FALSE);

    list = sFolListTemplate;
    list.items = sFolListItems;
    list.totalItems = count;
    list.maxShowed = maxShowed;
    list.windowId = gTasks[taskId].tWindowId;
    gTasks[taskId].tListTaskId = ListMenuInit(&list, 0, 0);
    CopyWindowToVram(gTasks[taskId].tWindowId, COPYWIN_FULL);
    ScheduleBgCopyTilemapToVram(0);
}

static void CloseFolList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    ClearStdWindowAndFrameToTransparent(gTasks[taskId].tWindowId, TRUE);
    RemoveWindow(gTasks[taskId].tWindowId);
    FREE_AND_SET_NULL(sFolListItems);
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static void Task_FolListInput(u8 taskId)
{
    s32 input = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    switch (input)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        gSpecialVar_Result = MULTI_B_PRESSED;
        CloseFolList(taskId);
        break;
    default:
        PlaySE(SE_SELECT);
        gSpecialVar_Result = input;
        CloseFolList(taskId);
        break;
    }
}

static void Task_FolListEnableScript(u8 taskId)
{
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static u8 AddCancel(u8 count)
{
    sFolListItems[count].name = gText_Cancel;
    sFolListItems[count].id = LIST_CANCEL;
    return count + 1;
}

// VAR_0x8004 = tipo de lista (FOL_LIST_*); para opções, VAR_0x8005 = categoria.
// Depois do waitstate, VAR_RESULT = id escolhido ou MULTI_B_PRESSED.
void FolHouse_ShowList(void)
{
    u8 i, count = 0;

    sFolListItems = AllocZeroed(sizeof(struct ListMenuItem) * (FOL_LIST_MAX + 1));
    LockPlayerFieldControls();

    switch (gSpecialVar_0x8004)
    {
    case FOL_LIST_FAST_TRAVEL:
        for (i = 0; i < FOL_HOUSE_COUNT && count < FOL_LIST_MAX; i++)
        {
            if (FolHouses_IsOwned(i) && i != GetCurrentHouse())
            {
                sFolListItems[count].name = sFolHouses[i].name;
                sFolListItems[count].id = i;
                count++;
            }
        }
        break;
    case FOL_LIST_FIXED_CATEGORY:
        for (i = 0; i < FOL_FIXED_COUNT; i++)
        {
            sFolListItems[count].name = sFixedDecorSlots[i].name;
            sFolListItems[count].id = i;
            count++;
        }
        break;
    case FOL_LIST_FIXED_OPTION:
        if (gSpecialVar_0x8005 < FOL_FIXED_COUNT)
        {
            const struct FolFixedDecorSlot *slot = &sFixedDecorSlots[gSpecialVar_0x8005];
            for (i = 0; i < slot->count && count < FOL_LIST_MAX; i++)
            {
                if (gSpecialVar_0x8005 == FOL_FIXED_PC_SIDE && i == FOL_PC_SIDE_TROPHY && !FlagGet(FLAG_SYS_GAME_CLEAR))
                    continue;
                sFolListItems[count].name = slot->options[i].name;
                sFolListItems[count].id = i;
                count++;
            }
        }
        break;
    case FOL_LIST_RADIO:
        for (i = 0; i < ARRAY_COUNT(sRadioTracks) && count < FOL_LIST_MAX; i++)
        {
            sFolListItems[count].name = sRadioTracks[i].name;
            sFolListItems[count].id = i;
            count++;
        }
        break;
    }

    if (count == 0)
    {
        // Nada para listar (ex.: nenhuma outra casa comprada).
        // O special tem waitstate=1: o script só continua quando a task
        // abaixo chamar ScriptContext_Enable no próximo frame.
        FREE_AND_SET_NULL(sFolListItems);
        gSpecialVar_Result = FOL_HOUSE_NONE;
        CreateTask(Task_FolListEnableScript, 8);
        return;
    }
    count = AddCancel(count);
    ShowFolList(count);
}
