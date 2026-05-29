#ifndef SPAWN_H
#define SPAWN_H

#include <fractal.h>
#include <task.h>

BEGIN_C_HEADER

// Spawn a new task not associated to any existing task.
// Currently only used by GUI consoles.
task_t *spawn_anon_task(char *binpath, char **argv, SpawnMode mode);

END_C_HEADER

#endif // SPAWN_H
