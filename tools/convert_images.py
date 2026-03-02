#!/usr/bin/env python3
"""Convert JPEG images to RGB565 PROGMEM C header arrays for TFT_eSPI."""

import sys
import os

try:
    from PIL import Image
except ImportError:
    print("Pillow not found. Installing...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
    from PIL import Image


def rgb_to_565(r, g, b):
    """Convert 8-bit RGB to 16-bit BGR565 for ST7735 GREENTAB (MADCTL BGR order)."""
    return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3)


def convert_image(input_path, output_path, array_name):
    """Convert a JPEG image to a PROGMEM uint16_t C array header file."""
    img = Image.open(input_path).convert("RGB")
    w, h = img.size
    pixels = list(img.getdata())

    print(f"  Image: {w}x{h}, {len(pixels)} pixels")

    with open(output_path, "w") as f:
        f.write(f"// Auto-generated from {os.path.basename(input_path)}\n")
        f.write(f"// Image size: {w}x{h} pixels, RGB565 format\n")
        f.write(f"#ifndef {array_name.upper()}_H\n")
        f.write(f"#define {array_name.upper()}_H\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define {array_name.upper()}_WIDTH  {w}\n")
        f.write(f"#define {array_name.upper()}_HEIGHT {h}\n\n")
        f.write(f"const uint16_t {array_name}[{w * h}] PROGMEM = {{\n")

        for i, (r, g, b) in enumerate(pixels):
            val = rgb_to_565(r, g, b)
            if i % 16 == 0:
                f.write("  ")
            f.write(f"0x{val:04X}")
            if i < len(pixels) - 1:
                f.write(",")
            if i % 16 == 15 or i == len(pixels) - 1:
                f.write("\n")

        f.write("};\n\n")
        f.write(f"#endif // {array_name.upper()}_H\n")

    file_size = os.path.getsize(output_path)
    print(f"  Output: {output_path} ({file_size:,} bytes)")


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    resources_dir = os.path.join(project_dir, "resources")
    src_dir = os.path.join(project_dir, "src")

    images = [
        ("Montykona128x160.jpeg", "MontykonaImg.h", "montykonaImg"),
        ("Holistica128x160.jpeg", "HolisticaImg.h", "holisticaImg"),
        ("logo 128x160.jpg", "LogoImg.h", "logoImg"),
    ]

    for filename, header, array_name in images:
        input_path = os.path.join(resources_dir, filename)
        output_path = os.path.join(src_dir, header)
        print(f"Converting {filename}...")
        convert_image(input_path, output_path, array_name)

    print("\nDone! Header files generated in src/")


if __name__ == "__main__":
    main()
