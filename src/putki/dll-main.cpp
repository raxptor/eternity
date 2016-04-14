#include <putki/data-dll/dllinterface.h>
#include <putki/builder/builder.h>

namespace inki
{
	void bind_eternity();
	void bind_eternity_editor();
}

void eternity_register_handlers(putki::builder::data *builder);

void setup_builder(putki::builder::data *builder)
{	
	eternity_register_handlers(builder);
}

extern "C"
{
	#if defined(_MSC_VER)
	__declspec(dllexport) putki::data_dll_i* __cdecl load_data_dll(const char *data_path)
	#else
	putki::data_dll_i* load_data_dll(const char *data_path)
	#endif
	{
		inki::bind_eternity();
		inki::bind_eternity_editor();
		putki::builder::set_builder_configurator(&setup_builder);
		// bind at startup.
		return putki::create_dll_interface(data_path);
	}
}
