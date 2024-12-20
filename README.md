# UTF-8 Keyboard Emulator with Layout Switching

This project implements a UTF-8 keyboard emulator for Arduino-based systems. It features dynamic layout switching between Russian and Latin layouts depending on the input characters. At the end of the input, the keyboard always returns to the default Latin layout.

## Features

- **UTF-8 Support**: Handles both single-byte (ASCII) and multi-byte (UTF-8) characters.
- **Dynamic Layout Switching**: Automatically toggles between Russian and Latin layouts as needed.
- **Compact Design**: Optimized code structure for better readability and maintainability.
- **BOM Handling**: Skips UTF-8 BOM (`0xEF 0xBB 0xBF`) if present at the beginning of the input.
- **Keyboard Control**: Utilizes `bleKeyboard` to send keystrokes.

## Project Structure

### Functions

1. **`processUTF8Line(const String& line)`**
   - Main function to process a UTF-8 encoded line.
   - Handles BOM detection, character parsing, and layout switching.

2. **`switchLayout(bool toRussian)`**
   - Switches the keyboard layout.
   - If `toRussian` is `true`, switches to Russian layout; otherwise, switches back to Latin.

3. **`sendChar(uint16_t c)`**
   - Sends a character to the keyboard.
   - Handles mapping between Russian and Latin characters.

### Character Mapping

The project includes mapping arrays:
- `russianToEnglishUpper` for uppercase Russian letters.
- `russianToEnglishLower` for lowercase Russian letters.

These arrays map Cyrillic letters to their corresponding Latin keyboard keys based on the current layout.

## Usage

### Prerequisites

1. **Hardware**: An Arduino-compatible board with BLE support (e.g., ESP32).
2. **Libraries**: Install the `BleKeyboard` library for Arduino.

### How to Use

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/utf8-keyboard-emulator.git
   ```
2. Open the project in Arduino IDE.
3. Upload the sketch to your Arduino-compatible board.
4. Send UTF-8 encoded strings to the emulator via Serial Monitor or any input device.
5. Observe the automatic layout switching and keystroke output.

### Example Input

```plaintext
Привет, мир! Hello, world!
```

### Example Output

- Automatically switches layouts as needed.
- Sends the corresponding keystrokes to the connected device.

## Contributing

Contributions are welcome! Feel free to submit issues, feature requests, or pull requests to improve the project.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Acknowledgments

- Inspired by the need to handle multilingual keyboard input seamlessly.
- Built using the `BleKeyboard` library for Arduino.

---
h web server on EPS32-S3-Dev kit
