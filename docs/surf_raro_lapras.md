# Surf raro (Lapras) — antigo tile do Feebas

No Emerald original, a Rota 119 tem 6 tiles de água secretos onde, **pescando**,
há 50% de chance de fisgar um Feebas. No Forest of Love essa mecânica virou
**surf**: surfando em cima de um desses tiles, cada encontro selvagem tem 50%
de chance de ser um **Lapras** (Lv 25–30). A pesca da Rota 119 agora usa só a
tabela normal (`fishing_mons`), sem tiles especiais.

Isso torna possível o requisito da Sealed Chamber (Lapras no primeiro slot,
Omastar no último), que não foi alterado.

## Como funciona

- Todos os tiles de água surfáveis do mapa (que não sejam cachoeira) são
  numerados de 1 a N, da esquerda para a direita e de cima para baixo
  (Rota 119: N = 447, a mesma numeração do Feebas original).
- A semente é `gSaveBlock1Ptr->dewfordTrends[0].rand` (a tendência de Dewford),
  igual ao Feebas: os 6 tiles mudam quando a tendência de Dewford muda.
- Os `FOL_RARE_SURF_SKIP_FIRST_SPOTS` primeiros números nunca são sorteados
  (na Rota 119 são 3 tiles inacessíveis no topo do mapa).
- Quando um encontro de surf acontece (taxa normal do mapa) e o jogador está
  num tile raro, há `FOL_RARE_SURF_CHANCE`% de o encontro virar o Pokémon raro.
  Senão sai um Pokémon normal da tabela `water_mons`.
- Repel vale normalmente (Lapras com nível abaixo do primeiro do time é
  bloqueado). Sweet Scent em cima do tile também pode gerar o Lapras.
- Ele aparece na tela de área da Pokédex no mapa configurado.

Código: `src/wild_encounter.c` (`CheckFolRareSurfSpot`,
`TryGenerateFolRareSurfMon`) e `src/pokedex_area_screen.c` (`sFeebasData`).

## Configuração — `include/fol_rare_surf.h`

| Constante | Valor | O que faz |
|---|---|---|
| `FOL_RARE_SURF_ENABLED` | `TRUE` | liga/desliga a mecânica |
| `FOL_RARE_SURF_MAP` | `MAP_ROUTE119` | mapa onde vale |
| `FOL_RARE_SURF_SPECIES` | `SPECIES_LAPRAS` | Pokémon raro |
| `FOL_RARE_SURF_MIN_LEVEL` / `MAX_LEVEL` | 25 / 30 | níveis |
| `FOL_RARE_SURF_CHANCE` | 50 | % por encontro em tile raro |
| `FOL_RARE_SURF_NUM_SPOTS` | 6 | quantos tiles raros |
| `FOL_RARE_SURF_SKIP_FIRST_SPOTS` | 3 | primeiros tiles de água ignorados |

## Mover para outra rota

1. Troque `FOL_RARE_SURF_MAP` pelo `MAP_...` da nova rota.
2. A rota precisa ter `water_mons` em `src/data/wild_encounters.json`
   (senão não há encontros de surf e o Lapras nunca aparece).
3. Ajuste `FOL_RARE_SURF_SKIP_FIRST_SPOTS`: conte quantos tiles de água
   inalcançáveis aparecem primeiro na ordem de leitura (esquerda→direita,
   cima→baixo). Se não houver, use 0. Dica: pinte a água inalcançável com um
   metatile não surfável (comportamento `MB_NORMAL`), como a Rota 119 faz, e
   ela deixa de contar.
4. Opcional: ajuste chance, níveis e número de tiles.
5. A contagem é feita no mapa inteiro na hora do encontro, então não há tabela
   pré-calculada para refazer (o custo só existe quando um encontro de surf
   acontece no mapa configurado).

## Rota 119 hoje

Com a semente de um jogo novo de teste (`dewfordTrends[0].rand = 54119`),
os tiles raros eram (x, y): (23,100), (25,43), (35,54), (24,102), (15,103),
(29,62). Em outro save a semente é outra, então os tiles também mudam.
