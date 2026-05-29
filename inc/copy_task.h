#ifndef COPY_TASK_H
#define COPY_TASK_H

#include <fractal.h>
#include <task.h>

BEGIN_C_HEADER

kret_t copy_from_task(task_t *t, void *dst, virt_t src_in_task, usize sz);
kret_t copy_to_task(task_t *t, virt_t dst_in_task, void *buf, usize sz);
kret_t copy_file_to_task(task_t *t, virt_t dst_in_task, void *file_handle, usize offset, usize sz);
kret_t memset_to_task(task_t *t, virt_t dst_in_task, i32 val, usize sz);

END_C_HEADER

#endif // COPY_TASK_H
