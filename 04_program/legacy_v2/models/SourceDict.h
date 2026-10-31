/**
 * @file SourceDict.h
 * @brief 来源字典数据模型，对齐 source_dict 表 schema
 *
 * 文档：TD 2.5（source_dict 表）、3.8
 * 约束：逻辑删除（enabled=0），历史任务保留原记录
 */
#pragma once
#include <QString>
#include <QDateTime>

struct SourceDict {
    int id = 0;
    QString name;
    bool isEnabled = true;
    int sortOrder = 0;
    QDateTime createTime;
};
