# Примеры новых вариантов использования кода

Разработанная в рамках проекта библиотека предоставляет следующие новые варианты использования кода:

## 1. Детектирование зданий и сооружений

```
// Projecting and shading vectors
cv::Mat tile_gray;
cv::cvtColor ( tile, tile_gray, cv::COLOR_BGR2GRAY );

Eigen::Vector2d proj_world ( feature->GetFieldAsDouble (           DEFAULT_PROJ_STEP_X_FIELD_NAME )
                           , feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_Y_FIELD_NAME ) );

Eigen::Vector2d shade_world ( feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_X_FIELD_NAME )
                            , feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_Y_FIELD_NAME ) );

const double L1_norm = std::fmax (
                                    std::fmax ( std::fabs ( proj_world [0] ),  std::fabs ( proj_world [1] ) )
                                  , std::fmax ( std::fabs ( shade_world [0] ), std::fabs ( shade_world [1] ) )
                                 );
proj_world  = proj_world / L1_norm;
shade_world = shade_world / L1_norm;

const double max_length = feature->GetFieldAsDouble ( DEFAULT_VECTOR_MAX_LENGTH_NAME );

// Creating a building model
building_models::prismatic model ( object, proj_world, shade_world );

const int current_file_index = ++file_counter;

auto roof = instance < gdal::polygon > ();
roof->addRingDirectly ( object->clone() );

// Estimating roof position
rsai::building_models::roof_estimator roof_estimator ( roof_position_walk, world_2_raster, proj_world
                                                       , shade_world, max_length, tile_gray );

auto responses = ( use_sam ) ? roof_estimator ( roof, -tile_bbox.top_left(), segments, roof_variants
                                                , dst_dir + object_index_str + "_" )
                             : roof_estimator ( roof, -tile_bbox.top_left(), roof_variants );
```
Приведенный код позволяет создать, настроить и использовать объект класса `roof_estimator`, который позволяет детектировать крыши зданий и сооружений на фрагменте изображения `tile_gray` и в зависимости от флага `use_sam` использует классическую или нейросетевую версию детектора. Результатом детестирования является массив responses, содержащий отклики детектора, ранжированные по достоверности.

## 2. Мультипроекционная реконструкция

```
for ( int ds_index = 0; ds_index < ds_vectors.size (); ++ds_index )
{
    auto & roof_layer_iter = roof_layer_iters [ds_index];
    auto roof_features = roof_layer_iter.find_feature ( id_field_name + " = " + object_index_str );

    if ( roof_features.size() == 1 )
    {
        auto current_roof_feature = roof_features.front();
        auto roof_geometry = current_roof_feature->GetGeometryRef();
        if ( gdal::is_polygon ( roof_geometry ) )
            roofs.push_back ( gdal::to_polygon ( roof_geometry ) );
        else
        {
            roofs.push_back ( nullptr );
            std::cerr << "WARNING: wrong geometry type for dataset " << ds_index << " actual type is " 
                      << roof_geometry->getGeometryType () << " features\n";
        }
    }
    else
        roofs.push_back ( nullptr );
}

const int max_length = feature->GetFieldAsDouble ( DEFAULT_VECTOR_MAX_LENGTH_NAME );

rsai::building_models::multiview_estimator estimator ( roofs, proj_steps, shade_steps, world_2_rasters
                                                       , 1, max_structure_length );

auto structures = estimator ( tile_shifts, tiles, segmentize_step, shade_variants );
```

Приведенный код позволяет создать, настроить и использовать объект класса `multiview_estimator`, для чего из переданных наборов векторных карт `ds_vectors` извлекаются контуры крыши, описывающие одно и то же здание, и сохраняются в массиве `roofs`. Данный массив, а также параметры проекций `proj_step` и теней `shades` на всех используемых растрах. Результатом реконструкции является массив `structures`, который содержит ранжированные по степени достоверности варианты простраснтвенной структуры (теней и проекций).

## 3. Генерация обучающей выборки зданий и сооружений

```
std::cout << "Balancing tiles...\n";
rsai::markup::tile_balancer balancer ( tiles );
tiles = balancer ( balance );

int populated = 0, not_populated = 0;
int population = 0;
for ( auto &tile : tiles )
{
    if ( tile.populated () )
    {
        ++populated;
        population += tile.population_size();
    }
    else
        ++not_populated;
}

std::cout << "\tMarked tiles " << populated << "/" << float (populated) / tiles.size() * 100.f << "% "
          << "clean tiles " << not_populated << "/" << float (not_populated) / tiles.size() * 100.f << "% "
          << "with objects count " << population << '\n';

std::cout << "Splitting train/validation tiles...\n";
rsai::markup::tile_splitter splitter ( tiles, validation );
auto train = splitter.train();
auto valid = splitter.validation();

int train_population = 0;
for ( auto &tile : train )
    train_population += tile.population_size();

int valid_population = 0;
for ( auto &tile : valid )
    valid_population += tile.population_size();

std::cout << "\tTrain part " << train.size () << "/" << float (train.size ()) / tiles.size() * 100.f 
          << "% with " << train_population << " objects, "
          << "validation part " << valid.size () << "/" << float (valid.size ()) / tiles.size() * 100.f 
          << "% with " << valid_population << " objects" << '\n';

auto writer = rsai::markup::writer::get_writer ( format, classes );

std::cout << "Saving training data...\n";
writer->save ( train, output_dir, rsai::markup::markup_part::train, mode );

std::cout << "Saving validation data...\n";
writer->save ( valid, output_dir, rsai::markup::markup_part::valid, write_mode::append );
```

Приведенный код использует автоматический балансировщих `tile_balancer` аннотированных (`populated`) и не аннотированных (`not_populated`) фрагментов, который позволяет привести реальное соотношение фрагментов к требуемому. `tile_splitter` используется для случайного разделения выборки на обучающую и валидационную части в требуемой пропорции. Наконец, объект `writer` используется для записи обучающей и валидационной частей выборки в требуемом формате.