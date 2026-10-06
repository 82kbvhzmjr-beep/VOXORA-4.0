# Voxora

Cadena vocal para FL Studio en Mac (VST3 y AU, Intel y Apple Silicon). Hecho con JUCE.

## Qué incluye

- **Input / Output** con medidores y ganancia.
- **EQ**: corte de graves, graves, medios y agudos.
- **Dynamics**: compresor (Smooth / Punchy / Aggressive) con medidor de reducción, y de-esser con frecuencia ajustable.
- **Tone**: saturación (Warm / Agudo / Saturación) y un control "Magic" que añade aire y armónicos.
- **Air**: dos bandas (Mid Air ~5 kHz y High Air ~12 kHz) que generan armónicos nuevos y añaden brillo, para dar aire y presencia a la voz. Es un diseño propio con el mismo objetivo que los excitadores de aire habituales, no una copia de ninguno.
- **Space**: reverb (Plate / Room / Large) y delay sincronizado al tempo (1/16, 1/8, 1/4), los dos con corte de graves y agudos.
- **Tune**: corrección de tono en tiempo real con tonalidad, escala (Chromatic, Major, Minor, Pentatonic Minor), velocidad, cantidad y tolerancia. Muestra la nota detectada.
- **Vocal Assist**: escucha unos 6 segundos de voz y ajusta ganancia, compresión, de-esser y EQ. Se puede comparar Original / Asistida.
- Tres vistas: **Assist**, **Simple** (4 macros) y **Advanced**.
- Categorías **General** y **Cantantes**.
- Presets vocales: Quevedo, Feid / Ferxxo, Lucho RK, Cruzzi, Roa, Omar Courtz y Xiyo & Fernandezz.
- Emulación tonal de micrófono con perfiles vocales y control de proximidad.
- Armonías de octava, ancho estéreo, micro-modulación, bitcrush, megáfono y glitch sincronizado.

## Compilar en tu Mac

1. Instala las herramientas de Xcode (si no las tienes): `xcode-select --install`
2. Instala CMake: `brew install cmake`
3. En la carpeta del proyecto:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j8
```

La primera vez descarga JUCE (hace falta internet) y tarda unos minutos.
Al terminar, los plugins se copian solos a:

- `~/Library/Audio/Plug-Ins/VST3/Voxora.vst3`
- `~/Library/Audio/Plug-Ins/Components/Voxora.component`

Si macOS se queja de que el plugin no está firmado:

```bash
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/VST3/Voxora.vst3
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/Components/Voxora.component
```

## Usarlo en FL Studio

1. `Options > Manage plugins` y pulsa **Find installed plugins** (o activa "Verify plugins").
2. Abre un canal de mezcla, elige un slot de efecto y busca **Voxora** en la lista.

## Notas honestas

- El código no se ha podido probar dentro de FL Studio. Lo que sí está comprobado: el núcleo del autotune (detección y corrección de tono) con un test de señal, y que los archivos pasan el análisis del compilador contra las cabeceras de JUCE. Si al compilar en el Mac sale algún error, pásamelo y lo corrijo.
- El plugin informa de **unos 15 ms de latencia** (es el cambiador de tono, que va siempre en la ruta para que la pista no se desalinee). FL Studio la compensa en la mezcla. Para grabar escuchándote en directo puede notarse.
- La corrección de tono usa detección YIN, puerta de confianza, suavizado de retune y un cambiador de tono de dominio temporal. Está orientada a voces monofónicas y a baja latencia; no se debe presentar como idéntica a un motor propietario de terceros sin hacer una prueba A/B en FL Studio.
- Los presets de fábrica incluyen ahora una categoría **Cantantes** y el estado se guarda con el proyecto de FL Studio.
