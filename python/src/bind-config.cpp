//------------------------Description------------------------
// Python bindings for optimizer configuration, catalog, and validation APIs.

// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"
#include "python-config.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/stl.h>

#include "component-catalog.h"
#include "config-codec-json.h"
#include "config-editor.h"
#include "config-validator.h"
#include "evoflow-constant.h"
#include "parameter-template.h"

namespace py = pybind11;
using namespace EvoFlow;

namespace EvoFlow
{
    namespace Python
    {
        Config::Config() : _state(std::make_shared<ConfigState>()) {}

        Config::Config(FullConfig value, std::string sourceVersion)
            : _state(std::make_shared<ConfigState>())
        {
            _state->config = std::move(value);
            _state->sourceVersion = sourceVersion.empty()
                ? std::string("v3.1") : std::move(sourceVersion);
        }

        FullConfig Config::snapshot() const { return _state->config; }
        const FullConfig& Config::value() const { return _state->config; }
        std::shared_ptr<ConfigState> Config::sharedState() const { return _state; }
    }
}

namespace
{
    using ConfigState = Python::ConfigState;

    struct ConfiguredComponent
    {
        std::string category;
        std::string tag;
        std::vector<double> parameters;
    };

    bool is_portable_token(const std::string& value, bool allowEmpty = false)
    {
        if (value.empty()) return allowEmpty;
        for (unsigned char ch : value)
            if (ch < 0x20 || std::isspace(ch) || ch == '"' || ch == '\\')
                return false;
        return true;
    }

    std::string require_token(const std::string& value, const char* argument,
                              bool allowEmpty = false)
    {
        if (!is_portable_token(value, allowEmpty))
            throw py::value_error(
                std::string(argument)
                + (allowEmpty
                    ? " must be empty or a whitespace-free config token"
                    : " must be a non-empty, whitespace-free config token"));
        return value;
    }

    std::vector<double> parameters_from_python(const py::object& value,
                                                const char* argument = "parameters")
    {
        if (value.is_none()) return {};
        std::vector<double> result;
        try
        {
            for (const py::handle item : py::reinterpret_borrow<py::iterable>(value))
            {
                if (item.is_none()) result.push_back(EMPTYVALUE);
                else result.push_back(py::cast<double>(item));
            }
        }
        catch (const py::cast_error&)
        {
            throw py::type_error(
                std::string(argument) + " must be an iterable of numbers or None");
        }
        catch (const py::error_already_set&)
        {
            throw py::type_error(
                std::string(argument) + " must be an iterable of numbers or None");
        }
        return result;
    }

    py::list parameters_to_python(const std::vector<double>& values)
    {
        py::list result;
        for (double value : values)
        {
            if (is_empty(value)) result.append(py::none());
            else result.append(value);
        }
        return result;
    }

    py::object limit_to_python(int value)
    {
        if (value < 0) return py::none();
        return py::int_(value);
    }

    void set_limit(int& target, const py::object& value, const char* argument)
    {
        if (value.is_none())
        {
            target = -1;
            return;
        }
        int parsed;
        try { parsed = py::cast<int>(value); }
        catch (const py::cast_error&)
        {
            throw py::type_error(std::string(argument) + " must be an integer or None");
        }
        if (parsed < 0)
            throw py::value_error(std::string(argument) + " must be non-negative or None");
        target = parsed;
    }

    std::string require_version(const py::object& requested,
                                const std::string& fallback)
    {
        const std::string version = requested.is_none()
            ? fallback : py::cast<std::string>(requested);
        if (version != "v3.1" && version != "v3.2")
            throw py::value_error("version must be 'v3.1' or 'v3.2'");
        return version;
    }

    std::string canonical_json(const FullConfig& config)
    {
        return ConfigCodecV32::encode(config);
    }

    std::string encode_config(const FullConfig& config, const std::string& version)
    {
        if (version == "v3.1") return ConfigCodecV31::encode(config);
        if (version == "v3.2") return ConfigCodecV32::encode(config);
        throw py::value_error("version must be 'v3.1' or 'v3.2'");
    }

    Python::Config config_from_text(const std::string& text)
    {
        const App::ConfigDraftResult draft = App::loadConfigDraft(text);
        if (!draft.ok) throw py::value_error(draft.error);
        return Python::Config(
            ConfigCodecV32::decode(draft.canonicalJson), draft.sourceVersion);
    }

    Python::Config minimal_config()
    {
        const App::ConfigDraftResult draft = App::createConfigDraft();
        if (!draft.ok) throw std::runtime_error(draft.error);
        return Python::Config(
            ConfigCodecV32::decode(draft.canonicalJson), draft.sourceVersion);
    }

    std::filesystem::path filesystem_path(const py::object& value)
    {
        const py::object native = py::module_::import("os").attr("fspath")(value);
#ifdef _WIN32
        if (py::isinstance<py::str>(native))
            return std::filesystem::path(py::cast<std::wstring>(native));
#endif
        return std::filesystem::u8path(py::cast<std::string>(native));
    }

    std::string read_text_file(const py::object& pathObject)
    {
        const std::filesystem::path path = filesystem_path(pathObject);
        std::ifstream input(path, std::ios::binary);
        if (!input) throw py::value_error("cannot read config file");
        return std::string(
            std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    void write_text_file(const py::object& pathObject, const std::string& text)
    {
        const std::filesystem::path path = filesystem_path(pathObject);
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        if (!output) throw py::value_error("cannot write config file");
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!output) throw py::value_error("failed while writing config file");
    }

    ModuleType workflow_category(const std::string& category)
    {
        const ModuleType type = codec::keywordType(category);
        if (type == ModuleType::T_default)
            throw py::value_error(
                "workflow component category must be topology, strategy, generator, "
                "selector, evaluator, or repair");
        return type;
    }

    ModuleType catalog_category(const std::string& category)
    {
        const ModuleType type = App::categoryOfKey(category);
        if (type == ModuleType::T_default)
            throw py::value_error("unknown component category: " + category);
        return type;
    }

    ConfiguredComponent configured_component(const ComponentConfig& source)
    {
        return { codec::typeKeyword(source.c_type), source.tag, source.para };
    }

    ConfiguredComponent configured_component(const std::string& category,
                                              const std::string& tag,
                                              const std::vector<double>& parameters)
    {
        return { category, tag, parameters };
    }

    std::size_t index_from_key(const py::object& key, std::size_t size,
                               const std::vector<std::string>& tags,
                               const char* kind)
    {
        if (py::isinstance<py::str>(key))
        {
            const std::string tag = py::cast<std::string>(key);
            const auto found = std::find(tags.begin(), tags.end(), tag);
            if (found == tags.end())
                throw py::value_error(std::string("unknown ") + kind + ": " + tag);
            return static_cast<std::size_t>(found - tags.begin());
        }
        int index;
        try { index = py::cast<int>(key); }
        catch (const py::cast_error&)
        {
            throw py::type_error(std::string(kind) + " key must be an index or tag");
        }
        if (index < 0 || static_cast<std::size_t>(index) >= size)
            throw py::index_error(std::string(kind) + " index out of range");
        return static_cast<std::size_t>(index);
    }

    std::vector<std::string> workflow_tags(const ConfigState& state)
    {
        std::vector<std::string> result;
        result.reserve(state.config.workflows.size());
        for (const WorkflowConfig& value : state.config.workflows)
            result.push_back(value.tag);
        return result;
    }

    std::vector<std::string> subpopulation_tags(const ConfigState& state)
    {
        std::vector<std::string> result;
        result.reserve(state.config.optimizer.subpopulations.size());
        for (const SubpopulationConfig& value : state.config.optimizer.subpopulations)
            result.push_back(value.tag);
        return result;
    }

    class WorkflowHandle
    {
    public:
        WorkflowHandle(std::shared_ptr<ConfigState> state, std::size_t index)
            : _state(std::move(state)), _index(index),
              _generation(_state->workflowGeneration) {}

        std::string tag() const { return checked().tag; }
        void set_tag(const std::string& value)
        {
            checked().tag = require_token(value, "workflow tag");
        }

        ConfiguredComponent initializer() const
        {
            const WorkflowConfig& value = checked();
            return configured_component("initializer", value.ini_tag, value.ini_para);
        }

        void set_initializer(const std::string& tag, const py::object& parameters)
        {
            WorkflowConfig& value = checked();
            value.ini_tag = require_token(tag, "initializer tag");
            value.ini_para = parameters_from_python(parameters);
        }

        std::vector<ConfiguredComponent> components() const
        {
            std::vector<ConfiguredComponent> result;
            for (const ComponentConfig& value : checked().components)
                result.push_back(configured_component(value));
            return result;
        }

        ConfiguredComponent component(int index) const
        {
            const WorkflowConfig& value = checked();
            require_component_index(value, index);
            return configured_component(value.components[static_cast<std::size_t>(index)]);
        }

        int add_component(const std::string& category, const std::string& tag,
                          const py::object& parameters)
        {
            WorkflowConfig& value = checked();
            value.components.emplace_back(
                workflow_category(category), require_token(tag, "component tag"),
                parameters_from_python(parameters));
            return static_cast<int>(value.components.size() - 1);
        }

        void replace_component(int index, const std::string& category,
                               const std::string& tag, const py::object& parameters)
        {
            WorkflowConfig& value = checked();
            require_component_index(value, index);
            value.components[static_cast<std::size_t>(index)] = ComponentConfig(
                workflow_category(category), require_token(tag, "component tag"),
                parameters_from_python(parameters));
        }

        void remove_component(int index)
        {
            WorkflowConfig& value = checked();
            require_component_index(value, index);
            value.components.erase(value.components.begin() + index);
        }

        void move_component(int source, int target)
        {
            WorkflowConfig& value = checked();
            require_component_index(value, source);
            require_component_index(value, target);
            if (source == target) return;
            ComponentConfig moving = std::move(value.components[static_cast<std::size_t>(source)]);
            value.components.erase(value.components.begin() + source);
            value.components.insert(value.components.begin() + target, std::move(moving));
        }

        void clear_components() { checked().components.clear(); }

        App::ConfigFlowCheckResult check_component(int index) const
        {
            const WorkflowConfig& value = checked();
            require_component_index(value, index);
            return App::checkConfigFlowConnection(
                canonical_json(_state->config), static_cast<int>(_index), index);
        }

        int add_topology(const std::string& tag, const py::object& parameters)
        { return add_component("topology", tag, parameters); }
        int add_strategy(const std::string& tag, const py::object& parameters)
        { return add_component("strategy", tag, parameters); }
        int add_generator(const std::string& tag, const py::object& parameters)
        { return add_component("generator", tag, parameters); }
        int add_selector(const std::string& tag, const py::object& parameters)
        { return add_component("selector", tag, parameters); }
        int add_evaluator(const std::string& tag, const py::object& parameters)
        { return add_component("evaluator", tag, parameters); }
        int add_repair(const std::string& tag, const py::object& parameters)
        { return add_component("repair", tag, parameters); }

    private:
        WorkflowConfig& checked() const
        {
            if (_generation != _state->workflowGeneration
                || _index >= _state->config.workflows.size())
                throw std::runtime_error(
                    "workflow handle is invalid after a structural workflow change");
            return _state->config.workflows[_index];
        }

        static void require_component_index(const WorkflowConfig& value, int index)
        {
            if (index < 0 || static_cast<std::size_t>(index) >= value.components.size())
                throw py::index_error("component index out of range");
        }

        std::shared_ptr<ConfigState> _state;
        std::size_t _index;
        std::size_t _generation;
    };

    class SubpopulationHandle
    {
    public:
        SubpopulationHandle(std::shared_ptr<ConfigState> state, std::size_t index)
            : _state(std::move(state)), _index(index),
              _generation(_state->subpopulationGeneration) {}

        std::string tag() const { return checked().tag; }
        void set_tag(const std::string& value)
        { checked().tag = require_token(value, "subpopulation tag"); }
        int size() const { return checked().size; }
        void set_size(int value) { checked().size = value; }
        std::string workflow() const { return checked().workflow_tag; }
        void set_workflow(const std::string& value)
        { checked().workflow_tag = require_token(value, "workflow reference"); }

        py::object max_fes() const { return limit_to_python(checked().terminate_conditions[0]); }
        py::object max_convergence() const { return limit_to_python(checked().terminate_conditions[1]); }
        py::object max_time() const { return limit_to_python(checked().terminate_conditions[2]); }
        void set_max_fes(const py::object& value)
        { set_limit(checked().terminate_conditions[0], value, "max_fes"); }
        void set_max_convergence(const py::object& value)
        { set_limit(checked().terminate_conditions[1], value, "max_convergence"); }
        void set_max_time(const py::object& value)
        { set_limit(checked().terminate_conditions[2], value, "max_time"); }

        ConfiguredComponent archive() const
        {
            const SubpopulationConfig& value = checked();
            return configured_component("archive", value.archive_tag, value.archive_para);
        }

        void set_archive(const std::string& tag, const py::object& parameters)
        {
            SubpopulationConfig& value = checked();
            value.archive_tag = require_token(tag, "archive tag");
            value.archive_para = parameters_from_python(parameters);
        }

    private:
        SubpopulationConfig& checked() const
        {
            if (_generation != _state->subpopulationGeneration
                || _index >= _state->config.optimizer.subpopulations.size())
                throw std::runtime_error(
                    "subpopulation handle is invalid after a structural subpopulation change");
            return _state->config.optimizer.subpopulations[_index];
        }

        std::shared_ptr<ConfigState> _state;
        std::size_t _index;
        std::size_t _generation;
    };

    WorkflowHandle config_workflow(Python::Config& self, const py::object& key)
    {
        const std::shared_ptr<ConfigState> state = self.sharedState();
        return WorkflowHandle(state, index_from_key(
            key, state->config.workflows.size(), workflow_tags(*state), "workflow"));
    }

    SubpopulationHandle config_subpopulation(Python::Config& self, const py::object& key)
    {
        const std::shared_ptr<ConfigState> state = self.sharedState();
        return SubpopulationHandle(state, index_from_key(
            key, state->config.optimizer.subpopulations.size(),
            subpopulation_tags(*state), "subpopulation"));
    }

    App::ResolvedParameterSchema resolved_schema(
        const std::string& category, const std::string& tag,
        const py::object& parameters)
    {
        if (category != "initializer") catalog_category(category);
        const App::ResolvedParameterSchema result = App::resolveParameterSchema(
            category, require_token(tag, "component tag"),
            parameters_from_python(parameters));
        if (!result.ok) throw py::value_error(result.error);
        return result;
    }

    std::vector<App::RequirementItem> requirements(
        const std::string& category, const std::string& tag,
        const py::object& parameters, bool provisions)
    {
        resolved_schema(category, tag, parameters);
        if (category == "initializer") return {};
        const ModuleType type = catalog_category(category);
        const std::vector<double> values = parameters_from_python(parameters);
        return provisions
            ? App::queryProvisionsAt(type, tag, values)
            : App::queryRequirementsAt(type, tag, values);
    }
}

void bind_config(py::module_& module)
{
    py::enum_<ParamKind>(module, "ParameterKind")
        .value("ENUM", ParamKind::Enum)
        .value("INT", ParamKind::Int)
        .value("REAL", ParamKind::Real);

    py::class_<ConfiguredComponent>(module, "ConfiguredComponent")
        .def_readonly("category", &ConfiguredComponent::category)
        .def_readonly("tag", &ConfiguredComponent::tag)
        .def_property_readonly("parameters", [](const ConfiguredComponent& self) {
            return parameters_to_python(self.parameters);
        });

    py::class_<App::ParamMeta>(module, "ParameterInfo")
        .def_readonly("name", &App::ParamMeta::name)
        .def_property_readonly("kind", [](const App::ParamMeta& self) {
            return static_cast<ParamKind>(self.kind);
        })
        .def_readonly("low", &App::ParamMeta::low)
        .def_readonly("high", &App::ParamMeta::high)
        .def_readonly("recommended_low", &App::ParamMeta::recLow)
        .def_readonly("recommended_high", &App::ParamMeta::recHigh)
        .def_readonly("allow_empty", &App::ParamMeta::allowEmpty);

    py::class_<App::ComponentMeta>(module, "ComponentInfo")
        .def_readonly("tag", &App::ComponentMeta::tag)
        .def_readonly("category", &App::ComponentMeta::categoryKey)
        .def_readonly("parameters", &App::ComponentMeta::params);

    py::class_<App::ResolvedParameterSchema>(module, "ParameterSchema")
        .def_readonly("ok", &App::ResolvedParameterSchema::ok)
        .def_readonly("error", &App::ResolvedParameterSchema::error)
        .def_readonly("parameters", &App::ResolvedParameterSchema::params)
        .def_property_readonly("defaults", [](const App::ResolvedParameterSchema& self) {
            return parameters_to_python(self.defaults);
        });

    py::class_<App::RequirementItem>(module, "RequirementInfo")
        .def_readonly("item", &App::RequirementItem::item)
        .def_readonly("number", &App::RequirementItem::number)
        .def_property_readonly("owner_category", [](const App::RequirementItem& self) {
            return App::categoryKeyOf(static_cast<ModuleType>(self.owner));
        })
        .def_readonly("detail", &App::RequirementItem::detail);

    py::class_<App::ConfigIssue>(module, "ConfigIssue")
        .def_readonly("file", &App::ConfigIssue::file)
        .def_readonly("scope", &App::ConfigIssue::scope)
        .def_readonly("scope_type", &App::ConfigIssue::scopeType)
        .def_readonly("scope_index", &App::ConfigIssue::scopeIndex)
        .def_readonly("component", &App::ConfigIssue::component)
        .def_readonly("category", &App::ConfigIssue::category)
        .def_readonly("component_index", &App::ConfigIssue::componentIndex)
        .def_readonly("reason", &App::ConfigIssue::reason)
        .def_readonly("message", &App::ConfigIssue::message)
        .def_readonly("severity", &App::ConfigIssue::severity)
        .def_readonly("is_error", &App::ConfigIssue::isError);

    py::class_<App::FileReport>(module, "ConfigFileReport")
        .def_readonly("file", &App::FileReport::file)
        .def_readonly("ok", &App::FileReport::ok)
        .def_readonly("error_count", &App::FileReport::errorCount)
        .def_readonly("warning_count", &App::FileReport::warningCount)
        .def_readonly("info_count", &App::FileReport::infoCount)
        .def_readonly("status", &App::FileReport::status)
        .def_readonly("load_error", &App::FileReport::loadError);

    py::class_<App::ValidateReport>(module, "ValidationReport")
        .def_property_readonly("ok", [](const App::ValidateReport& self) {
            return self.fatal.empty() && self.failed == 0;
        })
        .def_readonly("scanned", &App::ValidateReport::scanned)
        .def_readonly("passed", &App::ValidateReport::passed)
        .def_readonly("failed", &App::ValidateReport::failed)
        .def_readonly("error_files", &App::ValidateReport::errorFiles)
        .def_readonly("warning_files", &App::ValidateReport::warningFiles)
        .def_readonly("info_files", &App::ValidateReport::infoFiles)
        .def_readonly("pass_files", &App::ValidateReport::passFiles)
        .def_readonly("status", &App::ValidateReport::status)
        .def_readonly("deep", &App::ValidateReport::deepBuild)
        .def_readonly("files", &App::ValidateReport::files)
        .def_readonly("issues", &App::ValidateReport::issues)
        .def_readonly("fatal", &App::ValidateReport::fatal);

    py::class_<App::ConfigFlowCheckResult>(module, "ConfigFlowCheck")
        .def_readonly("ok", &App::ConfigFlowCheckResult::ok)
        .def_readonly("allowed", &App::ConfigFlowCheckResult::allowed)
        .def_readonly("error", &App::ConfigFlowCheckResult::error)
        .def_readonly("state", &App::ConfigFlowCheckResult::state)
        .def_readonly("reason", &App::ConfigFlowCheckResult::reason)
        .def_readonly("message", &App::ConfigFlowCheckResult::message)
        .def_readonly("errors", &App::ConfigFlowCheckResult::errors)
        .def_readonly("warnings", &App::ConfigFlowCheckResult::warnings)
        .def_readonly("infos", &App::ConfigFlowCheckResult::infos);

    py::class_<WorkflowHandle>(module, "Workflow")
        .def_property("tag", &WorkflowHandle::tag, &WorkflowHandle::set_tag)
        .def_property_readonly("initializer", &WorkflowHandle::initializer)
        .def("set_initializer", &WorkflowHandle::set_initializer,
             py::arg("tag"), py::arg("parameters") = py::tuple())
        .def_property_readonly("components", &WorkflowHandle::components)
        .def("component", &WorkflowHandle::component, py::arg("index"))
        .def("add_component", &WorkflowHandle::add_component,
             py::arg("category"), py::arg("tag"), py::arg("parameters") = py::tuple())
        .def("replace_component", &WorkflowHandle::replace_component,
             py::arg("index"), py::arg("category"), py::arg("tag"),
             py::arg("parameters") = py::tuple())
        .def("remove_component", &WorkflowHandle::remove_component, py::arg("index"))
        .def("move_component", &WorkflowHandle::move_component,
             py::arg("source"), py::arg("target"))
        .def("clear_components", &WorkflowHandle::clear_components)
        .def("check_component", &WorkflowHandle::check_component, py::arg("index"))
        .def("add_topology", &WorkflowHandle::add_topology,
             py::arg("tag"), py::arg("parameters") = py::tuple())
        .def("add_strategy", &WorkflowHandle::add_strategy,
             py::arg("tag"), py::arg("parameters") = py::tuple())
        .def("add_generator", &WorkflowHandle::add_generator,
             py::arg("tag"), py::arg("parameters") = py::tuple())
        .def("add_selector", &WorkflowHandle::add_selector,
             py::arg("tag"), py::arg("parameters") = py::tuple())
        .def("add_evaluator", &WorkflowHandle::add_evaluator,
             py::arg("tag"), py::arg("parameters") = py::tuple())
        .def("add_repair", &WorkflowHandle::add_repair,
             py::arg("tag"), py::arg("parameters") = py::tuple());

    py::class_<SubpopulationHandle>(module, "Subpopulation")
        .def_property("tag", &SubpopulationHandle::tag, &SubpopulationHandle::set_tag)
        .def_property("size", &SubpopulationHandle::size, &SubpopulationHandle::set_size)
        .def_property("workflow", &SubpopulationHandle::workflow,
                      &SubpopulationHandle::set_workflow)
        .def_property("max_fes", &SubpopulationHandle::max_fes,
                      &SubpopulationHandle::set_max_fes)
        .def_property("max_convergence", &SubpopulationHandle::max_convergence,
                      &SubpopulationHandle::set_max_convergence)
        .def_property("max_time", &SubpopulationHandle::max_time,
                      &SubpopulationHandle::set_max_time)
        .def_property_readonly("archive", &SubpopulationHandle::archive)
        .def("set_archive", &SubpopulationHandle::set_archive,
             py::arg("tag"), py::arg("parameters") = py::tuple());

    py::class_<Python::Config>(module, "Config")
        .def(py::init<>())
        .def_static("minimal", &minimal_config)
        .def_static("from_text", &config_from_text, py::arg("text"))
        .def_static("load", [](const py::object& path) {
            return config_from_text(read_text_file(path));
        }, py::arg("path"))
        .def("copy", [](const Python::Config& self) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            Python::Config result(self.snapshot(), state->sourceVersion);
            result.sharedState()->config.optimizer.logger_console_echo =
                state->config.optimizer.logger_console_echo;
            return result;
        })
        .def_property_readonly("source_version", [](const Python::Config& self) {
            return self.sharedState()->sourceVersion;
        })
        .def_property_readonly("canonical_json", [](const Python::Config& self) {
            return canonical_json(self.value());
        })
        .def("to_text", [](const Python::Config& self, const py::object& version) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            return encode_config(
                state->config, require_version(version, state->sourceVersion));
        }, py::arg("version") = py::none())
        .def("save", [](const Python::Config& self, const py::object& path,
                         const py::object& version) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            write_text_file(path, encode_config(
                state->config, require_version(version, state->sourceVersion)));
        }, py::arg("path"), py::arg("version") = py::none())
        .def("validate", [](const Python::Config& self, bool deep) {
            return App::validateConfigText(
                canonical_json(self.value()), deep, "(memory)");
        }, py::arg("deep") = false)
        .def_property("name", [](const Python::Config& self) {
            return self.value().optimizer.name;
        }, [](Python::Config& self, const std::string& value) {
            self.sharedState()->config.optimizer.name = require_token(value, "name");
        })
        .def_property("tag", [](const Python::Config& self) {
            return self.value().optimizer.tag;
        }, [](Python::Config& self, const std::string& value) {
            self.sharedState()->config.optimizer.tag = require_token(value, "tag", true);
        })
        .def_property("max_fes", [](const Python::Config& self) {
            return limit_to_python(self.value().optimizer.terminate_conditions[0]);
        }, [](Python::Config& self, const py::object& value) {
            set_limit(self.sharedState()->config.optimizer.terminate_conditions[0],
                      value, "max_fes");
        })
        .def_property("max_convergence", [](const Python::Config& self) {
            return limit_to_python(self.value().optimizer.terminate_conditions[1]);
        }, [](Python::Config& self, const py::object& value) {
            set_limit(self.sharedState()->config.optimizer.terminate_conditions[1],
                      value, "max_convergence");
        })
        .def_property("max_time", [](const Python::Config& self) {
            return limit_to_python(self.value().optimizer.terminate_conditions[2]);
        }, [](Python::Config& self, const py::object& value) {
            set_limit(self.sharedState()->config.optimizer.terminate_conditions[2],
                      value, "max_time");
        })
        .def_property("log_results", [](const Python::Config& self) {
            return self.value().optimizer.logger_full_result;
        }, [](Python::Config& self, bool value) {
            self.sharedState()->config.optimizer.logger_full_result = value;
        })
        .def_property("log_process", [](const Python::Config& self) {
            return self.value().optimizer.logger_process;
        }, [](Python::Config& self, bool value) {
            self.sharedState()->config.optimizer.logger_process = value;
        })
        .def_property("log_full_process", [](const Python::Config& self) {
            return self.value().optimizer.logger_full_process;
        }, [](Python::Config& self, bool value) {
            self.sharedState()->config.optimizer.logger_full_process = value;
        })
        .def_property("log_swarm", [](const Python::Config& self) {
            return self.value().optimizer.logger_swarm;
        }, [](Python::Config& self, bool value) {
            self.sharedState()->config.optimizer.logger_swarm = value;
        })
        .def_property("console_echo", [](const Python::Config& self) {
            return self.value().optimizer.logger_console_echo;
        }, [](Python::Config& self, bool value) {
            self.sharedState()->config.optimizer.logger_console_echo = value;
        })
        .def_property_readonly("manager", [](const Python::Config& self) {
            const CooperationConfig& value = self.value().optimizer.cooperation;
            return configured_component("manager", value.manager_tag, value.manager_para);
        })
        .def("set_manager", [](Python::Config& self, const std::string& tag,
                               const py::object& parameters) {
            CooperationConfig& value = self.sharedState()->config.optimizer.cooperation;
            value.manager_tag = require_token(tag, "manager tag");
            value.manager_para = parameters_from_python(parameters);
        }, py::arg("tag"), py::arg("parameters") = py::tuple())
        .def_property_readonly("cooperation_constructer", [](const Python::Config& self) {
            const CooperationConfig& value = self.value().optimizer.cooperation;
            return configured_component(
                "constructer", value.constructer_tag, value.constructer_para);
        })
        .def("set_cooperation_constructer", [](
            Python::Config& self, const std::string& tag, const py::object& parameters) {
            CooperationConfig& value = self.sharedState()->config.optimizer.cooperation;
            value.constructer_tag = require_token(tag, "constructer tag");
            value.constructer_para = parameters_from_python(parameters);
        }, py::arg("tag"), py::arg("parameters") = py::tuple())
        .def_property_readonly("cooperation_topology", [](const Python::Config& self) {
            const CooperationConfig& value = self.value().optimizer.cooperation;
            return configured_component(
                "ctopology", value.topology_tag, value.topology_para);
        })
        .def("set_cooperation_topology", [](
            Python::Config& self, const std::string& tag, const py::object& parameters) {
            CooperationConfig& value = self.sharedState()->config.optimizer.cooperation;
            value.topology_tag = require_token(tag, "cooperation topology tag");
            value.topology_para = parameters_from_python(parameters);
        }, py::arg("tag"), py::arg("parameters") = py::tuple())
        .def_property_readonly("global_archive", [](const Python::Config& self) {
            const OptimizerConfig& value = self.value().optimizer;
            return configured_component("archive", value.g_archive_tag, value.g_archive_para);
        })
        .def("set_global_archive", [](
            Python::Config& self, const std::string& tag, const py::object& parameters) {
            OptimizerConfig& value = self.sharedState()->config.optimizer;
            value.g_archive_tag = require_token(tag, "global archive tag");
            value.g_archive_para = parameters_from_python(parameters);
        }, py::arg("tag"), py::arg("parameters") = py::tuple())
        .def_property_readonly("workflows", [](Python::Config& self) {
            std::vector<WorkflowHandle> result;
            const std::shared_ptr<ConfigState> state = self.sharedState();
            for (std::size_t i = 0; i < state->config.workflows.size(); ++i)
                result.emplace_back(state, i);
            return result;
        })
        .def("workflow", &config_workflow, py::arg("key"))
        .def("add_workflow", [](Python::Config& self, const std::string& tag) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            state->config.workflows.emplace_back();
            state->config.workflows.back().tag = require_token(tag, "workflow tag");
            return WorkflowHandle(state, state->config.workflows.size() - 1);
        }, py::arg("tag"))
        .def("remove_workflow", [](Python::Config& self, const py::object& key) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            const std::size_t index = index_from_key(
                key, state->config.workflows.size(), workflow_tags(*state), "workflow");
            state->config.workflows.erase(state->config.workflows.begin() + index);
            ++state->workflowGeneration;
        }, py::arg("key"))
        .def("clear_workflows", [](Python::Config& self) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            state->config.workflows.clear();
            ++state->workflowGeneration;
        })
        .def_property_readonly("subpopulations", [](Python::Config& self) {
            std::vector<SubpopulationHandle> result;
            const std::shared_ptr<ConfigState> state = self.sharedState();
            for (std::size_t i = 0;
                 i < state->config.optimizer.subpopulations.size(); ++i)
                result.emplace_back(state, i);
            return result;
        })
        .def("subpopulation", &config_subpopulation, py::arg("key"))
        .def("add_subpopulation", [](
            Python::Config& self, const std::string& tag, int size,
            const std::string& workflow) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            state->config.optimizer.subpopulations.emplace_back();
            SubpopulationConfig& value = state->config.optimizer.subpopulations.back();
            value.tag = require_token(tag, "subpopulation tag");
            value.size = size;
            value.workflow_tag = require_token(workflow, "workflow reference");
            return SubpopulationHandle(
                state, state->config.optimizer.subpopulations.size() - 1);
        }, py::arg("tag"), py::arg("size") = 1, py::arg("workflow") = "wf")
        .def("remove_subpopulation", [](Python::Config& self, const py::object& key) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            std::vector<SubpopulationConfig>& values =
                state->config.optimizer.subpopulations;
            const std::size_t index = index_from_key(
                key, values.size(), subpopulation_tags(*state), "subpopulation");
            values.erase(values.begin() + index);
            ++state->subpopulationGeneration;
        }, py::arg("key"))
        .def("clear_subpopulations", [](Python::Config& self) {
            const std::shared_ptr<ConfigState> state = self.sharedState();
            state->config.optimizer.subpopulations.clear();
            ++state->subpopulationGeneration;
        });

    module.def("component_categories", &App::categoryKeys);
    module.def("list_components", [](const py::object& category) {
        if (category.is_none()) return App::listComponents();
        return App::listComponents(catalog_category(py::cast<std::string>(category)));
    }, py::arg("category") = py::none());
    module.def("list_initializers", &App::listInitializers);
    module.def("resolve_parameter_schema", &resolved_schema,
               py::arg("category"), py::arg("tag"),
               py::arg("parameters") = py::tuple());
    module.def("component_requirements", [](
        const std::string& category, const std::string& tag,
        const py::object& parameters) {
        return requirements(category, tag, parameters, false);
    }, py::arg("category"), py::arg("tag"),
       py::arg("parameters") = py::tuple());
    module.def("component_provisions", [](
        const std::string& category, const std::string& tag,
        const py::object& parameters) {
        return requirements(category, tag, parameters, true);
    }, py::arg("category"), py::arg("tag"),
       py::arg("parameters") = py::tuple());
    module.def("validate_config_text", [](const std::string& text, bool deep,
                                           const std::string& fileName) {
        return App::validateConfigText(text, deep, fileName);
    }, py::arg("text"), py::arg("deep") = false,
       py::arg("file_name") = "(memory)");
    module.def("validate_config_path", [](const py::object& path, bool recursive,
                                           bool deep) {
        App::ValidateOptions options;
        options.path = filesystem_path(path).u8string();
        options.recursive = recursive;
        options.deepBuild = deep;
        return App::validatePath(options);
    }, py::arg("path"), py::arg("recursive") = false, py::arg("deep") = false);
}
