// Copyright 2019-2022 The jdh99 Authors. All rights reserved.
// 重复帧检测模块
// duplicate frame简称df
// Authors: jdh99 <jdh821@163.com>

#ifndef DFDETECTOR_H
#define DFDETECTOR_H

#include "tztype.h"

// DFDetectorLoad 模块载入
// expirationTime 过期时间.单位:ms
// deltaIndexMax 最大不同的序号.如果序号是1个字节,建议是250,如果是2个字节建议是65000
bool DFDetectorLoad(int expirationTime, uint32_t deltaIndexMax);

// DFDetectorQuery 查询是否有重复帧
// 查询时会清除过期节点
bool DFDetectorQuery(uint32_t id, uint32_t index);

// DFDetectorInsert 插入帧信息
bool DFDetectorInsert(uint32_t id, uint32_t index);

// DFDetectorRemove 移除帧信息
void DFDetectorRemove(uint32_t id, uint32_t index);

// DFDetectorClear 清除所有帧信息
void DFDetectorClear(void);

#endif


