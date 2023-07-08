let sprite_vertex_source = `
struct screen_dims {
    float width;
    float height;
};
uniform screen_dims screenDims;

attribute vec3 pos;
attribute vec2 texCoords;
attribute vec4 color;
attribute float textureID;

varying vec2 vTexCoords;
varying vec4 vColor;
varying float vTextureID;

void main() {
    gl_Position = vec4(pos.x * (1.0 / screenDims.width) * 2.0 - 1.0,
                    pos.y * (1.0 / screenDims.height) * 2.0 - 1.0,
                    -1.0, 1.0);
    gl_Position.y = -gl_Position.y;
    vTexCoords = texCoords;
    vColor = color;
    vTextureID = textureID;
}`;


let sprite_fragment_source = `
precision highp float;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;

varying vec2 vTexCoords;
varying vec4 vColor;
varying float vTextureID;

void main() {
    vec4 baseColor;

    if (vTextureID < 0.5) {
        baseColor = texture2D(texture0, vTexCoords);
    }
    else if (vTextureID >= 0.5 && vTextureID < 1.5) {
        baseColor = texture2D(texture1, vTexCoords);
    }
    else if (vTextureID >= 1.5 && vTextureID < 2.5) {
        baseColor = texture2D(texture2, vTexCoords);
    }
    else if (vTextureID >= 2.5 && vTextureID < 3.5) {
        baseColor = texture2D(texture3, vTexCoords);
    }
    else if (vTextureID >= 3.5 && vTextureID < 4.5) {
        baseColor = texture2D(texture4, vTexCoords);
    }
    else if (vTextureID >= 4.5 && vTextureID < 5.5) {
        baseColor = texture2D(texture5, vTexCoords);
    }
    else if (vTextureID >= 5.5 && vTextureID < 6.5) {
        baseColor = texture2D(texture6, vTexCoords);
    }
    else {
        baseColor = texture2D(texture7, vTexCoords);
    }

    gl_FragColor = baseColor * vColor;
}`;

