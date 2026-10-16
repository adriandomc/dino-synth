# SLOOP 2.7 + Dinoperro

Esta versión parte del tag oficial `v2.7` (`ba54ba5f0e7f6e0a1a3499497809e1e6c1ad7e8c`). El comportamiento musical y el editor vuelven a SLOOP 2.7, con cuatro estilos adicionales de visualización.

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

## Compilación y pruebas

Los requisitos y la versión del SDK están en [BUILDING.md](BUILDING.md). En Windows, usa Ubuntu WSL2 con el toolchain JieLi instalado y el SDK en `build/deps/ac79`:

```powershell
.\build-dinoperro.ps1 -Test
python tools/package_dinoperro.py
```

En Linux o WSL2 se puede compilar con `./build.sh` y ejecutar `sh tests/run_dinoperro_tests.sh`. La suite original permanece en `tests/run_tests.sh`. El empaquetador copia `build/felucca.fwsc` como `sloop-2.7-dinoperro.fwsc` y genera el sitio local en `build/sloop-2.7-dinoperro/`.

Los renders y las animaciones sirven para revisar la imagen sin cargar otro firmware. Para comprobar estabilidad física, compara cada estilo nuevo con visuales apagados, usando motores exigentes, voces y efectos, y graba la salida analógica o USB. Comprueba respuesta de controles, notas perdidas, cortes y ruido.

La compilación conserva el logo de arranque original. Los contornos del personaje usan coordenadas relativas de un byte para ahorrar flash sin cambiar su geometría. Imagen: **581,368 / 581,564 B**; RAM `.data + .bss`: **58,908 / 98,304 B**; lowram: **22,772 / 29,488 B**; pool: **335,068 / 344,064 B**, con **8,996 B** libres frente al mínimo de 8,192 B exigido por upstream. No se han ampliado los límites del dispositivo.

Las pruebas comprueban los controles originales, grabación, secuenciador, arpegiador, canciones, SAVE, bancos y 20,000 frames de interfaz. Los visuales se prueban con el renderer real y ASan/UBSan. Los presupuestos estáticos originales siguen mostrando dos alertas ya presentes en la base intacta (`drums_mix` 6,793 frente a 6,153 y `fm1_alnk0_irq` 268 frente a 174); no se han elevado esos presupuestos. El audio durante uso prolongado del dispositivo sigue pendiente de comprobación.

## Instalación

1. Conecta el FM-1 por USB y abre Chrome o Edge.
2. Desde la carpeta del repositorio, ejecuta:

   ```powershell
   python -m http.server 8766 --bind 127.0.0.1 --directory build/sloop-2.7-dinoperro
   ```

3. Abre [el instalador local](http://localhost:8766/webapp/installer/), conecta el FM-1 y comprueba que el paquete indicado es **2.7-dinoperro** antes de instalar.
4. Espera la confirmación y el reinicio. No desconectes alimentación o USB durante la escritura.

También se puede instalar por línea de comandos con el instalador original:

```powershell
python tools/fm1_install.py build/felucca.fwsc
python tools/fm1_install.py --info
```

El dispositivo se identifica como **SLOOP 2.7+DINO**. El editor incluido es el original de SLOOP 2.7, con sus sonidos, canciones, samples, FM6 y backups. Las memorias PERFORMANCE de DINO SYNTH no son canciones de SLOOP y este firmware no las convierte. El historial del repositorio conserva el firmware DINO anterior.

## Recuperación

Si la actualización falla, vuelve a abrir el instalador local y sigue su modo de recuperación. Si el FM-1 no arranca ni responde al instalador, sigue el procedimiento de [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) que enlaza el manual original. No cambies calibración ni bancos para solucionar un fallo del instalador.

Los archivos anteriores de DINO SYNTH no son esta distribución. Para esta versión usa `sloop-2.7-dinoperro.fwsc` y el editor que lo acompaña.
