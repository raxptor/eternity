#include "ttf.h"
#include <stdint.h>

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

		struct table_head
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

		struct cmap_encoding
		{
			uint16_t platform;
			uint16_t encoding;
			uint32_t offset;
		};
		#pragma pack(pop)
	}

	uint16_t swap16(uint16_t in)
	{
		return ((in << 8) & 0xff00) | ((in >> 8) & 0xff);
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
			size_t size;
		};
		
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

		int32_t lookup_glyph(data* d, uint32_t character)
		{
			const ttf_fmt::table_record* record = get_table(d, make_tag("cmap"));
			if (!record)
			{
				d->error = "No cmap record";
				return -1;
			}

			uint32_t record_offset = swap32(record->offset);
			const ttf_fmt::cmap_hdr* cmap = (const ttf_fmt::cmap_hdr*)(d->buf + record_offset);
			if (cmap->version != swap16(0))
			{
				d->error = "Invalid cmap table";
				return -1;
			}

			uint16_t count = swap16(cmap->num_tables);
			const ttf_fmt::cmap_encoding* cmap_enc = (const ttf_fmt::cmap_encoding*)(d->buf + record_offset + sizeof(ttf_fmt::cmap_hdr));

			for (uint16_t i=0;i!=count;i++)
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
							break;
						}
					default:
						break;
				}

			}

			return -1;
		}

		data* open(const char* buf, size_t size)
		{
			data* d = new data();
			d->error = 0;
			d->buf = buf;
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

			lookup_glyph(d, (uint32_t)'A');

			return d;
		}
		
		void close(data* d)
		{
			delete d;	
		}
	}
}
