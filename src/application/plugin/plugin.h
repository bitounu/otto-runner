#ifndef STAK_PLUGIN_H
#define STAK_PLUGIN_H

struct stak_plugin_entry_s {
	int api_type;
	char* name;
};

int stak_plugins_load(struct stak_plugin_entry_s* plugins);
#endif