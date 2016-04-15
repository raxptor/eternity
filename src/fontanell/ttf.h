#ifndef __FONTANELL_OTF_H__
#define __FONTANELL_OTF_H__

namespace fontanell
{
	namespace ttf
	{
		struct data;
		data* open(const char* buf, size_t size);
		void close(data*);
	}
}

#endif
