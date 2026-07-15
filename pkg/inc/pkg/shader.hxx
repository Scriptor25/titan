#pragma once

#include <pkg.hxx>

#include <vector>

namespace pkg::shader
{
    struct Data
    {
        std::vector<char> Binary;
    };
}

namespace pkg
{
    template<>
    struct Serializer<shader::Data> : std::true_type
    {
        static void Serialize(std::ostream &stream, const shader::Data &value);
        static void Deserialize(std::istream &stream, shader::Data &value);
    };

    template<>
    class View<shader::Data> : public std::true_type
    {
    public:
        View(const void *data);

        [[nodiscard]] size_t GetBinarySize() const;
        [[nodiscard]] const void *GetBinaryData() const;

    private:
        const void *m_Data;
    };
}
