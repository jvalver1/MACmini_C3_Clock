import os
from PIL import Image, ImageDraw, ImageFont

H = 44
chars = "0123456789:"

out_c = "#pragma once\n#include <stdint.h>\n"
out_c += f"const int TALL_DIGIT_H = {H};\n"

widths = []
bitmaps = []

for c in chars:
    img = Image.new("1", (40, H*2))
    draw = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype("C:\\Windows\\Fonts\\impact.ttf", H)
    except:
        font = ImageFont.load_default()
        
    draw.text((0, 0), c, fill=1, font=font)
    bbox = img.getbbox()
    if not bbox:
        bbox = (0, 0, 10, H)
    
    img_cropped = img.crop(bbox)
    
    if c == ':':
        W = 6 # or 7
        H_colon = int(H * 0.6)
        img_small = img_cropped.resize((W, H_colon), Image.LANCZOS).convert("1")
        img_stretched = Image.new("1", (W, H))
        img_stretched.paste(img_small, (0, (H - H_colon) // 2))
    else:
        W = 15
        img_stretched = img_cropped.resize((W, H), Image.LANCZOS).convert("1")
    widths.append(W)
    
    bytes_per_row = (W + 7) // 8
    b_list = []
    for y in range(H):
        for bx in range(bytes_per_row):
            b = 0
            for it in range(8):
                x = bx * 8 + it
                if x < W and img_stretched.getpixel((x, y)):
                    b |= (1 << (7 - it))
            b_list.append(b)
    bitmaps.append(b_list)

out_c += "const uint8_t tall_digit_widths[11] = {" + ", ".join(map(str, widths)) + "};\n"

offsets = [0]
for idx, b_list in enumerate(bitmaps):
    offsets.append(offsets[-1] + len(b_list))

out_c += "const uint16_t tall_digit_offsets[11] = {" + ", ".join(map(str, offsets[:-1])) + "};\n"

out_c += "const uint8_t tall_digits[" + str(offsets[-1]) + "] = {\n  "
lines = []
for b_list in bitmaps:
    lines.append(", ".join([f"0x{b:02X}" for b in b_list]))
out_c += ",\n  ".join(lines)
out_c += "\n};\n"

with open("src/TallFont.h", "w") as f:
    f.write(out_c)
print("TallFont.h generated")

