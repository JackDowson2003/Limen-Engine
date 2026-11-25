#include "Limen/Scene/SceneSerializer.h"

#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include <toml.hpp>

namespace
{
    using Limen::SceneFileErrorCode;

    struct TestRunner
    {
        int Failures = 0;

        bool Check(std::string_view name, bool passed)
        {
            if (!passed)
            {
                ++Failures;
                std::cerr << "FAIL: " << name << '\n';
            }
            return passed;
        }
    };

    const toml::value *Find(const toml::value &table, const char *key)
    {
        if (!table.is_table())
            return nullptr;

        const auto &fields = table.as_table();
        const auto field = fields.find(key);
        return field == fields.end() ? nullptr : &field->second;
    }

    bool HasString(const toml::value &table, const char *key, std::string_view expected)
    {
        const auto *value = Find(table, key);
        return value && value->is_string() && std::string_view(value->as_string()) == expected;
    }

    bool HasFloat(const toml::value &value, float expected)
    {
        // 写端保留 float 扩宽为 double 后的精确值。
        return value.is_floating() && value.as_floating() == static_cast<double>(expected);
    }

    bool HasFloatArray(const toml::value &table, const char *key, std::initializer_list<float> expected)
    {
        const auto *value = Find(table, key);
        if (!value || !value->is_array() || value->as_array().size() != expected.size())
            return false;

        auto component = expected.begin();
        for (const auto &element : value->as_array())
            if (!HasFloat(element, *component++))
                return false;

        return true;
    }

    bool HasTransform(const toml::value &table, const Limen::SceneTransform &transform)
    {
        const auto &position = transform.Location;
        const auto &rotation = transform.Rotation;
        const auto &scale = transform.Scale;
        return HasFloatArray(table, "location", {position.x, position.y, position.z}) &&
               HasFloatArray(table, "rotation", {rotation.x, rotation.y, rotation.z, rotation.w}) &&
               HasFloatArray(table, "scale", {scale.x, scale.y, scale.z});
    }

    Limen::SceneDocument MakeDocument()
    {
        Limen::SceneDocument document;
        document.DisplayName = "实验场景 \"A\"\n入口";
        document.Ambient = {.Color = {0.125f, 0.25f, 0.5f}, .Intensity = 1.23456789f};
        document.DirectionLight = {
            .Direction = {1.0f, -2.0f, -3.0f}, .Color = {0.5f, 0.75f, 1.0f}, .Intensity = 2.75f
        };

        Limen::SceneObjectAuthoringData first;
        first.ID = {0xaaaaaaaa'bbbb'4cccULL, 0x8ddd'eeeeeeeeeeeeULL};
        first.Label = "测试对象";
        first.Transform = {
            .Location = {5.0f, -6.0f, 7.0f},
            .Rotation = glm::quat{0.5f, -0.5f, 0.5f, -0.5f},
            .Scale = {2.0f, 3.0f, 4.0f}
        };
        first.Components.push_back({
            .ID = {1, 2},
            .LocalTransform = {.Location = {0.25f, 0.0f, -0.25f}},
            .Data = Limen::ModelRendererComponent{
                .ModelAssetID = {0x00112233'4455'6677ULL, 0x8899'aabbccddeeffULL}
            }
        });
        first.Components.push_back({
            .ID = {3, 4},
            .LocalTransform = {.Scale = {0.5f, 0.75f, 1.25f}},
            .Data = Limen::PrimitiveRendererComponent{}
        });
        first.Components.push_back({
            .ID = {5, 6},
            .LocalTransform = {.Location = {1.5f, 2.5f, -3.5f}, .Rotation = glm::quat{0.0f, 1.0f, 0.0f, 0.0f}},
            .Data = Limen::PointLightComponent{.Color = {1.0f, 0.5f, 0.25f}, .Intensity = 4.5f}
        });

        Limen::SceneObjectAuthoringData second;
        second.ID = {0xbbbbbbbb'cccc'4dddULL, 0x8eee'ffffffffffffULL};
        second.Label = "Second";
        // 组件 ID 在所属对象内唯一，同一 ID 可以出现在另一个对象上。
        second.Components.push_back(first.Components[0]);

        document.Objects.push_back(std::move(first));
        document.Objects.push_back(std::move(second));
        return document;
    }

    void TestSceneDocumentLayout(TestRunner &runner)
    {
        const auto document = MakeDocument();
        Limen::LString text("previous document");
        Limen::SceneFileError error{SceneFileErrorCode::InvalidField, "old_key", "old message"};
        if (!runner.Check("serialize full scene document", Limen::TrySerializeSceneDocument(document, text, error)))
        {
            std::cerr << error.Key << ": " << error.Message << '\n';
            return;
        }

        runner.Check("success clears previous error",
                     error.Code == SceneFileErrorCode::None && error.Key.empty() && error.Message.empty());
        Limen::LString repeated;
        runner.Check("serialize twice produces identical text",
                     Limen::TrySerializeSceneDocument(document, repeated, error) && repeated == text);

        // 用 TOML 解析器核对磁盘格式的数据形状，避免依赖缩进或表字段顺序。
        const auto parsed = toml::try_parse_str(std::string(text.data(), text.size()), toml::spec::v(1, 0, 0));
        if (!runner.Check("serialized scene is valid TOML 1.0", parsed.is_ok()))
            return;

        const auto &root = parsed.unwrap();
        const auto *version = Find(root, "schema_version");
        runner.Check("scene schema version is integer 1", version && version->is_integer() && version->as_integer() == 1);
        runner.Check("scene display name preserves UTF-8 and escaped characters",
                     HasString(root, "display_name", "实验场景 \"A\"\n入口"));

        const auto *environment = Find(root, "environment");
        if (runner.Check("scene environment is a table", environment && environment->is_table()))
        {
            const auto *ambientIntensity = Find(*environment, "ambient_intensity");
            const auto *directionalIntensity = Find(*environment, "directional_intensity");
            runner.Check("scene ambient settings preserve float precision",
                         HasFloatArray(*environment, "ambient_color", {0.125f, 0.25f, 0.5f}) &&
                         ambientIntensity && HasFloat(*ambientIntensity, document.Ambient.Intensity));
            runner.Check("scene directional settings are preserved",
                         HasFloatArray(*environment, "directional_direction", {1.0f, -2.0f, -3.0f}) &&
                         HasFloatArray(*environment, "directional_color", {0.5f, 0.75f, 1.0f}) &&
                         directionalIntensity && HasFloat(*directionalIntensity, 2.75f));
        }

        const auto *objects = Find(root, "objects");
        if (!runner.Check("scene holds two objects", objects && objects->is_array_of_tables() && objects->as_array().size() == 2))
            return;

        const auto &first = objects->as_array()[0];
        const auto &second = objects->as_array()[1];
        runner.Check("object IDs, UTF-8 label and order are preserved",
                     HasString(first, "id", "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee") &&
                     HasString(first, "label", "测试对象") &&
                     HasString(second, "id", "bbbbbbbb-cccc-4ddd-8eee-ffffffffffff"));
        runner.Check("object root TRS preserves location, XYZW rotation and scale",
                     HasTransform(first, document.Objects[0].Transform) &&
                     HasTransform(second, document.Objects[1].Transform));

        const auto *components = Find(first, "components");
        if (!runner.Check("first object holds three component tables",
                          components && components->is_array_of_tables() && components->as_array().size() == 3))
            return;

        const auto &model = components->as_array()[0];
        const auto &cube = components->as_array()[1];
        const auto &pointLight = components->as_array()[2];
        const auto &sourceComponents = document.Objects[0].Components;
        runner.Check("model component preserves ID, asset reference and local TRS",
                     HasString(model, "id", "00000000-0000-0001-0000-000000000002") &&
                     HasString(model, "type", "model_renderer") &&
                     HasString(model, "model_asset_id", "00112233-4455-6677-8899-aabbccddeeff") &&
                     HasTransform(model, sourceComponents[0].LocalTransform));
        runner.Check("primitive component preserves ID, cube type and local TRS",
                     HasString(cube, "id", "00000000-0000-0003-0000-000000000004") &&
                     HasString(cube, "type", "primitive_renderer") && HasString(cube, "primitive", "cube") &&
                     HasTransform(cube, sourceComponents[1].LocalTransform));
        const auto *intensity = Find(pointLight, "intensity");
        runner.Check("point light component preserves ID, settings and local TRS",
                     HasString(pointLight, "id", "00000000-0000-0005-0000-000000000006") &&
                     HasString(pointLight, "type", "point_light") &&
                     HasFloatArray(pointLight, "color", {1.0f, 0.5f, 0.25f}) && intensity && HasFloat(*intensity, 4.5f) &&
                     HasTransform(pointLight, sourceComponents[2].LocalTransform));

        const auto *secondComponents = Find(second, "components");
        runner.Check("component ID can be reused by a different object",
                     secondComponents && secondComponents->is_array_of_tables() && secondComponents->as_array().size() == 1 &&
                     HasString(secondComponents->as_array()[0], "id", "00000000-0000-0001-0000-000000000002"));
    }

    void TestEmptySceneDocument(TestRunner &runner)
    {
        Limen::SceneDocument document;
        document.DisplayName = "Empty";
        Limen::LString text;
        Limen::SceneFileError error;
        if (!runner.Check("serialize empty scene document", Limen::TrySerializeSceneDocument(document, text, error)))
            return;

        const auto parsed = toml::try_parse_str(std::string(text.data(), text.size()), toml::spec::v(1, 0, 0));
        if (!runner.Check("serialized empty scene is valid TOML 1.0", parsed.is_ok()))
            return;

        const auto *objects = Find(parsed.unwrap(), "objects");
        runner.Check("empty scene writes an empty object array", objects && objects->is_array() && objects->as_array().empty());

        auto restored = MakeDocument();
        runner.Check("reading an empty scene replaces the previous objects",
                     Limen::TryDeserializeSceneDocument(text, restored, error) &&
                     restored.Objects.empty() && restored.DisplayName == "Empty");
    }

    void CheckRejection(TestRunner &runner, std::string_view name, const Limen::SceneDocument &document,
                        SceneFileErrorCode expectedCode, std::string_view expectedKey = {})
    {
        Limen::LString text("previous document");
        Limen::SceneFileError error;
        const bool success = Limen::TrySerializeSceneDocument(document, text, error);
        if (!runner.Check(name, !success && text == "previous document" &&
                               error.Code == expectedCode && !error.Key.empty() && !error.Message.empty() &&
                               (expectedKey.empty() || std::string_view(error.Key) == expectedKey)))
        {
            std::cerr << "  success=" << success << ", output unchanged=" << (text == "previous document")
                      << ", error code=" << static_cast<int>(error.Code) << ", key=" << error.Key
                      << ", message=" << error.Message << '\n';
        }
    }

    void TestInvalidSceneDocuments(TestRunner &runner)
    {
        const auto document = MakeDocument();
        auto invalid = document;
        invalid.SchemaVersion = Limen::SceneDocument::CurrentSchemaVersion + 1;
        CheckRejection(runner, "unsupported version preserves existing output", invalid,
                       SceneFileErrorCode::UnsupportedSchema, "schema_version");

        invalid = document;
        invalid.Objects[1].ID = invalid.Objects[0].ID;
        CheckRejection(runner, "duplicate object ID preserves existing output", invalid,
                       SceneFileErrorCode::DuplicateID, "objects[1].id");

        invalid = document;
        invalid.Objects[0].Components[1].ID = invalid.Objects[0].Components[0].ID;
        CheckRejection(runner, "duplicate component ID in one object preserves existing output", invalid,
                       SceneFileErrorCode::DuplicateID);

        // CPU 调用方未初始化日志器，非法组件也必须走结构化错误路径。
        invalid = document;
        invalid.Objects[0].Components[0].Data = Limen::ModelRendererComponent{};
        CheckRejection(runner, "model component with empty asset ID is rejected", invalid,
                       SceneFileErrorCode::InvalidField);

        invalid = document;
        invalid.Objects[0].Transform.Rotation = glm::quat{0.0f, 0.0f, 0.0f, 0.0f};
        CheckRejection(runner, "zero root quaternion preserves existing output", invalid, SceneFileErrorCode::InvalidField);

        invalid = document;
        invalid.Objects[0].Components[0].LocalTransform.Rotation = glm::quat{0.0f, 0.0f, 0.0f, 0.0f};
        CheckRejection(runner, "zero local quaternion preserves existing output", invalid, SceneFileErrorCode::InvalidField);

        invalid = document;
        invalid.Ambient.Intensity = std::numeric_limits<float>::infinity();
        CheckRejection(runner, "nonfinite ambient intensity preserves existing output", invalid, SceneFileErrorCode::InvalidField);

        invalid = document;
        invalid.DirectionLight.Direction = {0.0f, 0.0f, 0.0f};
        CheckRejection(runner, "zero directional light direction preserves existing output", invalid, SceneFileErrorCode::InvalidField);
    }

    void TestSceneDocumentRoundtrip(TestRunner &runner)
    {
        Limen::LString text;
        Limen::SceneFileError error;
        if (!runner.Check("serialize scene for roundtrip", Limen::TrySerializeSceneDocument(MakeDocument(), text, error)))
            return;

        Limen::SceneDocument restored;
        error = {SceneFileErrorCode::InvalidField, "old_key", "old message"};
        if (!runner.Check("read back serialized scene", Limen::TryDeserializeSceneDocument(text, restored, error)))
        {
            std::cerr << error.Key << ": " << error.Message << '\n';
            return;
        }

        runner.Check("reading succeeds and clears previous error",
                     error.Code == SceneFileErrorCode::None && error.Key.empty() && error.Message.empty());

        // 写端格式测试已逐项核对持久字段，重新编码可以核对读端是否完整恢复了它们。
        Limen::LString repeated;
        runner.Check("read and reserialize preserves the canonical scene text",
                     Limen::TrySerializeSceneDocument(restored, repeated, error) && repeated == text);
    }

    constexpr char kSceneWithBareObject[] = R"(
schema_version = 1
display_name = "Manual scene"

[environment]
ambient_color = [1, 0.5, 0.25]
ambient_intensity = 1
directional_direction = [1, -1, 0]
directional_color = [1, 1, 1]
directional_intensity = 1

[[objects]]
id = "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"
label = "Bare Object"
location = [0, 1.25, 2]
rotation = [0, 0, 3, 4]
scale = [1, 1, 1]
)";

    constexpr char kComponentFields[] = R"(
[[objects.components]]
id = "00000000-0000-0001-0000-000000000002"
location = [0, 0, 0]
rotation = [0, 0, 0, 1]
scale = [1, 1, 1]
)";

    void TestSceneObjectWithoutComponents(TestRunner &runner)
    {
        Limen::SceneDocument restored;
        Limen::SceneFileError error;
        if (!runner.Check("read scene object with omitted components",
                          Limen::TryDeserializeSceneDocument(kSceneWithBareObject, restored, error)))
        {
            std::cerr << error.Key << ": " << error.Message << '\n';
            return;
        }

        if (!runner.Check("scene retains object without components", restored.Objects.getSize() == 1))
            return;

        const auto &object = restored.Objects[0];
        runner.Check("omitted components become empty", object.Components.empty());
        runner.Check("read mixed integers and floats in a position",
                     object.Transform.Location == glm::vec3{0.0f, 1.25f, 2.0f});
        const auto &rotation = object.Transform.Rotation;
        runner.Check("read normalizes non-unit XYZW quaternion",
                     rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.6f && rotation.w == 0.8f);
    }

    void TestInvalidSceneText(TestRunner &runner)
    {
        const auto original = MakeDocument();
        Limen::LString before;
        Limen::SceneFileError originalError;
        if (!runner.Check("prepare previous document for read failures",
                          Limen::TrySerializeSceneDocument(original, before, originalError)))
            return;

        const auto check = [&](std::string_view name, std::string_view text,
                               SceneFileErrorCode expectedCode, std::string_view expectedKey)
        {
            auto output = original;
            Limen::SceneFileError error;
            const bool success = Limen::TryDeserializeSceneDocument(text, output, error);

            Limen::LString after;
            Limen::SceneFileError outputError;
            const bool unchanged = Limen::TrySerializeSceneDocument(output, after, outputError) && after == before;
            if (!runner.Check(name, !success && unchanged && error.Code == expectedCode &&
                                   std::string_view(error.Key) == expectedKey && !error.Message.empty()))
            {
                std::cerr << "  success=" << success << ", output unchanged=" << unchanged
                          << ", error code=" << static_cast<int>(error.Code) << ", key=" << error.Key
                          << ", message=" << error.Message << '\n';
            }
        };

        const std::string bareObject = kSceneWithBareObject;
        const std::string model = std::string(kComponentFields) +
            "type = \"model_renderer\"\nmodel_asset_id = \"00112233-4455-6677-8899-aabbccddeeff\"\n";
        check("malformed TOML preserves previous document", "schema_version = ]", SceneFileErrorCode::InvalidToml, "");
        check("UTF-8 BOM preserves previous document", "\xEF\xBB\xBF" + bareObject, SceneFileErrorCode::InvalidToml, "");
        check("unknown object field preserves previous document", bareObject + "foo = true\n",
              SceneFileErrorCode::InvalidField, "objects[0].foo");
        check("field for another component type preserves previous document", bareObject + model + "primitive = \"cube\"\n",
              SceneFileErrorCode::InvalidField, "objects[0].components[0].primitive");
        check("unknown component type preserves previous document",
              bareObject + kComponentFields + "type = \"unknown\"\n",
              SceneFileErrorCode::InvalidField, "objects[0].components[0].type");
        check("duplicate component ID preserves previous document", bareObject + model + model,
              SceneFileErrorCode::DuplicateID, "objects[0].components[1].id");
    }
}

int main()
{
    TestRunner runner;
    TestSceneDocumentLayout(runner);
    TestEmptySceneDocument(runner);
    TestInvalidSceneDocuments(runner);
    TestSceneDocumentRoundtrip(runner);
    TestSceneObjectWithoutComponents(runner);
    TestInvalidSceneText(runner);

    if (runner.Failures == 0)
        std::cout << "Scene document TOML serialization: PASS\n";
    return runner.Failures == 0 ? 0 : 1;
}
