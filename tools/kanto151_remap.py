#!/usr/bin/env python3
"""
Forest of Love - Kanto 151 remap.

Troca, nos dados onde um Pokémon pode ser obtido, enfrentado ou visto em
evento, toda espécie fora dos 151 de Kanto (SPECIES_* com índice interno >151)
por uma espécie de Kanto, usando a tabela MAPPING abaixo.

- Não apaga dados de espécies (species_info, gráficos, learnsets continuam).
- Só mexe nos arquivos/linhas listados em TARGETS.
- É idempotente: rodar de novo não muda nada.
- Gera docs/kanto151_mapping.md com a tabela e a contagem por arquivo.

Uso (na raiz do repositório):  python3 tools/kanto151_remap.py
"""
import glob, os, re, sys, collections

# Critério: mesmo tipo primário (ou o mais próximo quando Kanto não tem o tipo,
# ex.: Dark/Steel), estágio de evolução e força parecidos. Linhas evolutivas
# viram linhas evolutivas. Iniciais Johto/Hoenn -> iniciais de Kanto (mesma
# ordem Grama/Fogo/Água, o que mantém a lógica do rival). Lendários -> aves
# lendárias, Mew ou Mewtwo.
MAPPING = {
    # --- Johto ---
    "CHIKORITA": "BULBASAUR", "BAYLEEF": "IVYSAUR", "MEGANIUM": "VENUSAUR",
    "CYNDAQUIL": "CHARMANDER", "QUILAVA": "CHARMELEON", "TYPHLOSION": "CHARIZARD",
    "TOTODILE": "SQUIRTLE", "CROCONAW": "WARTORTLE", "FERALIGATR": "BLASTOISE",
    "SENTRET": "RATTATA", "FURRET": "RATICATE",
    "HOOTHOOT": "PIDGEY", "NOCTOWL": "PIDGEOT",
    "LEDYBA": "CATERPIE", "LEDIAN": "BUTTERFREE",
    "SPINARAK": "WEEDLE", "ARIADOS": "BEEDRILL",
    "CROBAT": "GOLBAT",
    "CHINCHOU": "TENTACOOL", "LANTURN": "TENTACRUEL",
    "PICHU": "PIKACHU", "CLEFFA": "CLEFAIRY", "IGGLYBUFF": "JIGGLYPUFF",
    "TOGEPI": "CLEFAIRY", "TOGETIC": "CLEFABLE",
    "NATU": "ABRA", "XATU": "KADABRA",
    "MAREEP": "MAGNEMITE", "FLAAFFY": "MAGNETON", "AMPHAROS": "ELECTABUZZ",
    "BELLOSSOM": "VILEPLUME",
    "MARILL": "PSYDUCK", "AZUMARILL": "GOLDUCK",
    "SUDOWOODO": "GRAVELER",
    "POLITOED": "POLIWRATH",
    "HOPPIP": "BELLSPROUT", "SKIPLOOM": "WEEPINBELL", "JUMPLUFF": "VICTREEBEL",
    "AIPOM": "MEOWTH",
    "SUNKERN": "EXEGGCUTE", "SUNFLORA": "EXEGGUTOR",
    "YANMA": "SCYTHER",
    "WOOPER": "SLOWPOKE", "QUAGSIRE": "SLOWBRO",
    "ESPEON": "JOLTEON", "UMBREON": "VAPOREON",
    "MURKROW": "SPEAROW",
    "SLOWKING": "SLOWBRO",
    "MISDREAVUS": "HAUNTER",
    "UNOWN": "DITTO",
    "WOBBUFFET": "MR_MIME",
    "GIRAFARIG": "KANGASKHAN",
    "PINECO": "METAPOD", "FORRETRESS": "PINSIR",
    "DUNSPARCE": "LICKITUNG",
    "GLIGAR": "SANDSLASH",
    "STEELIX": "ONIX",
    "SNUBBULL": "CLEFAIRY", "GRANBULL": "CLEFABLE",
    "QWILFISH": "TENTACRUEL",
    "SCIZOR": "SCYTHER",
    "SHUCKLE": "KABUTO",
    "HERACROSS": "PINSIR",
    "SNEASEL": "JYNX",
    "TEDDIURSA": "MEOWTH", "URSARING": "TAUROS",
    "SLUGMA": "VULPIX", "MAGCARGO": "NINETALES",
    "SWINUB": "SEEL", "PILOSWINE": "DEWGONG",
    "CORSOLA": "STARYU",
    "REMORAID": "HORSEA", "OCTILLERY": "SEADRA",
    "DELIBIRD": "JYNX",
    "MANTINE": "SEAKING",
    "SKARMORY": "FEAROW",
    "HOUNDOUR": "GROWLITHE", "HOUNDOOM": "ARCANINE",
    "KINGDRA": "SEADRA",
    "PHANPY": "CUBONE", "DONPHAN": "MAROWAK",
    "PORYGON2": "PORYGON",
    "STANTLER": "TAUROS",
    "SMEARGLE": "DITTO",
    "TYROGUE": "MACHOP", "HITMONTOP": "HITMONCHAN",
    "SMOOCHUM": "JYNX", "ELEKID": "ELECTABUZZ", "MAGBY": "MAGMAR",
    "MILTANK": "TAUROS", "BLISSEY": "CHANSEY",
    "RAIKOU": "ZAPDOS", "ENTEI": "MOLTRES", "SUICUNE": "ARTICUNO",
    "LARVITAR": "RHYHORN", "PUPITAR": "RHYHORN", "TYRANITAR": "RHYDON",
    "LUGIA": "MEWTWO", "HO_OH": "MOLTRES", "CELEBI": "MEW",
    # --- Hoenn ---
    "TREECKO": "BULBASAUR", "GROVYLE": "IVYSAUR", "SCEPTILE": "VENUSAUR",
    "TORCHIC": "CHARMANDER", "COMBUSKEN": "CHARMELEON", "BLAZIKEN": "CHARIZARD",
    "MUDKIP": "SQUIRTLE", "MARSHTOMP": "WARTORTLE", "SWAMPERT": "BLASTOISE",
    "POOCHYENA": "MEOWTH", "MIGHTYENA": "PERSIAN",
    "ZIGZAGOON": "RATTATA", "LINOONE": "RATICATE",
    "WURMPLE": "CATERPIE", "SILCOON": "METAPOD", "BEAUTIFLY": "BUTTERFREE",
    "CASCOON": "KAKUNA", "DUSTOX": "VENOMOTH",
    "LOTAD": "POLIWAG", "LOMBRE": "POLIWHIRL", "LUDICOLO": "POLIWRATH",
    "SEEDOT": "ODDISH", "NUZLEAF": "GLOOM", "SHIFTRY": "VILEPLUME",
    "NINCADA": "VENONAT", "NINJASK": "SCYTHER", "SHEDINJA": "HAUNTER",
    "TAILLOW": "SPEAROW", "SWELLOW": "FEAROW",
    "SHROOMISH": "PARAS", "BRELOOM": "PARASECT",
    "SPINDA": "LICKITUNG",
    "WINGULL": "PIDGEY", "PELIPPER": "PIDGEOT",
    "SURSKIT": "VENONAT", "MASQUERAIN": "VENOMOTH",
    "WAILMER": "SEEL", "WAILORD": "LAPRAS",
    "SKITTY": "EEVEE", "DELCATTY": "PERSIAN",
    "KECLEON": "DITTO",
    "BALTOY": "DIGLETT", "CLAYDOL": "DUGTRIO",
    "NOSEPASS": "ONIX",
    "TORKOAL": "MAGMAR",
    "SABLEYE": "HAUNTER",
    "BARBOACH": "KRABBY", "WHISCASH": "KINGLER",
    "LUVDISC": "GOLDEEN",
    "CORPHISH": "KRABBY", "CRAWDAUNT": "KINGLER",
    "FEEBAS": "MAGIKARP", "MILOTIC": "GYARADOS",
    "CARVANHA": "GOLDEEN", "SHARPEDO": "SEAKING",
    "TRAPINCH": "SANDSHREW", "VIBRAVA": "SANDSLASH", "FLYGON": "RHYDON",
    "MAKUHITA": "MANKEY", "HARIYAMA": "PRIMEAPE",
    "ELECTRIKE": "VOLTORB", "MANECTRIC": "ELECTRODE",
    "NUMEL": "PONYTA", "CAMERUPT": "RAPIDASH",
    "SPHEAL": "SEEL", "SEALEO": "DEWGONG", "WALREIN": "LAPRAS",
    "CACNEA": "BELLSPROUT", "CACTURNE": "VICTREEBEL",
    "SNORUNT": "SHELLDER", "GLALIE": "CLOYSTER",
    "LUNATONE": "OMASTAR", "SOLROCK": "KABUTOPS",
    "AZURILL": "PSYDUCK",
    "SPOINK": "DROWZEE", "GRUMPIG": "HYPNO",
    "PLUSLE": "PIKACHU", "MINUN": "PIKACHU",
    "MAWILE": "MAGNEMITE",
    "MEDITITE": "MACHOP", "MEDICHAM": "MACHOKE",
    "SWABLU": "DODUO", "ALTARIA": "DRAGONAIR",
    "WYNAUT": "MR_MIME",
    "DUSKULL": "GASTLY", "DUSCLOPS": "HAUNTER",
    "ROSELIA": "GLOOM",
    "SLAKOTH": "LICKITUNG", "VIGOROTH": "KANGASKHAN", "SLAKING": "SNORLAX",
    "GULPIN": "GRIMER", "SWALOT": "MUK",
    "TROPIUS": "EXEGGUTOR",
    "WHISMUR": "JIGGLYPUFF", "LOUDRED": "WIGGLYTUFF", "EXPLOUD": "WIGGLYTUFF",
    "CLAMPERL": "SHELLDER", "HUNTAIL": "CLOYSTER", "GOREBYSS": "STARMIE",
    "ABSOL": "PERSIAN",
    "SHUPPET": "GASTLY", "BANETTE": "GENGAR",
    "SEVIPER": "ARBOK", "ZANGOOSE": "PERSIAN",
    "RELICANTH": "OMASTAR",
    "ARON": "GEODUDE", "LAIRON": "GRAVELER", "AGGRON": "GOLEM",
    "CASTFORM": "PORYGON",
    "VOLBEAT": "BUTTERFREE", "ILLUMISE": "VENOMOTH",
    "LILEEP": "OMANYTE", "CRADILY": "OMASTAR",
    "ANORITH": "KABUTO", "ARMALDO": "KABUTOPS",
    "RALTS": "DROWZEE", "KIRLIA": "DROWZEE", "GARDEVOIR": "HYPNO",
    "BAGON": "DRATINI", "SHELGON": "DRAGONAIR", "SALAMENCE": "DRAGONITE",
    "BELDUM": "ABRA", "METANG": "KADABRA", "METAGROSS": "ALAKAZAM",
    "REGIROCK": "AERODACTYL", "REGICE": "ARTICUNO", "REGISTEEL": "ZAPDOS",
    "KYOGRE": "ARTICUNO", "GROUDON": "MOLTRES", "RAYQUAZA": "DRAGONITE",
    "LATIAS": "MEW", "LATIOS": "MEWTWO",
    "JIRACHI": "MEW", "DEOXYS": "MEWTWO",
    "CHIMECHO": "MR_MIME",
}

# Exceções só na Battle Frontier (battle_frontier_mons.h), aplicadas pelo nome
# da entrada [FRONTIER_MON_<NOME>_n]. Motivo: os sets dos colecionadores de
# iniciais (Gideon, Triston) e de Eevee (Anya) ficariam com só 3 espécies
# diferentes, e a Frontier sorteia até 4 Pokémon de espécies e itens
# diferentes (duplas) -> o sorteio entraria em loop infinito.
FRONTIER_OVERRIDES = {
    "MEGANIUM": "VILEPLUME",
    "TYPHLOSION": "ARCANINE",
    "FERALIGATR": "GYARADOS",
    "ESPEON": "ALAKAZAM",
    "UMBREON": "SNORLAX",
}

# (padrão glob, função que diz se a linha pode ser alterada) -- linhas 1-based
ALL = None
TARGETS = [
    ("src/data/trainer_parties.h", ALL),                       # treinadores
    ("src/data/trade.h", ALL),                                 # trocas in-game
    ("data/maps/*/scripts.inc", ALL),                          # givemon, setwildbattle, seteventmon, giveegg, cries, showmonpic
    ("data/scripts/*.inc", ALL),
    ("src/battle_controllers.c", ALL),                         # Zigzagoon que persegue o Birch (1a batalha)
    ("src/field_specials.c", ALL),                             # Zigzagoon emprestado ao Wally
    ("src/battle_setup.c", lambda n, l: "CreateMaleMon" in l), # Ralts do tutorial do Wally (não os cases de música lendária)
    ("src/roamer.c", ALL),                                     # Latias/Latios errantes
    ("src/tv.c", ALL),                                         # surtos (mass outbreak) da TV
    ("src/wild_encounter.c", ALL),                             # Feebas dos tiles especiais da Rota 119
    ("src/pokemon_size_record.c", ALL),                        # concursos de tamanho (Seedot/Lotad)
    ("src/braille_puzzles.c", ALL),                            # Wailord/Relicanth da Sealed Chamber (Regis)
    ("src/main_menu.c", ALL),                                  # BIRCH_INTRO_SPECIES
    ("src/frontier_util.c", lambda n, l: "gFrontierBannedSpecies" not in l and n < 670),  # Frontier Brains; não a lista de banidos
    ("src/battle_pike.c", ALL),
    ("src/battle_tower.c", ALL),                               # time do Steven na batalha dupla de Mossdeep
    ("src/data/battle_frontier/battle_frontier_mons.h", ALL),
    ("src/data/battle_frontier/battle_tent.h", ALL),
    ("src/data/battle_frontier/trainer_hill.h", ALL),
    ("src/data/battle_frontier/apprentice.h", ALL),
    ("src/data/battle_frontier/battle_pyramid_level_50_wild_mons.h", ALL),
    ("src/data/battle_frontier/battle_pyramid_open_level_wild_mons.h", ALL),
    ("src/data/contest_opponents.h", ALL),
    ("src/data/lilycove_lady.h", ALL),
]

TOKEN = re.compile(r"\bSPECIES_([A-Z0-9_]+)\b")

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root)
    species = {}
    for l in open("include/constants/species.h"):
        m = re.match(r"#define SPECIES_(\w+)\s+(\d+)\b", l)
        if m:
            species[m.group(1)] = int(m.group(2))
    nonkanto = {k for k, v in species.items() if 151 < v < 412 and not k.startswith("OLD_UNOWN")}
    missing = nonkanto - MAPPING.keys()
    if missing:
        sys.exit("faltam no MAPPING: " + ", ".join(sorted(missing)))
    for k, v in MAPPING.items():
        assert species.get(v, 999) <= 151, (k, v)

    used = collections.Counter()
    perfile = collections.Counter()
    for pattern, allow in TARGETS:
        for path in sorted(glob.glob(pattern)):
            with open(path, encoding="utf-8") as f:
                lines = f.readlines()
            changed = False
            for i, line in enumerate(lines):
                if allow is not None and not allow(i + 1, line):
                    continue
                def sub(m):
                    name = m.group(1)
                    if name in MAPPING:
                        used[name] += 1
                        perfile[path] += 1
                        return "SPECIES_" + MAPPING[name]
                    return m.group(0)
                new = TOKEN.sub(sub, line)
                if new != line:
                    lines[i] = new
                    changed = True
            if changed:
                with open(path, "w", encoding="utf-8") as f:
                    f.writelines(lines)

    apply_frontier_overrides(perfile)
    check_facility_sets()
    write_doc(species, used, perfile)
    print("trocas:", sum(perfile.values()), "em", len(perfile), "arquivos")

def apply_frontier_overrides(perfile):
    path = "src/data/battle_frontier/battle_frontier_mons.h"
    text = open(path, encoding="utf-8").read()
    pat = re.compile(r"(\[FRONTIER_MON_(\w+?)(?:_\d+)?\]\s*=\s*\{\s*\.species\s*=\s*)SPECIES_(\w+)")
    def sub(m):
        new = FRONTIER_OVERRIDES.get(m.group(2))
        if new and m.group(3) != new:
            perfile[path] += 1
            return m.group(1) + "SPECIES_" + new
        return m.group(0)
    new_text = pat.sub(sub, text)
    if new_text != text:
        open(path, "w", encoding="utf-8").write(new_text)

def _pickable(mons, need):
    # existe escolha de `need` Pokémon com espécies e itens todos diferentes?
    mons = sorted(set(mons))
    def rec(i, sp, it, n):
        if n == need:
            return True
        for j in range(i, len(mons)):
            s, t = mons[j]
            if s not in sp and t not in it and rec(j + 1, sp | {s}, it | {t}, n + 1):
                return True
        return False
    return rec(0, frozenset(), frozenset(), 0)

def check_facility_sets():
    import subprocess
    def table(path, keypat):
        t = open(path, encoding="utf-8").read()
        return {m.group(1): (m.group(2), m.group(3)) for m in re.finditer(
            r"\[(" + keypat + r")\]\s*=\s*\{\s*\.species\s*=\s*(SPECIES_\w+),\s*\.moves[^}]*\},\s*\.itemTableId\s*=\s*(\w+)", t)}
    def sets(text, keypat):
        return {m.group(1): re.findall(keypat, m.group(2))
                for m in re.finditer(r"const u16 (\w+)\[\]\s*=\s*\{(.*?)\};", text, re.S)}
    bad = []
    fm = table("src/data/battle_frontier/battle_frontier_mons.h", r"FRONTIER_MON_\w+")
    pre = subprocess.run(["cpp", "-P", "src/data/battle_frontier/battle_frontier_trainer_mons.h"],
                         capture_output=True, text=True).stdout
    for name, ids in sets(pre, r"FRONTIER_MON_\w+").items():
        if not _pickable([fm[i] for i in ids], 4):
            bad.append(name)
    tm = table("src/data/battle_frontier/battle_tent.h", r"\w+_TENT_MON_\w+")
    tent = open("src/data/battle_frontier/battle_tent.h", encoding="utf-8").read()
    for name, ids in sets(tent, r"\w+_TENT_MON_\w+").items():
        if not _pickable([tm[i] for i in ids], 3):
            bad.append(name)
    if bad:
        sys.exit("sets da Frontier/Tendas sem Pokémon suficientes (loop infinito): " + ", ".join(bad))

def write_doc(species, used, perfile):
    os.makedirs("docs", exist_ok=True)
    doc = "docs/kanto151_mapping.md"
    # contagens da última execução que alterou algo ficam salvas no próprio doc
    if not perfile and os.path.exists(doc):
        return
    out = ["# Forest of Love – remapeamento para os 151 de Kanto", "",
           "Gerado por `tools/kanto151_remap.py` (idempotente). Critério: mesmo tipo primário",
           "(ou o mais próximo quando Kanto não tem o tipo, ex. Dark/Steel), estágio de",
           "evolução e força parecidos; linhas evolutivas viram linhas evolutivas; iniciais",
           "de Johto/Hoenn viram os iniciais de Kanto na mesma ordem (Grama/Fogo/Água);",
           "lendários viram aves lendárias, Mew ou Mewtwo. Níveis e golpes customizados",
           "dos times foram mantidos. Nenhum dado de espécie foi apagado.", "",
           "| # | Antes | Depois | Ocorrências trocadas |", "|---|---|---|---|"]
    for name in sorted(MAPPING, key=lambda n: species[n]):
        out.append(f"| {species[name]} | {name} | {MAPPING[name]} | {used.get(name, 0)} |")
    out += ["", "## Exceções só na Battle Frontier (`battle_frontier_mons.h`)", "",
            "Para os sets de colecionadores (Gideon/Triston: iniciais; Anya: Eevees) continuarem",
            "com pelo menos 4 espécies diferentes (as duplas da Frontier sorteiam 4 Pokémon de",
            "espécies e itens diferentes; com menos o sorteio trava em loop):", "",
            "| Entrada | Vira |", "|---|---|"]
    for k, v in FRONTIER_OVERRIDES.items():
        out.append(f"| FRONTIER_MON_{k}_* | {v} |")
    out += ["", "## Arquivos alterados", "", "| Arquivo | Trocas |", "|---|---|"]
    for p, c in sorted(perfile.items(), key=lambda x: (-x[1], x[0])):
        out.append(f"| `{p}` | {c} |")
    out += ["", "## Mantido de propósito (lógica de código / dados, não obtível)", "",
            "- Tabelas de dados das espécies (species_info, gráficos, learnsets, cries, Pokédex, easy chat).",
            "- Checagens de código por espécie (Castform, Unown, Shedinja, Deoxys, Spinda...).",
            "- `gFrontierBannedSpecies` (frontier_util.c) e os `case` de música/transição de lendários em battle_setup.c.",
            "- `src/daycare.c`: lógica de incenso/bebês (Wynaut/Azurill/Volbeat/Illumise/Pichu) – inalcançável só com Kanto,",
            "  porque `evolution.h` já é só Kanto e nenhum ovo de Kanto vira essas espécies.",
            "- `src/battle_cups.c`: regra que proíbe Lugia/Ho-Oh.",
            "- Cutscenes: intro.c (Groudon/Kyogre), rayquaza_scene.c, roleta (cassino removido), pokemon_jump, e-Reader.",
            "- `src/starter_choose.c` é tratado à parte (iniciais Bulbasaur/Charmander/Squirtle fêmeas).", ""]
    with open(doc, "w", encoding="utf-8") as f:
        f.write("\n".join(out))

if __name__ == "__main__":
    main()
