# SLOOP 2.8.1 + Dinoperro para FM-1

[SLOOP v2.8.1](https://github.com/isod89/sloop-fm1/releases/tag/v2.8.1), commit `d89364253b79a5c6002c0d90c5f16b3c3c16ea9d`, con cuatro visualizadores adicionales del dinoperro cósmico.

Conserva el funcionamiento original de SLOOP 2.8.1: sus doce motores, cuatro pistas, secuenciador, grabación, canciones, arpegiador, efectos, controles, presets y editor web. HOME, SEL, SAVE, REC y los knobs conservan sus funciones de SLOOP.

Los doce visualizadores originales permanecen. Se añaden:

| Número | Visualizador | Imagen |
| --- | --- | --- |
| 13 | COSMIC DOG | Dinoperro, estrellas y movimiento con las notas y el nivel. |
| 14 | HALO | Ramas y simetrías procedurales alrededor del personaje. |
| 15 | NEBULA | Partículas y trayectorias reactivas. |
| 16 | CONWAY DREAM | Fondo de autómata celular con colores y estelas. |

Desde TRACKS, pulsa **HOME** para abrir el visualizador y gira **SELECT** para elegir estilo. HOME vuelve a TRACKS. El último estilo se conserva como en SLOOP.

[Guía de esta versión](SLOOP-DINOPERRO.md) · [Manual original de SLOOP](GUIDE.md) · [Firmware](docs/firmware/sloop-2.8.1-dinoperro.fwsc) · [Editor original offline](docs/webapp/editor/sloop-editor.html)

![Dinoperro con COSMIC DOG, HALO y NEBULA](assets/dino/cosmic-preview.png)

![CONWAY DREAM](assets/dino/conway-preview.png)

## Compilar

Prepara Ubuntu en WSL2, el toolchain JieLi y los tres archivos del SDK indicados en [BUILDING.md](BUILDING.md). Desde PowerShell:

```powershell
.\build-dinoperro.ps1 -Test
```

El firmware queda en `build/felucca.fwsc`. `python tools/package_dinoperro.py` reúne el firmware, el instalador, el editor original, las previews y las guías en `build/sloop-2.8.1-dinoperro.zip`.

Las capturas proceden del renderer del firmware. Las pruebas de host no sustituyen la comprobación de audio y rendimiento en el FM-1 físico.

## Créditos y licencia

Basado en [SLOOP](https://github.com/isod89/sloop-fm1) y [Felucca](https://github.com/hugelton/Felucca), de Leo Kuroshita / Hügelton Instruments, sus colaboradores y las bibliotecas citadas en el código. El README original permanece en [SLOOP-README.md](SLOOP-README.md).

Se conservan historial, créditos y [GPL-3.0-only](LICENSE). Las condiciones de assets y material de terceros están en [LICENSING.md](LICENSING.md).
