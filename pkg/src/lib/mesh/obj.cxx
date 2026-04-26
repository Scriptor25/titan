#include <pkg/mesh/obj.hxx>

#include <fstream>

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

void pkg::mesh::obj::Open(std::istream &stream, Data &data)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textures;

    data.BoxMin = glm::vec3{ std::numeric_limits<float>::infinity() };
    data.BoxMax = glm::vec3{ -std::numeric_limits<float>::infinity() };

    data.Vertices.clear();
    data.Indices.clear();

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
            auto &x = segments[1];
            auto &y = segments[2];
            auto &z = segments[3];

            positions.emplace_back(std::stof(x), std::stof(y), std::stof(z));
            continue;
        }

        if (segments.front() == "vn")
        {
            auto &x = segments[1];
            auto &y = segments[2];
            auto &z = segments[3];

            normals.emplace_back(std::stof(x), std::stof(y), std::stof(z));
            continue;
        }

        if (segments.front() == "vt")
        {
            auto &x = segments[1];
            auto &y = segments[2];

            textures.emplace_back(std::stof(x), std::stof(y));
            continue;
        }

        if (segments.front() == "f")
        {
            const auto first = data.Vertices.size();
            const auto count = segments.size() - 1;

            std::vector<Vertex> vertices(count);

            for (uint32_t i = 1; i < segments.size(); ++i)
            {
                auto &[position, normal, texture] = vertices[i - 1];
                auto indices = split(segments[i], '/');

                if (!indices.empty())
                {
                    auto index = std::stoi(indices[0]);
                    if (index < 0)
                        index = static_cast<int>(positions.size()) + index;
                    else
                        index = index - 1;
                    position = positions[index];
                }

                if (indices.size() >= 2)
                {
                    auto index = std::stoi(indices[1]);
                    if (index < 0)
                        index = static_cast<int>(textures.size()) + index;
                    else
                        index = index - 1;
                    texture = textures[index];
                }

                if (indices.size() >= 3)
                {
                    auto index = std::stoi(indices[2]);
                    if (index < 0)
                        index = static_cast<int>(normals.size()) + index;
                    else
                        index = index - 1;
                    normal = normals[index];
                }

                data.BoxMin = {
                    std::min(data.BoxMin.x, position.x),
                    std::min(data.BoxMin.y, position.y),
                    std::min(data.BoxMin.z, position.z),
                };
                data.BoxMax = {
                    std::max(data.BoxMax.x, position.x),
                    std::max(data.BoxMax.y, position.y),
                    std::max(data.BoxMax.z, position.z),
                };
            }

            data.Vertices.insert(
                data.Vertices.end(),
                std::make_move_iterator(vertices.begin()),
                std::make_move_iterator(vertices.end()));

            for (uint32_t i = 1; i < count - 1; ++i)
            {
                data.Indices.push_back(first);
                data.Indices.push_back(first + i);
                data.Indices.push_back(first + i + 1);
            }

            continue;
        }
    }
}

bool pkg::mesh::obj::Open(const std::filesystem::path &path, Data &data)
{
    std::ifstream stream(path);
    if (!stream)
        return false;

    Open(stream, data);
    return true;
}
