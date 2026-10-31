/**
 * @file ReferenceMaterial.h
 * @brief 参考资料数据模型，对齐 reference_material 表 schema
 *
 * 文档：TD 2.9（reference_material 表）、3.5
 * 约束：字段名使用 ref_no（非 reference_no），物理删除（无回收站）
 */
#pragma once
#include <QString>
#include <QDateTime>

struct ReferenceMaterial {
    int id = 0;
    QString refNo;            // ref_no（不是 reference_no），YYYYMMDD-NNN
    QString name;
    QString source;
    QString tag;
    QString description;
    QString folderName;
    QString folderPath;
    QDateTime createTime;
    QDateTime updateTime;
};
