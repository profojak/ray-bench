export module RayBench.Util:Arg;

import std;

namespace raybench::util
{

/// @brief Command line arguments parser
/// 
/// Options and arguments can be specified as comma-separated list of switches.
/// If an option or argument can be defined using more switches, they are
/// delimited using the pipe character.
/// 
/// Example: `-h|--help,--version`
export class Arg
{
public:

    // ------------------------------------------------------------------------

    /// @brief Parse command line arguments from `argc` and `argv`
    /// 
    /// @param argc Command line argument count
    /// @param argv Command line argument values
    /// @param options Supported options
    /// @param arguments Supported arguments
    Arg (std::int32_t argc, const char** const argv, std::string_view options,
         std::string_view arguments) : is_invalid_ (false)
    {
        if (argc > 1 && argv != nullptr)
        {
            std::vector<std::string> command_line_args;
            command_line_args.reserve (static_cast<size_t>(argc - 1));
            for (int32_t arg = 1; arg < argc; ++arg)
            {
                command_line_args.emplace_back (argv[arg]);
            }

            Initialize (std::move (command_line_args), options, arguments);
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Parse command line arguments from a string of arguments
    /// 
    /// @param first_is_exe_name Whether the first argument is the executable name
    /// @param args Command line arguments as a raw string
    /// @param options Supported options
    /// @param arguments Supported arguments
    Arg (bool first_is_exe_name, std::string_view args, std::string_view options,
         std::string_view arguments) : is_invalid_ (false)
    {
        if (args.empty ())
        {
            return;
        }

        std::vector<std::string> command_line_args;
        std::string arg_string {args};
        size_t last_start = 0;

        // If the first argument is the executable name, skip saving it
        bool save_component = !first_is_exe_name;

        for (size_t index = 0; index < arg_string.size (); ++index)
        {
            if (arg_string[index] == '\"')
            {
                size_t end_index = arg_string.find ('\"', index + 1);
                if (end_index == std::string::npos)
                {
                    is_invalid_ = true;
                    return;
                }

                if (save_component)
                {
                    command_line_args.emplace_back (arg_string.substr (index + 1, end_index - index - 1));
                }
                else
                {
                    save_component = true;
                }
                index = end_index + 1;
                last_start = index + 1;
            }
            else if (arg_string[index] == ' ')
            {
                if (save_component)
                {
                    command_line_args.emplace_back (arg_string.substr (last_start, index - last_start));
                }
                else
                {
                    save_component = true;
                }
                last_start = index + 1;
            }
        }

        if (save_component && last_start < arg_string.size ())
        {
            command_line_args.emplace_back (arg_string.substr (last_start));
        }

        if (!command_line_args.empty ())
        {
            Initialize (std::move (command_line_args), options, arguments);
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Check whether any invalid arguments were found
    /// 
    /// @return True if invalid arguments were found, false otherwise
    [[nodiscard]] bool IsInvalid () const noexcept
    {
        return is_invalid_;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the list of invalid argument values
    /// 
    /// @return A const reference to the vector of invalid argument values
    [[nodiscard]] const std::vector<std::string>& GetInvalidValues () const noexcept
    {
        return invalid_values_;
    }

    // ------------------------------------------------------------------------

    /// @brief Check whether an option is set
    /// 
    /// @param option Option to check
    /// @return True if the option is set, false otherwise
    [[nodiscard]] bool IsOptionSet (const std::string& option) const
    {
        if (auto it = options_indices_.find (option); it != options_indices_.end ())
        {
            return options_[it->second];
        }
        return false;
    }

    // ------------------------------------------------------------------------

    /// @brief Check whether an argument is set
    /// 
    /// @param argument Argument to check
    /// @return True if the argument is set, false otherwise
    [[nodiscard]] bool IsArgumentSet (const std::string& argument) const
    {
        if (auto it = arguments_indices_.find (argument); it != arguments_indices_.end ())
        {
            return arguments_[it->second];
        }
        return false;
    }

    // ------------------------------------------------------------------------

    /// @brief Retrieve the value of an argument
    /// 
    /// @param argument Argument to retrieve the value for
    /// @return The value of the argument, or an empty string if not set
    [[nodiscard]] const std::string_view GetArgumentValue (const std::string& argument) const
    {
        if (auto it = arguments_indices_.find (argument); it != arguments_indices_.end ())
        {
            return values_[it->second];
        }
        return {};
    }

    // ------------------------------------------------------------------------

    /// @brief Get the list of positional arguments
    /// 
    /// @return A const reference to the vector of positional arguments
    [[nodiscard]] const std::vector<std::string>& GetPositionalArguments () const noexcept
    {
        return positional_arguments_;
    }

private:

    // ------------------------------------------------------------------------

    /// @brief Initialize the argument parser
    /// 
    /// @param command_line_args Preprocessed command line arguments
    /// @param options Supported options
    /// @param arguments Supported arguments
    void Initialize (std::vector<std::string> command_line_args, std::string_view options,
                     std::string_view arguments)
    {
        auto parse_switches = [] (std::string_view input, auto& indices_map, uint32_t& index)
            {
                for (auto entry : std::views::split (input, ','))
                {
                    auto entry_sv = std::string_view {entry.begin (), entry.end ()};
                    if (entry_sv.contains ('|'))
                    {
                        for (auto alias : std::views::split (entry_sv, '|'))
                        {
                            indices_map[std::string {alias.begin (), alias.end ()}] = index;
                        }
                    }
                    else
                    {
                        indices_map[std::string {entry_sv}] = index;
                    }
                    ++index;
                }
            };

        uint32_t option_index = 0;
        if (!options.empty ())
        {
            parse_switches (options, options_indices_, option_index);
        }
        options_.resize (option_index);

        uint32_t argument_index = 0;
        if (!arguments.empty ())
        {
            parse_switches (arguments, arguments_indices_, argument_index);
        }
        arguments_.resize (argument_index);
        values_.resize (argument_index);

        auto strip_quotes = [] (std::string& str)
            {
                if (!str.empty () && str.front () == '\"')
                {
                    str.erase (str.begin ());
                }
                if (!str.empty () && str.back () == '\"')
                {
                    str.pop_back ();
                }
            };

        for (size_t arg = 0; arg < command_line_args.size (); ++arg)
        {
            std::string curr_arg = command_line_args[arg];
            strip_quotes (curr_arg);

            if (!curr_arg.starts_with ('-'))
            {
                positional_arguments_.emplace_back (std::move (curr_arg));
                continue;
            }

            if (auto opt_it = options_indices_.find (curr_arg); opt_it != options_indices_.end ())
            {
                options_[opt_it->second] = true;
                continue;
            }

            if (auto arg_it = arguments_indices_.find (curr_arg); arg_it != arguments_indices_.end ())
            {
                if (arg == command_line_args.size () - 1)
                {
                    invalid_values_.emplace_back (std::move (curr_arg));
                    is_invalid_ = true;
                }
                else
                {
                    std::string argument_value = command_line_args[++arg];
                    strip_quotes (argument_value);
                    arguments_[arg_it->second] = true;
                    values_[arg_it->second] = std::move (argument_value);
                }
                continue;
            }

            invalid_values_.emplace_back (curr_arg);
            is_invalid_ = true;
        }
    }

    // ------------------------------------------------------------------------

    ///< Indicate whether the arguments are invalid
    bool is_invalid_;
    ///< Vector of invalid values
    std::vector<std::string> invalid_values_;
    ///< Vector of values
    std::vector<std::string> values_;
    ///< Vector of positional arguments
    std::vector<std::string> positional_arguments_;

    ///< Map of option names to their indices in the values vector
    std::unordered_map<std::string, std::uint32_t> options_indices_;
    ///< Vector of presence of options
    std::vector<bool> options_;
    ///< Map of argument names to their indices in the values vector
    std::unordered_map<std::string, std::uint32_t> arguments_indices_;
    ///< Vector of presence of arguments
    std::vector<bool> arguments_;
};

}
