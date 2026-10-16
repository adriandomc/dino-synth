# Validación DINO SYNTH 0.1-dev

Compilación de desarrollo del 16 de octubre de 2026. **Sin instalar ni validar en un FM-1 físico.**

## Fuente y entorno

- Base: Sloop v2.7, `ba54ba5f0e7f6e0a1a3499497809e1e6c1ad7e8c`.
- Se incorpora de v2.7.1 la corrección del timeout del segundo núcleo en `fm1_cpu1.h` y `fm1_sys.h`: evita el acceso a una dirección de bus inválida en el reinicio del núcleo.
- Ubuntu en WSL2, JieLi `20260730.1`, SDK `AC79NN_SDK_V1.2.1_2023-12-13`. El wrapper verifica clang y los tres archivos SDK mediante SHA-256.

## Resultados

| Comprobación | Resultado |
| --- | --- |
| Compilación pi32v2 y validaciones de registros/memoria | PASS; cero advertencias |
| Imagen / paquete | 522,372 B / 610,035 B |
| RAM `.data + .bss` | 53,960 / 98,304 B; libres 44,344 B |
| Lowram / pool | 22,772 / 29,488 B y 330,084 / 344,064 B |
| Margen XIP | 59,192 B |
| 153 patches de los doce motores | Render real en A/B; defaults secos |
| MOD/BEND cero frente al renderer de voz upstream | Coincidencia bit a bit en los 153 patches |
| Render con modulación, uno/dos núcleos y retrasos simulados | Coincidencia bit a bit |
| Vibrato | Medido 4.9992 Hz, −50.000 a +49.657 cents; retorno exacto a cero |
| Notas USB/TRS/locales, pedal, cambios de foco/split/preset/canal | PASS; destino y canal originales de note-off |
| SAVE, USER, FM6, PERFORMANCE y reinicio | PASS; cancelación, silencio, corrupción, escritura interrumpida y error de flash simulados |
| UI y calibración | PASS; lista SEL real, macros DRUMS y fuzz de 20,000 frames |
| Visuales | 128 notas × tres estilos × dos cargas; límites, determinismo y wrap del reloj |
| Aleatoriedad visual / audio | Hash de audio NOISE inalterado al dibujar |
| Memoria host | ASan/UBSan; exclusiones de overflow firmado y shift-base heredadas del DSP upstream |
| Editor y paquete web | Protocolo, operaciones UI, backups, librería, instalador y sitio offline comprobados |

Las capturas y animaciones proceden del renderer C del firmware. Las mediciones de render en host excluyen SPI, interrupciones y contención del FM-1; no predicen su carga real.

## Diferencias en las pruebas de la base

Se compiló y ejecutó la base v2.7 por separado. Sus **182 golden renders** coinciden, sin fallos de salud, voces/routing, CPU host ni crashes. El comparador estático de instrucciones devuelve dos fallos tanto en la base intacta como en DINO, con los mismos costes:

- `drums_mix`: 6,793 frente al presupuesto 6,153 (aproximadamente +10.4%).
- `fm1_alnk0_irq`: 268 frente a 174 (aproximadamente +54%).

Los presupuestos originales se conservan: esta revisión no los eleva para ocultar los fallos. Hace falta contrastar estos costes y el margen temporal con el dispositivo. No se afirma que toda la suite upstream esté verde.

Los seis modos de golden render del reparto entre núcleos pasan, también con timeout y retrasos simulados. Los cuatro modos de `mix_split` pasan y ThreadSanitizer no detecta carreras. Se detuvo la repetición costosa de Callgrind dentro de `cpu1_modes` y se repitieron sus modos restantes sin ese contador; el presupuesto CPU ya se había comprobado en la primera regresión.

## Prueba física pendiente

Ver [DINO.md](DINO.md#prueba-pendiente-en-el-fm-1): salida analógica al DAW y audio USB, cada visual, capas exigentes, efectos, pedal y cambios de contexto; observar timeouts, clics/pops, latencia, pérdida de voces y high-water de stacks. A/B comparte las ocho voces disponibles. Los objetivos visuales de 10/5 cuadros por segundo son provisionales hasta esa medición.
