#ifndef FRACTAL_ASSETCATALOG_H
#define FRACTAL_ASSETCATALOG_H

#include <fractal.h>

BEGIN_C_HEADER
void load_asset(char *abspath, void *dest, usize sz);
void init_catalog();
END_C_HEADER

#endif // FRACTAL_ASSETCATALOG_H
