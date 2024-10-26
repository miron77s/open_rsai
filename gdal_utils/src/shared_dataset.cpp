#include "gdal_utils/shared_dataset.h"

#include <ogrsf_frmts.h>

class gdal_initer
{
public:
	gdal_initer ()
	{
		GDALAllRegister ();
	}

    static void all () { static gdal_initer initer; }
};

gdal::shared_dataset::shared_dataset ()
    : std::shared_ptr < GDALDataset > ( nullptr, GDALClose )
{}

gdal::shared_dataset::shared_dataset ( GDALDataset * p_ds )
    : std::shared_ptr < GDALDataset > ( p_ds, GDALClose )
{}

gdal::shared_spatial_reference gdal::shared_dataset::raster_srs () const
{
    auto ds = get ();
    if ( ds != nullptr )
    {
        auto proj_ref = ( ds != nullptr ) ? ds->GetProjectionRef () : "";
        return gdal::shared_spatial_reference ( new OGRSpatialReference ( proj_ref ), ds->GetDescription () );
    }
    else
        return gdal::shared_spatial_reference ();

}

gdal::shared_spatial_reference gdal::shared_dataset::vector_srs ( int layer ) const
{
    auto ds = get ();
    if ( ds != nullptr )
    {
        if ( ds->GetLayerCount () > layer )
        {
            gdal::shared_spatial_reference srs ( ds->GetLayer ( layer )->GetSpatialRef ()->Clone (), ds->GetDescription () );
            return srs;
        }
    }
    return gdal::shared_spatial_reference ();

}

int gdal::shared_dataset::layer_index_by_name ( const std::string &layer_name ) const
{
    auto ds = get ();
    if ( ds != nullptr )
    {
        for ( int i = 0; i < ds->GetLayerCount (); ++i )
        {
            if ( layer_name == ds->GetLayer ( i )->GetName () )
                return i;
        }
    }
    return -1;
}

gdal::shared_dataset gdal::open_dataset ( const std::string & file_name, int mode, const char * const * allowed_drivers,
                              const char * const * open_options, const char * const * sibling_files )
{
    gdal_initer::all ();

	return reinterpret_cast < GDALDataset * > ( GDALOpenEx ( file_name.c_str (), mode, allowed_drivers, open_options, sibling_files ) );
}

gdal::shared_dataset gdal::open_subdataset ( const std::string & file_name, int subdataset_index, int mode, const char * const * allowed_drivers,
                                 const char * const * open_options, const char * const * sibling_files )
{
	// открытие базового набора
	shared_dataset parent_ds = gdal::open_dataset ( file_name, mode, allowed_drivers, open_options, sibling_files );
	if ( !parent_ds ) 
		return nullptr;

	// Получение названия нужного поднабора
	char **subdatasets = GDALGetMetadata( parent_ds.get (), "SUBDATASETS" );
    std::string key_name = std::string ( "SUBDATASET_" ) + std::to_string ( subdataset_index ) + "_NAME";
	const char * subds_name = CSLFetchNameValue ( subdatasets, key_name.c_str () );

	if ( !subds_name )
		return nullptr;

	return gdal::open_dataset ( subds_name, mode, allowed_drivers, open_options, sibling_files );
}

gdal::shared_dataset gdal::create_dataset ( const std::string &driver_name, const std::string & file_name, int width, int height, int bands, GDALDataType type )
{
    gdal_initer::all ();

    auto driver = GetGDALDriverManager()->GetDriverByName( driver_name.c_str () );
    if ( driver == nullptr )
        return shared_dataset ();

    return reinterpret_cast < GDALDataset * > ( driver->Create ( file_name.c_str (), width, height, bands, type, nullptr ) );
}

bool gdal::operator ! ( const shared_datasets &datasets )
{
    for ( const auto &dataset : datasets )
    {
        if ( dataset == nullptr )
            return true;
    }
    return false;
}
