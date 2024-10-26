#include "rsai/projection_and_shade_locator.hpp"

#include <map>

run_mode rsai::run_mode_from_string ( const std::string &mode )
{
    static std::map < std::string, run_mode > mode_mapping = { { "auto", run_mode::automatic }, { "manual", run_mode::manual }, { "on_demand", run_mode::on_demand } };
    return mode_mapping [mode];
}

interaction_mode rsai::interaction_mode_from_string ( const std::string &mode )
{
    static std::map < std::string, interaction_mode > mode_mapping = { { "internal", interaction_mode::internal }, { "external", interaction_mode::external } };
    return mode_mapping [mode];
}
