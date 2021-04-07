uniform float4 gl_HalfPixel;

static float4 gl_Position;
static float3 in_position;
static float2 in_texcoord;
static float3 in_normal;

struct SPIRV_Cross_Input
{
    float3 in_position : TEXCOORD0;
    float2 in_texcoord : TEXCOORD1;
    float3 in_normal : TEXCOORD2;
};

struct SPIRV_Cross_Output
{
    float4 gl_Position : POSITION;
};

void vert_main()
{
    gl_Position = float4(in_position, 1.0f);
    gl_Position.x = gl_Position.x - gl_HalfPixel.x * gl_Position.w;
    gl_Position.y = gl_Position.y + gl_HalfPixel.y * gl_Position.w;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    in_position = stage_input.in_position;
    in_texcoord = stage_input.in_texcoord;
    in_normal = stage_input.in_normal;
    vert_main();
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = gl_Position;
    return stage_output;
}
