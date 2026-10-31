#include "application/maintenance/ScanImportUseCase.h"

#include "service/FileService.h"
#include "service/ScanMatchService.h"
#include "utils/Logger.h"

ScanImportData ScanImportUseCase::scan() const
{
    const QString workDir = FileService().getWorkDir();
    if (workDir.isEmpty()) {
        return {};
    }
    return {true, ScanMatchService().scanForImport(workDir)};
}

ScanImportSummary ScanImportUseCase::importSelected(
    const QList<ScanImportItem> &selectedItems, int totalItemCount) const
{
    const QString workDir = FileService().getWorkDir();
    ScanMatchService service;
    ScanImportSummary summary;
    for (const ScanImportItem &item : selectedItems) {
        const TaskResult result = service.importScanItem(item, workDir);
        if (result.ok) {
            ++summary.successCount;
        } else if (item.isDuplicate) {
            ++summary.skipCount;
        } else {
            ++summary.failCount;
            Logger::instance().warn(QStringLiteral("ScanImportDialog: 导入失败: %1 - %2")
                                        .arg(item.folderName, result.error));
        }
    }
    summary.skipCount += totalItemCount - selectedItems.size();
    return summary;
}
