//
// Created by chenlong on 2026/11/25.
//
#include <toml.hpp>
#include <limits>
#include <cmath>
#include <string>

#include "Limen/Scene/SceneSerializer.h"

#include "Limen/Core/Log.h"
#include "Limen/Scene/Scene.h"

namespace Limen
{
    namespace
    {
        // 确保 float 转成 double 后写进 TOML 的数值在读取时一字不差，方便后续做范围校验
        toml::ordered_value WriteFloat(const float value)
        {
            toml::ordered_value node(static_cast<double>(value));

            // 保留扩展为double 后的精确值，供读端做 float 范围校验
            node.as_floating_fmt().prec = std::numeric_limits<double>::max_digits10;
            return node;
        }

        toml::ordered_value WriteVec3(const glm::vec3 &value)
        {
            return toml::ordered_value(
                toml::ordered_array{
                    WriteFloat(value.x),
                    WriteFloat(value.y),
                    WriteFloat(value.z)
                }
            );
        }

        // 读取参数
        bool ReadFloat(const toml::value &node, float &output)
        {
            if (node.is_integer())
            {
                output = static_cast<float>(node.as_integer());
                return true;
            }

            if (!node.is_floating())
                return false;

            const double value = node.as_floating();
            constexpr double max = std::numeric_limits<float>::max();
            if (!std::isfinite(value) || value < -max || value > max)
                return false;

            const float converted = static_cast<float>(value);
            if (value != 0.0 && converted == 0.0f)
                return false;

            output = converted;
            return true;
        }

        bool ReadVec3(const toml::value &node, glm::vec3 &output)
        {
            if (!node.is_array() || node.as_array().size() != 3)
                return false;

            const auto &values = node.as_array();
            glm::vec3 candidate{0.0f};

            // 三个分量都都去取成功后，再替换输出
            if (!ReadFloat(values[0], candidate.x) ||
                !ReadFloat(values[1], candidate.y) ||
                !ReadFloat(values[2], candidate.z)
            )
            {
                return false;
            }

            output = candidate;
            return true;
        }

        bool ReadRotation(const toml::value &node, glm::quat &output)
        {
            if (!node.is_array() || node.as_array().size() != 4)
                return false;

            const auto &values = node.as_array();
            float x{}, y{}, z{}, w{};
            if (!ReadFloat(values[0], x) || !ReadFloat(values[1], y) ||
                !ReadFloat(values[2], z) || !ReadFloat(values[3], w))
                return false;

            // 全零四元数无法表示有效旋转。
            if (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f)
                return false;

            output = glm::quat(glm::normalize(glm::dquat::wxyz(w, x, y, z)));
            return true;
        }

        std::string ToTomlString(const LString &value)
        {
            return std::string(value.data(), value.size());
        }

        void WriteTransform(toml::ordered_table &fields, const SceneTransform &transform)
        {
            fields["location"] = WriteVec3(transform.Location);

            // .lscene 的四元数顺序是 x、y、z、w。
            const auto &q = transform.Rotation;
            fields["rotation"] = toml::ordered_array{
                WriteFloat(q.x), WriteFloat(q.y), WriteFloat(q.z), WriteFloat(q.w)
            };

            fields["scale"] = WriteVec3(transform.Scale);
        }

        toml::ordered_table WriteComponent(const SceneObjectComponent &component)
        {
            toml::ordered_table fields;
            fields["id"] = ToTomlString(SceneComponentIDToString(component.ID));
            WriteTransform(fields, component.LocalTransform);

            switch (GetSceneComponentKind(component))
            {
                case SceneComponentKind::ModelRenderer:
                {
                    const auto &[ModelAssetID] = std::get<ModelRendererComponent>(component.Data);
                    fields["type"] = "model_renderer";
                    fields["model_asset_id"] = ToTomlString(AssetIDToString(ModelAssetID));
                    break;
                }
                case SceneComponentKind::PrimitiveRenderer:
                {
                    fields["type"] = "primitive_renderer";
                    fields["primitive"] = "cube";
                    break;
                }
                case SceneComponentKind::PointLight:
                {
                    const auto &[Color, Intensity] = std::get<PointLightComponent>(component.Data);
                    fields["type"] = "point_light";
                    fields["color"] = WriteVec3(Color);
                    fields["intensity"] = WriteFloat(Intensity);
                    break;
                }
            }
            return fields;
        }

        // 对四元数做判断，参数是SceneTransform
        bool IsWritableTransform(const SceneTransform &transform) noexcept
        {
            const auto &q = transform.Rotation;
            return transform.IsFinite() &&
                   (q.x != 0.0f || q.y != 0.0f || q.z != 0.0f || q.w != 0.0f);
        }

        bool IsFiniteVec3(const glm::vec3 &value) noexcept
        {
            return std::isfinite(value.x) &&
                   std::isfinite(value.y) &&
                   std::isfinite(value.z);
        }
    }

    bool ValidateSceneDocument(const SceneDocument &document, SceneFileError &error)
    {
        // 复用对象
        const LArray<SceneObjectAuthoringData> &objects = document.Objects;

        using Code = SceneFileErrorCode;

        const auto fail = [&error](const Code code, const std::string_view key, const char *message)
        {
            error = {.Code = code, .Key = LString(key), .Message = message};
            return false;
        };

        if (document.SchemaVersion != SceneDocument::CurrentSchemaVersion)
            return fail(Code::UnsupportedSchema, "schema_version", "不支持的场景版本");

        const auto &[aColor, aIntensity] = document.Ambient;
        const auto &[Direction, dColor, dIntensity] = document.DirectionLight;

        if (!IsFiniteVec3(aColor))
            return fail(Code::InvalidField, "environment.ambient_color", "环境光颜色无效");
        if (!std::isfinite(aIntensity) || aIntensity < 0.0f)
            return fail(Code::InvalidField, "environment.ambient_intensity", "环境光强度无效");

        if (!IsFiniteVec3(dColor))
            return fail(Code::InvalidField, "environment.directional_color", "平行光颜色无效");
        if (!std::isfinite(dIntensity) || dIntensity < 0.0f)
            return fail(Code::InvalidField, "environment.directional_intensity", "平行光强度无效");

        const float lengthSquared = glm::dot(Direction, Direction);
        if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f)
            return fail(Code::InvalidField, "environment.directional_direction", "平行光方向无效");

        // 要求对象ID唯一
        for (std::uint32_t i = 0; i < objects.getSize(); ++i)
        {
            const auto &obj = objects[i];
            const std::string objectKey = "objects[" + std::to_string(i) + "]";
            const std::string idKey = objectKey + ".id";

            if (!obj.ID.isValid())
                return fail(Code::InvalidID, idKey, "对象 ID 无效");

            // 只和前面的对象比较，避免把自身判为重复。
            for (std::uint32_t j = 0; j < i; ++j)
            {
                if (objects[j].ID == obj.ID)
                    return fail(Code::DuplicateID, idKey, "对象 ID 重复");
            }

            if (!IsWritableTransform(obj.Transform))
                return fail(Code::InvalidField, objectKey, "对象的变换无效");

            const auto &components = obj.Components;
            for (std::uint32_t c = 0; c < components.getSize(); ++c)
            {
                const auto &component = components[c];
                const std::string key = objectKey + ".components[" + std::to_string(c) + "]";

                if (!component.ID.isValid())
                    return fail(Code::InvalidID, key + ".id", "组件 ID 无效");

                if (!IsWritableTransform(component.LocalTransform) ||
                    !IsSceneObjectComponentValid(component))
                    return fail(Code::InvalidField, key, "组件数据无效");

                // 只比较当前对象里已经检查过的组件。
                for (std::uint32_t previous = 0; previous < c; ++previous)
                {
                    if (components[previous].ID == component.ID)
                        return fail(Code::DuplicateID, key + ".id", "同一对象内的组件 ID 重复");
                }
            }
        }
        return true;
    }

    bool TrySerializeSceneDocument(const SceneDocument &document, LString &outText, SceneFileError &error)
    {
        if (!ValidateSceneDocument(document, error))
            return false;
        const auto &objects = document.Objects;

        // 组装成表
        toml::ordered_table environment;
        environment["ambient_color"] = WriteVec3(document.Ambient.Color);
        environment["ambient_intensity"] = WriteFloat(document.Ambient.Intensity);
        environment["directional_direction"] = WriteVec3(document.DirectionLight.Direction);
        environment["directional_color"] = WriteVec3(document.DirectionLight.Color);
        environment["directional_intensity"] = WriteFloat(document.DirectionLight.Intensity);

        toml::ordered_table root;
        root["schema_version"] = static_cast<std::uint64_t>(document.SchemaVersion);
        root["display_name"] = std::string(document.DisplayName.data(), document.DisplayName.size());
        root["environment"] = std::move(environment);

        toml::ordered_array objectTables;
        for (const auto &[ID, Label, Transform, Components]: objects)
        {
            toml::ordered_table fields;
            fields["id"] = ToTomlString(SceneObjectIDToString(ID));
            fields["label"] = ToTomlString(Label);
            WriteTransform(fields, Transform);

            toml::ordered_array componentTables;
            for (const auto &component: Components)
                componentTables.emplace_back(WriteComponent(component));

            fields["components"] = std::move(componentTables);
            objectTables.emplace_back(std::move(fields));
        }
        root["objects"] = std::move(objectTables);

        // 整理完整信息
        const LString text(toml::format(toml::ordered_value(std::move(root)), toml::spec::v(1, 0, 0)));
        outText = text;
        error = {};
        return true;
    }

    bool TryDeserializeSceneDocument(std::string_view text, SceneDocument &outDocument, SceneFileError &error)
    {
        using Code = SceneFileErrorCode;

        const auto fail = [&error](Code code, std::string_view key, std::string_view message)
        {
            error =
            {
                .Code = code,
                .Key = LString(key),
                .Message = LString(message)
            };
            return false;
        };

        if (text.starts_with("\xEF\xBB\xBF"))
            return fail(Code::InvalidToml, "", "场景文本必须使用无 BOM 的 UTF-8");

        const auto parsed = toml::try_parse_str(std::string(text), toml::spec::v(1, 0, 0));
        if (parsed.is_err())
        {
            const auto &problems = parsed.unwrap_err();
            if (problems.empty())
                return fail(Code::InvalidToml, "", "TOML 解析失败");

            return fail(Code::InvalidToml, "", toml::format_error(problems.front()));
        }

        const auto &root = parsed.unwrap();

        if (!root.contains("schema_version") || !root.at("schema_version").is_integer())
            return fail(Code::InvalidField, "schema_version", "必须提供整数版本的版本号");

        if (root.at("schema_version").as_integer() != SceneDocument::CurrentSchemaVersion)
            return fail(Code::UnsupportedSchema, "schema_version", "不支持的场景版本");

        for (const auto &entry: root.as_table())
        {
            const auto &key = entry.first;
            if (key != "schema_version" && key != "display_name" && key != "environment" && key != "objects")
                return fail(Code::InvalidField, key, "未知场景字段");
        }

        if (!root.contains("display_name") || !root.at("display_name").is_string())
            return fail(Code::InvalidField, "display_name", "必须提供字符串场景名称");

        if (!root.contains("environment") || !root.at("environment").is_table())
            return fail(Code::InvalidField, "environment", "必须提供环境表");

        if (!root.contains("objects") || !root.at("objects").is_array())
            return fail(Code::InvalidField, "objects", "必须提供对象数组");

        // 后续字段全部读进局部文档，校验通过后才提交给调用方。
        SceneDocument candidate;
        candidate.DisplayName = LString(root.at("display_name").as_string());

        const auto &environment = root.at("environment");
        for (const auto &entry: environment.as_table())
        {
            const auto &key = entry.first;
            if (key != "ambient_color" &&
                key != "ambient_intensity" &&
                key != "directional_direction" &&
                key != "directional_color" &&
                key != "directional_intensity")
                return fail(Code::InvalidField, "environment." + key, "未知环境字段");
        }

        const auto readEnvironmentField = [&](const char *key, auto read, auto &output)
        {
            if (environment.contains(key) && read(environment.at(key), output))
                return true;

            return fail(Code::InvalidField, std::string("environment.") + key,
                        "环境字段缺失或数值格式无效");
        };

        if (!readEnvironmentField("ambient_color", ReadVec3, candidate.Ambient.Color) ||
            !readEnvironmentField("ambient_intensity", ReadFloat, candidate.Ambient.Intensity) ||
            !readEnvironmentField("directional_direction", ReadVec3, candidate.DirectionLight.Direction) ||
            !readEnvironmentField("directional_color", ReadVec3, candidate.DirectionLight.Color) ||
            !readEnvironmentField("directional_intensity", ReadFloat, candidate.DirectionLight.Intensity))
            return false;

        // 读取objects
        const auto readField = [&fail](const toml::value &table, const std::string &path,
                                       const char *key, auto read, auto &output)
        {
            if (table.contains(key) && read(table.at(key), output))
                return true;

            return fail(Code::InvalidField, path + "." + key, "字段缺失或数值格式无效");
        };

        const auto &objects = root.at("objects").as_array();
        for (std::size_t i = 0; i < objects.size(); ++i)
        {
            const auto &table = objects[i];
            const std::string path = "objects[" + std::to_string(i) + "]";

            if (!table.is_table())
                return fail(Code::InvalidField, path, "对象必须是表");

            for (const auto &field: table.as_table())
            {
                const auto &key = field.first;
                if (key != "id" && key != "label" && key != "location" &&
                    key != "rotation" && key != "scale" && key != "components")
                    return fail(Code::InvalidField, path + "." + key, "未知对象字段");
            }

            SceneObjectAuthoringData object;

            if (!table.contains("id") || !table.at("id").is_string())
                return fail(Code::InvalidField, path + ".id", "对象 ID 必须是字符串");

            if (!TryParseSceneObjectID(table.at("id").as_string(), object.ID))
                return fail(Code::InvalidID, path + ".id", "对象 ID 格式无效");

            if (!table.contains("label") || !table.at("label").is_string())
                return fail(Code::InvalidField, path + ".label", "对象名称必须是字符串");

            object.Label = LString(table.at("label").as_string());

            if (!readField(table, path, "location", ReadVec3, object.Transform.Location) ||
                !readField(table, path, "rotation", ReadRotation, object.Transform.Rotation) ||
                !readField(table, path, "scale", ReadVec3, object.Transform.Scale))
                return false;

            if (table.contains("components"))
            {
                const auto &node = table.at("components");
                const std::string componentsPath = path + ".components";

                if (!node.is_array())
                    return fail(Code::InvalidField, componentsPath, "组件必须是数组");

                const auto &components = node.as_array();
                for (std::size_t c = 0; c < components.size(); ++c)
                {
                    const auto &entry = components[c];
                    const std::string componentPath =
                            componentsPath + "[" + std::to_string(c) + "]";

                    if (!entry.is_table())
                        return fail(Code::InvalidField, componentPath, "组件必须是表");

                    SceneObjectComponent component;

                    if (!entry.contains("id") || !entry.at("id").is_string())
                        return fail(Code::InvalidField, componentPath + ".id", "组件 ID 必须是字符串");

                    if (!TryParseSceneComponentID(entry.at("id").as_string(), component.ID))
                        return fail(Code::InvalidID, componentPath + ".id", "组件 ID 格式无效");

                    if (!entry.contains("type") || !entry.at("type").is_string())
                        return fail(Code::InvalidField, componentPath + ".type", "组件类型必须是字符串");

                    // 组件的 TRS 相对于所属对象，所以写入 LocalTransform。
                    auto &local = component.LocalTransform;
                    if (!readField(entry, componentPath, "location", ReadVec3, local.Location) ||
                        !readField(entry, componentPath, "rotation", ReadRotation, local.Rotation) ||
                        !readField(entry, componentPath, "scale", ReadVec3, local.Scale))
                        return false;

                    const auto &type = entry.at("type").as_string();

                    if (type == "model_renderer")
                    {
                        if (!entry.contains("model_asset_id") || !entry.at("model_asset_id").is_string())
                            return fail(Code::InvalidField, componentPath + ".model_asset_id",
                                        "模型资源 ID 必须是字符串");

                        ModelRendererComponent model;
                        if (!TryParseAssetID(entry.at("model_asset_id").as_string(), model.ModelAssetID))
                            return fail(Code::InvalidID, componentPath + ".model_asset_id",
                                        "模型资源 ID 格式无效");

                        component.Data = model;
                    } else if (type == "primitive_renderer")
                    {
                        if (!entry.contains("primitive") || !entry.at("primitive").is_string() ||
                            entry.at("primitive").as_string() != "cube")
                            return fail(Code::InvalidField, componentPath + ".primitive",
                                        "内置几何必须为 cube");

                        component.Data = PrimitiveRendererComponent{.Primitive = PrimitiveType::Cube};
                    } else if (type == "point_light")
                    {
                        PointLightComponent light;
                        if (!readField(entry, componentPath, "color", ReadVec3, light.Color) ||
                            !readField(entry, componentPath, "intensity", ReadFloat, light.Intensity))
                            return false;

                        component.Data = light;
                    } else
                    {
                        return fail(Code::InvalidField, componentPath + ".type", "未知组件类型");
                    }

                    // 组件表只接受公共字段和当前类型的专属字段。
                    for (const auto &field: entry.as_table())
                    {
                        const auto &key = field.first;
                        const bool common = key == "id" || key == "type" ||
                                            key == "location" || key == "rotation" || key == "scale";
                        const bool typeSpecific =
                                (type == "model_renderer" && key == "model_asset_id") ||
                                (type == "primitive_renderer" && key == "primitive") ||
                                (type == "point_light" && (key == "color" || key == "intensity"));

                        if (!common && !typeSpecific)
                            return fail(Code::InvalidField, componentPath + "." + key,
                                        "当前组件类型不支持此字段");
                    }

                    object.Components.push_back(std::move(component));
                }
            }

            candidate.Objects.push_back(std::move(object));
        }

        // 整份文档通过校验后，再替换调用方的输出。
        if (!ValidateSceneDocument(candidate, error))
            return false;

        outDocument = std::move(candidate);
        error = {};
        return true;
    }
}
