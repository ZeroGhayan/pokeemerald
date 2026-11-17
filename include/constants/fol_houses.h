#ifndef GUARD_CONSTANTS_FOL_HOUSES_H
#define GUARD_CONSTANTS_FOL_HOUSES_H

// HACKROM (Forest of Love): casas de viagem rápida.
// Ver docs/casas_fast_travel.md para o passo a passo de como adicionar cidades.

// --- Casas -------------------------------------------------------------------
// IDs fixos (gravados no save como bits de posse). Adicione novas casas no fim
// e atualize sFolHouses[] em src/fol_houses.c. Máximo: 32 (bits de um u32).
#define FOL_HOUSE_BEDROOM    0  // Quarto de Littleroot: grátis e sempre possuído.
#define FOL_HOUSE_OLDALE     1  // Casa de teste em Oldale
#define FOL_HOUSE_PETALBURG  2  // Casa de teste em Petalburg
#define FOL_HOUSE_RUSTBORO   3  // Casa de teste em Rustboro
#define FOL_HOUSE_COUNT      4
#define FOL_HOUSE_MAX        32
#define FOL_HOUSE_NONE       0xFF

// --- Decorações fixas (estilo Gold/Silver/Crystal) ---------------------------
// Cada categoria tem um "slot" fixo no mapa compartilhado FolHouse e várias
// opções escolhidas no PC. As opções estão em sFixedDecorOptions[] (fol_houses.c).
#define FOL_FIXED_BED        0  // Cama: salva e oferece voltar à tela de título
#define FOL_FIXED_TV         1  // TV: funciona como a TV normal
#define FOL_FIXED_CONSOLE    2  // Console (cosmético)
#define FOL_FIXED_TABLE      3  // Mesa central: aceita decorações em cima
#define FOL_FIXED_BATHTUB    4  // Banheira (cosmético)
#define FOL_FIXED_STOVE      5  // Fogão (cosmético)
#define FOL_FIXED_FRIDGE     6  // Geladeira (cosmético)
#define FOL_FIXED_CARPET     7  // Tapete (cosmético)
#define FOL_FIXED_PC_SIDE    8  // Item ao lado do PC: rádio / troféu / telefone
#define FOL_FIXED_COUNT      9

// Opções do item ao lado do PC (índices em sFixedDecorOptions[FOL_FIXED_PC_SIDE]).
#define FOL_PC_SIDE_RADIO    0
#define FOL_PC_SIDE_TROPHY   1  // Só aparece na lista depois de zerar o jogo
#define FOL_PC_SIDE_PHONE    2  // Gancho para o substituto do PokéNav

// Faixas do rádio (índices em sRadioTracks[]). 0 = rádio desligado (música do mapa).
#define FOL_RADIO_OFF        0

// Listas mostradas por special FolHouse_ShowList (VAR_0x8004)
#define FOL_LIST_FAST_TRAVEL     0
#define FOL_LIST_FIXED_CATEGORY  1
#define FOL_LIST_FIXED_OPTION    2  // VAR_0x8005 = categoria
#define FOL_LIST_RADIO           3

// --- Mapa compartilhado -------------------------------------------------------
// Posições (coordenadas do mapa FolHouse) usadas pelos warps de C/script.
#define FOL_HOUSE_DOOR_WARP  0   // warp 0 = tapete de saída
#define FOL_HOUSE_PC_FRONT_X 3   // tile em frente ao PC
#define FOL_HOUSE_PC_FRONT_Y 2
#define FOL_HOUSE_BED_FRONT_X 12 // tile em frente à cama (chegada após o Hall of Fame)
#define FOL_HOUSE_BED_FRONT_Y 3

#endif // GUARD_CONSTANTS_FOL_HOUSES_H
