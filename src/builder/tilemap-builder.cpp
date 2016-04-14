#include <putki/builder/build.h>
#include <putki/builder/builder.h>
#include <putki/builder/package.h>
#include <putki/builder/resource.h>
#include <putki/builder/build-db.h>
#include <putki/builder/log.h>
#include <putki/builder/db.h>
#include <kosmos-builder-utils/pngutil.h>
#include <iostream>

#include <inki/types/claw/tiles.h>

struct tilemapbuilder : putki::builder::handler_i
{
	virtual bool handle(putki::builder::build_context *context, putki::builder::data *builder, putki::build_db::record *record, putki::db::data *input, const char *path, putki::instance_t obj)
	{
		inki::tilemap *tilemap = (inki::tilemap *) obj;
		if (!tilemap->texture)
		{
			RECORD_WARNING(record, "Tilemap has no texture set")
			return false;
		}

		ccgui::pngutil::loaded_png png;
		if (ccgui::pngutil::load_info(putki::resource::real_path(builder, tilemap->texture->Source.c_str()).c_str(), &png))
		{
			int tilesx = png.width / tilemap->tile_width;
			int tilesy = png.height / tilemap->tile_height;

			RECORD_INFO(record, "Tilemap is " << png.width << "x" << png.height << " and has " << tilesx << "x" << tilesy << " tiles ")
			for (int y=0;y<tilesy;y++)
			{
				for (int x=0;x<tilesx;x++)
				{
					inki::tileinfo ti;
					ti.u0 = float((x+0) * tilemap->tile_width) / float(png.width);
					ti.u1 = float((x+1) * tilemap->tile_width) / float(png.width);
					ti.v0 = float((y+0) * tilemap->tile_height) / float(png.height);
					ti.v1 = float((y+1) * tilemap->tile_height) / float(png.height);
					tilemap->tiles.push_back(ti);
				}
			}

		}
		else
		{
			RECORD_WARNING(record, "Failed to load png!")
		}

		return false;
	}
};

void register_tilemap_builder(putki::builder::data *builder)
{
	static tilemapbuilder fb;
	putki::builder::add_data_builder(builder, "tilemap", &fb);
}
