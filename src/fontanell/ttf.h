#ifndef __FONTANELL_OTF_H__
#define __FONTANELL_OTF_H__

#include <stdint.h>
#include <stddef.h>

namespace fontanell
{
	namespace ttf
	{
		struct data;
		data* open(const char* buf, size_t size);
		bool map(data*d, const uint32_t* in, uint32_t count, uint32_t* out);
		void close(data*);
	}
}

#endif
