#include "sys_if_workqueue.h"

#include "linux_port/workqueue.h"
#include "pw_chrono/system_clock.h"
#include "pw_log/log.h"

void SysIfInitDelayedWork(SysIfDelayedWorkStruct *work, SysIfDelayedWorkCallback work_fn, void *ctx)
{
	*work = new delayed_work();

	if (*work) {
		INIT_DELAYED_WORK(reinterpret_cast<delayed_work *>(*work), work_fn, ctx);
	} else {
		PW_LOG_ERROR("%s(): Unable to allocate memory.", __func__);
	}
}

void SysIfDeinitDelayedWork(SysIfDelayedWorkStruct *work)
{
	SysIfCancelDelayedWork(work);

	if (*work) {
		delete reinterpret_cast<delayed_work *>(*work);
	}
}

bool SysIfScheduleDelayedWork(SysIfDelayedWorkStruct *work, uint32_t msecs)
{
	if (*work) {
		return schedule_delayed_work(reinterpret_cast<delayed_work *>(*work),
					     PW_SYSTEM_CLOCK_MS(msecs).ticks);
	}

	return false;
}

bool SysIfCancelDelayedWork(SysIfDelayedWorkStruct *work)
{
	if (*work) {
		return cancel_delayed_work_sync(reinterpret_cast<delayed_work *>(*work));
	}

	return true;
}
