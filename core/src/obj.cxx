#include <fstream>

#include <obj.hxx>

static std::vector<std::string> split(const std::string &line, const char delim)
{
    std::vector<std::string> result;

    size_t beg = 0;
    for (size_t end; (end = line.find(delim, beg)) != std::string::npos; beg = end + 1)
        if (auto str = line.substr(beg, end - beg); !str.empty())
            result.push_back(std::move(str));

    if (auto str = line.substr(beg); !str.empty())
        result.push_back(std::move(str));

    return result;
}

core::obj::Mesh core::obj::Open(std::istream &stream)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textures;

    Mesh mesh;

    for (std::string line; std::getline(stream, line);)
    {
        if (line.empty())
            continue;

        auto segments = split(line, ' ');
        if (segments.empty() || segments.front() == "#")
            continue;

        if (segments.front() == "o")
        {
            // TODO: define object
            continue;
        }

        if (segments.front() == "v")
        {
            auto x = segments[1];
            auto y = segments[2];
            auto z = segments[3];

            positions.emplace_back(std::stof(x), std::stof(y), std::stof(z));
            continue;
        }

        if (segments.front() == "vn")
        {
            auto x = segments[1];
            auto y = segments[2];
            auto z = segments[3];

            normals.emplace_back(std::stof(x), std::stof(y), std::stof(z));
            continue;
        }

        if (segments.front() == "vt")
        {
            auto x = segments[1];
            auto y = segments[2];

            textures.emplace_back(std::stof(x), std::stof(y));
            continue;
        }

        if (segments.front() == "f")
        {
            std::vector<Vertex> vertices(segments.size() - 1);

            for (uint32_t i = 1; i < segments.size(); ++i)
            {
                auto &[position, texture, normal] = vertices[i - 1];
                auto indices = split(segments[i], '/');

                if (indices.size() >= 1)
                {
                    auto index = std::stoi(indices[0]);
                    if (index < 0)
                        index = positions.size() + index;
                    else
                        index = index - 1;
                    position = positions[index];
                }

                if (indices.size() >= 2)
                {
                    auto index = std::stoi(indices[1]);
                    if (index < 0)
                        index = textures.size() + index;
                    else
                        index = index - 1;
                    texture = textures[index];
                }

                if (indices.size() >= 3)
                {
                    auto index = std::stoi(indices[2]);
                    if (index < 0)
                        index = normals.size() + index;
                    else
                        index = index - 1;
                    normal = normals[index];
                }
            }

            for (uint32_t i = 1; i < vertices.size() - 1; ++i)
            {
                mesh.Vertices.push_back(vertices[0]);
                mesh.Vertices.push_back(vertices[i]);
                mesh.Vertices.push_back(vertices[i + 1]);
            }
            continue;
        }

        // TODO
    }

    return mesh;
}

core::obj::Mesh core::obj::Open(const std::filesystem::path &path)
{
    std::ifstream stream(path);
    if (!stream)
        return {};
    return Open(stream);
}
