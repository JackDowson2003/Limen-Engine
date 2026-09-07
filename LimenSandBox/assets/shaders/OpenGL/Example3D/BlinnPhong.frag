#version 410 core

/**
 * 第一版前向渲染最多处理4个点光源。
 *
 * GLSL普通uniform数组的长度必须是编译期常量，
 * 因此不能直接使用u_PointLightCount作为数组长度。
 */
#define LIMEN_MAX_POINT_LIGHTS 4

layout (location = 0) out vec4 color;

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
 * 主平行光的光线传播方向。
 *
 * 它表示光从光源射向场景的方向。
 */
uniform vec3 u_DirectionalLightDirection;

/**
 * 主平行光的线性 RGB 颜色。
 */
uniform vec3 u_DirectionalLightColor;

/**
 * 主平行光的亮度倍率。
 */
uniform float u_DirectionalLightIntensity;

/**
 * @brief GAMES101 Blinn-Phong模型中的点光源。
 */
struct PointLight
{
    // 点光源在世界空间中的位置。
    vec3 Position;

    // 点光源发出的线性RGB颜色。
    vec3 Color;

    // 点光源的整体强度倍率。
    float Intensity;
};

/**
 * 当前参与着色的点光源数量。
 *
 * 有效范围为：
 * 0到LIMEN_MAX_POINT_LIGHTS。
 */
uniform int u_PointLightCount;

/**
 * 当前场景中的点光源数组。
 *
 * Shader只读取前u_PointLightCount个元素。
 */
uniform PointLight u_PointLights[LIMEN_MAX_POINT_LIGHTS];

/**
 * 当前材质的Albedo纹理。
 *
 * 当前约定从纹理槽0读取。
 */
uniform sampler2D u_AlbedoTexture;

/**
 * Blinn-Phong高光指数p。
 *
 * 数值越大，高光范围越小、越集中；
 * 数值越小，高光范围越大、越柔和。
 */
uniform float u_Shininess;

/**
 * GAMES101中的材质镜面反射系数k_s。
 *
 * RGB分别控制三个颜色通道反射镜面光的强度。
 */
uniform vec3 u_SpecularColor;

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
    vec3 k_s = u_SpecularColor;

    /**
     * Blinn-Phong高光指数。
     *
     * 越大，高光越集中；
     * 越小，高光越宽。
     */
    float p = u_Shininess;

    // 环境光强度I_a。
    const vec3 ambientLightIntensity = vec3(1.0);

    /**
     * 平行光强度I。
     *
     * 当前使用平行光，所以没有1/r²距离衰减。
     */
    vec3 lightIntensity = u_DirectionalLightColor * u_DirectionalLightIntensity;

    // n：世界空间中的单位表面法线。
    vec3 n = normalize(v_WorldNormal);

    /*
     * Direction 表示光从光源射向场景；
     * Blinn-Phong 中的 l 表示从表面点指向光源，
     * 因此两者方向相反。
     */
    vec3 l = normalize(-u_DirectionalLightDirection);

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

    /**
     * 所有点光源产生的漫反射总和。
     *
     * 从黑色开始，每处理一个点光源就累加一次贡献。
     */
    vec3 pointLightDiffuse = vec3(0.0);

    for (int pointLightIndex = 0; pointLightIndex < u_PointLightCount; pointLightIndex++)
    {
        /**
         * 从当前着色点p指向点光源位置的向量。
         *
         * lightVector = lightPosition - fragmentPosition
         */
        vec3 lightVector = u_PointLights[pointLightIndex].Position - v_WorldPosition;

        /**
         * r² = lightVector · lightVector
         *
         * 使用较小的下限，避免着色点恰好位于光源位置时除以0。
         */
        float distanceSquared = max(
            dot(lightVector, lightVector),
            0.0001
        );

        /**
         * l：从当前着色点指向点光源的单位方向。
         *
         * lightVector / length(lightVector)
         * 等价于：
         * lightVector * 1 / sqrt(distanceSquared)
         */
        vec3 pointLightDirection = lightVector * inversesqrt(distanceSquared);

        /**
         * GAMES101中的距离平方反比衰减：
         *
         * I / r²
         */
        vec3 pointLightIntensity =
        u_PointLights[pointLightIndex].Color *
        u_PointLights[pointLightIndex].Intensity /
        distanceSquared;

        float pointLightNDotL = max(dot(n,pointLightDirection),0.0);

        pointLightDiffuse += k_d * pointLightIntensity * pointLightNDotL;
    }

    color = vec4(ambient + diffuse + specular + pointLightDiffuse, albedoSample.a);
}