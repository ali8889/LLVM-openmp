#include <stdio.h>
#include <inttypes.h>
#include <omp.h>
#include <ompt.h>

static const char* ompt_thread_type_t_values[] = {
  NULL,
  "ompt_thread_initial",
  "ompt_thread_worker",
  "ompt_thread_other"
};

static ompt_get_task_data_t ompt_get_task_data;
static ompt_get_task_frame_t ompt_get_task_frame;
static ompt_get_thread_data_t ompt_get_thread_data;
static ompt_get_parallel_data_t ompt_get_parallel_data;

static int my_next_id()
{
//TODO: make sure this is thread-safe!
  static int ID=1;
  return __sync_fetch_and_add(&ID,1);
}

static void print_ids(int level)
{
  ompt_frame_t* frame = ompt_get_task_frame(level);
  if (frame)
    printf("%" PRIu64 ": level %d: parallel_id=%" PRIu64 ", task_id=%" PRIu64 ", exit_frame=%p, reenter_frame=%p\n", ompt_get_thread_data().value, level, ompt_get_parallel_data(level).value, ompt_get_task_data(level).value, frame->exit_runtime_frame, frame->reenter_runtime_frame);
  else
    printf("%" PRIu64 ": level %d: parallel_id=%" PRIu64 ", task_id=%" PRIu64 ", frame=%p\n", ompt_get_thread_data().value, level, ompt_get_parallel_data(level).value, ompt_get_task_data(level).value,               frame);
}

#define print_frame(level)\
do {\
  printf("%" PRIu64 ": __builtin_frame_address(%d)=%p\n", ompt_get_thread_data().value, level, __builtin_frame_address(level));\
} while(0)

static void
on_ompt_event_wait_atomic(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_wait_atomic: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_acquired_atomic(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_acquired_atomic: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_release_atomic(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_release_atomic: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_wait_barrier_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_wait_barrier_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_barrier_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
	printf("%" PRIu64 ": ompt_event_barrier_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
  print_ids(0);
}

static void
on_ompt_event_wait_barrier_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_wait_barrier_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_barrier_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_barrier_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_control(
  uint64_t command,
  uint64_t modifier)
{
  printf("%" PRIu64 ": ompt_event_control: command=%" PRIu64 ", modifier=%" PRIu64 "\n", ompt_get_thread_data().value, command, modifier);
}

static void
on_ompt_event_wait_critical(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_wait_critical: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_acquired_critical(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_acquired_critical: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_release_critical(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_release_critical: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_flush(void)
{
  printf("%" PRIu64 ": ompt_event_flush\n", ompt_get_thread_data().value);
}

static void
on_ompt_event_idle_begin(
  ompt_thread_data_t thread_data)
{
  printf("%" PRIu64 ": ompt_event_idle_begin: thread_id=%" PRIu64 "\n", ompt_get_thread_data().value, thread_data.value);
}

static void
on_ompt_event_idle_end(
  ompt_thread_data_t thread_data)
{
  printf("%" PRIu64 ": ompt_event_idle_end: thread_id=%" PRIu64 "\n", ompt_get_thread_data().value, thread_data.value);
}

static void
on_ompt_event_implicit_task_begin(
	ompt_parallel_data_t parallel_data,
	ompt_task_data_t* task_data)
{
        task_data->value = my_next_id();
	printf("%" PRIu64 ": ompt_event_implicit_task_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data->value);
}

static void
on_ompt_event_implicit_task_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_implicit_task_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_initial_task_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_initial_task_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_initial_task_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_initial_task_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_init_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_init_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_wait_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_wait_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_acquired_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_acquired_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_release_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_release_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_destroy_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_destroy_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_init_nest_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_init_nest_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_wait_nest_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_wait_nest_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_acquired_nest_lock_first(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_acquired_nest_lock_first: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_acquired_nest_lock_next(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_acquired_nest_lock_next: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_release_nest_lock_prev(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_release_nest_lock_prev: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_release_nest_lock_last(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_release_nest_lock_last: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_destroy_nest_lock(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_destroy_nest_lock: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_loop_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t parent_task_data,
  void *workshare_function)
{
  printf("%" PRIu64 ": ompt_event_loop_begin: parallel_id=%" PRIu64 ", parent_task_id=%" PRIu64 ", workshare_function=%p\n", ompt_get_thread_data().value, parallel_data.value, parent_task_data.value, workshare_function);
}

static void
on_ompt_event_loop_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_loop_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_master_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_master_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_master_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_master_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_parallel_begin(
  ompt_task_data_t parent_task_data,
  ompt_frame_t *parent_task_frame,
  ompt_parallel_data_t* parallel_data,
  uint32_t requested_team_size,
  void *parallel_function,
  ompt_invoker_t invoker)
{
        parallel_data->value = my_next_id();
  printf("%" PRIu64 ": ompt_event_parallel_begin: parent_task_id=%" PRIu64 ", parent_task_frame.exit=%p, parent_task_frame.reenter=%p, parallel_id=%" PRIu64 ", requested_team_size=%" PRIu32 ", parallel_function=%p, invoker=%d\n", ompt_get_thread_data().value, parent_task_data.value, parent_task_frame->exit_runtime_frame, parent_task_frame->reenter_runtime_frame, parallel_data->value, requested_team_size, parallel_function, invoker);
}

static void
on_ompt_event_parallel_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data,
  ompt_invoker_t invoker)
{
  printf("%" PRIu64 ": ompt_event_parallel_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 ", invoker=%d\n", ompt_get_thread_data().value, parallel_data.value, task_data.value, invoker);
}

static void
on_ompt_event_wait_ordered(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_wait_ordered: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_acquired_ordered(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_acquired_ordered: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_release_ordered(
  ompt_wait_id_t wait_id)
{
  printf("%" PRIu64 ": ompt_event_release_ordered: wait_id=%" PRIu64 "\n", ompt_get_thread_data().value, wait_id);
}

static void
on_ompt_event_runtime_shutdown(void)
{
  printf("%d: ompt_event_runtime_shutdown\n", omp_get_thread_num());
}

static void
on_ompt_event_sections_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t parent_task_data,
  void *workshare_function)
{
  printf("%" PRIu64 ": ompt_event_sections_begin: parallel_id=%" PRIu64 ", parent_task_id=%" PRIu64 ", workshare_function=%p\n", ompt_get_thread_data().value, parallel_data.value, parent_task_data.value, workshare_function);
}

static void
on_ompt_event_sections_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_sections_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_single_in_block_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t parent_task_data,
  void *workshare_function)
{
  printf("%" PRIu64 ": ompt_event_single_in_block_begin: parallel_id=%" PRIu64 ", parent_task_id=%" PRIu64 ", workshare_function=%p\n", ompt_get_thread_data().value, parallel_data.value, parent_task_data.value, workshare_function);
}

static void
on_ompt_event_single_in_block_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_single_in_block_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_single_others_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_single_others_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_single_others_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_single_others_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_task_begin(
    ompt_task_data_t parent_task_data,    /* id of parent task            */
    ompt_frame_t *parent_task_frame,  /* frame data for parent task   */
    ompt_task_data_t*  new_task_data,      /* id of created task           */
    void *task_function)               /* pointer to outlined function */
{
        new_task_data->value = my_next_id();
  printf("%" PRIu64 ": ompt_event_task_create: parent_task_id=%" PRIu64 ", parent_task_frame.exit=%p, parent_task_frame.reenter=%p, new_task_id=%" PRIu64 ", parallel_function=%p\n", ompt_get_thread_data().value, parent_task_data.value, parent_task_frame->exit_runtime_frame, parent_task_frame->reenter_runtime_frame, new_task_data->value, task_function);
}

static void
on_ompt_event_task_switch(
    ompt_task_data_t first_task_data,
    ompt_task_data_t second_task_data)
{
  printf("%" PRIu64 ": ompt_event_task_schedule: first_task_id=%" PRIu64 ", second_task_id=%" PRIu64 "\n", ompt_get_thread_data().value, first_task_data.value, second_task_data.value);
}

static void
on_ompt_event_task_end(
    ompt_task_data_t task_data)            /* id of task                   */
{
  printf("%" PRIu64 ": ompt_event_task_end: task_id=%" PRIu64 "\n", ompt_get_thread_data().value, task_data.value);
}

static void
on_ompt_event_task_dependences(
  ompt_task_data_t task_data,
  const ompt_task_dependence_t *deps,
  int ndeps)
{
  printf("%" PRIu64 ": ompt_event_task_dependences: task_id=%" PRIu64 ", deps=%p, ndeps=%d\n", ompt_get_thread_data().value, task_data.value, (void *)deps, ndeps);
}

static void
on_ompt_event_task_dependence_pair(
  ompt_task_data_t first_task_data,
  ompt_task_data_t second_task_data)
{
  printf("%" PRIu64 ": ompt_event_task_dependence_pair: first_task_id=%" PRIu64 ", second_task_id=%" PRIu64 "\n", ompt_get_thread_data().value, first_task_data.value, second_task_data.value);
}

static void
on_ompt_event_wait_taskgroup_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_wait_taskgroup_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_taskgroup_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_taskgroup_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_wait_taskgroup_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_wait_taskgroup_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_taskgroup_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_taskgroup_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_wait_taskwait_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_wait_taskwait_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_taskwait_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_taskwait_begin: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_wait_taskwait_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_wait_taskwait_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_taskwait_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_taskwait_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

static void
on_ompt_event_thread_begin(
  ompt_thread_type_t thread_type,
  ompt_thread_data_t thread_data)
{
  printf("%" PRIu64 ": ompt_event_thread_begin: thread_type=%s=%d, thread_id=%" PRIu64 "\n", ompt_get_thread_data().value, ompt_thread_type_t_values[thread_type], thread_type, thread_data.value);
}

static void
on_ompt_event_thread_end(
  ompt_thread_type_t thread_type,
  ompt_thread_data_t thread_data)
{
  printf("%" PRIu64 ": ompt_event_thread_end: thread_type=%s=%d, thread_id=%" PRIu64 "\n", ompt_get_thread_data().value, ompt_thread_type_t_values[thread_type], thread_type, thread_data.value);
}

static void
on_ompt_event_workshare_begin(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t parent_task_data,
  void *workshare_function)
{
  printf("%" PRIu64 ": ompt_event_workshare_begin: parallel_id=%" PRIu64 ", parent_task_id=%" PRIu64 ", workshare_function=%p\n", ompt_get_thread_data().value, parallel_data.value, parent_task_data.value, workshare_function);
}

static void
on_ompt_event_workshare_end(
  ompt_parallel_data_t parallel_data,
  ompt_task_data_t task_data)
{
  printf("%" PRIu64 ": ompt_event_workshare_end: parallel_id=%" PRIu64 ", task_id=%" PRIu64 "\n", ompt_get_thread_data().value, parallel_data.value, task_data.value);
}

#define register_callback(name)                               \
{                                                             \
  if (ompt_set_callback(name, (ompt_callback_t)&on_##name) == \
      ompt_has_event_no_callback)                             \
    printf("0: Could not register callback '" #name "'\n");   \
}

void ompt_initialize(
  ompt_function_lookup_t lookup,
  const char *runtime_version,
  unsigned int ompt_version)
{
  ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
  ompt_get_task_data = (ompt_get_task_data_t) lookup("ompt_get_task_data");
  ompt_get_task_frame = (ompt_get_task_frame_t) lookup("ompt_get_task_frame");
  ompt_get_thread_data = (ompt_get_thread_data_t) lookup("ompt_get_thread_data");
  ompt_get_parallel_data = (ompt_get_parallel_data_t) lookup("ompt_get_parallel_data");

  register_callback(ompt_event_wait_atomic);
  register_callback(ompt_event_acquired_atomic);
  register_callback(ompt_event_release_atomic);
  register_callback(ompt_event_wait_barrier_begin);
  register_callback(ompt_event_barrier_begin);
  register_callback(ompt_event_wait_barrier_end);
  register_callback(ompt_event_barrier_end);
  register_callback(ompt_event_control);
  register_callback(ompt_event_wait_critical);
  register_callback(ompt_event_acquired_critical);
  register_callback(ompt_event_release_critical);
  register_callback(ompt_event_flush);
  register_callback(ompt_event_idle_begin);
  register_callback(ompt_event_idle_end);
  register_callback(ompt_event_implicit_task_begin);
  register_callback(ompt_event_implicit_task_end);
  register_callback(ompt_event_initial_task_begin);
  register_callback(ompt_event_initial_task_end);
  register_callback(ompt_event_init_lock);
  register_callback(ompt_event_wait_lock);
  register_callback(ompt_event_acquired_lock);
  register_callback(ompt_event_release_lock);
  register_callback(ompt_event_destroy_lock);
  register_callback(ompt_event_init_nest_lock);
  register_callback(ompt_event_wait_nest_lock);
  register_callback(ompt_event_acquired_nest_lock_first);
  register_callback(ompt_event_acquired_nest_lock_next);
  register_callback(ompt_event_release_nest_lock_last);
  register_callback(ompt_event_release_nest_lock_prev);
  register_callback(ompt_event_destroy_nest_lock);
  register_callback(ompt_event_loop_begin);
  register_callback(ompt_event_loop_end);
  register_callback(ompt_event_master_begin);
  register_callback(ompt_event_master_end);
  register_callback(ompt_event_parallel_begin);
  register_callback(ompt_event_parallel_end);
  register_callback(ompt_event_wait_ordered);
  register_callback(ompt_event_acquired_ordered);
  register_callback(ompt_event_release_ordered);
  register_callback(ompt_event_runtime_shutdown);
  register_callback(ompt_event_sections_begin);
  register_callback(ompt_event_sections_end);
  register_callback(ompt_event_single_in_block_begin);
  register_callback(ompt_event_single_in_block_end);
  register_callback(ompt_event_single_others_begin);
  register_callback(ompt_event_single_others_end);
  register_callback(ompt_event_task_begin);
  register_callback(ompt_event_task_switch);
  register_callback(ompt_event_task_end);
  register_callback(ompt_event_task_dependences);
  register_callback(ompt_event_task_dependence_pair);
  register_callback(ompt_event_wait_taskgroup_begin);
  register_callback(ompt_event_taskgroup_begin);
  register_callback(ompt_event_wait_taskgroup_end);
  register_callback(ompt_event_taskgroup_end);
  register_callback(ompt_event_wait_taskwait_begin);
  register_callback(ompt_event_taskwait_begin);
  register_callback(ompt_event_wait_taskwait_end);
  register_callback(ompt_event_taskwait_end);
  register_callback(ompt_event_thread_begin);
  register_callback(ompt_event_thread_end);
  register_callback(ompt_event_workshare_begin);
  register_callback(ompt_event_workshare_end);
  printf("0: NULL_POINTER=%p\n", NULL);
}

ompt_initialize_t ompt_tool()
{
  return &ompt_initialize;
}
