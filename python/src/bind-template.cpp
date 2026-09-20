// SPDX-License-Identifier: BSD-3-Clause

#include "bindings.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/stl.h>

#include "app-facade.h"
#include "problem-template.h"
#include "python-problem.h"

namespace py = pybind11;
namespace fs = std::filesystem;
using namespace EvoFlow;

namespace
{
    constexpr std::size_t MAX_DIRECTORIES = 128;
    constexpr std::size_t MAX_FILES = 4096;
    constexpr std::size_t PREFIX_BYTES = 64 * 1024;

    struct ProblemInstance
    {
        std::string type;
        std::string family;
        std::string name;
        std::string path;
        std::string relativePath;
        std::uint64_t size = 0;
    };

    std::string path_text(const py::handle& value)
    {
        const py::object native = py::module_::import("os").attr("fspath")(value);
        if (py::isinstance<py::str>(native) || py::isinstance<py::bytes>(native))
            return py::cast<std::string>(native);
        throw py::type_error("path must be str, bytes, or os.PathLike");
    }

    void require_positive(int value, const char* name)
    {
        if (value <= 0) throw py::value_error(std::string(name) + " must be positive");
    }

    template <class T>
    void require_size(const std::vector<T>& values, std::size_t expected,
                      const char* name)
    {
        if (values.size() != expected)
            throw py::value_error(std::string(name) + " has an invalid length");
    }

    int square_size(const std::vector<std::vector<double>>& matrix,
                    const char* name)
    {
        if (matrix.empty())
            throw py::value_error(std::string(name) + " cannot be empty");
        const int n = static_cast<int>(matrix.size());
        for (const auto& row : matrix)
            if (static_cast<int>(row.size()) != n)
                throw py::value_error(std::string(name) + " must be square");
        return n;
    }

    std::vector<double> flatten_matrix(
        const std::vector<std::vector<double>>& matrix, const char* name)
    {
        const int n = square_size(matrix, name);
        std::vector<double> result;
        result.reserve(static_cast<std::size_t>(n) * n);
        for (const auto& row : matrix)
            result.insert(result.end(), row.begin(), row.end());
        return result;
    }

    void validate_edges(int vertices, const std::vector<int>& edges,
                        const char* name = "edges")
    {
        require_positive(vertices, "vertices");
        if (edges.size() % 2 != 0)
            throw py::value_error(std::string(name) + " must contain endpoint pairs");
        for (int endpoint : edges)
            if (endpoint < 0 || endpoint >= vertices)
                throw py::value_error(std::string(name) + " contains an out-of-range endpoint");
    }

    std::set<std::string> registered_types()
    {
        const std::vector<std::string> values =
            ProblemTemplateRegistry::instance().types();
        return std::set<std::string>(values.begin(), values.end());
    }

    std::set<std::string> requested_types(const py::object& value,
                                          const std::set<std::string>& registered,
                                          bool& all)
    {
        all = value.is_none();
        if (all) return registered;
        std::set<std::string> result;
        for (const py::handle item : value)
        {
            if (!py::isinstance<py::str>(item))
                throw py::type_error("types must contain strings");
            const std::string type = py::cast<std::string>(item);
            if (registered.find(type) == registered.end())
                throw py::value_error("unknown problem TYPE: " + type);
            result.insert(type);
        }
        return result;
    }

    bool confined_to(const fs::path& root, const fs::path& candidate)
    {
        std::error_code error;
        const fs::path relative = fs::relative(candidate, root, error);
        if (error || relative.is_absolute()) return false;
        for (const fs::path& part : relative)
            if (part == fs::path("..")) return false;
        return true;
    }

    std::string read_prefix(const fs::path& path, std::uintmax_t size)
    {
        const std::size_t wanted = static_cast<std::size_t>(
            std::min<std::uintmax_t>(size, PREFIX_BYTES));
        if (wanted == 0) return std::string();
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("cannot read problem candidate: " + path.u8string());
        std::string result(wanted, '\0');
        input.read(&result[0], static_cast<std::streamsize>(wanted));
        result.resize(static_cast<std::size_t>(input.gcount()));
        return result;
    }

    std::vector<ProblemInstance> discover_instances_native(
        const std::string& root_text, const std::set<std::string>& requested)
    {
        std::error_code error;
        const fs::path supplied = fs::u8path(root_text);
        const fs::file_status supplied_status = fs::symlink_status(supplied, error);
        if (error || !fs::is_directory(supplied_status) || fs::is_symlink(supplied_status))
            throw std::runtime_error("problem data root is unavailable or is a symlink: " + root_text);
        const fs::path root = fs::weakly_canonical(supplied, error);
        if (error) throw std::runtime_error("cannot resolve problem data root: " + root_text);

        std::vector<ProblemInstance> result;
        std::size_t directory_count = 0;
        std::size_t file_count = 0;
        fs::recursive_directory_iterator it(root, error), end;
        if (error) throw std::runtime_error("cannot scan problem data root: " + root_text);
        while (it != end)
        {
            const fs::directory_entry entry = *it;
            const fs::file_status status = entry.symlink_status(error);
            if (error) throw std::runtime_error("cannot inspect a problem data entry");
            if (fs::is_symlink(status))
            {
                if (fs::is_directory(status)) it.disable_recursion_pending();
                it.increment(error);
                if (error) throw std::runtime_error("cannot continue problem data scan");
                continue;
            }
            if (fs::is_directory(status))
            {
                if (++directory_count > MAX_DIRECTORIES)
                    throw std::runtime_error("problem data directory count exceeds 128");
                it.increment(error);
                if (error) throw std::runtime_error("cannot continue problem data scan");
                continue;
            }
            if (!fs::is_regular_file(status))
            {
                it.increment(error);
                if (error) throw std::runtime_error("cannot continue problem data scan");
                continue;
            }
            if (++file_count > MAX_FILES)
                throw std::runtime_error("problem data file count exceeds 4096");

            const fs::path canonical = fs::weakly_canonical(entry.path(), error);
            if (error || !confined_to(root, canonical))
                throw std::runtime_error("a problem data path escapes the configured root");
            const fs::path relative = fs::relative(canonical, root, error);
            if (error) throw std::runtime_error("cannot derive a problem data relative path");
            const std::uintmax_t size = fs::file_size(canonical, error);
            if (error) throw std::runtime_error("cannot inspect a problem data file size");

            if (canonical.filename() != fs::path("README.md"))
            {
                const std::string prefix = read_prefix(canonical, size);
                if (prefix.find('\0') == std::string::npos
                    && prefix.find("TYPE") != std::string::npos)
                {
                    const std::string type = peekProblemType(canonical.u8string());
                    if (requested.find(type) != requested.end())
                    {
                        ProblemInstance value;
                        value.type = type;
                        auto first = relative.begin();
                        value.family = first == relative.end()
                            ? std::string()
                            : first->u8string();
                        value.name = canonical.stem().u8string();
                        value.path = canonical.u8string();
                        value.relativePath = relative.generic_u8string();
                        value.size = static_cast<std::uint64_t>(size);
                        result.push_back(std::move(value));
                    }
                }
            }

            it.increment(error);
            if (error) throw std::runtime_error("cannot continue problem data scan");
        }
        std::sort(result.begin(), result.end(),
                  [](const ProblemInstance& left, const ProblemInstance& right) {
                      if (left.relativePath != right.relativePath)
                          return left.relativePath < right.relativePath;
                      return left.type < right.type;
                  });
        return result;
    }

    std::vector<ProblemInstance> discover_instances(
        const py::object& root_value, const py::object& type_value)
    {
        const std::string root = path_text(root_value);
        const std::set<std::string> registered = registered_types();
        bool all = false;
        const std::set<std::string> requested =
            requested_types(type_value, registered, all);
        if (!all && requested.empty()) return std::vector<ProblemInstance>();
        py::gil_scoped_release release;
        return discover_instances_native(root, requested);
    }

    std::string problem_type_file(const py::object& path_value)
    {
        const std::string path = path_text(path_value);
        py::gil_scoped_release release;
        return peekProblemType(path);
    }

    App::ProblemDescription describe_problem_file(
        const py::object& path_value, const std::string& logical_name)
    {
        const std::string path = path_text(path_value);
        py::gil_scoped_release release;
        return App::describeProblem(path, logical_name);
    }

    py::object load_problem_file(
        const py::object& path_value, const std::string& logical_name)
    {
        const std::string path = path_text(path_value);
        std::string type;
        std::unique_ptr<Problem> problem;
        {
            py::gil_scoped_release release;
            type = peekProblemType(path);
            problem.reset(loadProblem(path));
        }
        return EvoFlowPython::adoptProblem(
            std::move(problem), type, logical_name);
    }

    ProblemTemplate& load_template(ProblemTemplate& value, const py::object& path_value)
    {
        const std::string path = path_text(path_value);
        py::gil_scoped_release release;
        value.load(path);
        return value;
    }

    ProblemTemplate& clear_template(ProblemTemplate& value)
    {
        py::gil_scoped_release release;
        value.clear();
        return value;
    }

    ProblemTemplate& name_template(ProblemTemplate& value, const std::string& name)
    {
        if (name.empty()) throw py::value_error("template name cannot be empty");
        value.setName(name);
        return value;
    }

    ProblemTemplate& save_template(ProblemTemplate& value, bool overwrite)
    {
        py::gil_scoped_release release;
        value.save(overwrite);
        return value;
    }

    py::object template_problem(ProblemTemplate& value)
    {
        std::unique_ptr<Problem> problem;
        {
            py::gil_scoped_release release;
            problem.reset(value.getProblem());
        }
        return EvoFlowPython::adoptProblem(
            std::move(problem), "TEMPLATE");
    }

    class PyMKPTemplate : public ProblemTemplate
    {
    public:
        void setName(std::string name) override
        {
            if (name.empty()) throw py::value_error("template name cannot be empty");
            value.setName(name);
        }
        void setResourceNumber(int count)
        {
            require_positive(count, "resource count");
            resources = count;
            value.setResourceNumber(count);
        }
        void addBackpacks(int count, std::vector<double> capacities)
        {
            require_positive(count, "backpack count");
            if (resources <= 0) throw py::value_error("set_resource_number must be called first");
            require_size(capacities, static_cast<std::size_t>(count) * resources, "capacities");
            value.addBackpacks(count, capacities.data());
        }
        void addItems(int count, std::vector<double> profits,
                      std::vector<double> occupation)
        {
            require_positive(count, "item count");
            if (resources <= 0) throw py::value_error("set_resource_number must be called first");
            require_size(profits, static_cast<std::size_t>(count), "profits");
            require_size(occupation, static_cast<std::size_t>(count) * resources, "occupation");
            value.addItems(count, profits.data(), occupation.data());
        }
        void load(std::string path) override
        {
            value.load(path);
            resources = 0;
        }
        void clear() override { value.clear(); resources = 0; }
        void save(bool overwrite) override { value.save(overwrite); }
        Problem* getProblem() override { return value.getProblem(); }
        void setItemValueSort(bool enabled) { value.setItemValueSort(enabled); }

    private:
        PT_MKP value;
        int resources = 0;
    };

    class PyWFSTemplate : public ProblemTemplate
    {
    public:
        PT_WFS value;
        int tasks = 0;
        int machines = 0;

        void setData(int n, int m, std::vector<double> work,
                     std::vector<int> edge_sources,
                     std::vector<int> edge_targets,
                     std::vector<double> communication,
                     std::vector<double> speed)
        {
            require_positive(n, "task count");
            require_positive(m, "machine count");
            require_size(work, static_cast<std::size_t>(n) * m, "work");
            if (edge_sources.size() != edge_targets.size()
                || edge_sources.size() != communication.size())
                throw py::value_error("workflow edge arrays must have equal lengths");
            for (int endpoint : edge_sources)
                if (endpoint < 0 || endpoint >= n)
                    throw py::value_error("edge_sources contains an out-of-range task");
            for (int endpoint : edge_targets)
                if (endpoint < 0 || endpoint >= n)
                    throw py::value_error("edge_targets contains an out-of-range task");
            require_size(speed, static_cast<std::size_t>(m) * m, "speed");
            tasks = n;
            machines = m;
            value.setData(n, m, std::move(work), std::move(edge_sources),
                          std::move(edge_targets), std::move(communication),
                          std::move(speed));
        }
        void setResources(std::vector<double> memory,
                          std::vector<double> memory_capacity,
                          std::vector<double> storage_capacity)
        {
            if (tasks <= 0 || machines <= 0)
                throw py::value_error("set_data must be called before set_resources");
            require_size(memory, static_cast<std::size_t>(tasks), "memory");
            require_size(memory_capacity, static_cast<std::size_t>(machines), "memory_capacity");
            require_size(storage_capacity, static_cast<std::size_t>(machines), "storage_capacity");
            value.setResources(std::move(memory), std::move(memory_capacity),
                               std::move(storage_capacity));
        }
        void setDeadline(std::vector<double> deadline)
        {
            if (tasks <= 0) throw py::value_error("set_data must be called before set_deadline");
            require_size(deadline, static_cast<std::size_t>(tasks), "deadline");
            value.setDeadline(std::move(deadline));
        }
        void setName(std::string name) override
        {
            if (name.empty()) throw py::value_error("template name cannot be empty");
            value.setName(name);
        }
        void load(std::string path) override
        {
            value.load(path);
            tasks = value.getProblemSize();
            machines = 0;
        }
        void clear() override { value.clear(); tasks = 0; machines = 0; }
        void save(bool overwrite) override { value.save(overwrite); }
        Problem* getProblem() override { return value.getProblem(); }
    };
}

void bind_template(py::module_& module)
{
    py::class_<ProblemInstance>(module, "ProblemInstance")
        .def_readonly("type", &ProblemInstance::type)
        .def_readonly("family", &ProblemInstance::family)
        .def_readonly("name", &ProblemInstance::name)
        .def_readonly("path", &ProblemInstance::path)
        .def_readonly("relative_path", &ProblemInstance::relativePath)
        .def_readonly("size", &ProblemInstance::size);

    py::class_<ProblemTemplate>(module, "ProblemTemplate")
        .def("load", &load_template, py::arg("path"),
             py::return_value_policy::reference_internal)
        .def("clear", &clear_template,
             py::return_value_policy::reference_internal)
        .def("set_name", &name_template, py::arg("name"),
             py::return_value_policy::reference_internal)
        .def("save", &save_template, py::arg("overwrite") = false,
             py::return_value_policy::reference_internal)
        .def("to_problem", &template_problem);

    py::enum_<DistanceType>(module, "DistanceType")
        .value("EUCLIDEAN", DistanceType::Euclidean)
        .value("MANHATTAN", DistanceType::Manhattan)
        .value("CHEBYSHEV", DistanceType::Chebyshev)
        .value("GEO", DistanceType::Geo);
    py::enum_<RoundType>(module, "RoundType")
        .value("NONE", RoundType::None)
        .value("ROUND", RoundType::Round)
        .value("CEIL", RoundType::Ceil)
        .value("FLOOR", RoundType::Floor);

    py::class_<PT_TSP, ProblemTemplate>(module, "TSPTemplate")
        .def(py::init<>())
        .def("set_cities", [](PT_TSP& value, std::vector<double> positions, int dimension) {
            require_positive(dimension, "dimension");
            if (positions.empty() || positions.size() % static_cast<std::size_t>(dimension) != 0)
                throw py::value_error("positions must be non-empty and divisible by dimension");
            const int cities = static_cast<int>(positions.size() / dimension);
            require_positive(cities, "city count");
            value.setCitys(positions.data(), cities, dimension);
        }, py::arg("positions"), py::arg("dimension") = 2)
        .def("set_map", [](PT_TSP& value,
                            const std::vector<std::vector<double>>& matrix,
                            bool symmetric) {
            const int cities = square_size(matrix, "matrix");
            std::vector<double> flat = flatten_matrix(matrix, "matrix");
            value.setMap(flat.data(), cities, symmetric);
        }, py::arg("matrix"), py::arg("symmetric") = true)
        .def("set_edge_weight_type", &PT_TSP::setEdgeWeightType,
             py::arg("distance"), py::arg("rounding") = RoundType::None)
        .def_property_readonly("problem_size", &PT_TSP::getProblemSize);

    py::class_<PT_QAP, ProblemTemplate>(module, "QAPTemplate")
        .def(py::init<>())
        .def("set_facilities", [](PT_QAP& value,
                                   std::vector<std::vector<double>> matrix) {
            const int n = square_size(matrix, "flow matrix");
            value.setFacilities(n, std::move(matrix));
        })
        .def("set_locations", [](PT_QAP& value,
                                  std::vector<std::vector<double>> matrix) {
            const int n = square_size(matrix, "distance matrix");
            value.setLocations(n, std::move(matrix));
        });

    py::class_<PyMKPTemplate, ProblemTemplate>(module, "MKPTemplate")
        .def(py::init<>())
        .def("set_resource_number", &PyMKPTemplate::setResourceNumber)
        .def("add_backpacks", &PyMKPTemplate::addBackpacks)
        .def("add_items", &PyMKPTemplate::addItems)
        .def("set_item_value_sort", &PyMKPTemplate::setItemValueSort,
             py::arg("enabled") = true);

    py::class_<PT_MDS, ProblemTemplate>(module, "MDSTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_MDS::setPenalty)
        .def("set_graph", [](PT_MDS& value, int vertices, std::vector<int> edges) {
            validate_edges(vertices, edges);
            value.setGraph(vertices, edges);
        })
        .def_property_readonly("problem_size", &PT_MDS::getProblemSize)
        .def_property_readonly("permutation", [](const PT_MDS& value) {
            return value.getPermutation();
        });

    py::class_<PT_MIS, ProblemTemplate>(module, "MISTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_MIS::setPenalty)
        .def("set_degree_sort", &PT_MIS::setDegreeSort)
        .def("set_graph", [](PT_MIS& value, int vertices, std::vector<int> edges) {
            validate_edges(vertices, edges);
            value.setGraph(vertices, edges);
        })
        .def_property_readonly("problem_size", &PT_MIS::getProblemSize)
        .def_property_readonly("permutation", [](const PT_MIS& value) {
            return value.getPermutation();
        });

    py::enum_<PT_CVRP::SplitMode>(module, "CVRPSplitMode")
        .value("GREEDY", PT_CVRP::SplitMode::Greedy)
        .value("OPTIMAL", PT_CVRP::SplitMode::Optimal)
        .value("LINEAR", PT_CVRP::SplitMode::Linear);
    py::class_<PT_CVRP, ProblemTemplate>(module, "CVRPTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_CVRP::setPenalty)
        .def_property("split_mode", &PT_CVRP::getSplitMode, &PT_CVRP::setSplitMode)
        .def_property("vehicles", &PT_CVRP::getVehicles, &PT_CVRP::setVehicles)
        .def_property("max_route_distance", &PT_CVRP::getMaxRouteDistance,
                      &PT_CVRP::setMaxRouteDistance)
        .def("set_data", [](PT_CVRP& value, int customers,
                             std::vector<double> x, std::vector<double> y,
                             std::vector<double> demand, double capacity) {
            require_positive(customers, "customer count");
            require_size(x, static_cast<std::size_t>(customers + 1), "x");
            require_size(y, static_cast<std::size_t>(customers + 1), "y");
            require_size(demand, static_cast<std::size_t>(customers + 1), "demand");
            if (capacity <= 0) throw py::value_error("capacity must be positive");
            value.setData(customers, std::move(x), std::move(y),
                          std::move(demand), capacity);
        })
        .def_property_readonly("problem_size", &PT_CVRP::getProblemSize);

    py::class_<PT_CFLP, ProblemTemplate>(module, "CFLPTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_CFLP::setPenalty)
        .def("set_data", [](PT_CFLP& value, int facilities, int customers,
                             std::vector<double> capacities,
                             std::vector<double> opening_costs,
                             std::vector<double> demands,
                             std::vector<double> costs) {
            require_positive(facilities, "facility count");
            require_positive(customers, "customer count");
            require_size(capacities, static_cast<std::size_t>(facilities), "capacities");
            require_size(opening_costs, static_cast<std::size_t>(facilities), "opening_costs");
            require_size(demands, static_cast<std::size_t>(customers), "demands");
            require_size(costs, static_cast<std::size_t>(facilities) * customers, "costs");
            value.setData(facilities, customers, std::move(capacities),
                          std::move(opening_costs), std::move(demands),
                          std::move(costs));
        })
        .def_property_readonly("problem_size", &PT_CFLP::getProblemSize);

    py::enum_<PT_CPMP::Heuristic>(module, "CPMPHeuristic")
        .value("GREEDY_SEED", PT_CPMP::Heuristic::GreedySeed)
        .value("COMPOSITE", PT_CPMP::Heuristic::Composite);
    py::class_<PT_CPMP, ProblemTemplate>(module, "CPMPTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_CPMP::setPenalty)
        .def_property("heuristic", &PT_CPMP::getHeuristic, &PT_CPMP::setHeuristic)
        .def("set_data", [](PT_CPMP& value, int nodes, int medians,
                             std::vector<double> capacities,
                             std::vector<double> demands,
                             std::vector<double> distances) {
            require_positive(nodes, "node count");
            require_positive(medians, "median count");
            require_size(capacities, static_cast<std::size_t>(nodes), "capacities");
            require_size(demands, static_cast<std::size_t>(nodes), "demands");
            require_size(distances, static_cast<std::size_t>(nodes) * nodes, "distances");
            value.setData(nodes, medians, std::move(capacities),
                          std::move(demands), std::move(distances));
        })
        .def("set_coordinates", [](PT_CPMP& value, int nodes, int medians,
                                    std::vector<double> capacities,
                                    std::vector<double> demands,
                                    std::vector<double> coordinates) {
            require_positive(nodes, "node count");
            require_positive(medians, "median count");
            require_size(capacities, static_cast<std::size_t>(nodes), "capacities");
            require_size(demands, static_cast<std::size_t>(nodes), "demands");
            require_size(coordinates, static_cast<std::size_t>(nodes) * 2, "coordinates");
            value.setCoord(nodes, medians, std::move(capacities),
                           std::move(demands), std::move(coordinates));
        })
        .def("set_edge_weight_type", &PT_CPMP::setEdgeWeightType,
             py::arg("distance"), py::arg("rounding") = RoundType::None)
        .def_property_readonly("problem_size", &PT_CPMP::getProblemSize);

    py::enum_<PT_FJSP::SeqMode>(module, "FJSPSequenceMode")
        .value("CONST", PT_FJSP::SeqMode::Const)
        .value("VARIABLE", PT_FJSP::SeqMode::Variable);
    py::enum_<PT_FJSP::SeqHeur>(module, "FJSPSequenceHeuristic")
        .value("MWKR", PT_FJSP::SeqHeur::MWKR)
        .value("RANDOM", PT_FJSP::SeqHeur::Random);
    py::class_<PT_FJSP, ProblemTemplate>(module, "FJSPTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_FJSP::setPenalty)
        .def_property("sequence_mode", &PT_FJSP::getSeqMode, &PT_FJSP::setSeqMode)
        .def_property("sequence_heuristic", &PT_FJSP::getSeqHeur, &PT_FJSP::setSeqHeur)
        .def("set_data", [](PT_FJSP& value, int operations, int machines,
                             std::vector<double> processing,
                             std::vector<int> predecessors) {
            require_positive(operations, "operation count");
            require_positive(machines, "machine count");
            require_size(processing, static_cast<std::size_t>(operations) * machines, "processing");
            require_size(predecessors, static_cast<std::size_t>(operations), "predecessors");
            value.setData(operations, machines, std::move(processing),
                          std::move(predecessors));
        })
        .def_property_readonly("problem_size", &PT_FJSP::getProblemSize);

    py::enum_<PT_WFS::SeqMode>(module, "WFSSequenceMode")
        .value("CONST", PT_WFS::SeqMode::Const)
        .value("HEFT", PT_WFS::SeqMode::Heft)
        .value("HEFT_INSERT", PT_WFS::SeqMode::HeftInsert)
        .value("VARIABLE", PT_WFS::SeqMode::Variable);
    py::enum_<PT_WFS::SeqHeur>(module, "WFSSequenceHeuristic")
        .value("UPWARD_RANK", PT_WFS::SeqHeur::UpwardRank)
        .value("RANDOM", PT_WFS::SeqHeur::Random);
    py::enum_<PT_WFS::MapHeur>(module, "WFSMappingHeuristic")
        .value("FASTEST", PT_WFS::MapHeur::Fastest)
        .value("EFT", PT_WFS::MapHeur::EFT);
    py::enum_<PT_WFS::ObjMode>(module, "WFSObjectiveMode")
        .value("MAX_MAKESPAN", PT_WFS::ObjMode::MaxMakespan)
        .value("AVG_MAKESPAN", PT_WFS::ObjMode::AvgMakespan);
    py::class_<PyWFSTemplate, ProblemTemplate>(module, "WFSTemplate")
        .def(py::init<>())
        .def("set_penalty", [](PyWFSTemplate& owner, double value) {
            owner.value.setPenalty(value);
        })
        .def_property("sequence_mode",
                      [](const PyWFSTemplate& owner) { return owner.value.getSeqMode(); },
                      [](PyWFSTemplate& owner, PT_WFS::SeqMode value) {
                          owner.value.setSeqMode(value);
                      })
        .def_property("sequence_heuristic",
                      [](const PyWFSTemplate& owner) { return owner.value.getSeqHeur(); },
                      [](PyWFSTemplate& owner, PT_WFS::SeqHeur value) {
                          owner.value.setSeqHeur(value);
                      })
        .def_property("mapping_heuristic",
                      [](const PyWFSTemplate& owner) { return owner.value.getMapHeur(); },
                      [](PyWFSTemplate& owner, PT_WFS::MapHeur value) {
                          owner.value.setMapHeur(value);
                      })
        .def_property("objective_mode",
                      [](const PyWFSTemplate& owner) { return owner.value.getObjMode(); },
                      [](PyWFSTemplate& owner, PT_WFS::ObjMode value) {
                          owner.value.setObjMode(value);
                      })
        .def("set_data", &PyWFSTemplate::setData)
        .def("set_resources", &PyWFSTemplate::setResources)
        .def("set_deadline", &PyWFSTemplate::setDeadline)
        .def_property_readonly("problem_size", [](PyWFSTemplate& value) {
            return value.value.getProblemSize();
        })
        .def_property_readonly("permutation", [](const PyWFSTemplate& value) {
            return value.value.getPermutation();
        });

    py::enum_<PT_STP::StpHeur>(module, "STPHeuristic")
        .value("SPANNING", PT_STP::StpHeur::Spanning)
        .value("STEINER_PRUNE", PT_STP::StpHeur::SteinerPrune);
    py::class_<PT_STP, ProblemTemplate>(module, "STPTemplate")
        .def(py::init<>())
        .def("set_penalty", &PT_STP::setPenalty)
        .def_property("heuristic", &PT_STP::getHeuristic, &PT_STP::setHeuristic)
        .def("set_graph", [](PT_STP& value, int vertices,
                              std::vector<int> edges,
                              std::vector<double> weights,
                              std::vector<int> terminals) {
            validate_edges(vertices, edges);
            require_size(weights, edges.size() / 2, "weights");
            for (int terminal : terminals)
                if (terminal < 0 || terminal >= vertices)
                    throw py::value_error("terminals contains an out-of-range vertex");
            value.setGraph(vertices, edges, weights, terminals);
        })
        .def_property_readonly("problem_size", &PT_STP::getProblemSize)
        .def_property_readonly("permutation", [](const PT_STP& value) {
            return value.getPermutation();
        });

    py::enum_<PT_SR::Func>(module, "SRFunction")
        .value("ADD", PT_SR::Func::Add)
        .value("SUB", PT_SR::Func::Sub)
        .value("MUL", PT_SR::Func::Mul)
        .value("DIV", PT_SR::Func::Div)
        .value("NEG", PT_SR::Func::Neg)
        .value("SIN", PT_SR::Func::Sin)
        .value("COS", PT_SR::Func::Cos)
        .value("EXP", PT_SR::Func::Exp)
        .value("LOG", PT_SR::Func::Log)
        .value("SQRT", PT_SR::Func::Sqrt);
    py::enum_<PT_SR::ObjMode>(module, "SRObjectiveMode")
        .value("REGRESSION", PT_SR::ObjMode::Regression)
        .value("HEURISTIC", PT_SR::ObjMode::Heuristic);
    py::class_<PT_SR, ProblemTemplate>(module, "SRTemplate")
        .def(py::init<>())
        .def("add_function", &PT_SR::addFunction)
        .def("add_functions", [](PT_SR& value, const std::vector<PT_SR::Func>& functions) {
            for (PT_SR::Func function : functions) value.addFunction(function);
        })
        .def("clear_functions", &PT_SR::clearFunctions)
        .def_property("head_length", &PT_SR::getHeadLength, &PT_SR::setHeadLength)
        .def("set_constant_pool", &PT_SR::setConstantPool)
        .def("set_constants", &PT_SR::setConstants)
        .def("set_parsimony", &PT_SR::setParsimony)
        .def("set_minimize", &PT_SR::setMinimize)
        .def("set_data", [](PT_SR& value, int variables,
                             std::vector<double> features,
                             std::vector<double> targets) {
            require_positive(variables, "variable count");
            if (targets.empty()) throw py::value_error("targets cannot be empty");
            require_size(features, static_cast<std::size_t>(variables) * targets.size(), "features");
            value.setData(variables, std::move(features), std::move(targets));
        })
        .def_property_readonly("mode", &PT_SR::getMode)
        .def_property_readonly("problem_size", &PT_SR::getProblemSize);

    module.def("_problem_template_types", []() {
        return ProblemTemplateRegistry::instance().types();
    });
    module.def("_create_problem_template", [](const std::string& type) {
        std::unique_ptr<ProblemTemplate> value(
            ProblemTemplateRegistry::instance().create(type));
        if (!value) throw py::value_error("unknown problem TYPE: " + type);
        return value;
    }, py::arg("type"));
    module.def("_discover_problem_instances", &discover_instances,
               py::arg("root"), py::arg("types") = py::none());
    module.def("_problem_type_file", &problem_type_file, py::arg("path"));
    module.def("_describe_problem_file", &describe_problem_file,
               py::arg("path"), py::arg("logical_name") = std::string());
    module.def("_load_problem_file", &load_problem_file,
               py::arg("path"), py::arg("logical_name") = std::string());
}
