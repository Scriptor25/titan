#pragma once

#include <titan/result.hxx>

#include <iostream>

namespace titan
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
    concept wrappable = std::convertible_to<T, bool> && std::default_initializable<T>;

    template<wrappable T, typename... Tags>
    class wrapper_t
    {
        template<wrappable, typename...>
        friend class wrapper_t;

    public:
        using traits = traits_t<T, Tags...>;

        using value_type = traits::value_type;
        using collection_type = std::vector<wrapper_t>;

        using make_destroy_args_traits = function_traits_t<decltype(traits::make_destroy_args)>;
        using destroy_args_type = make_destroy_args_traits::result_type;

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

        template<typename... Args>
        static wrapper_t make(value_type value, Args &&... args)
        {
            return wrapper_t(traits::make_destroy_args(args...), value);
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
            return static_cast<bool>(value);
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
        static toolkit::result<wrapper_t> create(Args &&... args)
        {
            using create_traits = function_traits_t<decltype(traits::create)>;
            using create_result_type = create_traits::result_type;

            value_type value;
            if constexpr (std::is_void_v<create_result_type>)
                traits::create(std::forward<Args>(args)..., value);
            else if (auto res = traits::create(std::forward<Args>(args)..., value))
                return toolkit::make_error("{} => {}", traits::create_name, res);

            return make(value, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static toolkit::result<collection_type> create_collection(Args &&... args)
        {
            using create_traits = function_traits_t<decltype(traits::create)>;
            using create_result_type = create_traits::result_type;

            std::vector<value_type> values;
            if constexpr (std::is_void_v<create_result_type>)
                traits::create_collection(std::forward<Args>(args)..., values);
            else if (auto res = traits::create_collection(std::forward<Args>(args)..., values))
                return toolkit::make_error("{} => {}", traits::create_name, res);

            collection_type wrappers(values.size());
            for (size_t i = 0; i < values.size(); ++i)
                wrappers[i] = make(values[i], std::forward<Args>(args)...);

            return wrappers;
        }

    protected:
        void destroy()
        {
            using destroy_traits = function_traits_t<decltype(traits::destroy)>;
            using destroy_result_type = destroy_traits::result_type;

            if (!value)
                return;

            if (args.has_value())
            {
                if constexpr (std::is_void_v<destroy_result_type>)
                {
                    std::apply(
                        [this]<typename... Args>(Args &&... unpacked)
                        {
                            return traits::destroy(std::forward<Args>(unpacked)..., value);
                        },
                        args.value());
                }
                else
                {
                    if (auto res = std::apply(
                        [this]<typename... Args>(Args &&... unpacked)
                        {
                            return traits::destroy(std::forward<Args>(unpacked)..., value);
                        },
                        args.value()))
                    {
                        std::cerr << std::format("{} => {}", traits::destroy_name, res) << std::endl;
                        __builtin_debugtrap();
                    }
                }
            }

            value = {};
        }

    private:
        std::optional<destroy_args_type> args{};
        value_type value{};
    };
}
