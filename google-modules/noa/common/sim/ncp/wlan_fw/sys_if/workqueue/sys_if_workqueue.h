#ifndef SYS_IF_WORKQUEUE_H
#define SYS_IF_WORKQUEUE_H

#include "sys_if/types/types.h"

/// @brief Callback function type for delayed work.
typedef void (*SysIfDelayedWorkCallback)(void *);

/// @brief Type definition for a delayed work structure.
typedef void *SysIfDelayedWorkStruct;

/// @brief Initializes a delayed work structure.
///
/// @param[in] work The work struct reference.
/// @param[in] work_fn Callback function to be executed when the work is
/// processed.
/// @param[in] ctx User-provided context pointer that will be passed to the
/// callback.
extern void SysIfInitDelayedWork(SysIfDelayedWorkStruct *work, SysIfDelayedWorkCallback work_fn,
				 void *ctx);

/// @brief Deinitializes a delayed work structure.
///
/// @param[in] work The work struct reference.
extern void SysIfDeinitDelayedWork(SysIfDelayedWorkStruct *work);

/// @brief Schedules delayed work for execution.
///
/// @param[in] work The work struct reference.
/// @param[in] msecs Delay in milliseconds before the work is executed.
/// @return `true` if the work was successfully scheduled, `false` otherwise.
extern bool SysIfScheduleDelayedWork(SysIfDelayedWorkStruct *work, uint32_t msecs);

/// @brief Cancels delayed work.
///
/// @param[in] work The work struct reference.
/// @return `true` if the work was successfully canceled, `false` otherwise.
extern bool SysIfCancelDelayedWork(SysIfDelayedWorkStruct *work);

#endif // SYS_IF_WORKQUEUE_H
