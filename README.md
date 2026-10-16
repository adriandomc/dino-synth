# SLOOP 2.7 + Dinoperro para FM-1

[SLOOP v2.7](https://github.com/isod89/sloop-fm1/releases/tag/v2.7), commit `ba54ba5f0e7f6e0a1a3499497809e1e6c1ad7e8c`, con cuatro visualizadores adicionales del dinoperro cósmico.

El instrumento vuelve al funcionamiento original de SLOOP 2.7: sus doce motores, cuatro pistas, secuenciador, grabación, canciones, arpegiador, efectos, controles, presets y editor web. HOME, SEL, SAVE, REC y los knobs conservan sus funciones de SLOOP. Los cambios de interpretación, A/B y guardado de DINO SYNTH se han retirado.

Los doce visualizadores originales permanecen. Se añaden:

| Número | Visualizador | Imagen |
| --- | --- | --- |
| 13 | COSMIC DOG | Dinoperro, estrellas y movimiento con las notas y el nivel. |
| 14 | HALO | Ramas y simetrías procedurales alrededor del personaje. |
| 15 | NEBULA | Partículas y trayectorias reactivas. |
| 16 | CONWAY DREAM | Fondo de autómata celular con colores y estelas. |

Desde TRACKS, pulsa **HOME** para abrir el visualizador y gira **SELECT** para elegir estilo. HOME vuelve a TRACKS. El último estilo se conserva como en SLOOP.

[Guía de esta versión](SLOOP-DINOPERRO.md) · [Manual original de SLOOP](GUIDE.md) · [Firmware](docs/firmware/sloop-2.7-dinoperro.fwsc) · [Editor original offline](docs/webapp/editor/sloop-editor.html)

![Dinoperro con COSMIC DOG, HALO y NEBULA](assets/dino/cosmic-preview.png)

![CONWAY DREAM](assets/dino/conway-preview.png)

## Compilar

Prepara Ubuntu en WSL2, el toolchain JieLi y los tres archivos del SDK indicados en [BUILDING.md](BUILDING.md). Desde PowerShell:

```powershell
.\build-dinoperro.ps1 -Test
```

El firmware queda en `build/felucca.fwsc`. `python tools/package_dinoperro.py` reúne el firmware, el instalador, el editor original, las previews y las guías en `build/sloop-2.7-dinoperro.zip`.

Las capturas proceden del renderer del firmware. Las pruebas de host no sustituyen la comprobación de audio y rendimiento en el FM-1 físico.

## Créditos y licencia

Basado en [SLOOP](https://github.com/isod89/sloop-fm1) y [Felucca](https://github.com/hugelton/Felucca), de Leo Kuroshita / Hügelton Instruments, sus colaboradores y las bibliotecas citadas en el código. El README original permanece en [SLOOP-README.md](SLOOP-README.md).

Se conservan historial, créditos y [GPL-3.0-only](LICENSE). Las condiciones de assets y material de terceros están en [LICENSING.md](LICENSING.md).
