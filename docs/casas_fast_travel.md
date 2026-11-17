# Casas de viagem rápida (Forest of Love)

As casas são construídas em cima do código de **Base Secreta** da pret. Todas as
casas usam **um único mapa interior** (`MAP_FOL_HOUSE`, layout `LAYOUT_FOL_HOUSE`)
e **um único conjunto de decorações**: o que você muda numa casa aparece em
todas.

- **Quarto de Littleroot** (`FOL_HOUSE_BEDROOM`) é a primeira casa: grátis,
  sempre do jogador. Depois que o relógio da parede é acertado
  (`FLAG_SET_WALL_CLOCK`), a escada do 1F leva à casa compartilhada em vez do
  2F vanilla. Ao continuar o jogo ou voltar de um warp de cura (Hall da Fama,
  desmaio), o jogador aparece em frente à cama.
- As outras casas são **compradas com dinheiro** falando com o corretor
  (NPC cavalheiro) ao lado da porta.

## Interior

| Objeto | Função |
|---|---|
| PC | **FAST TRAVEL** (viaja para qualquer casa sua, inclusive o quarto) e **DECORATION** → **FIXED** (móveis fixos, estilo Gold/Silver/Crystal) / **MOVABLE** (decorações normais de base secreta). Não há saque de itens. |
| Cama | Salva o jogo e oferece voltar à tela de título. |
| TV | TV normal (`EventScript_TV`). |
| Mesa central | É uma "DESK" de base secreta, aceita decorações em cima. |
| Item ao lado do PC | Rádio (escolha da trilha da casa), troféu da Liga Sylvaria (só aparece como opção com `FLAG_SYS_GAME_CLEAR`) ou telefone (gancho `FolHouse_PhoneHook`, por enquanto só um texto — será usado quando o PokéNav for removido). |
| Banheira, fogão, geladeira, tapete | Cosméticos. |

Os gráficos dos móveis fixos são **decorações vanilla usadas como placeholder**
(tabela `sFixedDecorSlots` em `src/fol_houses.c`). Para trocar a arte, crie
novas decorações (veja abaixo) e troque o `decorId` da opção.

## Onde ficam os dados no save

Nada novo foi adicionado ao SaveBlock1; espaço vanilla foi reaproveitado:

| Campo | Uso |
|---|---|
| `secretBases[0]` | Decorações MÓVEIS da casa (o código vanilla de base continua funcionando). |
| `secretBases[1..19]` | `struct FolHouseSave` (`gFolHouseSave`): bitmask `ownedHouses` (offset 0x0C, depois do `trainerName` que `ClearSecretBase` enche de 0xFF) + 40 bytes reservados. |
| `folFixedDecor[12]` (ex-`playerRoomDecorations`) | Opção escolhida para cada móvel fixo (`FOL_FIXED_*`). |
| `folHouseMisc[12]` (ex-`playerRoomDecorationPositions`) | `[0]` faixa do rádio, `[1]` casa atual. |

`STATIC_ASSERT`s em `src/fol_houses.c` garantem que tudo cabe. As asserções
de tamanho do save da pret continuam passando (o tamanho do SaveBlock1 não
mudou).

## Como adicionar uma casa numa cidade

1. **Constante**: em `include/constants/fol_houses.h`, adicione
   `#define FOL_HOUSE_MINHACIDADE N` antes de `FOL_HOUSE_COUNT` e aumente
   `FOL_HOUSE_COUNT`. Máximo 32 (`FOL_HOUSE_MAX`, bits de `ownedHouses`).
   **Nunca reordene** os IDs existentes depois de lançar uma versão: eles são
   bits no save.
2. **Tabela**: em `src/fol_houses.c`, crie o nome
   (`static const u8 sText_HouseMinhaCidade[] = _("MINHA CIDADE");`) e uma linha
   em `sFolHouses`: `{ nome, MAP_DA_CIDADE, x, y, preço }`. `x, y` é o tile
   **externo** onde o jogador aparece ao sair (normalmente em frente à porta).
3. **Corretor**: em `data/maps/FolHouse/scripts.inc`, crie
   ```
   FolHouse_EventScript_AgentMinhaCidade::
   	setvar VAR_0x8004, FOL_HOUSE_MINHACIDADE
   	goto FolHouse_EventScript_Agent
   	end
   ```
4. **NPC**: no `map.json` da cidade, adicione um object event (ex.
   `OBJ_EVENT_GFX_GENTLEMAN`, `MOVEMENT_TYPE_FACE_DOWN`) com
   `"script": "FolHouse_EventScript_AgentMinhaCidade"`, ao lado/na frente da
   porta escolhida. Se quiser que a porta física leve à casa, troque o
   `dest_map` do warp da porta por um `coord_event`/script que chame
   `FolHouse_EventScript_AgentMinhaCidade` (ou um script próprio que faça
   `special FolHouse_PrepareEnter` + `warp MAP_FOL_HOUSE, FOL_HOUSE_DOOR_WARP`).
5. `make MODERN=0` e teste: comprar, entrar, sair, viajar pelo PC.

As casas de teste atuais: Oldale (3,9) ₽10000, Petalburg (18,26) ₽20000,
Rustboro (31,21) ₽30000 — corretor um tile acima da posição de saída.

## Como adicionar opções de móveis fixos

Em `src/fol_houses.c`, cada categoria (`FOL_FIXED_BED` ... `FOL_FIXED_PC_SIDE`)
tem um array de `struct FolFixedDecorOption { nome, decorId }`. Adicione uma
linha ao array certo. O `decorId` deve ter o **mesmo formato** (shape) do
slot, senão o desenho invade outros móveis. A posição de cada móvel é o canto
inferior esquerdo em `sFixedDecorSlots`. Se mexer nos tiles do layout, ajuste
também os `bg_events` (placas invisíveis) em `data/maps/FolHouse/map.json`.

## Novas decorações móveis

Foram criadas como andaime (sem arte nova ainda): `DECOR_FOL_DOLL`,
`DECOR_FOL_ORNAMENT`, `DECOR_FOL_PLANT`, `DECOR_FOL_CUSHION`, nas categorias
DOLL / ORNAMENT / PLANT / CUSHION. Usam gráficos vanilla (boneco Pikachu,
ornamento de vidro, planta vermelha, almofada Pika). Ainda **não são obtíveis**
no jogo; para dar ao jogador use `adddecoration DECOR_FOL_DOLL` num script
(loja, presente etc.). Para novas: adicione o ID em
`include/constants/decorations.h` (atualize `NUM_DECORATIONS`), a entrada em
`src/data/decoration/header.h`, `description.h` e `icon.h`.

## Rádio

Faixas em `sRadioTracks` (`src/fol_houses.c`): nome + música. A escolha fica em
`folHouseMisc[0]` e é aplicada no `OnTransition` do mapa
(`Overworld_SetSavedMusic`).

## O que foi desligado

- **Poder Secreto**: não aparece mais no menu do Pokémon; olhar para uma
  árvore/arbusto/caverna de base mostra só o texto vanilla de descrição da
  entrada (o mesmo de quando não se tem Poder Secreto), sem criar base.
- **Voar**: só em batalha. Não aparece no menu do Pokémon fora de batalha.
- **Mistura de bases** (Record Mix): `ReceiveSecretBasesData` não faz nada,
  porque `secretBases[1..19]` agora guarda os dados das casas.
- Entradas de base ocupadas não são mais desenhadas no overworld.
