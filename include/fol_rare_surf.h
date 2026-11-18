#ifndef GUARD_FOL_RARE_SURF_H
#define GUARD_FOL_RARE_SURF_H

// HACKROM (Forest of Love): "casas raras" de surf.
// É a mecânica dos tiles secretos do Feebas da Rota 119, convertida de PESCA
// para SURF: enquanto o jogador surfa, alguns tiles de água escondidos (os
// mesmos escolhidos pela semente do Feebas original) podem gerar um encontro
// raro. Veja docs/surf_raro_lapras.md para mover para outro mapa.

#define FOL_RARE_SURF_ENABLED        TRUE

// Mapa onde a mecânica vale (precisa ter tabela water_mons em wild_encounters.json).
#define FOL_RARE_SURF_MAP            MAP_ROUTE119

// Pokémon raro, níveis (inclusive) e chance (%) de ele aparecer quando um
// encontro de surf acontece em cima de um tile raro.
#define FOL_RARE_SURF_SPECIES        SPECIES_LAPRAS
#define FOL_RARE_SURF_MIN_LEVEL      25
#define FOL_RARE_SURF_MAX_LEVEL      30
#define FOL_RARE_SURF_CHANCE         50

// Quantidade de tiles raros sorteados no mapa (Feebas original: 6).
#define FOL_RARE_SURF_NUM_SPOTS      6

// Os N primeiros tiles de água (contando da esquerda para a direita, de cima
// para baixo) nunca são sorteados. Na Rota 119 são 3 tiles inacessíveis no
// topo, em (9,7), (7,13) e (15,16). Em outro mapa, use 0 ou o número de
// tiles de água inalcançáveis que vierem primeiro na contagem.
#define FOL_RARE_SURF_SKIP_FIRST_SPOTS 3

#endif // GUARD_FOL_RARE_SURF_H
