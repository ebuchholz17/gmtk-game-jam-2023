struct pixel_color {
    float2 texCoords: TEX_COORD;
    float4 color: COLOR;
    uint textureID: TEX_ID;
};

Texture2D textures[16] : register(t0);
SamplerState defaultSampler : register(s0);

float4 main (pixel_color pixel): SV_Target {
    float4 sample = textures[NonUniformResourceIndex(pixel.textureID)].Sample(defaultSampler, pixel.texCoords);

    return sample * pixel.color;
}
