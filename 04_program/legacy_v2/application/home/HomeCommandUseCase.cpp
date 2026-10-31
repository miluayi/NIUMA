#include "application/home/HomeCommandUseCase.h"

#include "service/FocusService.h"
#include "service/PeriodicTaskService.h"

namespace {
template <typename Result>
HomeCommandResult toHomeCommandResult(const Result &result)
{
    return {result.ok, result.error};
}
}

HomeCommandResult HomeCommandUseCase::setFocus(int taskId, bool focused) const
{
    return toHomeCommandResult(FocusService().setFocus(taskId, focused));
}

HomeCommandResult HomeCommandUseCase::createTaskFromPeriodicTemplate(int templateId) const
{
    return toHomeCommandResult(PeriodicTaskService().createTaskFromTemplate(templateId));
}

HomeCommandResult HomeCommandUseCase::ignorePeriodicReminder(int templateId) const
{
    return toHomeCommandResult(PeriodicTaskService().ignoreCurrent(templateId));
}
