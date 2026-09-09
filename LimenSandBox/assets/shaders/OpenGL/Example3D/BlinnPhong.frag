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

uniform vec3 u_AmbientReflectance;

/**
 * GAMES101常量环境光I_a的RGB颜色。
 */
uniform vec3 u_AmbientLightColor;

/**
 * GAMES101常量环境光I_a的强度倍率。
 */
uniform float u_AmbientLightIntensity;

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

/**
 * @brief 计算一个光源对当前着色点产生的Blinn-Phong直接光照。
 *
 * 这个函数不计算环境光，因为环境光只应在整个片元中计算一次，
 * 不能跟随光源数量重复累加。
 *
 * @param k_d
 * 材质漫反射系数。
 *
 * @param k_s
 * 材质镜面反射系数。
 *
 * @param p
 * Blinn-Phong高光指数。
 *
 * @param n
 * 当前着色点的单位法线。
 *
 * @param l
 * 从当前着色点指向光源的单位方向。
 *
 * @param v
 * 从当前着色点指向相机的单位方向。
 *
 * @param lightIntensityAtPoint
 * 实际到达当前着色点的RGB光照强度。
 *
 * 平行光传入：
 * Color * Intensity
 *
 * 点光源传入：
 * Color * Intensity / r²
 *
 * @return
 * 当前光源产生的漫反射与镜面反射之和。
 */
vec3 EvaluateBlinnPhongDirectLight(
    vec3 k_d,
    vec3 k_s,
    float p,
    vec3 n,
    vec3 l,
    vec3 v,
    vec3 lightIntensityAtPoint
)
{
    float nDotL = max(0.0, dot(n, l));

    // 如果光线在表面背面时，直接光照贡献为0
    if (nDotL <= 0.0)
    return vec3(0.0);

    vec3 h = normalize(v + l);

    // L_d = k_d * I * max(0, n·l)
    vec3 diffuse = k_d * lightIntensityAtPoint * nDotL;

    float nDotH = max(0.0, dot(n, h));

    // L_s = k_s * I * max(0, n·h) ^p
    vec3 specular = k_s * lightIntensityAtPoint * pow(nDotH, p);

    return diffuse + specular;

}

/**
    u_AmbientReflectance
        = k_a
        = 材质属性
        = 物体能反射多少环境光

    u_AmbientLightColor × u_AmbientLightIntensity
        = I_a
        = 场景属性
        = 场景中存在多少环境光
*/
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
    vec3 k_a = u_AmbientReflectance;

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
    vec3 ambientLightIntensity = u_AmbientLightColor * u_AmbientLightIntensity;

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

    /**
     * v：从着色点指向相机的单位方向。
     */
    vec3 v = normalize(u_CameraPosition - v_WorldPosition);

    // L_a = k_a * I_a
    vec3 ambient = k_a * ambientLightIntensity;

    vec3 directionalLightContribution = EvaluateBlinnPhongDirectLight(k_d, k_s, p, n, l, v, lightIntensity);

    /**
     * 所有点光源产生的直接光照总和。
     *
     * 每个点光源的直接光照包含：
     * 漫反射 + 镜面反射。
     */
    vec3 pointLightContribution = vec3(0.0);

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

        /**
         * 当前点光源的l和I/r²已经计算完成，
         * 交给通用函数计算漫反射与镜面反射。
         */
        pointLightContribution +=
        EvaluateBlinnPhongDirectLight(
            k_d,
            k_s,
            p,
            n,
            pointLightDirection,
            v,
            pointLightIntensity
        );
    }

    color = vec4(ambient
        + directionalLightContribution
        + pointLightContribution,
        albedoSample.a
    );
}