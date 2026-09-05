#ifndef GPU_OPCODE
#error GPU_OPCODE must select a compiled operation
#endif

struct Command {
    uint op;
    int left;
    int top;
    int right;
    int bottom;
    uint p[59];
};

#if GPU_OPCODE == 0
StructuredBuffer<uint> heap : register(t0, space0);
RWTexture2D<float4> outputColor : register(u0, space1);
#else
StructuredBuffer<Command> commands : register(t0, space0);
StructuredBuffer<uint> tileIndices : register(t1, space0);
RWStructuredBuffer<uint> heap : register(u0, space1);
#endif

cbuffer Batch : register(b0, space2) {
    uint commandCount;
    int originX;
    int originY;
    uint exportColor;
    uint frameWidth;
    uint frameHeight;
    uint colorOffset;
    uint depthOffset;
    uint tilesX;
    uint padding0;
    uint padding1;
    uint padding2;
};

uint LoadByte(uint offset, uint index)
{
    return (heap[offset + index / 4] >> ((index & 3) * 8)) & 255;
}

uint LoadWord16(uint offset, uint index)
{
    return LoadByte(offset, index) | (LoadByte(offset, index + 1) << 8);
}

#if GPU_OPCODE != 0
#include "gpu_texture.hlsl"
#include "gpu_graph.hlsl"
#include "gpu_soft_z.hlsl"
#endif

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 tile : SV_GroupID)
{
#if GPU_OPCODE == 0
    if (id.x < frameWidth && id.y < frameHeight) {
        uint value = heap[colorOffset + id.y * frameWidth + id.x];
        outputColor[id.xy] = float4((value >> 16) & 255, (value >> 8) & 255, value & 255, value >> 24) / 255.0;
    }
#else
    int2 pixel = int2(id.xy) + int2(originX, originY);
    uint tileIndex = (tile.y * tilesX + tile.x) * 2;
    uint first = tileIndices[tileIndex];
    uint count = tileIndices[tileIndex + 1];
    for (uint i = 0; i < count; ++i) {
        Command c = commands[tileIndices[first + i]];
        c.op = GPU_OPCODE;
        if (pixel.x < c.left || pixel.y < c.top || pixel.x >= c.right || pixel.y >= c.bottom) continue;
        if (c.op == 1) {
            uint index = pixel.y * frameWidth + pixel.x;
            heap[colorOffset + index] = c.p[0];
            heap[depthOffset + index] = c.p[1];
        } else if (c.op >= 100 && c.op < 200) {
            DrawTexture(c, pixel);
        } else if (c.op >= 200 && c.op < 300) {
            DrawVid(c, pixel);
        } else if (c.op >= 300 && c.op < 500) {
            DrawGraph(c, pixel);
        } else if (c.op >= 500 && c.op < 600) {
            DrawSoftZ(c, pixel);
        }
    }
#endif
}
