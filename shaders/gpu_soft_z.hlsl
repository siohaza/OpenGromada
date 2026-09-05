void DrawSoftZ(Command c, int2 pixel)
{
    uint destination = c.p[2] + pixel.y * c.p[3] + pixel.x;
    int x = pixel.x + int(c.p[4]);
    int y = pixel.y + int(c.p[5]);
    uint result = 0;
    if (x >= int(c.p[8]) && x < int(c.p[9]) && x >= 0 && x < int(frameWidth) && y >= 0 && y < int(frameHeight)) {
        uint row = heap[c.p[1] + pixel.y];
        int cursor = 0;
        while (row + 2 <= c.p[10]) {
            uint skip = LoadByte(c.p[0], row);
            uint count = LoadByte(c.p[0], row + 1);
            if ((skip | count) == 0) break;
            cursor += int(skip);
            if (pixel.x >= cursor && pixel.x < cursor + int(count)) {
                uint offset = uint(pixel.x - cursor) * 2;
                int delta = int(LoadWord16(c.p[0], row + 2 + offset) << 16) >> 16;
                uint source = LoadWord16(c.p[0], row + 2 + count * 2 + offset);
                int z = int(c.p[6]) + delta;
                int oldZ = int(heap[depthOffset + y * frameWidth + x] << 16) >> 16;
                if (z >= oldZ) {
                    if (z > oldZ + 127) {
                        result = c.p[7] != 0 ? source : source | 0xf000;
                    } else if (c.p[7] != 0) {
                        int alpha = ((z - oldZ) * int(source)) >> 7;
                        alpha = alpha > 65535 ? 0xf000 : alpha & 0xf000;
                        result = (source & 0xfff) + uint(alpha);
                    } else {
                        result = (uint((z - oldZ) / 8) << 12) | source;
                    }
                }
                break;
            }
            cursor += int(count);
            row += 2 + 4 * count;
        }
    }
    heap[destination] = result;
}
