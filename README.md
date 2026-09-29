# Dokumentacija

Į šį katalogą įkelkite vizualinę medžiagą, kurios reikia namų darbo ataskaitai.

| Failas | Kas tai |
|--------|---------|
| `schema.png` | Tinkercad schemos vaizdas (mygtukas **Schematic View** grandinės lange) |
| `grandine.png` | Maketavimo plokštės vaizdas su visais komponentais |
| `lcd.png` | LCD ekrano nuotrauka veikimo metu |
| `demo.md` | Nuoroda į 1–2 min demonstracinį vaizdo įrašą |

Įkėlę failus, juos galima įterpti į pagrindinį `README.md`:

```markdown
![Schema](docs/schema.png)
```

## Demonstracinio įrašo planas

1. **Paleidimas** — matosi pasisveikinimo užrašas LCD ekrane, servo pradeda suktis.
2. **SAFE zona** — kliūtis toli, LED žalias, tyla.
3. **Artėjimas** — kliūtis tempiama arčiau, matosi perėjimas per geltoną ir oranžinę spalvas, girdisi dažnėjantys pyptelėjimai.
4. **STOP! zona** — LED raudonas, greiti pyptelėjimai, ekrane matosi atstumas ir kampas.
5. **Režimo perjungimas** — mygtuko paspaudimas, servo sustoja, ekrane pasikeičia į `PARK`.
6. **Jautrumo reguliavimas** — potenciometro sukimas keičia, kuriuo atstumu suveikia įspėjimas.
7. **Serial Monitor** — trumpai parodomos `kampas,atstumas` poros.
