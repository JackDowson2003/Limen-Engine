//
// Created by chenlong on 2026/9/14.
//

#include "Limen/Asset/ModelImporter.h"

#include <cmath>
#include <cctype>
#include <limits>
#include <optional>
#include <system_error>

#include <tiny_obj_loader.h>

#include "Limen/Core/Log.h"

namespace Limen
{
    namespace
    {
        /**
         * @brief 唯一标识 OBJ 中的一条完整顶点
         *
         * OBJ 的位置、UV和法线分别使用独立索引
         * 三个索引全部相同，才表示同一个GPU顶点
         */
        struct OBJVertexKey final
        {
            int PositionIndex = -1;
            int TexCoordIndex = -1;
            int NormalIndex = -1;
            /**
             * @brief 缺少源法线时，决定哪些面可以共享生成的顶点法线。
             *
             * 源OBJ提供法线时保持为0；
             * 缺少法线时由平滑组或面编号生成。
             */
            uint64_t NormalGroupKey = 0;

            /*
             * C++20会依次比较三个成员。
             * 三个索引都相同，两个Key才相同。
             */
            bool operator==(
                const OBJVertexKey &
            ) const noexcept = default;
        };

        /**
         * @brief 生成unordered_map需要的哈希值。
         */
        struct OBJVertexKeyHash final
        {
            std::size_t operator()(const OBJVertexKey &key) const noexcept
            {
                const std::size_t positionHash = std::hash<int>{}(key.PositionIndex);
                const std::size_t texCoordHash = std::hash<int>{}(key.TexCoordIndex);
                const std::size_t normalHash = std::hash<int>{}(key.NormalIndex);
                const std::size_t normalGroupHash = std::hash<uint64_t>{}(key.NormalGroupKey);

                return positionHash ^ (texCoordHash << 1) ^ (normalHash << 2) ^ (normalGroupHash << 3);
            }
        };

        /**
         * @brief 构建一个OBJ材质分组所需的临时数据。
         *
         * 同一个Shape内使用同一种材质的三角形，
         * 会被收集到同一个MeshData中。
         */
        struct OBJMeshBuildData final
        {
            /**
            * tinyobjloader提供的源材质下标。
            *
            * -1表示OBJ没有为这些三角形指定材质。
            */
            int SourceMaterialIndex = -1;

            /**
             * @brief 当前材质分组是否存在缺少源法线的顶点。
             *
             * 只要有一个顶点缺少法线，就为整个Mesh重新生成法线，
             * 避免一个Mesh中混合使用源法线和未初始化法线。
             */
            bool RequiresGeneratedNormals = false;

            /**
             * 当前材质分组收集到的顶点和索引。
             */
            MeshData Geometry;

            /**
             * 当前材质分组自己的顶点查重表。
             *
             * 不同Mesh拥有不同顶点数组，
             * 因此不能跨材质分组共享顶点索引。
             */
            std::unordered_map<OBJVertexKey, uint32_t, OBJVertexKeyHash> VertexLookup;
        };

        /**
         * @brief 将 tinyobjloader 的一组索引转换为 Limen 顶点。
         *
         * Position必须存在；
         * Normal和TexCoord允许缺失。
         *
         * Normal缺失时先写入零向量，Mesh构建完成后统一生成；
         * TexCoord缺失时保持默认值(0, 0)。
         */
        [[nodiscard]]
        std::optional<MeshVertex> TryCreateMeshVertex(
            const tinyobj::attrib_t &attributes,
            const tinyobj::index_t &sourceIndex
        )
        {
            // OBJ 面必须引用有效的位置
            if (sourceIndex.vertex_index < 0)
                return std::nullopt;

            const std::size_t positionOffset = static_cast<std::size_t>(sourceIndex.vertex_index) * 3;

            // Position 需要连续读取 x、y、z 三个数。
            if (positionOffset + 2 >= attributes.vertices.size())
                return std::nullopt;

            MeshVertex vertex;

            vertex.Position = {
                attributes.vertices[positionOffset + 0],
                attributes.vertices[positionOffset + 1],
                attributes.vertices[positionOffset + 2]
            };

            // 当前Blinn-Phong Shader需要法线；存在源法线时直接读取
            if (sourceIndex.normal_index >= 0)
            {
                /*
                 * OBJ提供了法线索引，直接读取对应法线。
                 */
                const std::size_t normalOffset =
                        static_cast<std::size_t>(
                            sourceIndex.normal_index
                        ) * 3;

                if (normalOffset + 2 >= attributes.normals.size())
                    return std::nullopt;

                vertex.Normal = {
                    attributes.normals[normalOffset + 0],
                    attributes.normals[normalOffset + 1],
                    attributes.normals[normalOffset + 2]
                };
            } else
            {
                /*
                 * 当前顶点没有源法线时先写入零向量。
                 *
                 * AddOBJVertex会将RequiresGeneratedNormals设为true，
                 * 完成Mesh构建后再为整个Mesh统一生成法线。
                 */
                vertex.Normal = glm::vec3(0.0f);
            }

            // UV 在 OBJ 中允许缺失
            if (sourceIndex.texcoord_index >= 0)
            {
                const std::size_t texCoordOffset = static_cast<std::size_t>(sourceIndex.texcoord_index) * 2;
                if (texCoordOffset + 1 >= attributes.texcoords.size())
                    return std::nullopt;

                vertex.TexCoord = {
                    attributes.texcoords[texCoordOffset + 0],
                    attributes.texcoords[texCoordOffset + 1]
                };
            }
            return vertex;
        }

        /**
         * @brief 将一个OBJ索引加入指定的材质Mesh构建数据。
         *
         * 已存在的顶点复用索引；
         * 不存在的顶点才创建新的MeshVertex。
         */
        [[nodiscard]]
        bool AddOBJVertex(
            const tinyobj::attrib_t &attributes,
            const tinyobj::index_t &sourceIndex,
            const uint64_t normalGroupKey,
            OBJMeshBuildData &buildMeshData
        )
        {
            const OBJVertexKey vertexKey
            {
                .PositionIndex = sourceIndex.vertex_index,
                .TexCoordIndex = sourceIndex.texcoord_index,
                .NormalIndex = buildMeshData.RequiresGeneratedNormals ? -1 : sourceIndex.normal_index,
                .NormalGroupKey = buildMeshData.RequiresGeneratedNormals ? normalGroupKey : 0
            };

            if (const auto existingVertex = buildMeshData.VertexLookup.find(vertexKey);
                existingVertex != buildMeshData.VertexLookup.end()
            )
            {
                buildMeshData.Geometry.Indices.push_back(existingVertex->second);
                return true;
            }

            const std::optional<MeshVertex> vertex = TryCreateMeshVertex(attributes, sourceIndex);

            if (!vertex.has_value())
                return false;

            if (buildMeshData.Geometry.Vertices.size() >= std::numeric_limits<uint32_t>::max())
                return false;


            const uint32_t newVertexIndex = static_cast<uint32_t>(buildMeshData.Geometry.Vertices.size());

            buildMeshData.Geometry.Vertices.push_back(*vertex);
            buildMeshData.Geometry.Indices.push_back(newVertexIndex);

            buildMeshData.VertexLookup.emplace(vertexKey, newVertexIndex);

            return true;
        }

        /**
         * @brief 根据triangle 的geometry 生成顶点法线
         *
         * @param meshData 三角形的vertex 和 Index
         * @return 是否生成法线成功
         */
        [[nodiscard]]
        bool GenerateVertexNormals(MeshData &meshData)
        {
            if (meshData.Indices.size() % 3 != 0)
                return false;

            // 清空默认法线
            for (MeshVertex &vertex: meshData.Vertices)
                vertex.Normal = glm::vec3(0.0f);

            const std::size_t indexCount = meshData.Indices.size();

            const std::size_t vertexCount = meshData.Vertices.size();
            for (std::size_t i = 0; i < indexCount; i += 3)
            {
                const uint32_t firstIndex = meshData.Indices[i + 0];
                const uint32_t secondIndex = meshData.Indices[i + 1];
                const uint32_t thirdIndex = meshData.Indices[i + 2];

                if (firstIndex >= vertexCount || secondIndex >= vertexCount || thirdIndex >= vertexCount)
                    return false;

                MeshVertex &firstVertex = meshData.Vertices[firstIndex];
                MeshVertex &secondVertex = meshData.Vertices[secondIndex];
                MeshVertex &thirdVertex = meshData.Vertices[thirdIndex];

                const glm::vec3 firstEdge = secondVertex.Position - firstVertex.Position;

                const glm::vec3 secondEdge = thirdVertex.Position - firstVertex.Position;

                /*
                 * 叉乘方向由OBJ三角形的顶点绕序决定。
                 * 这里暂时不归一化，使面积较大的三角形具有更大权重。
                 */
                const glm::vec3 faceNormal = glm::cross(firstEdge, secondEdge);

                // 最小的长度 1e-12f
                // 面积为0的不参与法线积累
                if (glm::dot(faceNormal, faceNormal) <= 1e-12f)
                    continue;

                firstVertex.Normal += faceNormal;
                secondVertex.Normal += faceNormal;
                thirdVertex.Normal += faceNormal;
            }


            for (MeshVertex &vertex: meshData.Vertices)
            {
                if (const float normalLengthSquared = glm::dot(vertex.Normal, vertex.Normal);
                    normalLengthSquared <= 1e-12f
                )
                    return false;

                vertex.Normal = glm::normalize(vertex.Normal);
            }

            return true;
        }

        /**
         * @brief 根据三角形的位置、UV和法线生成每个顶点的切线。
         *
         * @param meshData 即将用于创建Mesh的顶点和索引数据。
         * @return 是否成功生成切线。
         */
        [[nodiscard]]
        bool GenerateVertexTangents(MeshData &meshData)
        {
            if (meshData.Vertices.empty() || meshData.Indices.size() % 3 != 0)
            {
                return false;
            }

            const std::size_t vertexCount = meshData.Vertices.size();

            /*
             * 一个顶点通常被多个三角形共享。
             * 因此先累加相邻三角形贡献的T和B，
             * 最后再统一正交化、归一化并计算手性。
             */
            std::vector tangentSums(vertexCount, glm::vec3(0.0f));

            std::vector bitangentSums(vertexCount, glm::vec3(0.0f));

            const std::size_t indexCount = meshData.Indices.size();

            // 收集每个三角形顶点对 B T的贡献
            for (std::size_t i = 0; i < indexCount; i += 3)
            {
                const uint32_t firstIndex = meshData.Indices[i + 0];
                const uint32_t secondIndex = meshData.Indices[i + 1];
                const uint32_t thirdIndex = meshData.Indices[i + 2];

                if (firstIndex >= vertexCount
                    || secondIndex >= vertexCount
                    || thirdIndex >= vertexCount
                )
                    return false;

                const MeshVertex &firstVertex = meshData.Vertices[firstIndex];
                const MeshVertex &secondVertex = meshData.Vertices[secondIndex];
                const MeshVertex &thirdVertex = meshData.Vertices[thirdIndex];

                // 三维模型中的两条边
                const glm::vec3 firstEdge = secondVertex.Position - firstVertex.Position;
                const glm::vec3 secondEdge = thirdVertex.Position - firstVertex.Position;

                // 同样两条边在UV空间中的变化
                const glm::vec2 firstUVEdge = secondVertex.TexCoord - firstVertex.TexCoord;
                const glm::vec2 secondUVEdge = thirdVertex.TexCoord - firstVertex.TexCoord;

                // M      = u1 v1
                //          u2 v2
                //          (E1, E2) = M * (T B )
                // E1 = Δu1 * T + Δv1 * B
                // E2 = Δu2 * T + Δv2 * B
                //
                // 如果 M 可逆，则可以通过 E * M-1 得到(T B)
                const float determinant = firstUVEdge.x * secondUVEdge.y - firstUVEdge.y * secondUVEdge.x;
                /*
                 * 行列式接近0，表示UV三角形退化成了线或点。
                 * 此时无法从UV变化唯一地反推出T和B。
                 */
                if (std::abs(determinant) <= 1e-8f)
                    continue;

                const float inverseDeterminant = 1.0f / determinant;

                const glm::vec3 triangleTangent =
                        inverseDeterminant * (firstEdge * secondUVEdge.y - secondEdge * firstUVEdge.y);
                const glm::vec3 triangleBitangent =
                        inverseDeterminant * (secondEdge * firstUVEdge.x - firstEdge * secondUVEdge.x);

                /*
                 * 一个三角形内部的UV映射是线性的，因此该三角形的
                 * 三个顶点共享同一份三角形T、B贡献。
                 *
                 * 顶点可能被多个三角形共享，所以这里只累加，
                 * 暂时不归一化，也不直接写入MeshVertex::Tangent。
                 */
                tangentSums[firstIndex] += triangleTangent;
                tangentSums[secondIndex] += triangleTangent;
                tangentSums[thirdIndex] += triangleTangent;

                bitangentSums[firstIndex] += triangleBitangent;
                bitangentSums[secondIndex] += triangleBitangent;
                bitangentSums[thirdIndex] += triangleBitangent;
            }

            // 整理并保存 T B
            for (std::size_t i = 0; i < vertexCount; ++i)
            {
                MeshVertex &vertex = meshData.Vertices[i];

                if (glm::dot(vertex.Normal, vertex.Normal) <= 1e-12f)
                    return false;

                // 创建单位法线
                const glm::vec3 normal = glm::normalize(vertex.Normal);

                glm::vec3 tangent = tangentSums[i];

                // 如果n 和 t 不正交则矫正
                // 删除T在N方向上的分量, 即进行施密特正交化
                // n2 = n2 - (<n2, n1>/ <n1, n1>) * n1, 由于n 是标准化过的 所以n1带入为1, 就不写了
                tangent -= normal * glm::dot(tangent, normal);

                if (glm::dot(tangent, tangent) <= 1e-12f)
                {
                    /*
                     * 当前顶点没有有效UV切线时，选择一个不与N平行的参考轴，
                     * 再通过叉乘构造与N垂直的备用切线。
                     */
                    const glm::vec3 referenceAxis =
                            std::abs(normal.z) < 0.999f
                                ? glm::vec3(0.f, 0.f, 1.f)
                                : glm::vec3(0.f, 1.f, 0.f);
                    tangent = glm::normalize(glm::cross(referenceAxis, normal));
                } else
                {
                    tangent = glm::normalize(tangent);
                }
                /*
                 * N×T得到由当前正交基推导出的B方向。
                 * 与UV计算出的累计B比较，确定切线空间的左右手性。
                 *
                 * 没有有效UV时bitangentSums[i]为零，
                 * 点积结果为0，默认使用+1。
                 */
                const float handedness =
                        glm::dot(glm::cross(normal, tangent), bitangentSums[i]) < 0.0f
                            ? -1.0f
                            : 1.0f;
                // xyz 保存单位切线，w 保存重建 BitTangent所需的方位符号
                vertex.Tangent = glm::vec4(tangent, handedness);
            }
            return true;
        }
    }

    Ref<Model> ModelImporter::Import(
        const std::filesystem::path &sourcePath
    )
    {
        /*
         * 使用 error_code 避免文件系统查询失败时抛出异常。
         * 文件不存在、权限不足等情况统一返回 nullptr。
         */
        std::error_code errorCode;

        if (!std::filesystem::exists(sourcePath, errorCode) ||
            errorCode)
        {
            LM_CORE_ERROR(
                "Model source file does not exist: '{}'",
                sourcePath.string()
            );

            return nullptr;
        }

        if (!std::filesystem::is_regular_file(
                sourcePath,
                errorCode
            ) || errorCode)
        {
            LM_CORE_ERROR(
                "Model source path is not a regular file: '{}'",
                sourcePath.string()
            );

            return nullptr;
        }

        /*
         * 将扩展名转换为小写，使 .OBJ、.Obj 和 .obj
         * 都能够进入同一个导入实现。
         */
        std::string extension = sourcePath.extension().string();

        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](const unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character)
                );
            }
        );

        if (extension != ".obj")
        {
            LM_CORE_ERROR(
                "Unsupported model format '{}': '{}'",
                extension,
                sourcePath.string()
            );

            return nullptr;
        }

        /**
         * 配置 OBJ 读取方式
         * Renderer 当前使用 TrangleList, so transform quad and other polygon to triangle
         */
        tinyobj::ObjReaderConfig readerConfig;
        readerConfig.triangulate = true;

        // 当前 MeshVertex 不保存顶点颜色，暂时不解析该扩展数据。
        readerConfig.vertex_color = false;

        // OBJ 中引用的MTL文件常位于 OBJ 所在目录
        readerConfig.mtl_search_path = sourcePath.parent_path().string();

        tinyobj::ObjReader reader;

        const bool parseSucceeded = reader.ParseFromFile(
            sourcePath.string(),
            readerConfig
        );

        // Waring 不代表导入失败，例如缺少 MTL 模型
        if (!reader.Warning().empty()) //警告字符串不为空时才打印
        {
            LM_CORE_WARN(
                "OBJ import warning for '{}': {}",
                sourcePath.string(),
                reader.Warning()
            );
        }

        if (!parseSucceeded)
        {
            LM_CORE_ERROR(
                "Failed to parse OBJ '{}': {}",
                sourcePath.string(),
                reader.Error()
            );
            return nullptr;
        }

        //现在只是取得 tinyobjloader 的解析结果, 下一步才会转换为 Limen::MeshData

        const tinyobj::attrib_t &attributes = reader.GetAttrib();
        const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();
        const std::vector<tinyobj::material_t> &materials = reader.GetMaterials();

        LM_CORE_INFO(
            "Parsed OBJ '{}': {} positions, {} shapes, {} materials",
            sourcePath.string(),
            attributes.vertices.size() / 3,
            shapes.size(),
            materials.size()
        );

        /**
         * 保存源模型中的材质名称和基础Blinn-Phong参数。
         *
         * 它仍然只是CPU端资源描述，
         * 不是Renderer使用的Material。
         */
        std::vector<ModelMaterialSlot> materialSlots;
        materialSlots.reserve(materials.size());

        for (const tinyobj::material_t &material: materials)
        {
            ModelMaterialSlot slot;

            /*
             * 名称为空时生成备用名称；
             * 名称处理与材质参数转换彼此独立。
             */
            if (material.name.empty())
                slot.Name = "Material_" + std::to_string(materialSlots.size());
            else
                slot.Name = material.name;

            // MTL的Ka。
            slot.AmbientReflectance = glm::vec3(
                material.ambient[0],
                material.ambient[1],
                material.ambient[2]
            );

            // MTL的Kd。
            slot.DiffuseReflectance = glm::vec3(
                material.diffuse[0],
                material.diffuse[1],
                material.diffuse[2]
            );

            // MTL的Ks。
            slot.SpecularReflectance = glm::vec3(
                material.specular[0],
                material.specular[1],
                material.specular[2]
            );

            // MTL的Ns。
            slot.Shininess = static_cast<float>(material.shininess);

            /*
             * tinyobjloader已经从map_Kd中解析出纹理文件名。
             *
             * 相对路径暂时以OBJ所在目录为基准；
             * 后续AssetManager会统一负责更完整的路径解析。
             */
            if (!material.diffuse_texname.empty())
            {
                std::filesystem::path texturePath = material.diffuse_texname;

                // 相对路径的情况下加上pre path
                if (texturePath.is_relative())
                    texturePath = sourcePath.parent_path() / texturePath;
                slot.AlbedoTexturePath = texturePath.lexically_normal();
            }

            materialSlots.push_back(std::move(slot));
        }


        /*
         * 一个tinyobj Shape会根据material_ids拆分为
         * 一个或多个ModelPart。
         *
         * 每个输出ModelPart只对应一个材质槽，
         * 并在自己的Mesh内部进行顶点去重。
         */
        std::vector<ModelPart> modelParts;
        modelParts.reserve(shapes.size());

        for (const tinyobj::shape_t &shape: shapes)
        {
            // 没有三角形索引的 Shape 不需要创建 Mesh。
            if (shape.mesh.indices.empty())
                continue;

            /*
             * 已经要求 tinyobjloader 执行三角化，
             * 所以 TriangleList 的索引数量应当是3的倍数。
             */
            if (shape.mesh.indices.size() % 3 != 0)
            {
                LM_CORE_ERROR(
                    "OBJ shape '{}' does not contain triangulated indices",
                    shape.name
                );

                return nullptr;
            }

            const std::size_t triangleCount = shape.mesh.indices.size() / 3;

            // 开启三角化后 每个输出面都应该是一个三角形
            if (shape.mesh.num_face_vertices.size() != triangleCount)
            {
                LM_CORE_ERROR(
                    "OBJ shape '{}' has inconsistent face data",
                    shape.name
                );

                return nullptr;
            }

            // 因为开启了三角化，所以每个小的都是三个顶点
            for (const unsigned int vertexCount: shape.mesh.num_face_vertices)
            {
                if (vertexCount != 3)
                {
                    LM_CORE_ERROR(
                        "OBJ shape '{}' contains a non-triangle face",
                        shape.name
                    );

                    return nullptr;
                }
            }

            // material_ids 与三角形一一对应
            if (shape.mesh.material_ids.size() != triangleCount)
            {
                LM_CORE_ERROR(
                    "OBJ shape '{}' has inconsistent material data",
                    shape.name
                );
                return nullptr;
            }

            /*
             * 在正式创建顶点之前，先检查每个材质分组是否存在缺失法线。
             *
             * 如果一个分组中有任何顶点缺少法线，
             * 后续构建该分组时就统一忽略所有源法线并重新生成。
             */
            std::unordered_map<int, bool> materialRequiresGeneratedNormals;

            for (std::size_t i = 0; i < triangleCount; ++i)
            {
                const int materialIndex = shape.mesh.material_ids[i];

                bool &requiresGeneratedNormal = materialRequiresGeneratedNormals[materialIndex];

                const std::size_t indexOffset = i * 3;

                for (std::size_t corner = 0; corner < 3; ++corner)
                {
                    const tinyobj::index_t &sourceIndex = shape.mesh.indices[indexOffset + corner];

                    if (sourceIndex.normal_index < 0)
                    {
                        requiresGeneratedNormal = true;
                        break;
                    }
                }
            }


            /*
             * 一个Shape可能使用多个材质。
             *
             * vector保持材质第一次出现时的稳定顺序；
             * unordered_map用于根据源材质编号快速找到构建器。
             */
            std::vector<OBJMeshBuildData> meshBuildDataList;

            std::unordered_map<int, std::size_t> materialToBuildData;

            /*
             * 必须按三角形遍历，因为material_ids的一个元素
             * 对应一个三角形，而不是一个顶点。
             */
            for (std::size_t i = 0; i < triangleCount; ++i)
            {
                const int sourceMaterialIndex = shape.mesh.material_ids[i];

                // -1 表示没有材质 其他附属或者超过数组上线都是无效数据
                if (sourceMaterialIndex < -1 ||
                    (sourceMaterialIndex >= 0 &&
                     static_cast<std::size_t>(sourceMaterialIndex) >= materials.size()
                    )
                )
                {
                    LM_CORE_ERROR(
                        "OBJ shape '{}' contains invalid material index {}",
                        shape.name,
                        sourceMaterialIndex
                    );

                    return nullptr;
                }

                std::size_t buildDataIndex = 0;

                // 查找这个 material 是否有 对应的 buildData
                if (const auto existingBuildData = materialToBuildData.find(sourceMaterialIndex);
                    existingBuildData != materialToBuildData.end()
                )
                {
                    buildDataIndex = existingBuildData->second;
                } else
                {
                    //第一次遇到这个 Material 创建新的 构造器
                    buildDataIndex = meshBuildDataList.size();

                    OBJMeshBuildData newBuildData;
                    newBuildData.SourceMaterialIndex = sourceMaterialIndex;

                    newBuildData.RequiresGeneratedNormals = materialRequiresGeneratedNormals[sourceMaterialIndex];

                    meshBuildDataList.push_back(std::move(newBuildData));

                    materialToBuildData.emplace(sourceMaterialIndex, buildDataIndex);
                }

                OBJMeshBuildData &buildData = meshBuildDataList[buildDataIndex];

                /*
                 * 当前三角形在扁平indices数组中的开始位置。
                 */
                const std::size_t triangleIndexOffset = i * 3;

                // tinyobjloader 使用0 表示关闭平滑, 没有记录的时候按照关闭处理
                const uint32_t smoothingGroup = i < shape.mesh.smoothing_group_ids.size()
                                                    ? shape.mesh.smoothing_group_ids[i]
                                                    : 0;

                uint64_t normalGroupKey = 0;

                if (smoothingGroup != 0)
                    normalGroupKey = smoothingGroup;
                else
                    // 会与真实的平滑组编号冲突 所以引入高位1作为sign
                    normalGroupKey = (uint64_t{1} << 63) | static_cast<uint64_t>(i);

                for (std::size_t corner = 0; corner < 3; ++corner)
                {
                    if (const tinyobj::index_t sourceIndex = shape.mesh.indices[triangleIndexOffset + corner];
                        !AddOBJVertex(attributes, sourceIndex, normalGroupKey, buildData)
                    )
                    {
                        LM_CORE_ERROR(
                            "OBJ shape '{}' contains invalid vertex data",
                            shape.name
                        );

                        return nullptr;
                    }
                }
            }

            // 每个材质构建起 最终对应一个Model Part
            for (OBJMeshBuildData &buildData: meshBuildDataList)
            {
                if (buildData.Geometry.Vertices.empty() || buildData.Geometry.Indices.empty())
                    continue;

                if (buildData.RequiresGeneratedNormals && !GenerateVertexNormals(buildData.Geometry))
                {
                    LM_CORE_ERROR(
                        "Failed to generate normals for OBJ shape '{}'",
                        shape.name
                    );

                    return nullptr;
                }

                /**
                 * 切线正交化依赖最终顶点的法线
                 * 因此必须在读取或者生成法线后执行
                 */
                if (!GenerateVertexTangents(buildData.Geometry))
                {
                    LM_CORE_ERROR(
                        "Failed to generate tangents for OBJ shape '{}'",
                        shape.name
                    );

                    return nullptr;
                }

                ModelPart part;

                const std::string basePartName = shape.name.empty() ? "Unnamed Part" : shape.name;

                if (buildData.SourceMaterialIndex >= 0)
                {
                    part.MaterialSlot = static_cast<uint32_t>(buildData.SourceMaterialIndex);
                    /*
                     * 在Part名称后加入材质名，
                     * 便于调试同一个Shape拆出的多个部分。
                     */
                    part.Name = basePartName + "/" + materialSlots[part.MaterialSlot].Name;
                } else
                {
                    part.MaterialSlot = ModelPart::InvalidMaterialSlot;

                    part.Name = basePartName;
                }


                LM_CORE_INFO(
                    "Built OBJ part '{}': {} vertices, {} indices, normals={}",
                    part.Name,
                    buildData.Geometry.Vertices.size(),
                    buildData.Geometry.Indices.size(),
                    buildData.RequiresGeneratedNormals
                    ? "generated"
                    : "source"
                );
                part.MeshResource = CreateRef<Mesh>(buildData.Geometry);
                part.LocalTransform = glm::mat4(1.0f);
                modelParts.push_back(std::move(part));
            }
        }


        if (modelParts.empty())
        {
            LM_CORE_ERROR(
                "OBJ '{}' does not contain any renderable shapes",
                sourcePath.string()
            );

            return nullptr;
        }

        LM_CORE_INFO(
            "Imported OBJ '{}' as {} model parts",
            sourcePath.string(),
            modelParts.size()
        );

        return CreateRef<Model>(std::move(modelParts), std::move(materialSlots));
    }
}
