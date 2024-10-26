#pragma once

#include <iostream>

#include <filesystem>
#include "gdal_utils/shared_dataset.h"
#include <ogrsf_frmts.h>

static auto rewrite_directory_promt_dummy = [] ( const std::string &dir_name, const bool force ) -> bool
{
    return force;
};

static auto rewrite_directory_promt_func = [] ( const std::string &dir_name, const bool force ) -> bool
{
    // if directory already exists in destination, promting rewrite
    if ( std::filesystem::exists ( dir_name ) )
    {
        if ( !force )
        {
            std::cout << "Directory '" << dir_name << "' already exists. Overwrite (y/n)? ";

            char responce = 'n';
            std::cin >> responce;

            if ( responce != 'y' )
            {
                std::cout << "Cancelled overwriting..." << std::endl;
                return false;;
            }
        }
        else
            std::cout << "Directory '" << dir_name << "' already exists. Overwriting... " << std::endl;

        auto removed_entries = std::filesystem::remove_all ( dir_name );

        std::cout << "Deleted " << removed_entries << " entries in directory " << dir_name << '\n';
    }
    return true;
};

static auto rewrite_layer_promt_dummy = [] ( gdal::shared_dataset ds_out, const std::string &layer_name, const bool force ) -> bool
{
    int layer_index = ds_out.layer_index_by_name ( layer_name );
    ds_out->DeleteLayer ( layer_index );
    return force;
};

static auto rewrite_layer_promt_func = [] ( gdal::shared_dataset ds_out, const std::string &layer_name, const bool force ) -> bool
{
    int layer_index = ds_out.layer_index_by_name ( layer_name );

    // if layer already exists in destination, promting rewrite
    if ( layer_index >= 0 )
    {
        if ( !force )
        {
            std::cout << "Layer '" << layer_name << "' already exists in dataset '" << ds_out->GetDescription () << "'. Overwrite (y/n)? ";

            char responce = 'n';
            std::cin >> responce;

            if ( responce != 'y' )
            {
                std::cout << "Cancelled overwriting..." << std::endl;
                return false;;
            }
        }
        else
            std::cout << "Layer '" << layer_name << "' already exists in dataset '" << ds_out->GetDescription () << "'. Overwriting... " << std::endl;

        ds_out->DeleteLayer ( layer_index );
    }
    return true;
};
