#!/usr/bin/env python3
"""Generate Arduboy2 plus-mask sprites for Big Pig Bash."""

from pathlib import Path


class Canvas:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = [[None for _ in range(width)] for _ in range(height)]

    def pixel(self, x, y, color=1):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = color

    def rect(self, x, y, width, height, color=1):
        for py in range(y, y + height):
            for px in range(x, x + width):
                self.pixel(px, py, color)

    def ellipse(self, cx, cy, rx, ry, color=1):
        scale = rx * rx * ry * ry
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                dx = x - cx
                dy = y - cy
                if dx * dx * ry * ry + dy * dy * rx * rx <= scale:
                    self.pixel(x, y, color)

    def line(self, x0, y0, x1, y1, color=1):
        dx = abs(x1 - x0)
        sx = 1 if x0 < x1 else -1
        dy = -abs(y1 - y0)
        sy = 1 if y0 < y1 else -1
        error = dx + dy
        while True:
            self.pixel(x0, y0, color)
            if x0 == x1 and y0 == y1:
                return
            twice = error * 2
            if twice >= dy:
                error += dy
                x0 += sx
            if twice <= dx:
                error += dx
                y0 += sy

    def mirror(self):
        result = Canvas(self.width, self.height)
        for y, row in enumerate(self.pixels):
            result.pixels[y] = list(reversed(row))
        return result

    def flip_vertical(self):
        result = Canvas(self.width, self.height)
        result.pixels = list(reversed([row[:] for row in self.pixels]))
        return result


def pig(step=0):
    c = Canvas(16, 16)
    c.ellipse(7, 9, 6, 4)
    c.ellipse(11, 7, 4, 4)
    c.line(8, 4, 10, 1)
    c.line(10, 1, 12, 4)
    c.rect(11, 8, 5, 4)
    c.rect(12, 9, 1, 1, 0)
    c.rect(15, 9, 1, 1, 0)
    c.rect(12, 5, 1, 2, 0)
    c.line(2, 8, 0, 6)
    c.pixel(0, 7)
    c.pixel(1, 6, 0)
    c.rect(4, 12, 2, 3 if step else 2)
    c.rect(10, 12, 2, 2 if step else 3)
    c.pixel(5, 15, 0)
    c.pixel(11, 15, 0)
    c.pixel(8, 10, 0)
    return c


def crow(frame=0):
    c = Canvas(16, 16)
    c.ellipse(8, 8, 5, 5)
    c.ellipse(8, 8, 3, 3, 0)
    c.ellipse(11, 6, 3, 3)
    c.pixel(12, 5, 0)
    c.line(14, 6, 15, 7)
    c.line(14, 7, 15, 7)
    if frame:
        c.line(7, 8, 2, 2)
        c.line(7, 9, 1, 5)
        c.line(8, 9, 13, 14)
    else:
        c.line(7, 8, 1, 10)
        c.line(7, 9, 2, 13)
        c.line(9, 9, 14, 11)
    c.line(6, 13, 5, 15)
    c.line(10, 13, 11, 15)
    return c


def mole(frame=0):
    c = Canvas(16, 16)
    c.line(1, 13, 14, 13)
    c.line(3, 14, 12, 14)
    c.ellipse(8, 9, 5, 5)
    c.ellipse(11, 9, 4, 3)
    c.rect(12, 9, 3, 2)
    c.pixel(13, 9, 0)
    c.pixel(10, 6, 0)
    c.rect(11, 11, 1, 3, 0)
    c.rect(13, 11, 1, 3, 0)
    if frame:
        c.line(4, 10, 0, 7)
        c.line(5, 11, 1, 12)
    else:
        c.line(4, 10, 0, 12)
        c.line(5, 11, 2, 7)
    return c


def bee(frame=0):
    c = Canvas(16, 16)
    if frame:
        c.ellipse(5, 4, 3, 4)
        c.ellipse(10, 4, 3, 4)
    else:
        c.ellipse(4, 7, 4, 3)
        c.ellipse(11, 7, 4, 3)
    c.ellipse(8, 9, 5, 4)
    c.rect(5, 6, 2, 7, 0)
    c.rect(9, 6, 2, 7, 0)
    c.rect(6, 5, 4, 2)
    c.pixel(7, 6, 0)
    c.pixel(10, 6, 0)
    c.line(13, 9, 15, 9)
    return c


def crow_king(frame=0):
    c = Canvas(24, 24)
    c.ellipse(12, 14, 8, 7)
    c.ellipse(12, 14, 6, 5, 0)
    c.ellipse(16, 10, 5, 5)
    c.pixel(17, 9, 0)
    c.rect(20, 10, 4, 2)
    c.line(12, 6, 10, 1)
    c.line(10, 1, 14, 4)
    c.line(14, 4, 17, 0)
    c.line(17, 0, 19, 6)
    c.line(10, 6, 19, 6)
    if frame:
        c.line(9, 13, 1, 4)
        c.line(8, 15, 0, 9)
        c.line(13, 15, 21, 22)
    else:
        c.line(8, 14, 0, 17)
        c.line(9, 16, 2, 22)
        c.line(14, 14, 23, 16)
    c.line(9, 20, 8, 23)
    c.line(15, 20, 16, 23)
    return c


def mudzilla(frame=0):
    c = Canvas(24, 24)
    c.ellipse(12 + frame, 15, 10, 8)
    c.ellipse(8, 10, 6, 6)
    c.ellipse(17, 11, 6, 7)
    c.ellipse(8, 10, 2, 2, 0)
    c.ellipse(17, 11, 2, 2, 0)
    c.pixel(9, 9)
    c.pixel(16, 10)
    c.line(8, 17, 16, 17, 0)
    c.pixel(11, 18, 0)
    c.pixel(13, 18, 0)
    c.line(3, 13, 0, 10 + frame)
    c.line(20, 14, 23, 11 - frame)
    c.rect(4, 20, 3, 4)
    c.rect(16, 20, 4, 4)
    c.rect(12, 5, 3, 3, 0)
    c.pixel(13, 6)
    return c


def acorn(direction=0):
    c = Canvas(8, 8)
    c.ellipse(4, 4, 2, 3)
    c.rect(2, 2, 5, 2, 0)
    c.line(4, 1, 5, 0)
    if direction == 1:
        return c.flip_vertical()
    if direction >= 2:
        rotated = Canvas(8, 8)
        for y, row in enumerate(c.pixels):
            for x, value in enumerate(row):
                if value is not None:
                    if direction == 2:
                        rotated.pixel(y, 7 - x, value)
                    else:
                        rotated.pixel(7 - y, x, value)
        return rotated
    return c


def encode(name, frames):
    width = frames[0].width
    height = frames[0].height
    data = [width, height]
    for frame in frames:
        for page in range((height + 7) // 8):
            for x in range(width):
                image = 0
                mask = 0
                for bit in range(8):
                    y = page * 8 + bit
                    if y >= height:
                        continue
                    value = frame.pixels[y][x]
                    if value is not None:
                        mask |= 1 << bit
                        if value:
                            image |= 1 << bit
                data.extend((image, mask))
    lines = []
    for start in range(0, len(data), 16):
        lines.append("  " + ", ".join(f"0x{value:02X}" for value in data[start:start + 16]) + ",")
    return f"const uint8_t {name}[] PROGMEM = {{\n" + "\n".join(lines) + "\n};\n"


sprites = {
    "pigSprites": [pig(0), pig(1), pig(0).mirror(), pig(1).mirror()],
    "crowSprites": [crow(0), crow(1)],
    "moleSprites": [mole(0), mole(1)],
    "beeSprites": [bee(0), bee(1)],
    "crowKingSprites": [crow_king(0), crow_king(1)],
    "mudzillaSprites": [mudzilla(0), mudzilla(1)],
    "acornSprites": [acorn(i) for i in range(4)],
}

output = "#pragma once\n\n#include <Arduino.h>\n\n"
output += "\n".join(encode(name, frames) for name, frames in sprites.items())
Path(__file__).resolve().parents[1].joinpath("src/sprites.h").write_text(output)
