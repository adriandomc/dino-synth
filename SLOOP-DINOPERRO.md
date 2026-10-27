# SLOOP 2.8.1 + Dinoperro

Esta versión parte del tag oficial `v2.8.1` (`d89364253b79a5c6002c0d90c5f16b3c3c16ea9d`). Conserva el comportamiento musical y el editor de SLOOP 2.8.1, con los mismos cuatro estilos adicionales de visualización de Dinoperro.

## Controles

El [manual original](GUIDE.md) describe los controles completos. En particular:

- HOME abre TRACKS; otro toque desde TRACKS abre el visualizador. Mantener HOME abre el menú.
- SELECT cambia el estilo mientras está abierto el visualizador. HOME o un botón de página lo cierra.
- PRESETS cambia el sonido de la pista seleccionada; SEL conserva las funciones de escalas y acordes de SLOOP.
- PLAY inicia o detiene la reproducción y REC conserva la grabación de SLOOP.
- SAVE conserva las páginas y combinaciones originales de canción, secciones y presets. Ya no abre PATCH/PERFORMANCE de DINO SYNTH.
- Los knobs, las capas mantenidas y los ajustes de efectos tienen sus asignaciones originales. Los patches de fábrica recuperan los efectos originales.

Los cuatro estilos nuevos son **13 COSMIC DOG**, **14 HALO**, **15 NEBULA** y **16 CONWAY DREAM**. Reaccionan al audio y a las notas de las cuatro pistas. El dinoperro conserva cuerpo verde, hocico amarillo y sombrero violeta. CONWAY DREAM añade el fondo celular de colores, con estelas y actividad reactiva.

La visualización usa el nivel previo a MASTER, como en SLOOP: bajar MASTER no reduce su movimiento. El dibujo se realiza en el bucle principal con geometría y cantidades limitadas; los nuevos estilos apuntan a 10 cuadros/s y bajan a 5 bajo carga. El rendimiento real depende del motor, las voces y los efectos activos.

## Cambios de SLOOP 2.8.1

Incluye el editor de notas de las pistas de sintetizador (SEQ → NOTES), la ficha por paso al mantener OCT+ en NOTES y DRUMS, resonancia de filtro, filtro FX en la batería, TUNE para percusiones de ruido, fade ajustable de punch-in y MIDI Bank Select. En 2.8.1, la ficha del paso queda al pie de la pantalla y recuerda la última fila usada. Las correcciones de USB RESCUE, detección del cargador y reloj MIDI proceden de upstream. Consulta los detalles en [el README original](SLOOP-README.md#whats-new-in-28).

## Compilación y pruebas

Los requisitos y la versión del SDK están en [BUILDING.md](BUILDING.md). En Windows, usa Ubuntu WSL2 con el toolchain JieLi instalado y el SDK en `build/deps/ac79`:

```powershell
.\build-dinoperro.ps1 -Test
python tools/package_dinoperro.py
```

En Linux o WSL2 se puede compilar con `./build.sh` y ejecutar `sh tests/run_dinoperro_tests.sh`. La suite original permanece en `tests/run_tests.sh`. El empaquetador copia `build/felucca.fwsc` como `sloop-2.8.1-dinoperro.fwsc` y genera el sitio local en `build/sloop-2.8.1-dinoperro/`.

Los renders y las animaciones sirven para revisar la imagen sin cargar otro firmware. Para comprobar estabilidad física, compara cada estilo nuevo con visuales apagados, usando motores exigentes, voces y efectos, y graba la salida analógica o USB. Comprueba respuesta de controles, notas perdidas, cortes y ruido.

La compilación conserva el logo de arranque original y comprueba los límites originales, sin ampliarlos: imagen **571,524 / 581,564 B** (10,040 B libres); RAM `.data + .bss`: **59,004 / 98,304 B**; lowram: **22,772 / 29,488 B**; pool: **335,068 / 344,064 B**, con **8,996 B** libres frente al mínimo de 8,192 B de upstream. Los contornos del personaje usan coordenadas relativas de un byte para ahorrar flash sin cambiar su geometría.

Las pruebas comprueban los controles originales, grabación, secuenciador, arpegiador, canciones, SAVE, bancos y 20,000 frames de interfaz. Los visuales se prueban con el renderer real y ASan/UBSan. El audio durante uso prolongado del dispositivo sigue pendiente de comprobación.

La base oficial intacta también muestra dos alertas de presupuesto estático (`drums_mix`: 6,811 frente a 6,153; `fm1_alnk0_irq`: 268 frente a 174). Los presupuestos se conservan; estas alertas se documentan por separado de las pruebas funcionales.

## Instalación

1. Conecta el FM-1 por USB y abre Chrome o Edge.
2. Desde la carpeta del repositorio, ejecuta:

   ```powershell
   python -m http.server 8766 --bind 127.0.0.1 --directory build/sloop-2.8.1-dinoperro
   ```

3. Abre [el instalador local](http://localhost:8766/webapp/installer/), conecta el FM-1 y comprueba que el paquete indicado es **2.8.1-dinoperro** antes de instalar.
4. Espera la confirmación y el reinicio. No desconectes alimentación o USB durante la escritura.

También se puede instalar por línea de comandos con el instalador original:

```powershell
python tools/fm1_install.py build/felucca.fwsc
python tools/fm1_install.py --info
```

La consulta de información del dispositivo devuelve **FELUCCA SLOOP 2.8.1+DINO**. El editor incluido es el original de SLOOP 2.8.1, con sus sonidos, canciones, samples, FM6 y backups. Las memorias PERFORMANCE de DINO SYNTH no son canciones de SLOOP y este firmware no las convierte. El historial del repositorio conserva el firmware DINO anterior.

## Recuperación

Si se interrumpe la actualización, vuelve a abrir el instalador local y repite INSTALL: el FM-1 espera en modo de actualización. Si SLOOP no arranca, mantén **OCT−** al encender para abrir **SLOOP USB RESCUE** e instala de nuevo. Si el FM-1 no arranca ni responde al instalador, sigue el procedimiento de [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) que enlaza el manual original. No cambies calibración ni bancos para solucionar un fallo del instalador. Para regresar al firmware oficial de M-VAVE, utiliza **M-UPGRADE**; SLOOP 2.8.1 retiró esa opción del instalador web.

Los archivos anteriores de DINO SYNTH no son esta distribución. Para esta versión usa `sloop-2.8.1-dinoperro.fwsc` y el editor que lo acompaña.
