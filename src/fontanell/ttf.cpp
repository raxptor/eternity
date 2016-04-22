#include "ttf.h"
#include <stdint.h>
#include <stdio.h>

namespace fontanell
{
	namespace ttf_fmt
	{
		#pragma pack(push, 1)
		struct file_hdr
		{
			uint32_t version;
			uint16_t num_tables;
			uint16_t search_range;
			uint16_t entry_selector;
			uint16_t range_shift;
		};

		struct head
		{
			uint32_t version;
			uint32_t font_revision;
			uint32_t checksum_adjustment;
			uint32_t magic_number;
			uint16_t flags;
			uint16_t units_per_em;
			uint64_t created;
			uint64_t modified;
			int16_t min_x;
			int16_t min_y;
			int16_t max_x;
			int16_t max_y;
			uint16_t mac_style;
			uint16_t lowest_readable_ppem;
			int16_t font_direction_hint;
			int16_t index_to_loc_format;
			int16_t glyph_data_format;
		};

		struct table_record
		{
			uint32_t tag;
			uint32_t checksum;
			uint32_t offset;
			uint32_t length;
		};

		struct cmap_hdr
		{
			uint16_t version;
			uint16_t num_tables;
		};

		struct cmap
		{
			uint16_t platform;
			uint16_t encoding;
			uint32_t offset;
		};

		struct maxp
		{
			uint32_t version;
			uint16_t num_glyphs;
			uint16_t max_points;
			uint16_t max_contours;
			uint16_t max_component_points;
			uint16_t max_component_contours;
			uint16_t max_zones;
			uint16_t max_twilight_points;
			uint16_t max_storage;
			uint16_t max_function_defs;
			uint16_t max_instruction_defs;
			uint16_t max_stack_elements;
			uint16_t max_size_of_instructions;
			uint16_t max_component_elements;
			uint16_t max_component_depth;
		};

		struct glyph_description
		{
			uint16_t num_contours;
			int16_t min_x;
			int16_t min_y;
			int16_t max_x;
			int16_t max_y;
		};

		#pragma pack(pop)
	}

	uint16_t swap16(uint16_t in)
	{
		return ((in << 8) & 0xff00) | ((in >> 8) & 0xff);
	}

	int16_t swapi16(uint16_t in)
	{
		return (int16_t)((in << 8) & 0xff00) | ((in >> 8) & 0xff);
	}

	uint32_t swap32(uint32_t in)
	{
		return (swap16(in & 0xffff) << 16) | (swap16((in >> 16) & 0xffff));
	}

	uint32_t make_tag(const char* name)
	{
		return name[0] | (name[1] << 8) | (name[2] << 16) | (name[3] << 24);
	}
	
	namespace ttf
	{
		struct data
		{
			const char* error;
			const char* buf;
			const char* end;
			size_t size;

			const ttf_fmt::head* head;
			const ttf_fmt::maxp* maxp;
			const ttf_fmt::table_record* record_loca;
			const ttf_fmt::table_record* record_glyf;
		};

		template<typename T>
		T get(data *d, uint32_t* pos)
		{
			const char* ptr = d->buf + *pos;
			if (ptr < d->end)
			{
				(*pos) = (*pos) + sizeof(T);
				return *((T*)ptr);				
			}
			d->error = "Read past end.";
			return 0;
		}
		
		const ttf_fmt::table_record* get_table(data* d, uint32_t tag)
		{
			if (d->error)
			{
				return 0;
			}

			const ttf_fmt::file_hdr* hdr = (const ttf_fmt::file_hdr*)(d->buf);
			const ttf_fmt::table_record* table = (ttf_fmt::table_record*)(d->buf + sizeof(ttf_fmt::file_hdr));
			uint16_t count = swap16(hdr->num_tables);
			for (uint16_t i=0;i!=count;i++)
			{
				if (table[i].tag == tag)
					return &table[i];
			}

			return 0;
		}

		bool map(data*d, const uint32_t* in, uint32_t count, uint32_t* out)
		{
			if (d->error)
			{
				return false;
			}

			const ttf_fmt::table_record* record = get_table(d, make_tag("cmap"));
			if (!record)
			{
				d->error = "No cmap record";
				return false;
			}

			uint32_t record_offset = swap32(record->offset);
			const ttf_fmt::cmap_hdr* cmap = (const ttf_fmt::cmap_hdr*)(d->buf + record_offset);
			if (cmap->version != swap16(0))
			{
				d->error = "Invalid cmap table";
				return false;
			}

			uint16_t tables = swap16(cmap->num_tables);
			const ttf_fmt::cmap* cmap_enc = (const ttf_fmt::cmap*)(d->buf + record_offset + sizeof(ttf_fmt::cmap_hdr));

			for (uint16_t i=0;i!=tables;i++)
			{
				uint16_t platform = swap16(cmap_enc[i].platform);
				uint16_t encoding = swap16(cmap_enc[i].encoding);

				const char* table = d->buf + record_offset + swap32(cmap_enc[i].offset);
				const uint16_t* format_ptr = (const uint16_t*) table;
				const uint16_t format = swap16(*format_ptr);
				switch (format)
				{
					case 0: // Byte encoding table
						{

							uint16_t length = swap16(format_ptr[1]);
							uint16_t language = swap16(format_ptr[2]);
							const unsigned char* glyphs = (const unsigned char*) &format_ptr[3];
							for (uint32_t i=0;i!=count;i++)
							{
								if (in[i] < length)
								{
									out[i] = glyphs[in[i]];
								}
								else
								{
									out[i] = 0;
								}								
							}
							return true;							
						}
					default:
						break;
				}

			}

			return false;
		}

		bool get_locations(data* d, uint32_t* glyph_index, uint32_t* location, uint32_t count)
		{
			if (d->error)
			{
				return false;
			}

			const char* begin = d->buf + swap32(d->record_loca->offset);
			const char* end = d->buf + swap32(d->record_loca->offset) + swap32(d->record_loca->length);

			if (d->head->index_to_loc_format)
			{
				const uint32_t* loca_begin = (const uint32_t*)(begin);
				const uint32_t* loca_end = (const uint32_t*)(end);
				for (uint32_t i=0;i!=count;i++)
				{
					const uint32_t* ptr = loca_begin + glyph_index[i];
					if (ptr >= loca_end)
						return false;
					location[i] = swap32(*ptr);
				}
			}
			else
			{
				if (d->head->index_to_loc_format)
				{
					const uint16_t* loca_begin = (const uint16_t*)(begin);
					const uint16_t* loca_end = (const uint16_t*)(end);
					for (uint32_t i=0;i!=count;i++)
					{
						const uint16_t* ptr = loca_begin + glyph_index[i];
						if (ptr >= loca_end)
							return false;
						location[i] = swap16(*ptr);
					}
				}
			}
			return true;
		}

		bool get_glyph(data *d, uint32_t index)
		{
			uint32_t location;
			if (!get_locations(d, &index, &location, 1))
			{
				return false;
			}

			const ttf_fmt::glyph_description* desc = (const ttf_fmt::glyph_description*)(d->buf + swap32(d->record_glyf->offset) + location);

			int16_t contours = swapi16(desc->num_contours);
			if (contours > 0)
			{
		
			}

			return true;
		}

		data* open(const char* buf, size_t size)
		{
			data* d = new data();
			d->buf = buf;
			d->end = buf + size;
			d->size = size;
			
			const ttf_fmt::file_hdr* hdr = (const ttf_fmt::file_hdr*)buf;
			if (size < sizeof(hdr))
			{
				d->error = "Input buffer not enough.";
				return d;
			}

			if (swap32(hdr->version) != 0x00010000)
			{
				d->error = "Unsupported file format";
				return d;
			}

			const ttf_fmt::table_record* record;
			
			record = get_table(d, make_tag("head"));
			if (!record)
			{
				d->error = "No head table";
				return d;
			}
			d->head = (const ttf_fmt::head*)(d->buf + swap32(record->offset));

			record = get_table(d, make_tag("maxp"));
			if (!record)
			{
				d->error = "No maxp table";
				return d;
			}
			d->maxp = (const ttf_fmt::maxp*)(d->buf + swap32(record->offset));
			
			d->record_loca = get_table(d, make_tag("loca"));
			if (!d->record_loca)
			{
				d->error = "No loca table";
				return d;
			}

			d->record_glyf = get_table(d, make_tag("glyf"));
			if (!d->record_glyf)
			{
				d->error = "No glyf table";
				return d;
			}

			return d;
		}
		
		void close(data* d)
		{
			delete d;	
		}
	}
}
