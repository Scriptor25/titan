#pragma once

#include <titan/result.hxx>

namespace core
{
    template<typename T, typename... Tags>
    struct traits_t;

    template<typename>
    struct function_traits_t;

    template<typename R, typename... Args>
    struct function_traits_t<R(Args...)>
    {
        using result_type = R;
        using args_type = std::tuple<Args...>;
    };

    template<typename T>
    concept wrappable = std::convertible_to<T, bool>;

    template<wrappable T, typename... Tags>
    class wrapper_t
    {
    public:
        using traits = traits_t<T, Tags...>;

        using make_destroy_args_traits = function_traits_t<decltype(traits::make_destroy_args)>;
        using create_traits = function_traits_t<decltype(traits::create)>;
        using destroy_traits = function_traits_t<decltype(traits::destroy)>;

        using value_type = traits::value_type;
        using collection_type = std::vector<wrapper_t>;

        using destroy_args_type = make_destroy_args_traits::result_type;
        using create_result_type = create_traits::result_type;
        using destroy_result_type = destroy_traits::result_type;

    private:
        explicit wrapper_t(destroy_args_type args, value_type value)
            : args(args),
              value(value)
        {
        }

        explicit wrapper_t(value_type value)
            : value(value)
        {
        }

    public:
        wrapper_t() = default;

        ~wrapper_t()
        {
            destroy();
        }

        wrapper_t(const wrapper_t &) = delete;
        wrapper_t &operator=(const wrapper_t &) = delete;

        wrapper_t(wrapper_t &&other) noexcept
        {
            std::swap(args, other.args);
            std::swap(value, other.value);
        }

        wrapper_t &operator=(wrapper_t &&other) noexcept
        {
            std::swap(args, other.args);
            std::swap(value, other.value);
            return *this;
        }

        [[nodiscard]] T get() const
        {
            return value;
        }

        operator T() const
        {
            return value;
        }

        explicit operator bool() const
        {
            return !!value;
        }

        bool operator!() const
        {
            return !value;
        }

        static wrapper_t wrap(destroy_args_type args, value_type value)
        {
            return wrapper_t(args, value);
        }

        static wrapper_t wrap(value_type value)
        {
            return wrapper_t(value);
        }

        template<typename... Args>
        static result<wrapper_t> create(Args &&... args)
        {
            value_type value;
            if constexpr (std::is_void_v<create_result_type>)
                traits::create(std::forward<Args>(args)..., value);
            else if (auto res = traits::create(std::forward<Args>(args)..., value))
                return error<wrapper_t>("{} => {}", traits::create_name, res);
            return wrapper_t(traits::make_destroy_args(std::forward<Args>(args)...), value);
        }

        template<typename... Args>
        static result<collection_type> create_collection(Args &&... args)
        {
            std::vector<value_type> values;
            if constexpr (std::is_void_v<create_result_type>)
                traits::create_collection(std::forward<Args>(args)..., values);
            else if (auto res = traits::create_collection(std::forward<Args>(args)..., values))
                return error<collection_type>("{} => {}", traits::create_name, res);

            collection_type wrappers(values.size());
            for (size_t i = 0; i < values.size(); ++i)
                wrappers[i] = wrapper_t(traits::make_destroy_args(std::forward<Args>(args)...), values[i]);
            return wrappers;
        }

    protected:
        void destroy()
        {
            if (!value)
                return;

            if (args.has_value())
                std::apply(
                    [this]<typename... Args>(Args &&... unpacked)
                    {
                        return traits::destroy(std::forward<Args>(unpacked)..., value);
                    },
                    args.value());

            value = {};
        }

    private:
        std::optional<destroy_args_type> args{};
        value_type value{};
    };
}
