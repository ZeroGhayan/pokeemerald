#!/usr/bin/env python3
"""
Forest of Love - paletas das sprites femininas (front_f.png / back_f.png).

O GBA usa UMA paleta de 16 cores por Pokémon, igual para frente e costas. O
gbagfx só grava os índices das cores do PNG; as cores em si vêm do .pal.
Este script, para cada graphics/pokemon/<espécie>/ que tem front_f.png:

1. Lê as cores USADAS em front_f.png e back_f.png (índice 0 = transparente,
   o do front_f manda).
2. Monta uma paleta única: primeiro as cores do front_f na ordem dele, depois
   as cores do back_f que faltarem. Se passar de 16 cores, para com erro.
3. Regrava back_f.png (e front_f.png, se necessário) só trocando os ÍNDICES
   para essa paleta. A imagem fica pixel a pixel igual.
4. Grava normal_f.pal (JASC-PAL, 16 cores).
5. NÃO cria shiny_f.pal: sem ele o jogo usa normal_f.pal também para a fêmea
   shiny (cores certas, só que sem variação shiny). Para ter shiny fêmea,
   copie normal_f.pal para shiny_f.pal e troque as cores mantendo a ORDEM.
6. Gera src/data/pokemon_graphics/female_palettes.h com a lista das paletas.

Uso (na raiz do repositório):  python3 tools/female_palettes.py [espécie ...]
Sem argumentos processa todas. É idempotente.
"""
import glob, os, sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

def used_colors(im):
    pal = im.getpalette()
    idx = sorted(set(im.getdata()))
    return idx, {i: tuple(pal[i*3:i*3+3]) for i in idx}

def remap(im, order):
    """Regrava im (modo P) com a paleta `order` (lista de cores RGB, até 16)."""
    pal = im.getpalette()
    lut = {}
    for i in set(im.getdata()):
        c = tuple(pal[i*3:i*3+3])
        lut[i] = 0 if i == 0 else order.index(c)
    out = Image.new("P", im.size, 0)
    flat = []
    for c in order + [(0, 0, 0)] * (16 - len(order)):
        flat += list(c)
    out.putpalette(flat)
    out.putdata([lut[p] for p in im.getdata()])
    return out

def write_pal(path, order):
    cols = order + [(0, 0, 0)] * (16 - len(order))
    with open(path, "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n16\n")
        for c in cols:
            f.write("%d %d %d\n" % c)

def process(d):
    ff, bf = d + "front_f.png", d + "back_f.png"
    if not os.path.exists(ff):
        return None
    front = Image.open(ff)
    if front.mode != "P":
        sys.exit(f"{ff}: precisa ser PNG indexado (modo P), está {front.mode}")
    fidx, fcol = used_colors(front)
    order = [tuple(front.getpalette()[0:3])]  # índice 0 = cor transparente
    for i in fidx:
        if i != 0 and fcol[i] not in order:
            order.append(fcol[i])
    back = None
    if os.path.exists(bf):
        back = Image.open(bf)
        if back.mode != "P":
            sys.exit(f"{bf}: precisa ser PNG indexado (modo P), está {back.mode}")
        bidx, bcol = used_colors(back)
        for i in bidx:
            if i != 0 and bcol[i] not in order:
                order.append(bcol[i])
    if len(order) > 16:
        # Não dá para juntar: o Pedro precisa reduzir/igualar as cores das duas
        # imagens. Sem normal_f.pal o jogo usa a paleta normal (cores erradas).
        print(f"AVISO {d}: front_f + back_f usam {len(order)} cores (máx. 16) - pulado")
        if os.path.exists(d + "normal_f.pal"):
            os.remove(d + "normal_f.pal")
        return None
    # Só regrava o PNG se os índices precisarem mudar (a imagem fica igual).
    new_front = remap(front, order)
    if list(new_front.getdata()) != list(front.getdata()):
        new_front.save(ff)
    if back is not None:
        new_back = remap(back, order)
        if list(new_back.getdata()) != list(back.getdata()):
            new_back.save(bf)
    write_pal(d + "normal_f.pal", order)
    return os.path.exists(d + "shiny_f.pal")

def camel(name):
    return "".join(p.capitalize() for p in name.split("_"))

def main():
    only = set(sys.argv[1:])
    done = {}
    for d in sorted(glob.glob("graphics/pokemon/*/")):
        n = os.path.basename(d.rstrip("/"))
        if only and n not in only:
            if os.path.exists(d + "normal_f.pal"):
                done[n] = os.path.exists(d + "shiny_f.pal")
            continue
        r = process(d)
        if r is not None:
            done[n] = r
    with open("src/data/pokemon_graphics/female_palettes.h", "w") as f:
        f.write("// Gerado por tools/female_palettes.py - não edite à mão.\n")
        f.write("// Paletas das sprites femininas. Espécie sem entrada usa a paleta normal.\n\n")
        f.write("#ifdef FOL_FEMALE_PAL_DATA\n")
        for n, shiny in sorted(done.items()):
            f.write(f'const u32 gMonPalette_{camel(n)}F[] = INCGFX_U32("graphics/pokemon/{n}/normal_f.pal", ".gbapal.lz");\n')
            if shiny:
                f.write(f'const u32 gMonShinyPalette_{camel(n)}F[] = INCGFX_U32("graphics/pokemon/{n}/shiny_f.pal", ".gbapal.lz");\n')
        f.write("#endif\n\n#ifdef FOL_FEMALE_PAL_TABLE\n")
        for n, shiny in sorted(done.items()):
            f.write(f"extern const u32 gMonPalette_{camel(n)}F[];\n")
            if shiny:
                f.write(f"extern const u32 gMonShinyPalette_{camel(n)}F[];\n")
        f.write("\n")
        f.write("const struct CompressedSpritePalette gMonPaletteTableFemale[NUM_SPECIES] =\n{\n")
        for n in sorted(done):
            f.write(f"    [SPECIES_{n.upper()}] = {{gMonPalette_{camel(n)}F, SPECIES_{n.upper()}}},\n")
        f.write("};\n\nconst struct CompressedSpritePalette gMonShinyPaletteTableFemale[NUM_SPECIES] =\n{\n")
        for n, shiny in sorted(done.items()):
            if shiny:
                f.write(f"    [SPECIES_{n.upper()}] = {{gMonShinyPalette_{camel(n)}F, SPECIES_{n.upper()} + SPECIES_SHINY_TAG}},\n")
        f.write("};\n#endif\n")
    print("paletas femininas:", len(done), "(com shiny_f:", sum(done.values()), ")")

if __name__ == "__main__":
    main()
