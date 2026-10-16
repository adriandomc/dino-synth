# DINO SYNTH para FM-1

Firmware para tocar el M-VAVE FM-1 como sintetizador y grabar su salida en un DAW. La base es SLOOP **v2.7**, commit `ba54ba5f0e7f6e0a1a3499497809e1e6c1ad7e8c`. El historial, los bancos, las licencias y los créditos de SLOOP/Felucca se conservan.

**Estado: versión de desarrollo, sin validar en un FM-1 físico.** La compilación y las pruebas de host no comprueban la latencia del panel, el SPI de la pantalla, el ruido de la salida analógica ni las interrupciones del hardware real. Antes de una actuación, completa la prueba de hardware descrita abajo.

## Tocar y grabar

Conecta la salida de audio del FM-1 a tu interfaz y arma una pista de audio estéreo en el DAW. MASTER controla el nivel analógico. En el primer arranque DINO, SPEAKER LOWCUT comienza apagado; después puedes cambiarlo en el menú AUDIO.

También se conserva el audio USB estéreo de SLOOP: 16 bits, 44.1 kHz y opción 48 kHz mediante remuestreo. Puede aparecer como **Felucca**. AUDIO → USB AUDIO permite MASTER o FULL; FULL ignora el mando de volumen para la señal USB. Para evitar una segunda interpretación o un bucle MIDI, revisa el monitoreo y el MIDI echo de la pista que uses.

Los motores son ANALOG, DIGITAL, PHASE, LOFI, SAMPLE, VOICE, TRIO, WHEEL, GRAIN, FM6, PHYS y NOISE. Se conservan los 153 patches de fábrica, samples, kits y bancos FM6. Los patches de fábrica cargan con DIST, CHORUS, DELAY, REVERB, INSERT y SLICER apagados; sus niveles calibrados y características propias del motor permanecen.

| Modo | Comportamiento |
| --- | --- |
| SINGLE | Un timbre: A o B según el foco. Arranque inicial: SINGLE/A. |
| LAYER | La misma nota toca A y B. |
| SPLIT | A por debajo del punto de división, B desde ese punto. Inicialmente C4/MIDI 60. |
| DRUMS | Un kit de percusión, separado de los timbres A/B. |

A y B usan internamente las partes 0 y 2 para repartir el trabajo entre núcleos. Comparten el presupuesto de **ocho voces** de SLOOP; FM6 conserva su límite de seis por parte y los demás motores conservan sus protecciones. LAYER consume voces en ambos timbres: dos sonidos no duplican la polifonía disponible. DRUMS conserva el presupuesto propio de la percusión.

## Controles

| Control | Acción |
| --- | --- |
| HOME, toque | Alternar LIVE y el visualizador; desde otra pantalla, volver. |
| HOME, mantener | Abrir/cerrar ajustes del dispositivo. |
| KNOB 1 en LIVE, visualizador o SEL | Vibrato, 0–127; mismo estado que MIDI CC1. |
| ARP + KNOB 2 | Pitch bend; soltar ARP vuelve al centro con el suavizado del motor. |
| KNOB 3/4 en LIVE, visualizador o SEL | Dos primeras macros del timbre enfocado; sus nombres aparecen en pantalla. |
| SEL | Abrir/cerrar la lista de sonidos, con nombre, motor y FACTORY/USER. |
| SELECT / PRESETS dentro de SEL | Recorrer y cargar inmediatamente el sonido resaltado. HOME/SEL cierran conservando ese sonido. |
| SELECT con el visualizador abierto | Cambiar estilo visual. |
| PRESETS | Cambiar el sonido del timbre enfocado; en DRUMS, cambiar kit. |
| ALGORITHM | Cambiar motor del timbre enfocado. |
| REC | Cambiar foco A/B. |
| SEQ | Abrir PERFORMANCE: K1 modo, K2 split, K3 nivel A, K4 nivel B. |
| GLO | K1 tempo para modulaciones/efectos, K2 afinación global, K3 canal MIDI de entrada, K4 rango de bend. |
| ENV / LFO / EDIT / FX | Abrir sus páginas; SELECT recorre páginas y K1–K4 editan los parámetros mostrados. |
| OCT− / OCT+ | Cambiar octava; ambos juntos vuelven a cero. |
| PLAY | Panic: soltar notas y reiniciar sustain, vibrato y bend. |
| SAVE | Abrir el guardado de PATCH/PERFORMANCE. |

En DRUMS, K3 ajusta TUNE y K4 DECAY del último sonido tocado; modulación y bend quedan inactivos. EDIT abre la edición de percusión: K1 sonido, K2 TUNE, K3 DECAY, K4 kit.

El vibrato añadido es sinusoidal, a 5 Hz y hasta ±50 cents. MOD=0 conserva el render original. El bend utiliza el rango MIDI configurable y su suavizado existente. Las páginas de edición toman los knobs de forma contextual; LIVE, SEL y los visualizadores mantienen los controles de interpretación activos.

MIDI USB y TRS conservan propietarios independientes para notas y pedal. La entrada sigue el canal de recepción elegido en GLO y la división/capa del modo actual. Los note-off liberan los destinos de sus note-on aunque cambien el foco o el split. Los mensajes MIDI START/CONTINUE no arrancan patrones.

## SAVE y memoria

1. Toca SAVE. SELECT cambia entre PATCH y PERFORMANCE; PRESETS o K1 eligen el destino.
2. Revisa el nombre existente y la indicación de reemplazo. Mantén SAVE **un segundo** para confirmar. Una canción antigua aparece como `LEGACY SONG`.
3. `WAIT FOR SILENCE` (estado SAVE PENDING) significa que el firmware espera silencio: voces, colas de efectos y salida final. MASTER a cero no evita esa espera. HOME cancela un guardado pendiente.
4. El éxito solo se muestra después del commit de flash. Si hay un error, el destino conserva su copia válida anterior. OCT+ carga el destino seleccionado; una PERFORMANCE exige soltar las notas y esperar silencio.

| Tipo | Capacidad y contenido |
| --- | --- |
| PATCH | 32 slots USER. Motor, parámetros, transposición, nivel, pan, filtro y efectos individuales del timbre enfocado. |
| PERFORMANCE | Cuatro memorias: A/B, modo, split, foco, octava, niveles, efectos compartidos, kit/ajustes de percusión y visualizador. Incluye las voces FM6 efectivas de A/B. |

Las notas activas, sustain, posiciones de vibrato y bend no se guardan. Los samples y kits guardan sus referencias: no se duplica el audio en cada PERFORMANCE. Un patch USER recupera sus efectos guardados; los buses compartidos pertenecen a PERFORMANCE.

Un PATCH FM6 necesita una referencia persistente a su voz. SAVE reutiliza una coincidencia exacta del banco FM6 o escribe un espacio libre B1–B27. Si el banco está lleno, muestra `FM6 BANK FULL`: PRESETS/K1 eligen un banco y mantener SAVE otro segundo confirma **explícitamente** su reemplazo. Los demás patches que referencien ese banco oirán la voz nueva cuando vuelvan a cargarse. HOME cancela. Las voces RAM que estén editadas en el otro timbre permanecen intactas.

Las cuatro PERFORMANCE reutilizan los objetos de flash de las canciones A–D. DINO no reproduce ni sobrescribe automáticamente las canciones anteriores. Una memoria `LEGACY SONG` solo se reemplaza con el guardado explícito sobre ese destino. El autosave DINO usa otra firma: los autosaves SLOOP antiguos no activan secuencias ni ajustes de canción. El autosave requiere silencio, 2.5 s sin tocar el panel y al menos 20 s entre escrituras; al apagar antes, puede perderse la edición aún no guardada. Usa SAVE para conservar cambios importantes.

Las escrituras usan el commit A/B de SLOOP. Escribir flash puede detener el procesamiento durante decenas de milisegundos; por eso DINO espera silencio. Una textura continua o un efecto que siga sonando mantiene el guardado pendiente hasta que lo detengas.

## Visualizadores

Se conservan los doce estilos de SLOOP y se añaden **COSMIC DOG**, **HALO** y **NEBULA**. El dinoperro mantiene cuerpo verde, hocico amarillo y sombrero violeta. Las figuras, simetrías y partículas reaccionan a notas y energía de ambos timbres. COSMIC DOG es el estilo inicial del primer arranque.

El renderer usa el canvas existente, geometría entera, cantidades máximas de elementos y aleatoriedad independiente del audio. Se ejecuta en el bucle principal, interrumpible por el audio, con objetivos de 10 cuadros/s o 5 bajo carga alta. Esos objetivos requieren confirmar el comportamiento físico; las mediciones de host excluyen SPI y preempción real.

Los previews se generan con el renderer C del firmware:

```sh
sh tests/run_dino_tests.sh
```

Resultados en `build/dino-visuals/`: `cosmic-preview.png`, `cosmic-contact-sheet.png` y `cosmic-animation.gif`.
Las capturas de LIVE, SEL, PERFORMANCE, DRUMS y SAVE se generan en `build/dino-ui/` con el renderer real de sus pantallas.

## Editor, backups e instalación

Usa el editor DINO incluido en esta revisión. Permite sonidos, A/B, split, efectos, samples, FM6, nombres y backups. El protocolo DINO es v13, comando SysEx 78; las operaciones antiguas de patrones/grabación/canciones están bloqueadas también en el firmware. Los IDs existentes de presets, samples y bancos se conservan. Los backups incluyen las cuatro memorias PERFORMANCE; también pueden conservar y restaurar objetos de canciones SLOOP como datos, sin reproducirlos.

Antes de actualizar desde SLOOP, conserva un backup completo de SLOOP en tu computadora. Conserva también un backup DINO antes de reemplazar una PERFORMANCE o un banco FM6. Volver a SLOOP requiere su firmware y tu backup SLOOP: el formato DINO no es un formato de canción compatible con SLOOP. No dependas de un backup DINO para recuperar una canción sobrescrita.

## Instalación y recuperación

El sitio listo para usar está en `docs/`, con su firmware incluido. Desde la raíz del repositorio, inicia:

```powershell
py -m http.server 8767 --bind 127.0.0.1 --directory docs
```

Abre `http://127.0.0.1:8767/webapp/installer/` en Chrome/Edge, conecta el FM-1 por USB y pulsa Install. El instalador verifica y usa el paquete DINO incluido; no pide seleccionar un archivo. El editor está en `http://127.0.0.1:8767/webapp/editor/`. Autoriza MIDI/SysEx para localhost cuando el navegador lo solicite. Cierra el DAW u otras aplicaciones si están ocupando el puerto MIDI.

El ZIP de distribución contiene el mismo sitio en su raíz: descomprímelo, abre una terminal en esa carpeta y ejecuta `py -m http.server 8767 --bind 127.0.0.1`. El editor offline descargado puede abrirse como archivo local en Chrome/Edge. La compilación por sí sola **no instala** firmware.

También existe el instalador CLI heredado, ejecutado desde Windows para acceder al MIDI USB de Windows:

```powershell
py -m pip install mido python-rtmidi
py tools/fm1_install.py --info
py tools/fm1_install.py build/felucca.fwsc
```

No desconectes alimentación/USB durante la escritura. Si la aplicación no inicia, mantén **OCT− solo al encender** y busca `SLOOP USB RESCUE`; el nombre de rescate se conserva de upstream. Vuelve a instalar desde el instalador. El rescate depende de que el arranque de la aplicación siga intacto; si falla, usa el procedimiento ROM de [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter). Mantener OCT− y OCT+ al encender abre calibración; mantener ambos cinco segundos con el dispositivo encendido solicita UBOOT. Consulta también [BUILDING.md](BUILDING.md) y los procedimientos originales de [GUIDE.md](GUIDE.md).

## Compilar en Windows/WSL2

La configuración usada es Ubuntu x86-64 en WSL2. Se fijan:

- Fuente SLOOP: tag `v2.7`, commit `ba54ba5f0e7f6e0a1a3499497809e1e6c1ad7e8c`.
- Toolchain JieLi: `jieli-linux-toolchains-20260730.1`, clang 4.0.1 para pi32v2. SHA-256 de `pi32v2/bin/clang`: `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`.
- SDK: `AC79NN_SDK_V1.2.1_2023-12-13`; los tres archivos usados deben coincidir con los SHA-256 de `tools/build.py` y del wrapper DINO.

Preparación inicial:

```powershell
wsl --install -d Ubuntu
wsl -d Ubuntu -u root -- bash -lc "apt-get update && apt-get install -y build-essential python3 python3-pil git curl xz-utils nodejs"
```

Desde Ubuntu, en el directorio del repositorio montado bajo `/mnt/c/`:

```sh
sh tools/get_toolchain.sh /root/.jieli
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
  https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git build/deps/ac79
```

El descargador upstream de toolchain solicita la versión que ofrece su servidor. Para reproducir esta revisión, conserva o instala la carpeta exacta `20260730.1`: el wrapper rechaza un clang o archivos SDK distintos, en vez de usar silenciosamente otra versión. Los binarios de toolchain/SDK y `build/` no se guardan en Git.

En PowerShell, desde el repositorio:

```powershell
.\build-dino.ps1
.\build-dino.ps1 -Test
```

`-Distro` permite elegir el nombre de la distribución Ubuntu ya configurada. El wrapper ejecuta como root en WSL para usar la ubicación fijada del toolchain, compila, valida y genera `build/felucca.fwsc`; no instala paquetes del sistema ni escribe al FM-1. En Linux x86-64, el equivalente es:

```sh
sh tools/build_dino.sh
sh tests/run_dino_tests.sh
```

La suite DINO comprueba routing/propiedad de notas, sustain y controles; UI y patches secos; persistencia, corrupción y fallos de escritura; visualizadores; render de los 153 patches con MOD=0 frente al código de voz upstream y uno/dos núcleos; sanitizers; editor web y tests heredados de almacenamiento/FM6. El registro del compilador y el linker siguen siendo la fuente de verdad para límites de memoria. Los resultados y los dos fallos de presupuesto que también aparecen en la base intacta están en [DINO-VALIDATION.md](DINO-VALIDATION.md).

En la compilación de desarrollo del 16 de octubre de 2026 se midieron `.data + .bss` **53,960 / 98,304 B**, lowram **22,772 / 29,488 B** y pool **330,084 / 344,064 B**: márgenes de 44,344 B, 6,716 B y 13,980 B. Los tamaños cambian con el código; revisa siempre el nuevo resultado del build. El presupuesto de stacks y sus guardas se conserva, pero la profundidad real y el margen temporal deben medirse en hardware.

## Prueba pendiente en el FM-1

Conecta la salida analógica al DAW y compara visuales apagados, estilos originales y los tres estilos nuevos. Usa capas de motores exigentes, notas repetidas, pedal, cambios de foco/split/preset, varios efectos y bend. Comprueba respuesta de los controles, finales de nota, pops, clics, timeouts del segundo núcleo, pérdida de voces dentro del presupuesto y margen de stacks. Prueba SAVE pendiente, cancelación, recuperación tras reiniciar y backups con tus samples/FM6. Repite con audio USB si lo usarás en una actuación.

No se ha realizado esta prueba física en esta revisión. Las pruebas de host y los previews no certifican ausencia de glitches en tu unidad.

## Créditos y licencias

[SLOOP](https://github.com/isod89/sloop-fm1) de isod89, basado en [Felucca](https://github.com/hugelton/Felucca) de Leo Kuroshita / Hügelton Instruments y sus colaboradores. Los créditos completos originales permanecen en [SLOOP-README.md](SLOOP-README.md), [LICENSING.md](LICENSING.md) y los avisos de cada fuente. El código mantiene GPL-3.0-only; las licencias particulares de msfa, modelos físicos, fuentes y assets conservan sus condiciones. El código fuente correspondiente debe acompañar a las distribuciones de firmware según su licencia.
