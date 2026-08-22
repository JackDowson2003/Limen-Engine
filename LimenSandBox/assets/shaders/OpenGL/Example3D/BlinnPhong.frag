#version 410 core

layout(location = 0) out vec4 color;

/**
 * Vertex Shader输出并经过光栅化插值的数据。
 */
in vec3 v_WorldNormal;
in vec3 v_WorldPosition;
in vec2 v_TexCoord;

/**
 * 相机在世界空间中的位置。
 *
 * 用于计算从着色点指向相机的观察方向v。
 */
uniform vec3 u_CameraPosition;

/**
 * 当前材质的Albedo纹理。
 *
 * 当前约定从纹理槽0读取。
 */
uniform sampler2D u_AlbedoTexture;

void main()
{
     /**
     * 对Albedo纹理进行采样。
     *
     * RGB用于材质漫反射系数k_d；
     * Alpha用于最终输出透明度。
     */
    vec4 albedoSample = texture(u_AlbedoTexture, v_TexCoord);

    // GAMES101中的材质漫反射系数。
    vec3 k_d = albedoSample.rgb;

    // 当前使用Albedo的15%近似环境光反射系数。
    vec3 k_a = 0.15 * k_d;

    // 材质镜面反射系数。
    const vec3 k_s = vec3(0.35);

     /**
     * Blinn-Phong高光指数。
     *
     * 越大，高光越集中；
     * 越小，高光越宽。
     */
    const float p = 64.0;

    // 环境光强度I_a。
    const vec3 ambientLightIntensity = vec3(1.0);

     /**
     * 平行光强度I。
     *
     * 当前使用平行光，所以没有1/r²距离衰减。
     */
    const vec3 lightIntensity = vec3(1.0, 0.95, 0.85);

    // n：世界空间中的单位表面法线。
    vec3 n = normalize(v_WorldNormal);

    // l：从着色点指向光源的单位方向。
    vec3 l = normalize(vec3(-1.0, 1.0, 1.0));

    float nDotL = max(dot(n, l), 0.0);

     /**
     * v：从着色点指向相机的单位方向。
     */
    vec3 v = normalize(u_CameraPosition - v_WorldPosition);

     /**
     * h：光照方向l与观察方向v之间的半程向量。
     */
    vec3 h = normalize(l + v);

    float nDotH = max(dot(n, h), 0.0);

    // L_a = k_a * I_a
    vec3 ambient = k_a * ambientLightIntensity;

    // L_d = k_d * I * max(0, n dot l)
    vec3 diffuse = k_d * lightIntensity * nDotL;

     /**
     * 当光源位于表面背面时，不允许产生镜面高光。
     */
    float specularStrength = nDotL > 0.0 ? pow(nDotH, p) : 0.0;

    // L_s = k_s * I * max(0, n dot h)^p
    vec3 specular = k_s * lightIntensity * specularStrength;

    color = vec4(ambient + diffuse + specular, albedoSample.a);
}