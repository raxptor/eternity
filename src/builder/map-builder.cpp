#include <putki/builder/build.h>
#include <putki/builder/builder.h>
#include <putki/builder/resource.h>
#include <putki/builder/build-db.h>
#include <putki/builder/db.h>
#include <putki/builder/log.h>

#include <kosmos-builder-utils/pngutil.h>

#include <map>
#include <math.h>

#include <inki/types/claw/tiles.h>

struct mapbuilder : putki::builder::handler_i
{
	const char *version() { return "mapbuilder-1"; }

	static inline int mdpos(int x, int y)
	{
		return x * 10000 + y;
	}

	static inline void add(std::vector<inki::tile_collision_line> *out, int x0, int y0, int x1, int y1)
	{
		inki::tile_collision_line line;
		line.x0 = x0;
		line.y0 = y0;
		line.x1 = x1;
		line.y1 = y1;
		out->push_back(line);
	}

	virtual bool handle(putki::builder::build_context *context, putki::builder::data *builder, putki::build_db::record *record, putki::db::data *input, const char *path, putki::instance_t obj)
	{
		inki::map *tilemap = (inki::map *) obj;

		std::vector<inki::tile_collision_line> all_lines;
		std::vector<int> cancelled;

		for (unsigned int i=0;i<tilemap->layers.size();i++)
		{
			inki::maplayer *layer = tilemap->layers[i];
			if (!layer)
			{
				continue;
			}

			if (layer->rtti_type_ref() != inki::maplayer_graphics::type_id())
			{
				continue;
			}

			inki::maplayer_graphics *graphics = (inki::maplayer_graphics*) layer;
			inki::tilemap *tiles = graphics->tiles;
			if (!tiles || !tiles->texture)
			{
				continue;
			}

			// need to rebuild collision map if tiles are updated
			putki::build_db::add_input_dependency(record, putki::db::pathof(input, tiles));

			ccgui::pngutil::loaded_png png;
			if (!ccgui::pngutil::load_info(putki::resource::real_path(builder, tiles->texture->Source.c_str()).c_str(), &png))
			{
				continue;
			}

			const int tilesx = png.width / tiles->tile_width;
			const int tilesy = png.height / tiles->tile_height;

			// First construct the collision map which mirrors layer's data but points into the
			// tile collision data.

			inki::tile_collision **collision_map = new inki::tile_collision*[graphics->width * graphics->height];
			inki::tile_collision **pos = collision_map;

			for (int y=0;y<graphics->height;y++)
			{
				for (int x=0;x<graphics->width;x++, pos++)
				{
					*pos = 0;

					const int tile = graphics->data[y * graphics->width + x];
					if (tile == -1)
					{
						continue;
					}

					const int tx = tile % tilesx;
					const int ty = tile / tilesy;
					for (int k=0;k<tiles->collision_tiles.size();k++)
					{
						if (tiles->collision_tiles[k].x == tx && tiles->collision_tiles[k].y == ty)
						{
							*pos = &tiles->collision_tiles[k];
							break;
						}
					}
				}
			}

			// Now starts crazy.
			for (int y=0;y<graphics->height;y++)
			{
				for (int x=0;x<graphics->width;x++, pos++)
				{
					const int ox = x * tiles->tile_width;
					const int oy = y * tiles->tile_height;

					const int pos = y * graphics->width + x;
					inki::tile_collision *cur    = collision_map[pos];
					if (!cur)
					{
						continue;
					}

					std::vector<inki::tile_collision_line> lines = cur->lines;

					switch (cur->prefab)
					{
						case inki::TILE_COLLISION_FULLBLOCK:
							add(&lines, 0, 0, tiles->tile_width, 0);
							add(&lines, tiles->tile_width, 0, tiles->tile_width, tiles->tile_height);
							add(&lines, tiles->tile_width, tiles->tile_height, 0, tiles->tile_height);
							add(&lines, 0, tiles->tile_height, 0, 0);
							break;
						case inki::TILE_COLLISION_RAMP_UR:
							add(&lines, 0, tiles->tile_height, tiles->tile_width, 0);
							add(&lines, tiles->tile_width, 0, tiles->tile_width, tiles->tile_height);
							add(&lines, tiles->tile_width, tiles->tile_height, 0, tiles->tile_height);
							break;
						case inki::TILE_COLLISION_RAMP_DR:
							add(&lines, 0, 0, tiles->tile_width, tiles->tile_height);
							add(&lines, tiles->tile_width, tiles->tile_height, 0, tiles->tile_height);
							add(&lines, 0, tiles->tile_height, 0, 0);
							break;
						case inki::TILE_COLLISION_CUSTOM:
							break;
					}

					for (int i=0;i<lines.size();i++)
					{
						inki::tile_collision_line *l = &lines[i];
						inki::tile_collision_line nl;
						nl.x0 = ox + l->x0;
						nl.y0 = oy + l->y0;
						nl.x1 = ox + l->x1;
						nl.y1 = oy + l->y1;

						// cancel out opposites! maybe do this in a faster way.
						int c = 0;
						for (int j=0;j<all_lines.size();j++)
						{
							if (all_lines[j].x0 == nl.x1 && all_lines[j].y0 == nl.y1 &&
							    all_lines[j].x1 == nl.x0 && all_lines[j].y1 == nl.y0)
							{
								cancelled[j] = 1;
								c = 1;
								break;
							}
						}

						all_lines.push_back(nl);
						cancelled.push_back(c);
					}
				} // x
			} // y

			// done with this now
			delete [] collision_map;
		}


		// take out all non-cancelled for the merge step.
		std::vector<inki::tile_collision_line> tomerge;
		std::vector<int> eaten;
		for (unsigned int i=0;i<all_lines.size();i++)
		{
			if (!cancelled[i])
			{
				tomerge.push_back(all_lines[i]);
				eaten.push_back(0);
			}
		}

		typedef std::multimap<int, int> MM;
		MM lookup;

		// construct lookup based on start
		for (unsigned int i=0;i<tomerge.size();i++)
			lookup.insert(MM::value_type(mdpos(tomerge[i].x0, tomerge[i].y0), i));

		int iterations = 0;
		while (iterations++ < 100)
		{
			bool ateany = false;
			for (unsigned int i=0;i<tomerge.size();i++)
			{
				// all lines starting where this ends.
				inki::tile_collision_line *c = &tomerge[i];
				if (eaten[i])
				{
					continue;
				}

				const float c_dx = (c->x1 - c->x0);
				const float c_dy = (c->y1 - c->y0);
				const float c_len = sqrtf(c_dx * c_dx + c_dy * c_dy);
				if (c_len == 0)
				{
					continue;
				}

				std::pair<MM::iterator, MM::iterator> range = lookup.equal_range(mdpos(tomerge[i].x1, tomerge[i].y1));
				for (MM::iterator j=range.first;j!=range.second;j++)
				{
					if (eaten[j->second])
					{
						continue;
					}

					// line 's' starts where 'c' ends.
					inki::tile_collision_line *s = &tomerge[j->second];
					const float s_dx = (s->x1 - s->x0);
					const float s_dy = (s->y1 - s->y0);
					const float s_len = sqrtf(s_dx * s_dx + s_dy * s_dy);

					// extend the current line to the length of both and see if the ends would match
					const float s_ex = c->x0 + c_dx * (s_len + c_len) / c_len;
					const float s_ey = c->y0 + c_dy * (s_len + c_len) / c_len;

					const float dx = s_ex - s->x1;
					const float dy = s_ey - s->y1;
					const float d = dx * dx + dy * dy;

					if (d < 4)
					{
						// since lines are indexed by starting point, change the ending of the first one
						// so there's no need to update the lookup
						c->x1 = s->x1;
						c->y1 = s->y1;
						eaten[j->second] = 1;
						ateany = true;
					}
				}
			} // line examination

			if (!ateany)
			{
				break;
			}

		} // infinite loop

		for (unsigned int i=0;i<tomerge.size();i++)
		{
			if (!eaten[i])
			{
				tilemap->collision_lines.push_back(tomerge[i]);
			}
		}

		RECORD_INFO(record, "Generated " << tilemap->collision_lines.size() << " collision lines (" << iterations << " iterations). (Unprocessed " << all_lines.size() << " lines)")

		return false;
	}
};

void register_map_builder(putki::builder::data *builder)
{
	static mapbuilder fb;
	putki::builder::add_data_builder(builder, "map", &fb);
}
