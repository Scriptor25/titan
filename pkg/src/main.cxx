#include <pkg/mesh/obj.hxx>

#include <json/json.hxx>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <pkg/pipeline.hxx>
#include <pkg/shader.hxx>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

struct mesh_t
{
    std::string type;
    std::string path;
    std::string name;
    std::string format;
    bool is_static{};
};

struct shader_t
{
    std::string type;
    std::string path;
    std::string name;
    std::string format;
    std::string stage;
};

struct stage_t
{
    std::string module;
    std::string name;
};

struct vertex_attribute_t
{
    uint32_t location{};
    uint32_t binding{};
    std::string reference;
};

struct pipeline_t
{
    std::string type;
    std::string name;
    std::unordered_map<std::string, std::vector<stage_t>> stages;
    std::vector<vertex_attribute_t> vertex;
};

using resource_t = std::variant<mesh_t, shader_t, pipeline_t>;

template<>
struct data::serializer<mesh_t>
{
    static bool from_data(const json::Node &node, mesh_t &value)
    {
        if (!node.Is<json::Object>())
            return false;

        auto ok = true;

        ok &= node["type"] >> value.type;
        ok &= node["path"] >> value.path;
        ok &= node["name"] >> value.name;
        ok &= node["format"] >> value.format;
        ok &= node["static"] >> value.is_static;

        return ok && value.type == "mesh";
    }

    static void to_data(json::Node &node, const mesh_t &value)
    {
        node = json::Object
        {
            { "type", value.type },
            { "path", value.path },
            { "name", value.name },
            { "format", value.format },
            { "static", value.is_static },
        };
    }
};

template<>
struct data::serializer<shader_t>
{
    static bool from_data(const json::Node &node, shader_t &value)
    {
        if (!node.Is<json::Object>())
            return false;

        auto ok = true;

        ok &= node["type"] >> value.type;
        ok &= node["path"] >> value.path;
        ok &= node["name"] >> value.name;
        ok &= node["format"] >> value.format;
        ok &= node["stage"] >> value.stage;

        return ok && value.type == "shader";
    }

    static void to_data(json::Node &node, const shader_t &value)
    {
        node = json::Object
        {
            { "type", value.type },
            { "path", value.path },
            { "name", value.name },
            { "format", value.format },
            { "stage", value.stage },
        };
    }
};

template<>
struct data::serializer<stage_t>
{
    static bool from_data(const json::Node &node, stage_t &value)
    {
        if (!node.Is<json::Object>())
            return false;

        auto ok = true;

        ok &= node["module"] >> value.module;
        ok &= node["name"] >> value.name;

        return ok;
    }

    static void to_data(json::Node &node, const stage_t &value)
    {
        node = json::Object
        {
            { "module", value.module },
            { "name", value.name },
        };
    }
};

template<>
struct data::serializer<vertex_attribute_t>
{
    static bool from_data(const json::Node &node, vertex_attribute_t &value)
    {
        if (!node.Is<json::Object>())
            return false;

        auto ok = true;

        ok &= node["location"] >> value.location;
        ok &= node["binding"] >> value.binding;
        ok &= node["reference"] >> value.reference;

        return ok;
    }

    static void to_data(json::Node &node, const vertex_attribute_t &value)
    {
        node = json::Object
        {
            { "location", value.location },
            { "binding", value.binding },
            { "reference", value.reference },
        };
    }
};

template<>
struct data::serializer<pipeline_t>
{
    static bool from_data(const json::Node &node, pipeline_t &value)
    {
        if (!node.Is<json::Object>())
            return false;

        auto ok = true;

        ok &= node["type"] >> value.type;
        ok &= node["name"] >> value.name;
        ok &= node["stages"] >> value.stages;
        ok &= node["vertex"] >> value.vertex;

        return ok && value.type == "pipeline";
    }

    static void to_data(json::Node &node, const pipeline_t &value)
    {
        node = json::Object
        {
            { "type", value.type },
            { "name", value.name },
            { "stages", value.stages },
            { "vertex", value.vertex },
        };
    }
};

template<typename... A>
    requires (std::convertible_to<A, std::string>, ...)
static int exec(std::ostream &stream, const char *file, A &&... args)
{
    int out_pipe[2];
    int err_pipe[2];

    pipe(out_pipe);
    pipe(err_pipe);

    auto pid = fork();

    if (pid == 0)
    {
        close(out_pipe[0]);
        close(err_pipe[0]);

        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);

        close(out_pipe[1]);
        close(err_pipe[1]);

        const std::vector<std::string> strings{ file, std::forward<A>(args)... };

        std::vector<char *> argv(strings.size() + 1);
        for (size_t i = 0; i < strings.size(); ++i)
            argv[i] = const_cast<char *>(strings[i].c_str());
        argv[strings.size()] = nullptr;

        execvp(file, argv.data());

        _exit(127);
    }

    close(out_pipe[1]);
    close(err_pipe[1]);

    char chunk[4096];
    ssize_t count;

    while ((count = read(out_pipe[0], chunk, sizeof(chunk))) > 0)
        stream.write(chunk, count);

    while ((count = read(err_pipe[0], chunk, sizeof(chunk))) > 0)
        fwrite(chunk, 1, count, stderr);

    close(out_pipe[0]);
    close(err_pipe[0]);

    int status;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status))
    {
        std::cerr << "exec failed:";
        ((std::cerr << ' ' << args), ...);
        return -1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return -1;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    const std::filesystem::path dst = argv[1];
    const std::vector<std::filesystem::path> paths(argv + 2, argv + argc);

    std::ofstream index(dst / "index", std::ios::binary);
    if (!index)
        return 1;

    constexpr auto MAX_CHUNK_SIZE = 1024ull * 1024ull * 64ull;

    std::ofstream *chunk{};
    size_t chunk_index{};

    for (auto path : paths)
    {
        path = std::filesystem::relative(path);

        if (!std::filesystem::is_regular_file(path))
        {
            std::cerr << "path '" << path.string() << "' does not describe a regular file." << std::endl;
            continue;
        }

        std::ifstream is(path);
        if (!is)
        {
            std::cerr << "failed to read '" << path.string() << "'" << std::endl;
            continue;
        }

        json::Node node;
        is >> node;

        if (!node)
        {
            std::cerr << "failed to parse '" << path.string() << "'" << std::endl;
            continue;
        }

        resource_t resource;
        if (!(node >> resource))
        {
            std::cerr << "failed to convert '" << path.string() << "'" << std::endl;
            continue;
        }

        if (chunk && static_cast<size_t>(chunk->tellp()) >= MAX_CHUNK_SIZE)
        {
            delete chunk;
            chunk = {};
        }

        if (!chunk)
        {
            auto chunk_path = dst / std::format("chunk{:02x}", chunk_index++);
            chunk = new std::ofstream(chunk_path, std::ios::binary);
        }

        std::string name;
        size_t begin = chunk->tellp();

        struct
        {
            auto operator()(const mesh_t &resource) const
            {
                (void) resource.is_static;

                std::filesystem::path src(resource.path);
                if (src.is_relative())
                    src = path.parent_path() / src;

                pkg::mesh::Data data;
                if (resource.format == "obj")
                {
                    if (!pkg::mesh::obj::Open(src, data))
                        return false;
                }
                else
                    return false;

                name = resource.name;
                pkg::serialize(stream, data);
                return true;
            }

            auto operator()(const shader_t &resource) const
            {
                std::filesystem::path src(resource.path);
                if (src.is_relative())
                    src = path.parent_path() / src;
                src = std::filesystem::absolute(src);

                std::ostringstream data_stream;
                if (exec(data_stream, "glslc", "-fshader-stage=" + resource.stage, "-o", "-", src.string()))
                    return false;

                const pkg::shader::Data data
                {
                    .Binary = { data_stream.view().begin(), data_stream.view().end() },
                };

                name = resource.name;
                pkg::serialize(stream, data);
                return true;
            }

            auto operator()(const pipeline_t &resource) const
            {
                pkg::pipeline::Data data;

                for (auto &[key, val] : resource.stages)
                {
                    static const std::unordered_map<std::string_view, pkg::pipeline::StageName> map
                    {
                        { "vertex", pkg::pipeline::StageName::Vertex },
                        { "fragment", pkg::pipeline::StageName::Fragment },
                        { "geometry", pkg::pipeline::StageName::Geometry },
                        { "tesselation-control", pkg::pipeline::StageName::TesselationControl },
                        { "tesselation-evaluation", pkg::pipeline::StageName::TesselationEvaluation },
                        { "compute", pkg::pipeline::StageName::Compute },
                    };

                    auto &ref = data.Stages[map.at(key)];
                    ref.resize(val.size());

                    for (size_t i = 0; i < val.size(); ++i)
                        ref[i] = {
                            .Module = val[i].module,
                            .Name = val[i].name,
                        };
                }

                data.Vertex.resize(resource.vertex.size());
                for (size_t i = 0; i < resource.vertex.size(); ++i)
                {
                    static const std::unordered_map<std::string_view, pkg::pipeline::VertexReference> map
                    {
                        { "position", pkg::pipeline::VertexReference::Position },
                        { "normal", pkg::pipeline::VertexReference::Normal },
                        { "texture", pkg::pipeline::VertexReference::Texture },
                    };

                    data.Vertex[i] = {
                        .Location = resource.vertex[i].location,
                        .Binding = resource.vertex[i].binding,
                        .Reference = map.at(resource.vertex[i].reference),
                    };
                }

                name = resource.name;
                pkg::serialize(stream, data);
                return true;
            }

            const std::filesystem::path &path;

            std::string &name;
            std::ostream &stream;
        } visitor{ path, name, *chunk };

        if (!std::visit(visitor, resource))
        {
            std::cerr << "failed to compile '" << path.string() << "'." << std::endl;
            continue;
        }

        size_t end = chunk->tellp();

        auto count = end - begin;
        if (!count)
            continue;

        index << std::format("{}\n{:02x}\n{:08x}\n{:08x}\n", name, chunk_index - 1ull, begin, count);
    }

    if (chunk)
    {
        delete chunk;
        chunk = {};
    }

    return 0;
}
